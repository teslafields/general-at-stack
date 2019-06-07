#include "at-interface.h"

int RUN = 1;

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

void init_port(struct pollfd *fds, char *device, struct termios *s_port, int vmin, int vtime)
{
    fds->fd = open(device, O_RDWR | O_NOCTTY);
    fds->events = POLLIN;
    if (fds->fd == -1)
        exit_error("Error opening device\n");
    int ret;
    ret = tcgetattr(fds->fd, s_port);
    s_port->c_cflag = B9600 | CS8 | CLOCAL | CREAD;
    s_port->c_iflag = 0;
    s_port->c_oflag = 0;
    s_port->c_lflag = 0;
    s_port->c_cc[VMIN]= vmin;
    s_port->c_cc[VTIME]= vtime;

    ret = tcflush(fds->fd, TCIFLUSH);
    ret = tcsetattr(fds->fd, TCSANOW, s_port);

    if (ret != 0)
        exit_error("Error! in Termios attributes\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        exit_error("provide a USB device\n");

    pthread_t tid[2];
    rx_queue = createQueue(100);
    info_queue = createQueue(10);

    init_port(&modemfd, argv[1], &s_portsettings[0], VMIN_USB, VTIME_USB);
    if (pthread_create(&tid[0], NULL, read_at_data, NULL))
        exit_error("pthread_create read_at_data\n");

    at_control();

    destroyQueue(rx_queue);
    destroyQueue(info_queue);
    pthread_join(tid[0], NULL);
    return 0;
}
