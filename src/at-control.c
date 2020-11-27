#include "at-interface.h"
#ifdef SIM7100
#include "sim7100x.h"
#endif

enum ModemStates {
    MODEM_OFF,
    ECHO_OFF,
    GET_CGMI,
    GET_CGMM,
    GET_CGMR,
    GET_CGSN,
    GET_CIMI,
    GET_ICCID,
    CHECK_CPIN,
    CHECK_CREG,
#ifdef SIM7100
    SET_CVHU,
    SET_CMEE,
    GET_UEINFO,
    GET_NETWORK,
    CHECK_GPS,
    SET_GPS,
    GET_GPSINFO,
#endif
    CHECK_CSQ,
    CHECK_COPS,
    CHECK_CGREG,
    LOG_INFO,
    SET_CGDCONT,
    START_PPP,
    HANGUP,
    POWER_OFF,
    OK
};

/* Modem states */
enum ModemStates modem_state;


/* Global flags */
int REQUEST = 0;
int RETRIES = 3;
int ATH_ERR = 0;

int csq_index = 0;
int csq_buffer[CSQ_BUF] = {0};

struct timespec ts_cond_tout, ts_global_timer, ts_csq, ts_gps, ts_info;
ModemInfo modem_info;
ModemFlags modem_flags;
GPSInfo gps_info;

void decode_at_data();


void init_global() {
    modem_state = MODEM_OFF;
    modem_procedure = SETUP;
    memset(&modem_info, 0, sizeof(modem_info));
    memset(&modem_flags, 0, sizeof(modem_flags));
    memset(&gps_info, 0, sizeof(gps_info));
}

void force_reboot() {
    printf("forcing reboot now!!!\n");
    system("reboot");
}

void exit_control() {
    RUN = 0;
    EXIT = 1;
    modem_state = OK;
    modem_procedure = WAIT;
#ifdef PPP
    pthread_cond_signal(&ppp_cond);
#endif
}

void reset_tx_flags(int mask) {
    if (mask & 1)
        REQUEST = 0;
    mask = mask>>1;
    if (mask & 1) 
        RETRIES = 3;
}

void log_modem_info() {
    printf("MI: %s\nMM: %s\nMR: %s\nSN: %s\nCIMI: %s\nCPIN: %d\nICCID: %s\n"
            "CREG: %d\nCOPS: %s\nCSQ: %u,%u\nNETW: %s\n",
            modem_info.cgmi, modem_info.cgmm, modem_info.cgmr, modem_info.cgsn, 
            modem_info.cimi, modem_info.cpin, modem_info.iccid, modem_info.creg, 
            modem_info.cops, modem_info.rssi, modem_info.ber, modem_info.netw);
}

void convert_gps_coordinates(int type, GPSInfo *inf) {
    switch (type) {
        case 0:
            inf->latdd = inf->latraw/100;
            inf->lngdd = inf->lngraw/100;
            inf->latmm = ((int) inf->latraw)%100;
            inf->lngmm = ((int) inf->lngraw)%100;
            inf->latss = (float) 60*(inf->latraw - floor(inf->latraw));
            inf->lngss = (float) 60*(inf->lngraw - floor(inf->lngraw));
            break;
    }
}

void sync_timers(struct timespec *ts_timer, struct timespec *ts_aux) {
    ts_aux->tv_sec = ts_timer->tv_sec;
    ts_aux->tv_nsec = ts_timer->tv_nsec;
}

void sync_all_timers() {
    clock_gettime(CLOCK_MONOTONIC, &ts_global_timer);
    sync_timers(&ts_global_timer, &ts_csq);
    sync_timers(&ts_global_timer, &ts_gps);
    sync_timers(&ts_global_timer, &ts_info);
}

void reset_states(int pof, char *msg) {
    if (msg)
        printf("WARNING: %s\n", msg);
    ATH_ERR = 0;
    modem_procedure = SETUP;
    modem_state = MODEM_OFF;
    modem_info.cpin = 0;
    modem_info.creg = 0;
    modem_info.cgreg = 0;
    modem_info.rssi = RSSI_UKW;
    modem_info.ber = RSSI_UKW;
    strcpy(modem_info.cops, "");
    strcpy(modem_info.netw, "");
    reset_tx_flags(3);
    if (pof && !modem_flags.pof)
        modem_flags.pof = 1;
}

