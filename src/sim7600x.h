#ifndef SIM7600X_H
#define SIM7600X_H

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
#define ATSIM       "+SIMCARD"
#define ATCVHU      "+CVHU"
#define ATSTIN      "+STIN"
#define ATCMEE      "+CMEE"
#define ATCME       "+CME ERROR"
#define ATCMS       "+CMS ERROR"

#define NETW_LEN 16

extern const char* networks[NETW_LEN];

#endif