#ifndef IOT_CLIENT_H_
#define IOT_CLIENT_H_

#include <MQTTClient.h>
#include "log.h"
#include "cJSON.h"
#include "utils.h"
#include "generic.h"
#include <time.h>

#if defined(SECURE_CONNECTION)
#define port (int)8883
#else
#define port (int)1883
#endif

#define topic_pre (char *)"/devices"
#define data_topic (char *)"/telemetry"
#define command_topic (char *)"/command"
#define event_topic (char *)"/events"

#define zclient_addNumber(...) zclient_addNumberToAsset(__VA_ARGS__, NULL)
#define zclient_addNumberToAsset(client, val_name, val_int, assetName, ...) zclient_addNumber(client, val_name, val_int, assetName)

#define zclient_addString(...) zclient_addStringToAsset(__VA_ARGS__, NULL)
#define zclient_addStringToAsset(client, val_name, val_string, assetName, ...) zclient_addString(client, val_name, val_string, assetName)

#define zclient_markDataPointAsError(...) zclient_markDataPointAsErrorInAsset(__VA_ARGS__, NULL)
#define zclient_markDataPointAsErrorInAsset(client, val_name, assetName, ...) zclient_markDataPointAsError(client, val_name, assetName)

typedef struct
{
    char *client_id;
    char *hostname;
    char *auth_token;
    char *MqttUserName;
    int retry_limit;
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

typedef struct
{
    cJSON *cJsonPayload;
    cJSON *data;
} Payload;

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
    Payload message;
#if defined(SECURE_CONNECTION)
    Certificates certs;
#endif
} IOTclient;

int zclient_init(IOTclient *iot_client, char *MQTTUserName, char *MQTTPassword, certsParseMode mode, char *ca_crt, char *client_cert, char *client_key, char *cert_password);
int zclient_connect(IOTclient *client);
int zclient_publish(IOTclient *client, char *payload);
int zclient_disconnect(IOTclient *client);
int zclient_subscribe(IOTclient *client, messageHandler on_message);
int zclient_yield(IOTclient *client, int time_out);
//TODO: to be tested and removed.
int zclient_reconnect(IOTclient *client);
int zclient_dispatch(IOTclient *client);

void zclient_addConnectionParameter(char *connectionParamKey, char *connectionParamValue);
int zclient_markDataPointAsError(IOTclient *client, char *val_name, char *assetName);
// In add String method , assetName parameter as optional.
int zclient_addString(IOTclient *client, char *val_name, char *val_string, char *assetName);
// In add Number method , assetName parameter as optional.
int zclient_addNumber(IOTclient *client, char *val_name, int val_int, char *assetName);
int zclient_setRetrycount(IOTclient *client, int count);
//char *zclient_getpayload();
#endif //# IOT_CLIENT_H_