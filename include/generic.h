#ifndef GENERIC_H_
#define GENERIC_H_

#define COMMAND_RECIEVED_ACK_CODE 1000
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

#endif //GENERIC_H_