int write_at_data()
{
    int n;
    n = strlen(tx_modem);
    if (n > MAX_SIZE-1 || n <= 0 || tx_modem[0] == 0) {
        return 0;
    }
    REQUEST = 1;
    if (modem_procedure != DONE)
        printf("--> [%3d] %s\n", n+2, tx_modem);
    tx_modem[n] = 0xD;
    tx_modem[n+1] = 0xA;
    n = n + 2;
    n = write(modemfd.fd, tx_modem, n);
    if (n > 0) {
        memset(tx_modem, 0, CMD_SIZE);
        RETRIES--;
    }
    else {
        printf("USB write() error\n");
        exit_control();
    }
    return n;
}

void *at_control()
{
    unsigned long diff;
    short ppp_hyst = 0;
    int n, ucli_modem_send_flag = 1, ucli_ports_send_flag = 1;
    int pof_retries = 10;
    init_global();
    sync_all_timers();
    while (RUN) {
        clock_gettime(CLOCK_MONOTONIC, &ts_global_timer);
        if (ppp_status.run) {
            if (ppp_hyst >= PPP_HYST) {
                if ( (n = check_ppp_process()) ) { // if ppp has exited
                    ppp_hyst = 0;
                    ucli_send_ppp_exit_code(n);
                    if (n == 8 && !modem_flags.pof) // connect script failed
                        modem_flags.pof = 1;
                }
            }
            else
                ppp_hyst++;
        }

        /* Decode AT events */
        decode_at_data();

        if (modem_flags.pof && modem_state != POWER_OFF) {
            modem_state = POWER_OFF;
            reset_tx_flags(3);
        }
        else if (modem_procedure == DONE) {

            if (modem_flags.ring && !ATH_ERR) {
                modem_procedure = WAIT;
                modem_state = HANGUP;
            }
#ifdef PPP
            else if (!ppp_status.run && modem_info.rssi > RSSI_MIN &&
                    modem_info.rssi < RSSI_UKW)
                modem_state = START_PPP;
#endif
            else {
#ifdef UNIX_CLI
                if (ucli_ports_send_flag)
                    ucli_ports_send_flag = ucli_send_port_info(1, &modem_ports) > 0 ? 0 : 1;
                if (ts_global_timer.tv_sec - ts_info.tv_sec >= TINFO || ucli_modem_send_flag) {
                    if (ucli_send_modem_info(&modem_info, &gps_info, 0) > 0)
                        ucli_modem_send_flag = 0;
                    else {
                        ucli_modem_send_flag = 1;
                        ucli_ports_send_flag = 1;
                    }
                    sync_timers(&ts_global_timer, &ts_info);
                }
                else if (ts_global_timer.tv_sec - ts_gps.tv_sec >= TGPS) {
#else
                if (ts_global_timer.tv_sec - ts_gps.tv_sec >= TGPS) {
#endif
                    sync_timers(&ts_global_timer, &ts_gps);
                    modem_state = GET_GPSINFO;
                }
                else if (ts_global_timer.tv_sec - ts_csq.tv_sec >= TCSQ) {
                    sync_timers(&ts_global_timer, &ts_csq);
                    modem_state = GET_NETWORK;
                }
            }
        }
        switch (modem_state) {
            case MODEM_OFF:
                strcpy(tx_modem, AT);
                break;
            case ECHO_OFF:
                strcpy(tx_modem, ATE);
                break;
            case GET_CGMI:
                sprintf(tx_modem, "AT%s", ATCGMI);
                break;
            case GET_CGMM:
                sprintf(tx_modem, "AT%s", ATCGMM);
                break;
            case GET_CGMR:
                sprintf(tx_modem, "AT%s", ATCGMR);
                break;
            case GET_CGSN:
                sprintf(tx_modem, "AT%s", ATCGSN);
                break;
#ifdef SIM7100
            case SET_CMEE:
                sprintf(tx_modem, "AT%s=1", ATCMEE);
                break;
            case CHECK_GPS:
                sprintf(tx_modem, "AT%s?", ATGPS);
                break;
            case SET_GPS:
                sprintf(tx_modem, "AT%s=1,1", ATGPS);
                break;
#endif
            case GET_CIMI:
                sprintf(tx_modem, "AT%s", ATCIMI);
                break;
            case CHECK_CPIN:
                sprintf(tx_modem, "AT%s?", ATCPIN);
                break;
            case GET_ICCID:
#ifdef SIM7100
                strcpy(tx_modem, "AT+CICCID");
#else
                sprintf(tx_modem, "AT%s", ATICCID);
#endif
                break;
            case CHECK_CREG:
                sprintf(tx_modem, "AT%s?", ATCREG);
                break;
#ifdef SIM7100
            case SET_CVHU:
                sprintf(tx_modem, "AT%s=0", ATCVHU);
                break;
            case GET_UEINFO:
                sprintf(tx_modem, "AT%s?", ATCPSI);
                break;
            case GET_NETWORK:
                sprintf(tx_modem, "AT%s?", ATCNSMOD);
                break;
            case GET_GPSINFO:
                sprintf(tx_modem, "AT%s", ATGPSINFO);
#endif
                break;
            case CHECK_CSQ:
                sprintf(tx_modem, "AT%s", ATCSQ);
                break;
            case CHECK_COPS:
                sprintf(tx_modem, "AT%s?", ATCOPS);
                break;
            case SET_CGDCONT:
                sprintf(tx_modem, "AT%s=1,\"IP\",\"\"", ATCGDCONT);
                break;
            case START_PPP:
#ifdef PPP
                pthread_cond_signal(&ppp_cond);
#endif
                modem_state = OK;
                break;
            case LOG_INFO:
                log_modem_info();
                modem_state = OK;
                break;
            case HANGUP:
                strcpy(tx_modem, ATH);
                break;
            case POWER_OFF:
                sprintf(tx_modem, "AT%s", ATPOF);
                break;
            case OK:
                break;
            default:
                break;
        }
        if (!REQUEST && RETRIES) {
            strcpy(last_sent_cmd, tx_modem);
            write_at_data();
        }
        else if (!RETRIES) {
            printf("RETRIES exceed, powering off\n");
            modem_flags.pof = 1;
        }
        else {
            if (modem_state == POWER_OFF) {
                if (!pof_retries)
                    force_reboot();
                else
                    pof_retries--;
            }
        }
        do_wait(&state_machine_lock, &state_machine_cond, TSLEEP);
    }
    close(modemfd.fd);
    printf("Exiting at_control()\n");
    return NULL;
}

void decode_at_data()
{
    int n, dummy, err;
    char *pch=NULL, *ok=NULL, payload[CMD_SIZE]={0}, fmt[ARG_SIZE]={0};
    pch = dequeue(rx_queue);
    if (pch) {
        strcpy(payload, pch);
        if (strcmp(pch, ATOK) && strcmp(pch, ATERR)) {
            if (!strcmp(pch, AT) || !strcmp(pch, ATE))
                decode_at_data();
            else if (!strcmp(pch, ATRING)) {
                modem_flags.ring = 1;
            }
            else if (!strcmp(pch, "NO CARRIER") || strstr(pch, "+PPPD") || 
                    strstr(pch, "MISSED_CALL") || strstr(pch, ATSTIN))
                decode_at_data();
            else if (strstr(pch, ATCME) || strstr(pch, ATCMS)) {
                pch += strlen(ATCME) + 2;
                err = strtol(pch, NULL, 10);
                sprintf(fmt, "Received ERROR code: %d", err);
                switch (err) {
                    case 3:
                    case 13:
                        reset_states(1, fmt);
                        break;
                    default:
                        reset_states(1, fmt);
                }
#ifdef UNIX_CLI
                printf("ucli: CME error received, modem_info: sent %d bytes\n",
                    ucli_send_modem_info(&modem_info, &gps_info, 1));
#endif
            }
            else if (strstr(pch, ATSIM)) {
                pch += strlen(ATSIM) + 2;
                if (!strcmp(pch, "NOT AVAILABLE") && modem_state != POWER_OFF)
                    reset_states(0, pch);
            }
            else if (strstr(pch, ATCPIN)) {
                pch += strlen(ATCPIN) + 2;
                /*
                 * READY not pending for any password
                 * SIM PIN waiting SIM PIN to be given
                 * SIM PUK waiting SIM PUK to be given
                 * PH-SIM PIN waiting phone-to-SIM card password to be given
                 * SIM PIN2 waiting SIM PIN2 to be given
                 * SIM PUK2 waiting SIM PUK2 to be given
                 * PH-NET PIN waiting network personalization password A to be given
                 */
                if (!strcmp(pch, "READY")) {
                    modem_info.cpin = 1;
                    if (strstr(last_sent_cmd, ATCPIN)) {
                        dequeue(rx_queue);
                        modem_state = GET_ICCID;
                        reset_tx_flags(3);
                    }
                }
                else
                    reset_states(0, pch);
            }
            else if (strstr(pch, ATCGMR)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    strncpy(modem_info.cgmr, payload+strlen(ATCGMR)+2, CHK_SIZE-1);
                    if (strstr(last_sent_cmd, ATCGMR)) {
                        modem_state = GET_CGSN;
                        reset_tx_flags(3);
                    }
                }
                else
                    strcpy(modem_info.cgmr, "");
            }
            else if (strstr(pch, ATICCID)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    strncpy(modem_info.iccid, payload+strlen(ATICCID)+2, CHK_SIZE-1);
                    if (strstr(last_sent_cmd, "ICCID")) {
                        modem_state = CHECK_CREG;
                        reset_tx_flags(3);
                    }
                }
                else
                    strcpy(modem_info.iccid, "");
            }
            else if (strstr(pch, ATCREG)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    sprintf(fmt, "%s: %%*d,%%d", ATCREG);
                    n = sscanf(payload, fmt, &modem_info.creg);
                    if (n == -1)
                        modem_info.creg = 0;
                    else {
                        switch (modem_info.creg) {
                            case 1:
                            case 5:
#ifdef SIM7100
                                modem_state = SET_CVHU;

                                reset_tx_flags(3);
#endif
                                break;
                            case 0:
                            case 3:
                            case 4:
                                reset_tx_flags(1);
                                modem_flags.pof = 1;
                                break;
                            case 2:
                                reset_tx_flags(1);
                                sleep(10);
#ifdef UNIX_CLI
                                printf("ucli: Not registered! modem_info: sent %d bytes\n",
                                    ucli_send_modem_info(&modem_info, &gps_info, 1));
#endif
                            default:
                                break;
                        }
                    }
                }
                else
                    modem_state = MODEM_OFF;
            }
            else if (strstr(pch, ATCNSMOD)) {
                if (strstr(last_sent_cmd, ATCNSMOD) && !strcmp(dequeue(rx_queue), ATOK)) {
                    sprintf(fmt, "%s: %%*d,%%d", ATCNSMOD);
                    sscanf(payload, fmt, &dummy);
                    if (dummy >= 0 && dummy < sizeof(networks)/sizeof(char *)) {
                        strcpy(modem_info.netw, networks[dummy]);
                    }
                    modem_state = CHECK_CSQ;
                    reset_tx_flags(3);
                }
            }
            else if (strstr(pch, ATCSQ)) {
                if (strstr(last_sent_cmd, ATCSQ) && !strcmp(dequeue(rx_queue), ATOK)) {
                    sprintf(fmt, "%s: %%u,%%u", ATCSQ);
                    n = sscanf(payload, fmt, &dummy, &modem_info.ber);
                    if (n == -1) {
                        modem_info.rssi = RSSI_UKW;
                        modem_info.ber = RSSI_UKW;
                    }
                    else {
                        if (abs(dummy - modem_info.rssi) > CSQ_THRSHLD) {
#ifdef UNIX_CLI
                                n = modem_info.rssi;
                                modem_info.rssi = dummy;
                                printf("ucli: CSQ changed from %d to %d - %s. modem_info: sent %d bytes\n",
                                        n, dummy, modem_info.netw, 
                                        ucli_send_modem_info(&modem_info, &gps_info, 1));
#endif
                        }
                        if (csq_index < CSQ_BUF) {
                            csq_buffer[csq_index] = dummy;
                            csq_index++;
                        }
                        else {
                            printf("CSQ Report: [");
                            for (csq_index=0; csq_index<CSQ_BUF; csq_index++)
                                printf("%d,", csq_buffer[csq_index]);
                            printf("%d], %d, %s\n", dummy, modem_info.ber, modem_info.netw);
                            csq_index = 0;
                        }
                        modem_info.rssi = dummy;
                    }
                    if (modem_procedure == SETUP)
                        modem_state = CHECK_COPS;
                    else {
#ifdef REPORT_AGENT
                        report_csq_to_agent(modem_info.rssi, modem_info.ber,
                                modem_info.netw);
#endif
                        modem_state = OK;
                    }
                    reset_tx_flags(3);
                }
            }
            else if (strstr(pch, ATCOPS)) {
                if (strstr(last_sent_cmd, ATCOPS) && !strcmp(dequeue(rx_queue), ATOK)) {
                    sprintf(fmt, "%s: %%*d,%%*d,\"%%%d[^,\"]", ATCOPS, CHK_SIZE);
                    n = sscanf(payload, fmt, modem_info.cops); 
                    modem_state = SET_CGDCONT;
                    reset_tx_flags(3);
                }
            }
#ifdef SIM7100
            else if (strstr(pch, ATGPSINFO)) {
                memset(&gps_info, 0, sizeof(gps_info));
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    sprintf(fmt, "%s: %%lf,%%c,%%lf,%%c,%%u,%%u.%%*d,%%f,%%f,%%d", ATGPSINFO);
                    n = sscanf(payload, fmt, &gps_info.latraw, &gps_info.latdir, &gps_info.lngraw,
                            &gps_info.lngdir, &gps_info.date, &gps_info.time, &gps_info.alt,
                            &gps_info.speed, &gps_info.course);
                    if (n) {
                        convert_gps_coordinates(0, &gps_info);
                        printf("%u°%u'%.2f\"%c %u°%u'%.2f\"%c %.2f %.2f\n", gps_info.latdd, gps_info.latmm, gps_info.latss, gps_info.latdir, gps_info.lngdd, gps_info.lngmm, gps_info.lngss, gps_info.lngdir, gps_info.alt, gps_info.speed);
                    }
                }
                modem_state = OK;
                reset_tx_flags(3);
            }
            else if (strstr(pch, ATGPS) && strstr(last_sent_cmd, ATGPS)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    sprintf(fmt, "%s: %%d", ATGPS);
                    n = sscanf(payload, fmt, &dummy);
                    if (n && dummy)
                        modem_state = GET_CIMI;
                    else
                        modem_state = SET_GPS;
                }
                else
                    modem_state = SET_GPS;
                reset_tx_flags(3);
            }
