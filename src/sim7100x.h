#ifndef SIM100X_H
#define SIM100X_H

#define ATURC       "AT+CATR"
#define ATCPIN      "+CPIN"
#define ATICCID     "+ICCID"
#define ATCREG      "+CREG"
#define ATCPSI      "+CPSI"
#define ATCNSMOD    "+CNSMOD"
#define ATCSQ       "+CSQ"
#define ATCOPS      "+COPS"
#define ATCGREG     "+CGREG"
#define ATCGDCONT   "+CGDCONT"
#define ATRST       "+CRESET"
#define ATPOF       "+CPOF"
#define ATGPS       "+CGPS"
#define ATGPSINFO   "+CGPSINFO"

#define NETW_LEN 16

extern const char* networks[NETW_LEN];

#endif
