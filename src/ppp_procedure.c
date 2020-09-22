#include "at-interface.h"
#include "ppp_procedure.h"

PPPStatus ppp_status;

void reset_ppp_status() {
    memset(&ppp_status, 0, sizeof(ppp_status));
}

int is_running(pid_t pid)
{
    char cmd[CMD_SIZE];
    snprintf( cmd, CMD_SIZE, "ps -o pid | grep %d > /dev/null", pid);
    return system( cmd ) == 0;
}

pid_t get_ppp_pid()
{
    int ret;
    FILE *ps_stream;
    char ps_buffer[CMD_SIZE];
    char *pch, *ptr;

    ps_stream = popen("ps -o pid,comm | grep pppd", "r");
    if (!ps_stream)
        return 0;
    ret = fread(ps_buffer, 1, CMD_SIZE, ps_stream);
    pclose(ps_stream);
    if(ret <= 0)
        return ret;

    pch = strtok(ps_buffer, " ");
    if (pch)
        return strtol(pch, &ptr, 10);
    return 0;
}

void kill_ppp()
{
    pid_t ppp_pid = get_ppp_pid();
    if (ppp_pid)
    {
        int ret, timeout = 10;
        ret = kill(ppp_pid, SIGTERM);
        printf("pppd kill(%d) returned %d", ppp_pid, ret);
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
    FILE *fsrc, *fdest;
    fsrc = fopen(PPP_RESOLV, "r");
    if (!fsrc)
        return -1;
    fdest = fopen(RESOLV_CONF, "w+");
    if (!fdest)
        return -1;
    char content[CMD_SIZE] = {0};
    while (!feof(fsrc)) {
        fgets(content, sizeof(content)-1, fsrc);
        fwrite(content, sizeof(char), strlen(content), fdest);
    }
    fclose(fsrc);
    fclose(fdest);
    return 0;
}

void check_ppp_process()
{
    int code;
    pthread_mutex_lock(&ppp_lock);
    code = ppp_status.rc;
    pthread_mutex_unlock(&ppp_lock);
    if(code && (WIFEXITED(code) || WIFSIGNALED(code))) {
        code = WEXITSTATUS(code);
        switch(code) {
            case SUCCESS:
                printf("pppd: Detached or success terminated");
                break;
            case FATAL:
                printf("pppd: Fatal error: syscall or memory fail");
                break;
            case OPTION:
                printf("pppd: Options error");
                break;
            case PERMISSION:
                printf("pppd: Permission error");
                break;
            case DRIVER:
                printf("pppd: Driver error: kernel do not support ppp");
                break;
            case SIGNAL:
                printf("pppd: Signaled: SIGINT, SIGTERM or SIGHUP received");
                break;
            case LOCKPORT:
                printf("pppd: Lock port error");
                break;
            case OPENPORT:
                printf("pppd: Open port error");
                break;
            case CONNECTION:
                printf("pppd: Connect script failed");
                break;
            case COMMAND:
                printf("pppd: Command failed");
                break;
            case NEGOTIATION:
                printf("pppd: PPP negotiation failed");
                break;
            case PREAUTH:
                printf("pppd: Failed to authenticate itself");
                break;
            case IDLE:
                printf("pppd: Terminated because it was idle");
                break;
            case TIMEOUT:
                printf("pppd: Terminated because connect time limit");
                break;
            case INCALL:
                printf("pppd: Incoming call");
                break;
            case ECHOREQUEST:
                printf("pppd: Echo requests failed");
                break;
            case HANGUP:
                printf("pppd: Hangup");
                break;
            case LOOPBACK:
                printf("pppd: Serial loopback detected");
                break;
            case INIT:
                printf("pppd: Init script failed");
                break;
            case POSAUTH:
                printf("pppd: Failed to authenticate to the peer");
                break;
            default:
                printf("pppd exit code[%d]", code);
                break;
        }
        reset_ppp_status();
    }
    else {
        if (check_ppp_interface()) {
            if (!ppp_status.dns_updated) {
                printf("update dns[%d]\n", update_dns_servers());
                ppp_status.dns_updated = 1;
            }
        }
    }
}

void *ppp_procedure()
{
    int ret;
    char cmd[ARG_SIZE] = {0};
    reset_ppp_status();
    do {
        do_wait(&ppp_lock, &ppp_cond, 0);
        // kill_ppp();
        sprintf(cmd, "/usr/sbin/pppd ttyUSB%d call sim7100", modem_ports.ppp);
        printf("%s", cmd);
        ppp_status.run = 1;
        ret = system(cmd);
        pthread_mutex_lock(&ppp_lock);
        ppp_status.rc = ret;
        pthread_mutex_unlock(&ppp_lock);
    } while (RUN);
    return NULL;
}