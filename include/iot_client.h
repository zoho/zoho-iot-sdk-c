// #ifndef IOT_CLIENT_H_
// #define IOT_CLIENT_H_

#include <MQTTClient.h>

typedef struct
{
    char *device_id;
    char *auth_token;
} Config;

typedef struct
{
    Client mqtt_client;
    Config dev_config;

} IOTclient;

// #endif //# IOT_CLIENT_H_