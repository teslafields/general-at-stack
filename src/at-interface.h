#include <termios.h>
#include <poll.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include "at-common.h"

#define ATDELIM "\r\n\r\n"

/* Time values */
#define NS 1000000000
#define POLL_TOUT 3000
#define TSLEEP 5
#define TCSQ 30

/* RSSI boundaries */
#define RSSI_MIN 6
#define RSSI_UKW 99

/* Hysteresis */
#define PPP_HYST 2

/* Buffer sizes */
#define MAX_SIZE 1024
#define CMD_SIZE 256
#define ARG_SIZE 64

/* Termios configuration */
#define VMIN_USB 255
#define VTIME_USB 2

#define DMESGLOG "/var/log/kern"
#define RESOLV_CONF "/etc/resolv.conf"

/* General modem structs */
typedef struct CellularModemInfo {
    unsigned int rssi;
    unsigned int ber;
    int cpin;
    int creg;
    int cgreg;
    char cgmi[ARG_SIZE];
    char cgmm[ARG_SIZE];
    char cgmr[ARG_SIZE];
    char cgsn[ARG_SIZE];
    char cimi[ARG_SIZE];
    char iccid[ARG_SIZE];
    char cops[ARG_SIZE];
    char netw[ARG_SIZE];
} ModemInfo;

typedef struct CellularModemFlags {
    unsigned char ring;
    unsigned char sms;
    unsigned char pof;
    unsigned char rst;
} ModemFlags;

/* Custom structs */
typedef struct Queue
{
    int front, rear, size, capacity;
    char** cmd;
} ATQueue;


typedef struct USBPorts {
    int diag;
    int gps;
    int at;
    int ppp;
    int audio;
} ModemUSBPorts;

typedef struct PPPStatus {
    int rc;
    int run;
    int dns_updated;
} PPPStatus;

struct pollfd modemfd;
struct termios s_portsettings[2];


/* Syncronization purpose */
pthread_cond_t state_machine_cond, ppp_cond;
pthread_mutex_t state_machine_lock, ppp_lock;

/* I/O buffers */
char rx_modem[MAX_SIZE];
char tx_modem[CMD_SIZE];
char last_sent_cmd[CMD_SIZE];

/* AT functions */
void *at_control();
void *read_at_data();

void do_wait(pthread_mutex_t *the_lock, pthread_cond_t *the_cond, unsigned int tsleep);

/* PPP function */
void *ppp_procedure();
int check_ppp_process();

/* Queue functions */
ATQueue* createQueue(unsigned capacity);
void destroyQueue(ATQueue* queue);
void enqueue(ATQueue* queue, char* item);
char* dequeue(ATQueue* queue);
void clearQueue(ATQueue* queue);
int isEmpty(ATQueue* queue);
int isFull(ATQueue* queue);
char* front(ATQueue* queue);
void create_queues();
void destroy_queues();

/* system functions */
int get_tty_port_script(ModemUSBPorts *ports);
int get_tty_port(ModemUSBPorts *ports, int iteration);
int init_port(struct pollfd *fds, char *device, struct termios *s_port, int vmin, int vtime);

extern ATQueue* rx_queue;
extern ATQueue* info_queue;
extern ModemInfo modem_info;
extern ModemUSBPorts modem_ports;
extern PPPStatus ppp_status;
extern int RUN;
extern int EXIT;
