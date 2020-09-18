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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <linux/types.h>

/* Modem model (defined in Makefile) */
// #define SIM7600

#define ATDELIM "\r\n\r\n"
/* Time values */
#define NS 1000000000
#define POLL_TOUT 3000
#define TSLEEP 5
#define MAX_SIZE 1024
#define CMD_SIZE 256

#define VMIN_USB 255
#define VTIME_USB 5

#define DMESGLOG "/var/log/kern.log"

char at_usb_device[20];
struct pollfd modemfd, serialfd;
struct termios s_portsettings[2];
struct timespec ts_timeout;

/* Syncronization purpose */
pthread_cond_t state_machine_cond;
pthread_mutex_t state_machine_lock;

/* I/O buffers */
char rx_modem[MAX_SIZE];
char tx_modem[CMD_SIZE];
char last_sent_cmd[CMD_SIZE];

/* Custom structs */
typedef struct Queue
{
    int front, rear, size, capacity;
    char** cmd;
} ATQueue;
ATQueue* rx_queue;
ATQueue* info_queue;

typedef struct USBPorts {
    int diag;
    int gps;
    int at;
    int ppp;
    int audio;
} ModemUSBPorts;

/* AT functions */
void *at_control();
void *read_at_data();

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
int get_tty_port(ModemUSBPorts *ports);
int init_port(struct pollfd *fds, char *device, struct termios *s_port, int vmin, int vtime);

extern ModemUSBPorts modem_ports;
extern int RUN;
