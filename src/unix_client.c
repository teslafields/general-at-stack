#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "unix_client.h"

int sock_fd = 0, UCLISOCK = 0;

int ucli_create_socket() {
    struct sockaddr_un addr;
    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
        return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, UCLISOCKPATH, sizeof(addr.sun_path)-1);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        return -1;

    UCLISOCK = 1;
    return 0;
}

int ucli_send_data(void *data, unsigned size) {
    unsigned n;
    if (!size)
        return -1;
    if ((n = write(sock_fd, data, size)) != size) {
        if (n > 0)
            return 0;
        else
            return -1;
    }
    return n;
}

void ucli_close_socket() {
    if (sock_fd > 0)
        close(sock_fd);
    UCLISOCK = 0;
}

int ucli_connect_and_send(void *data, unsigned size) {
    if (!data)
        return -3;
    int rc;
    if (ucli_create_socket() < 0) {
        printf("Connect(%s) failed\n", UCLISOCKPATH);
        return -2;
    }
    rc = ucli_send_data(data, size);
    ucli_close_socket();
    if (rc == -1)
        printf("Write(%s) error\n", UCLISOCKPATH);
    else if (rc == 0)
        printf("Partial write, total bytes: %d\n", rc);
    else
        printf("ucli: successfully sent %d bytes\n", rc);
    return rc;
}

void ucli_mount_package(char cmd, char *data) {
    if (!data)
        return;
    data[0] = PKG_START;
    data[1] = cmd;
}

int ucli_send_port_info(char result, ModemUSBPorts *ports) {
    static char ports_tx[PKG_PORTS_LEN];
    memset(ports_tx, 0, PKG_MODEM_LEN);
    ports_tx[2] = result;
    memcpy(ports_tx + 3, ports, sizeof(ModemUSBPorts));
    ucli_mount_package(CMD_PORTS_INF, ports_tx);
    return ucli_connect_and_send((void *) ports_tx, PKG_PORTS_LEN);
}

int ucli_send_modem_info(ModemInfo *minfo, GPSInfo *ginfo) {
    static char modem_tx[PKG_MODEM_LEN];
    memset(modem_tx, 0, PKG_MODEM_LEN);
    int n = sizeof(ModemInfo);
    memcpy(modem_tx + PKG_HEAD_LEN, minfo, n);
    n += PKG_HEAD_LEN;
    if (ginfo->date > 0) {
        memcpy(modem_tx + n, ginfo, PKG_GPS_LEN);
        n += PKG_GPS_LEN;
    }
    ucli_mount_package(CMD_MODEM_INF, modem_tx);
    return ucli_connect_and_send((void *) modem_tx, n);
}
