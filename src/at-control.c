#include "at-interface.h"
#include "at-common.h"
#if defined (SIM7600) && !defined (SIM7100)
#include "SIM7600X-H.h"
#endif
#if defined (SIM7100) && !defined (SIM7600)
#include "SIM7100X.h"
#endif

enum modem_states
{
    MODEM_OFF,
    ECHO_OFF,
    MODEM_INFO,
    CHECK_CPIN,
    GET_CIMI,
    GET_ICCID,
    CHECK_CREG,
#ifdef SIM7100
    GET_UEINFO,
    GET_NETWORK,
#endif
    CHECK_CSQ,
    CHECK_COPS,
    CHECK_CGREG,
    SET_CGDCONT,
    START_PPP,
    OK
};
enum modem_states modem_state;

int REQUEST = 0;
int RETRIES = 3;

void decode_at_data();
void do_wait(pthread_mutex_t *the_lock, pthread_cond_t *the_cond, unsigned int tsleep);

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
            case MODEM_INFO:
                strcpy(tx_modem, ATI);
                break;
            case CHECK_CPIN:
                strcpy(tx_modem, ATCPIN);
                break;
            case GET_CIMI:
                strcpy(tx_modem, ATCIMI);
                break;
            case GET_ICCID:
                strcpy(tx_modem, ATICCID);
                break;
            case CHECK_CREG:
                strcpy(tx_modem, ATCREG);
                break;
#ifdef SIM7100
            case GET_UEINFO:
                strcpy(tx_modem, ATCPSI);
                break;
            case GET_NETWORK:
                strcpy(tx_modem, ATCNSMOD);
                break;
#endif
            case CHECK_CSQ:
                strcpy(tx_modem, ATCSQ);
                break;
            case CHECK_COPS:
                strcpy(tx_modem, ATCOPS);
                break;
            case SET_CGDCONT:
                sprintf(tx_modem, "%s=1,\"IP\",\"\"", ATCGDCONT);
                break;
            case START_PPP:
                printf("Starting START_PPPd\n");
                system("/usr/sbin/START_PPPd call sim7100");
                modem_state = MODEM_OFF;
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
        do_wait(&state_machine_lock, &state_machine_cond, 1);
    }
    close(modemfd.fd);
    return NULL;
}

void decode_at_data()
{
    char *pch = NULL;
    pch = dequeue(rx_queue);
    if (pch) {
        if (strcmp(pch, ATOK) && strcmp(pch, ATERR)) {
            if (!strcmp(pch, AT) || !strcmp(pch, ATE))
                decode_at_data();
            else if (!strcmp(last_sent_cmd, ATI)) {
                while (strcmp(pch, ATOK) && strcmp(pch, ATERR)) {
                    enqueue(info_queue, pch);
                    pch = dequeue(rx_queue);
                }
                modem_state = CHECK_CPIN;
                RETRIES = 3;
                REQUEST = 0;
            }
            else if (!strcmp(last_sent_cmd, ATCPIN)) {
                if (!strcmp(pch, "+CPIN: READY")) {
                    if (!strcmp(dequeue(rx_queue), ATOK)) {
                        modem_state = GET_CIMI;
                        RETRIES = 3;
                        REQUEST = 0;
                    }
                }
            }
            else if (!strcmp(last_sent_cmd, ATCIMI)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = GET_ICCID;
                    RETRIES = 3;
                    REQUEST = 0;
                }
            }
            else if (!strcmp(last_sent_cmd, ATICCID)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = CHECK_CREG;
                    RETRIES = 3;
                    REQUEST = 0;
                }
            }
            else if (!strcmp(last_sent_cmd, ATCREG)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = GET_UEINFO;
                    RETRIES = 3;
                    REQUEST = 0;
                }
            }
            else if (!strcmp(last_sent_cmd, ATCPSI)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = GET_NETWORK;
                    RETRIES = 3;
                    REQUEST = 0;
                }
            }
            else if (!strcmp(last_sent_cmd, ATCNSMOD)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = CHECK_CSQ;
                    RETRIES = 3;
                    REQUEST = 0;
                }
            }
            else if (!strcmp(last_sent_cmd, ATCSQ)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = CHECK_COPS;
                    RETRIES = 3;
                    REQUEST = 0;
                }
            }
            else if (!strcmp(last_sent_cmd, ATCOPS)) {
                if (!strcmp(dequeue(rx_queue), ATOK)) {
                    modem_state = SET_CGDCONT;
                    RETRIES = 3;
                    REQUEST = 0;
                }
            }
            else
                decode_at_data();
        }
        else if (!strcmp(last_sent_cmd, AT) && !strcmp(pch, ATOK)) {
            modem_state = ECHO_OFF;
            RETRIES = 3;
            REQUEST = 0;
        }
        else if (!strcmp(last_sent_cmd, ATE) && !strcmp(pch, ATOK)) {
            modem_state = MODEM_INFO;
            RETRIES = 3;
            REQUEST = 0;
        }
        else if (strstr(last_sent_cmd, ATCGDCONT) && !strcmp(pch, ATOK)) {
            modem_state = OK;
            RETRIES = 3;
            REQUEST = 0;
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
