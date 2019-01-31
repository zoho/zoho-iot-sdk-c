#ifndef IOT_CLIENT_H_
#define IOT_CLIENT_H_

#include <MQTTClient.h>
#include "log.h"
#include "cJSON.h"
#include "utils.h"
#include "generic.h"
#include <time.h>

#define hostname (char *)"iotdevices.localzoho.com"
// #define hostname (char *)"shahul-6029.csez.zohocorpin.com"
// #define hostname (char *)"keerthi-pt2055"

#if defined(SECURE_CONNECTION)
#define port (int)8883
#else
#define port (int)1883
#endif

#define topic_pre (char *)"/devices"
#define data_topic (char *)"/telemetry"
#define command_topic (char *)"/command"
#define event_topic (char *)"/events"
#define retry_limit 5
#define CONNECTION_ERROR -3

typedef struct
{
    char *device_id;
    char *auth_token;
} Config;

#if defined(SECURE_CONNECTION)
typedef struct
{
    char *ca_crt;
    char *client_cert;
    char *client_key;
    char *cert_password;
} Certificates;
#endif

typedef enum
{
    Initialized = 1,
    Connected = 2,
    Disconnected = 3
} ConnectionState;

typedef struct
{
    MQTTClient mqtt_client;
    Config config;
    ConnectionState current_state;
#if defined(SECURE_CONNECTION)
    Certificates certs;
#endif
} IOTclient;

int zclient_init(IOTclient *iot_client, char *device_id, char *auth_token, certsParseMode mode, char *ca_crt, char *client_cert, char *client_key, char *cert_password);
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