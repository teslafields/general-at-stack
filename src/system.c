#include "at-interface.h"


int get_tty_port(ModemUSBPorts *ports, int iteration){
    static char fname[25] = {0};
    if (!iteration)
        sprintf(fname, "%s.log", DMESGLOG);
    else
        sprintf(fname, "%s.log.%d", DMESGLOG, iteration);
    printf("opening %s\n", fname);
    FILE *fdmesg = fopen(fname, "r");
    long size;
    if (!fdmesg) {
        printf("Error opening %s\n", fname);
        return -1;
    }
    if (fseek(fdmesg, 0, SEEK_END) != 0) {
        printf("Error seting file position\n");
        return -1;
    }
    size = ftell(fdmesg);
    if (size == -1) {
        printf("Error telling file size\n");
        return -1;
    }
    if (fseek(fdmesg, 0, SEEK_SET) != 0) {
        printf("Error seting file position\n");
        return -1;
    }
    char content[256] = {0}, filtered[256] = {0};
    while (!feof(fdmesg)) {
        fgets(content, sizeof(content)-1, fdmesg);
        if (strstr(content, "GSM modem"))
            strcpy(filtered, content);
    }
    char *pch = strstr(filtered, "ttyUSB");
    if (!pch) {
        if (iteration < 5) {
            iteration++;
            get_tty_port(ports, iteration);
            if (ports->at)
                return 0;
        }
        else
            printf("TTY not found!\n");
            return -1;
    }
    if (strstr(filtered, "disconnected from")) {
        printf("TTY disconnected!\n");
        return -1;
    }
#ifdef SIM7100
    ports->audio = (int) strtol(pch + strlen("ttyUSB"), NULL, 10);
    ports->ppp = ports->audio - 1;
    ports->at = ports->ppp - 1;
    ports->gps = ports->at - 1;
    ports->diag = ports->gps - 1;
    printf("diag=ttyUSB%d, gps=ttyUSB%d, at=ttyUSB%d, ppp=ttyUSB%d, audio=ttyUSB%d\n", 
            ports->diag, ports->gps, ports->at, ports->ppp, ports->audio);
#endif
    return 0;
}

int init_port(struct pollfd *fds, char *device, struct termios *s_port, int vmin, int vtime)
{
    fds->fd = open(device, O_RDWR | O_NOCTTY);
    fds->events = POLLIN;
    if (fds->fd == -1)
        return -1;
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
        return -1;
    return 0;
}
