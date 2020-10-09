#include "at-interface.h"

#define PKG_START      '!'
#define PKG_HEAD_LEN   2
#define PKG_MODEM_LEN  (sizeof(ModemInfo)+PKG_GPS_LEN+PKG_HEAD_LEN)
#define PKG_PORTS_LEN  (sizeof(ModemUSBPorts)+1+PKG_HEAD_LEN)
#define CMD_MODEM_WRN  9
#define CMD_MODEM_INF  10
#define CMD_PORTS_INF  11


