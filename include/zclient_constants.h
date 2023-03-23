#ifndef ZCLIENT_CONSTANTS_H_
#define ZCLIENT_CONSTANTS_H_

#define RECIEVED_ACK_CODE 1000
typedef enum
{
    REFERENCE = 0,
    EMBED = 1
} certsParseMode;

typedef enum
{
    ZSUCCESS = 0,
    ZFAILURE = -1,
    ZCONNECTION_ERROR = -2
} transactionStatus;

#endif //ZCLIENT_CONSTANTS_H_