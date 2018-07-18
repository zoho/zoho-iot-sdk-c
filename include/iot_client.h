#ifndef IOT_CLIENT_H_
#define IOT_CLIENT_H_

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

int init_config(IOTclient *iot_client, char *device_id, char *auth_token);
int iot_connect(IOTclient *client, char *host_name, int port);
int sendTestMessage(IOTclient *client, char *topic, char *payload);
int iot_disconnect(IOTclient *client);

#endif //# IOT_CLIENT_H_