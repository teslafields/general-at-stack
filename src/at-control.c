#include "at-interface.h"
#include "at-common.h"
#if defined (SIM7600) && !defined (SIM7100)
#include "SIM7600X-H.h"
#endif
#if defined (SIM7100) && !defined (SIM7600)
#include "sim7100x.h"
#endif

enum modem_states {
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
    GET_UEINFO,
    GET_NETWORK,
#endif
    CHECK_CSQ,
    CHECK_COPS,
    CHECK_CGREG,
    LOG_INFO,
    SET_CGDCONT,
    START_PPP,
    OK
};
enum modem_states modem_state;

int REQUEST = 0;
int RETRIES = 3;

void decode_at_data();
void do_wait(pthread_mutex_t *the_lock, pthread_cond_t *the_cond, unsigned int tsleep);


void init_global() {
    memset(&modem_info, 0, sizeof(modem_info));
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

int write_at_data()
{
    int n;
    n = strlen(tx_modem);
    if (n > MAX_SIZE-1 || n <= 0 || tx_modem[0] == 0) {
        return 0;
    }
    REQUEST = 1;
    printf("--> [%3d] %s\n", n+2, tx_modem);
    tx_modem[n] = 0xD;
    tx_modem[n+1] = 0xA;
    n = n + 2;
    n = write(modemfd.fd, tx_modem, n);
    if (n > 0) {
        memset(tx_modem, 0, CMD_SIZE);
        RETRIES--;
    }
    else
        printf("USB write() error\n");
    return n;
}

void *at_control()
{
    init_global();
    while (RUN)
    {
        decode_at_data();
        switch (modem_state)
        {
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
            case GET_UEINFO:
                sprintf(tx_modem, "AT%s?", ATCPSI);
                break;
            case GET_NETWORK:
                sprintf(tx_modem, "AT%s?", ATCNSMOD);
                break;
#endif
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
                printf("Starting START_PPPd\n");
                system("/usr/sbin/START_PPPd call sim7100");
                modem_state = MODEM_OFF;
                break;
            case LOG_INFO:
                log_modem_info();
                modem_state = OK;
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
        }
        do_wait(&state_machine_lock, &state_machine_cond, 5);
    }
    close(modemfd.fd);
    return NULL;
}

void decode_at_data()
{
    int n, dummy;
    char *pch = NULL, payload[CMD_SIZE] = {0}, fmt[ARG_SIZE] = {0};
    pch = dequeue(rx_queue);
    if (pch) {
        strcpy(payload, pch);
        if (strcmp(pch, ATOK) && strcmp(pch, ATERR)) {
            if (!strcmp(pch, AT) || !strcmp(pch, ATE))
                decode_at_data();
            else if (strstr(pch, ATCPIN)) {
                if (strstr(pch, "READY") && !strcmp(dequeue(rx_queue), ATOK)) {
                    modem_info.cpin = 1; 
                    if (strstr(last_sent_cmd, ATCPIN)) {
                        modem_state = GET_ICCID;
                        reset_tx_flags(3);
                    }
                }
                else {
                    modem_state = MODEM_OFF;
                    modem_info.cpin = 0;
                }
            }
            else if (strstr(pch, ATCGMR)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    strncpy(modem_info.cgmr, payload+strlen(ATCGMR)+2, ARG_SIZE-1);
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
                    strncpy(modem_info.iccid, payload+strlen(ATICCID)+2, ARG_SIZE-1);
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
                                modem_state = GET_NETWORK;
                                reset_tx_flags(3);
#endif
                                break;
                            case 0:
                            case 3:
                            case 4:
                                modem_state = MODEM_OFF;
                                break;
                            default:
                                break;
                        }
                    }
                }
                else
                    modem_state = MODEM_OFF;
            }
            else if (strstr(pch, ATCNSMOD)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    sprintf(fmt, "%s: %%*d,%%d", ATCNSMOD);
                    sscanf(payload, fmt, &dummy);
                    if (dummy >= 0 && dummy < sizeof(networks)/sizeof(char *)) {
                        strcpy(modem_info.netw, networks[dummy]);
                    }
                    if (strstr(last_sent_cmd, ATCNSMOD)) {
                        modem_state = CHECK_CSQ;
                        reset_tx_flags(3);
                    }
                }
            }
            else if (strstr(pch, ATCSQ)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    sprintf(fmt, "%s: %%u,%%u", ATCSQ);
                    n = sscanf(payload, fmt, &modem_info.rssi, &modem_info.ber);
                    if (n == -1) {
                        modem_info.rssi = 99;
                        modem_info.ber = 99;
                    }
                    if (strstr(last_sent_cmd, ATCSQ)) {
                        modem_state = CHECK_COPS;
                        reset_tx_flags(3);
                    }
                }
            }
            else if (strstr(pch, ATCOPS)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    sprintf(fmt, "%s: %%*d,%%*d,%%[^,]", ATCOPS);
                    n = sscanf(payload, fmt, modem_info.cops); 
                    if (strstr(last_sent_cmd, ATCOPS)) {
                        modem_state = SET_CGDCONT;
                        reset_tx_flags(3);
                    }
                }
            }
            else if (strstr(last_sent_cmd, ATCGMI)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = GET_CGMM;
                    reset_tx_flags(3);
                    strncpy(modem_info.cgmi, payload, ARG_SIZE-1);
                }
            }
            else if (strstr(last_sent_cmd, ATCGMM)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = GET_CGMR;
                    reset_tx_flags(3);
                    strncpy(modem_info.cgmm, payload, ARG_SIZE-1);
                }
            }
            else if (strstr(last_sent_cmd, ATCGMR)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = GET_CGSN;
                    reset_tx_flags(3);
                    strncpy(modem_info.cgmr, payload+strlen(ATCGMR)+2, ARG_SIZE-1);
                }
            }
            else if (strstr(last_sent_cmd, ATCGSN)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = GET_CIMI;
                    reset_tx_flags(3);
                    strncpy(modem_info.cgsn, payload, ARG_SIZE-1);
                }
            }
            else if (strstr(last_sent_cmd, ATCIMI)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = CHECK_CPIN;
                    reset_tx_flags(3);
                    strncpy(modem_info.cimi, payload, ARG_SIZE-1);
                }
            }
            else
                decode_at_data();
        }
        else if (!strcmp(last_sent_cmd, AT) && !strcmp(pch, ATOK)) {
            modem_state = ECHO_OFF;
            reset_tx_flags(3);
        }
        else if (!strcmp(last_sent_cmd, ATE) && !strcmp(pch, ATOK)) {
            modem_state = GET_CGMI;
            reset_tx_flags(3);
        }
        else if (strstr(last_sent_cmd, ATCGDCONT) && !strcmp(pch, ATOK)) {
            modem_state = LOG_INFO;
            reset_tx_flags(3);
        }
    }
}

void do_wait(pthread_mutex_t *the_lock, pthread_cond_t *the_cond, unsigned int tsleep)
{
    pthread_mutex_lock(the_lock);
    if (tsleep)
    {
        clock_gettime(CLOCK_REALTIME, &ts_timeout);
        ts_timeout.tv_sec += tsleep;
        pthread_cond_timedwait(the_cond, the_lock, &ts_timeout);
    }
    else
        pthread_cond_wait(the_cond, the_lock);
    pthread_mutex_unlock(the_lock);
}
