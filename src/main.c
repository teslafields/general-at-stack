#include "at-interface.h"

int RUN = 1;
ModemUSBPorts modem_ports;

void exit_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    if (modemfd.fd > 0)
        close(modemfd.fd);
    exit(-1);
}

int main(int argc, char *argv[])
{
    /*if (argc < 2)
        exit_error("provide a USB device\n");
    */
    if (get_tty_port(&modem_ports) != 0)
        exit_error("USB ports error\n");

    pthread_t tid[2];
    rx_queue = createQueue(100);
    info_queue = createQueue(10);
    
    char port[20] = {0};
    snprintf(port, sizeof(port)-1, "/dev/ttyUSB%d", modem_ports.at);
    if (init_port(&modemfd, port, &s_portsettings[0], VMIN_USB, VTIME_USB) != 0)
        exit_error("Unable to open and setup TTY port!");
    if (pthread_create(&tid[0], NULL, read_at_data, NULL))
        exit_error("pthread_create read_at_data\n");

    at_control();

    destroyQueue(rx_queue);
    destroyQueue(info_queue);
    pthread_join(tid[0], NULL);
    return 0;
}
