#ifndef AT_INTERFACE_H
#define AT_INTERFACE_H
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
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include "at-common.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 3

#define ATDELIM "\r\n\r\n"

/* Time values */
#define NS 1000000000
#define POLL_TOUT 3000
#define TSLEEP 5
#define TGPS 15
#define TCSQ 10
#define TINFO 20

/* RSSI boundaries */
#define RSSI_MIN 6
#define RSSI_UKW 99

/* Hysteresis */
#define PPP_HYST 2
#define CSQ_BUF 10
#define CSQ_THRSHLD 5

/* Buffer sizes */
#define MAX_SIZE 1024
#define CMD_SIZE (MAX_SIZE>>2)
#define ARG_SIZE (CMD_SIZE>>2)
#define CHK_SIZE ((ARG_SIZE>>1)-8)
#define PKG_GPS_LEN (sizeof(unsigned)*4+sizeof(char)*2+sizeof(float)*4)

/* Termios configuration */
#define VMIN_USB 255
#define VTIME_USB 2

/* Socket path */
#define UCLISOCKPATH "/run/core-app.sock"
#define DMESGLOG "/var/log/kern"
#define RESOLV_CONF "/etc/resolv.conf"
#define INFO_AGENT "/var/lib/losant-edge-agent/data/modem_info.json"

/* Modem procdeures */
enum ModemProcedure {
    SETUP,
    WAIT,
    DONE
};

/* General modem structs */
typedef struct CellularModemInfo {
    unsigned rssi;
    unsigned ber;
    int cpin;
    int creg;
    int cgreg;
    char cgmi[CHK_SIZE];
    char cgmm[CHK_SIZE];
    char cgmr[CHK_SIZE];
    char cgsn[CHK_SIZE];
    char cimi[CHK_SIZE];
    char iccid[CHK_SIZE];
    char cops[CHK_SIZE];
    char netw[12];
} ModemInfo;

typedef struct CellularModemFlags {
    unsigned char ring;
    unsigned char sms;
    unsigned char pof;
    unsigned char rst;
} ModemFlags;

/* GPS sturct */
typedef struct ModemGPSInfo {
    unsigned latdd;
    unsigned latmm;
    unsigned lngdd;
    unsigned lngmm;
    float latss;
    float lngss;
    float alt;
    float speed;
    char latdir;
    char lngdir;
    unsigned date;
    unsigned time;
    int course;
    double latraw;
    double lngraw;
} GPSInfo;

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

void exit_control();

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
void report_csq_to_agent(unsigned int rssi, unsigned int ber, char *network);

/* unix client functions */
int ucli_create_socket();
int ucli_send_data(void *data, unsigned size);
void ucli_close_socket();
int ucli_send_modem_info(ModemInfo *minfo, GPSInfo *ginfo, int warn);
int ucli_send_port_info(char result, ModemUSBPorts *ports);
int ucli_send_ppp_exit_code(int code);

enum ModemProcedure modem_procedure;
extern ATQueue* rx_queue;
extern ATQueue* info_queue;
extern ModemInfo modem_info;
extern ModemUSBPorts modem_ports;
extern GPSInfo gps_info;
extern PPPStatus ppp_status;
extern int REQUEST;
extern int RUN;
extern int EXIT;
extern int UCLISOCK;
#endif
