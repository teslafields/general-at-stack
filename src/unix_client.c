#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "at-interface.h"

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
    int rc;
    if (ucli_create_socket() < 0)
        return -2;
    rc = ucli_send_data(data, size);
    ucli_close_socket();
    return rc;
}
