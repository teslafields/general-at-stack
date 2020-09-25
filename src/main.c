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

    printf("MODEM-APP start up\n");
    if (get_tty_port_script(&modem_ports) != 0)
        exit_error("USB ports error\n");

    pthread_t tid[2];
    rx_queue = createQueue(100);
    info_queue = createQueue(10);
    
    char port[20] = {0};
    sprintf(port, "/dev/ttyUSB%d", modem_ports.at);
    if (init_port(&modemfd, port, &s_portsettings[0], VMIN_USB, VTIME_USB) != 0)
        exit_error("Unable to open and setup TTY port!");
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
    if (EXIT)
        sleep(5);
    return 0;
}
