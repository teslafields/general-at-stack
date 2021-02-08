#include <sys/socket.h>
#include <sys/un.h>
#include "unix_client.h"

int sock_fd = 0, UCLISOCK = 0, LOG_ONCE = 1;

int ucli_create_socket() {
    struct stat sb;
    struct sockaddr_un addr;

    if (!(stat(UCLISOCKPATH, &sb) == 0 && S_ISSOCK(sb.st_mode)))
        return -1;

    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
        return -2;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, UCLISOCKPATH, sizeof(addr.sun_path)-1);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        return -3;

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
    int rc = ucli_create_socket();
    if (rc < 0) {
        if (LOG_ONCE) {
            if (rc == -1)
                printf("Socket path(%s) error\n", UCLISOCKPATH);
            else if (rc == -2)
                printf("Socket create(%s) failed\n", UCLISOCKPATH);
            else
                printf("Connect(%s) failed\n", UCLISOCKPATH);
            LOG_ONCE = 0;
        }
        return -2;
    }
    rc = ucli_send_data(data, size);
    ucli_close_socket();
    if (rc == -1)
        printf("Write(%s) error\n", UCLISOCKPATH);
    else if (rc == 0)
        printf("Partial write, total bytes: %d\n", rc);
    // else
    //     printf("ucli: successfully sent %d bytes\n", rc);
    LOG_ONCE = 1;
    return rc;
}

void ucli_mount_package(char cmd, char *data) {
    if (!data)
        return;
    data[0] = PKG_START;
    data[1] = cmd;
}

int ucli_send_port_info(char result, ModemUSBPorts *ports) {
    static char package_tx[PKG_PORTS_LEN];
    memset(package_tx, 0, PKG_PORTS_LEN);
    package_tx[2] = result;
    memcpy(package_tx + 3, ports, sizeof(ModemUSBPorts));
    ucli_mount_package(CMD_PORTS_INF, package_tx);
    return ucli_connect_and_send((void *) package_tx, PKG_PORTS_LEN);
}

int ucli_send_modem_info(ModemInfo *minfo, GPSInfo *ginfo, int warn) {
    static char package_tx[PKG_MODEM_LEN];
    memset(package_tx, 0, PKG_MODEM_LEN);
    package_tx[2] = VERSION_MAJOR;
    package_tx[3] = VERSION_MINOR;
    int n = sizeof(ModemInfo);
    memcpy(package_tx + PKG_HEAD_LEN, minfo, n);
    n += PKG_HEAD_LEN;
    if (ginfo->date > 0) {
        memcpy(package_tx + n, ginfo, PKG_GPS_LEN);
        n += PKG_GPS_LEN;
    }
    if (warn)
        ucli_mount_package(CMD_MODEM_WRN, package_tx);
    else
        ucli_mount_package(CMD_MODEM_INF, package_tx);
    return ucli_connect_and_send((void *) package_tx, n);
}

int ucli_send_ppp_exit_code(int code) {
    static char package_tx[PKG_PPP_E_LEN];
    memset(package_tx, 0, PKG_PPP_E_LEN);
    memcpy(package_tx + PKG_HEAD_LEN, &code, sizeof(int));
    ucli_mount_package(CMD_PPP_ECODE, package_tx);
    return ucli_connect_and_send((void *) package_tx, PKG_PPP_E_LEN);
}

