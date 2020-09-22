#include "at-interface.h"


ATQueue* rx_queue;
ATQueue* info_queue;

char* strtoke(char *str, const char *delim)
{
    static char *start = NULL;
    char *token = NULL;
    if (str)
        start = str;
    if (!start)
        return NULL;
    token = start;
    start = strpbrk(start, delim);
    if (start)
        *start++ = '\0';
    return token;
}

void enqueue_at_data(ATQueue *queue, char* buffer, char* delimiter)
{
    char* pch;
    pch = strtoke(buffer, delimiter);
    while (pch != NULL)
    {
        if (*pch)
        {
            enqueue(queue, pch);
        }
        pch = strtoke(NULL, delimiter);
    }
}

void *read_at_data()
{
    int ret, n, timeout;
    while (RUN)
    {
        ret = poll(&modemfd, 1, POLL_TOUT);
        if (ret < 0)
        {
            printf("poll() error\n");
            break;
        }
        else if (ret > 0)
        {
            n = read(modemfd.fd, rx_modem, MAX_SIZE);
            if(n > 0)
            {
                printf("<-- [%3d] \"", n);
                for (ret=0; ret<n; ret++)
                {
                    if (rx_modem[ret] >= 0x20)
                        printf("%c", rx_modem[ret]);
                    else
                        printf("\\x%x", rx_modem[ret]);
                }
                printf("\"\n");
                enqueue_at_data(rx_queue, rx_modem, ATDELIM);
                memset(rx_modem, 0, MAX_SIZE);
                pthread_cond_signal(&state_machine_cond);
            }
            else
            {
                printf("read() error\n");
                RUN = 0;
                break;
            }
        }
    }
    return NULL;
}
