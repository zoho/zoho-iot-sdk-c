#ifndef IOT_CLIENT_H_
#define IOT_CLIENT_H_

#include <MQTTClient.h>

typedef struct
{
    char *device_id;
    char *auth_token;
    char *username;
    char *password;
    
} Config;

typedef struct
{
    Client mqtt_client;
    Config dev_config;

} IOTclient;

int zclient_init(IOTclient *iot_client, char *device_id, char *auth_token, char* username ,char* password);
int zclient_connect(IOTclient *client, char *host_name, int port);
int zclient_publish(IOTclient *client, char *topic, char *payload);
int zclient_disconnect(IOTclient *client);
int zclient_subscribe(IOTclient *client, char *topic, messageHandler on_message);
int zclient_yield(IOTclient *client, int time_out);

#endif //# IOT_CLIENT_H_