#endif
            else if (strstr(last_sent_cmd, ATCGMI)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = GET_CGMM;
                    reset_tx_flags(3);
                    strncpy(modem_info.cgmi, payload, CHK_SIZE-1);
                }
            }
            else if (strstr(last_sent_cmd, ATCGMM)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = GET_CGMR;
                    reset_tx_flags(3);
                    strncpy(modem_info.cgmm, payload, CHK_SIZE-1);
                }
            }
            else if (strstr(last_sent_cmd, ATCGMR)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = GET_CGSN;
                    reset_tx_flags(3);
                    strncpy(modem_info.cgmr, payload+strlen(ATCGMR)+2, CHK_SIZE-1);
                }
            }
            else if (strstr(last_sent_cmd, ATCGSN)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
#ifdef SIM7100
                    modem_state = SET_CMEE;
#else
                    modem_state = GET_CIMI;
#endif
                    reset_tx_flags(3);
                    strncpy(modem_info.cgsn, payload, CHK_SIZE-1);
                }
            }
            else if (strstr(last_sent_cmd, ATCIMI)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = CHECK_CPIN;
                    reset_tx_flags(3);
                    strncpy(modem_info.cimi, payload, CHK_SIZE-1);
                }
            }
            else
                decode_at_data();
        }
        /* Commands that return OK or ERROR should be handled here */
        else if (!strcmp(last_sent_cmd, ATH)) {
            modem_procedure = DONE;
            modem_state = OK;
            modem_flags.ring = 0;
            reset_tx_flags(3);
        }
        else if (!strcmp(last_sent_cmd, AT) && !strcmp(pch, ATOK)) {
            modem_state = ECHO_OFF;
            reset_tx_flags(3);
        }
        else if (!strcmp(last_sent_cmd, ATE) && !strcmp(pch, ATOK)) {
            modem_state = GET_CGMI;
            reset_tx_flags(3);
        }
