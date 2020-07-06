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

#define zclient_dispatchEventFromJSONString(...) zclient_dispatchEventFromJSONStringWithAsset(__VA_ARGS__, NULL)
#define zclient_dispatchEventFromJSONStringWithAsset(client, eventType, eventDescription, eventDataJSONString, assettName, ...) zclient_dispatchEventFromJSONString(client, eventType, eventDescription, eventDataJSONString, assettName)

#define zclient_addNumber(...) zclient_addNumberToAsset(__VA_ARGS__, NULL)
#define zclient_addNumberToAsset(client, key, val, assetName, ...) zclient_addNumber(client, key, val, assetName)

#define zclient_addString(...) zclient_addStringToAsset(__VA_ARGS__, NULL)
#define zclient_addStringToAsset(client, key, val_string, assetName, ...) zclient_addString(client, key, val_string, assetName)

#define zclient_markDataPointAsError(...) zclient_markDataPointAsErrorInAsset(__VA_ARGS__, NULL)
#define zclient_markDataPointAsErrorInAsset(client, key, assetName, ...) zclient_markDataPointAsError(client, key, assetName)

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
    INITIALIZED = 1,
    CONNECTED = 2,
    DISCONNECTED = 3
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

int zclient_init_config_file(IOTclient *iot_client, char *MqttConfigFilePath, certsParseMode mode);
int zclient_init(IOTclient *iot_client, char *MQTTUserName, char *MQTTPassword, certsParseMode mode, char *ca_crt, char *client_cert, char *client_key, char *cert_password);
int zclient_connect(IOTclient *client);
int zclient_publish(IOTclient *client, char *payload);
int zclient_disconnect(IOTclient *client);
int zclient_subscribe(IOTclient *client, messageHandler on_message);
int zclient_yield(IOTclient *client, int time_out);
//TODO: to be tested and removed.
int zclient_reconnect(IOTclient *client);
int zclient_dispatch(IOTclient *client);

int zclient_dispatchEventFromJSONString(IOTclient *client, char *eventType, char *eventDescription, char *eventDataJSONString, char *assettName);
int zclient_dispatchEventFromEventDataObject(IOTclient *client, char *eventType, char *eventDescription, char *assetName);
int zclient_addEventDataNumber(char *key, double val);
int zclient_addEventDataString(char *key, char *val);

void zclient_addConnectionParameter(char *connectionParamKey, char *connectionParamValue);
int zclient_markDataPointAsError(IOTclient *client, char *key, char *assetName);
// In add String method , assetName parameter as optional.
int zclient_addString(IOTclient *client, char *key, char *val_string, char *assetName);
// In add Number method , assetName parameter as optional.
int zclient_addNumber(IOTclient *client, char *key, double val_int, char *assetName);
int zclient_setRetrycount(IOTclient *client, int count);
//char *zclient_getpayload();
#endif //# IOT_CLIENT_H_