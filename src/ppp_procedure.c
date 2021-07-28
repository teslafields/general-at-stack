#include "at-interface.h"
#include "ppp_procedure.h"

PPPStatus ppp_status;

void reset_ppp_status() {
    memset(&ppp_status, 0, sizeof(ppp_status));
}

pid_t get_ppp_pid() {
    int ret;
    pid_t ppp;
    FILE *ps_stream;
    char ps_buffer[CMD_SIZE];
    ps_stream = popen("ps -C pppd -o pid", "r");
    if (!ps_stream)
        return -1;
    ret = fread(ps_buffer, 1, CMD_SIZE, ps_stream);
    pclose(ps_stream);
    if(ret <= 0)
        return -1;
    if (sscanf(ps_buffer, "%*[^\n]%d", &ppp) != 1)
        return -1;
    return ppp;
}

void kill_ppp()
{
    printf("checking if ppp is running\n");
    pid_t ppp_pid = get_ppp_pid();
    if (ppp_pid > 0) {
        int ret, timeout = 10;
        ret = kill(ppp_pid, SIGTERM);
        printf("pppd kill(%d) returned %d\n", ppp_pid, ret);
        while (kill(ppp_pid, 0) == 0 && --timeout)
            sleep(1);
    }
}

int check_ppp_interface()
{
    int ret = 0;
    struct ifaddrs *addrs, *tmp;
    getifaddrs(&addrs);
    tmp = addrs;
    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
            if (strstr(tmp->ifa_name, "ppp"))
                ret = 1;
        }
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);
    return ret;
}

int update_dns_servers() {
    FILE *fsrc = NULL, *fdest = NULL;
    fsrc = fopen(PPP_RESOLV, "r");
    char *opw = "w+", *opa = "a+", *op = NULL;
    if (!fsrc) {
        printf("Error fopen read %s\n", PPP_RESOLV);
        op = opw;
    }
    else
        op = opa;
    fdest = fopen(RESOLV_CONF, op);
    if (!fdest) {
        printf("Error fopen write %s\n", RESOLV_CONF);
        return -1;
    }
    char content[CMD_SIZE] = {0};
    int dns_nr = 2;
    if (fsrc) {
        while (!feof(fsrc) && dns_nr--) {
            fgets(content, sizeof(content)-1, fsrc);
            fwrite(content, sizeof(char), strlen(content), fdest);
        }
        fclose(fsrc);
    }
    /* This might be configurable in the future */
    fwrite("\n", sizeof(char), strlen("\n"), fdest);
    fwrite(DNS1, sizeof(char), strlen(DNS1), fdest);
    fwrite(DNS2, sizeof(char), strlen(DNS2), fdest);
    fclose(fdest);
    return 0;
}

int check_ppp_process() {
    int code;
    pthread_mutex_lock(&ppp_lock);
    code = ppp_status.rc;
    pthread_mutex_unlock(&ppp_lock);
    if(code && (WIFEXITED(code) || WIFSIGNALED(code))) {
        code = WEXITSTATUS(code);
        switch(code) {
            case SUCCESS:
                printf("pppd: Detached or success terminated\n");
                break;
            case FATAL:
                printf("pppd: Fatal error: syscall or memory fail\n");
                break;
            case OPTION:
                printf("pppd: Options error\n");
                break;
            case PERMISSION:
                printf("pppd: Permission error\n");
                break;
            case DRIVER:
                printf("pppd: Driver error: kernel do not support ppp\n");
                break;
            case SIGNAL:
                printf("pppd: Signaled: SIGINT, SIGTERM or SIGHUP received\n");
                break;
            case LOCKPORT:
                printf("pppd: Lock port error\n");
                break;
            case OPENPORT:
                printf("pppd: Open port error\n");
                break;
            case CONNECTION:
                printf("pppd: Connect script failed\n");
                break;
            case COMMAND:
                printf("pppd: Command failed\n");
                break;
            case NEGOTIATION:
                printf("pppd: PPP negotiation failed\n");
                break;
            case PREAUTH:
                printf("pppd: Failed to authenticate itself\n");
                break;
            case IDLE:
                printf("pppd: Terminated because it was idle\n");
                break;
            case TIMEOUT:
                printf("pppd: Terminated because connect time limit\n");
                break;
            case INCALL:
                printf("pppd: Incoming call\n");
                break;
            case ECHOREQUEST:
                printf("pppd: Echo requests failed\n");
                break;
            case HANGUP:
                printf("pppd: Hangup\n");
                break;
            case LOOPBACK:
                printf("pppd: Serial loopback detected\n");
                break;
            case INIT:
                printf("pppd: Init script failed\n");
                break;
            case POSAUTH:
                printf("pppd: Failed to authenticate to the peer\n");
                break;
            default:
                printf("pppd: unmapped exit code[%d]\n", code);
                break;
        }
        reset_ppp_status();
        code = 1;
    }
    else {
        if (check_ppp_interface()) {
            if (!ppp_status.dns_updated) {
#ifndef RESOLVED
                printf("ppp: update dns[%d]\n", update_dns_servers());
#endif
                ppp_status.dns_updated = 1;
            }
        }
        code = 0;
    }
    return code;
}

void *ppp_procedure() {
    int ret;
    char cmd[ARG_SIZE] = {0};
    reset_ppp_status();
    do {
        do_wait(&ppp_lock, &ppp_cond, 0);
        if (EXIT)
            break;
        kill_ppp();
        sprintf(cmd, "/usr/sbin/pppd ttyUSB%d call simpeer > /dev/null", modem_ports.ppp);
        printf("%s\n", cmd);
        ppp_status.run = 1;
        ret = system(cmd);
        pthread_mutex_lock(&ppp_lock);
        ppp_status.rc = ret;
        pthread_mutex_unlock(&ppp_lock);
    } while (RUN);
    printf("Exiting ppp_procedure()\n");
    return NULL;
}