#ifdef SIM7100
        else if (strstr(last_sent_cmd, ATCMEE)) {
            if (strcmp(pch, ATOK)) {
                printf("Error setting CMEE\n");
            }
            modem_state = CHECK_GPS;
            reset_tx_flags(3);
        }
        else if (strstr(last_sent_cmd, ATGPS)) {
            if (strcmp(pch, ATOK)) {
                printf("Error setting GPS\n");
            }
            modem_state = GET_CIMI;
            reset_tx_flags(3);
        }
        else if (strstr(last_sent_cmd, ATCVHU)) {
            if (strcmp(pch, ATOK)) {
                printf("Error setting CVHU\n");
                ATH_ERR = 1;
            }
            modem_state = GET_NETWORK;
            reset_tx_flags(3);
        }
#endif
        else if (strstr(last_sent_cmd, ATCGDCONT) && !strcmp(pch, ATOK)) {
            // modem_state = LOG_INFO;
            modem_state = OK;
            modem_procedure = DONE;
            reset_tx_flags(3);
        }
        else if (strstr(last_sent_cmd, ATPOF)) {
            if (!strcmp(pch, ATOK)) {
                exit_control();
                if (modem_flags.pof)
                    modem_flags.pof = 0;
            } else {
                force_reboot();
            }
        }
    }
}

void do_wait(pthread_mutex_t *the_lock, pthread_cond_t *the_cond, unsigned int tsleep)
{
    pthread_mutex_lock(the_lock);
    if (tsleep)
    {
        clock_gettime(CLOCK_REALTIME, &ts_cond_tout);
        ts_cond_tout.tv_sec += tsleep;
        pthread_cond_timedwait(the_cond, the_lock, &ts_cond_tout);
    }
    else
        pthread_cond_wait(the_cond, the_lock);
    pthread_mutex_unlock(the_lock);
}
