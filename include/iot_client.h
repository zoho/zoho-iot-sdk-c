#ifndef IOT_CLIENT_H_
#define IOT_CLIENT_H_

#include <MQTTClient.h>
#include "log.h"
#include "cJSON.h"
#include "utils.h"


#define hostname (char *)"172.22.142.33"
#if defined(SECURE_CONNECTION)
    #define port (int)8883
#else
    #define port (int)1883
#endif

#define data_topic (char*)"test_topic9876" 
#define command_topic (char*)"test_topic9877"

typedef struct
{
    char *device_id;
    char *auth_token;
    char *username;
    char *password;

} Config;

#if defined(SECURE_CONNECTION)
typedef struct certificates
{
    char *ca_crt;
    char *client_cert;
    char *client_key;
    char *cert_password;
} Certificates;
#endif

typedef struct
{
    Client mqtt_client;
    Config config;
#if defined(SECURE_CONNECTION)
    Certificates certs;
#endif

} IOTclient;

int zclient_init(IOTclient *iot_client, char *device_id, char *auth_token, char *username, char *password, char *ca_crt, char *client_cert, char *client_key, char *cert_password);
int zclient_connect(IOTclient *client);
int zclient_publish(IOTclient *client, char *payload);
int zclient_disconnect(IOTclient *client);
int zclient_subscribe(IOTclient *client, messageHandler on_message);
int zclient_yield(IOTclient *client, int time_out);
//TODO: to be tested and removed.
int zclient_reconnect(IOTclient *client);
int zclient_dispatch(IOTclient *client);

int zclient_addString(char *val_name, char *val_string);
int zclient_addNumber(char *val_name, int val_int);
//char *zclient_getpayload();
#endif //# IOT_CLIENT_H_