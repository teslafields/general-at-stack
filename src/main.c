#include "at-interface.h"

int RUN = 1;
int EXIT = 0;
ModemUSBPorts modem_ports = {0};

void exit_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    if (modemfd.fd > 0)
        close(modemfd.fd);
    exit(1);
}

int main(int argc, char *argv[])
{
    /* This is used for systemd log */
    setbuf(stdout, NULL);

    int retries = 5;

    printf("MODEM-APP start up\n");
    while (get_tty_port_script(&modem_ports) != 0 && retries) {
        printf("Port not found.. %d\n", retries);
        sleep(3);
        retries--;
    }

    ucli_send_port_info((char) retries, &modem_ports);

    if (!retries)
        exit_error("USB ports error\n");

    pthread_t tid[2];
    rx_queue = createQueue(100);
    info_queue = createQueue(10);
    
    char port[20] = {0};
    sprintf(port, "/dev/ttyUSB%d", modem_ports.at);
    if (init_port(&modemfd, port, &s_portsettings[0], VMIN_USB, VTIME_USB) != 0)
        exit_error("Unable to open and setup TTY port!\n");
    if (pthread_create(&tid[0], NULL, read_at_data, NULL))
        exit_error("pthread_create read_at_data\n");
#ifdef PPP
    if (pthread_create(&tid[1], NULL, ppp_procedure, NULL))
        exit_error("pthread_create read_at_data\n");
#endif

    at_control();

    destroyQueue(rx_queue);
    destroyQueue(info_queue);
    pthread_join(tid[0], NULL);
#ifdef PPP
    pthread_join(tid[1], NULL);
#endif
    return 0;
}
