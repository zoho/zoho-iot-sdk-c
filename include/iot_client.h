#ifndef IOT_CLIENT_H_
#define IOT_CLIENT_H_

#include <MQTTClient.h>
#include "log.h"
#include "cJSON.h"

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
int zclient_connect(IOTclient *client, char *host_name, int port,char *ca_crt, char* client_cert,char *client_key,char* cert_password);
int zclient_publish(IOTclient *client, char *topic, char *payload);
int zclient_disconnect(IOTclient *client);
int zclient_subscribe(IOTclient *client, char *topic, messageHandler on_message);
int zclient_yield(IOTclient *client, int time_out);

cJSON *zclient_createJsonObject();
int zclient_addString(cJSON *monitor, char *val_name, char *val_string);
int zclient_addNumber(cJSON *monitor, char *val_name, int val_int);

#endif //# IOT_CLIENT_H_