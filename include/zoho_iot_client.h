#ifndef ZOHO_IOT_CLIENT_H_
#define ZOHO_IOT_CLIENT_H_
#if defined(Z_SECURE_CONNECTION)
#include "tls_network.h"
#else
#include <MQTTLinux.h>
#endif
#include <MQTTClient.h>
#define MQTT_TASK 1
#include "zoho_log.h"
#include "cJSON.h"
#include "zoho_utils.h"
#include "zclient_constants.h"
#include <time.h>
#if defined(Z_CLOUD_LOGGING)
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#endif


#define MAX_PAYLOAD_SIZE (int)100000
#define DEFAULT_PAYLOAD_SIZE (int)40000
#define TOPIC_PRE (char *)"/devices"
#define DATA_TOPIC (char *)"/telemetry"
#define COMMAND_TOPIC (char *)"/commands"
#define EVENT_TOPIC (char *)"/events"
#define COMMAND_ACK_TOPIC (char *)"/commands/ack"
#define CONFIG_TOPIC (char *)"/configsettings"
#define CONFIG_ACK_TOPIC (char *)"/configsettings/ack"

#define zclient_dispatchEventFromJSONString(...) zclient_dispatchEventFromJSONStringWithAsset(__VA_ARGS__, NULL)
#define zclient_dispatchEventFromJSONStringWithAsset(client, eventType, eventDescription, eventDataJSONString, assettName, ...) zclient_dispatchEventFromJSONString(client, eventType, eventDescription, eventDataJSONString, assettName)

#define zclient_addNumber(...) zclient_addNumberToAsset(__VA_ARGS__, NULL)
#define zclient_addNumberToAsset(client, key, val, assetName, ...) zclient_addNumber(client, key, val, assetName)

#define zclient_addString(...) zclient_addStringToAsset(__VA_ARGS__, NULL)
#define zclient_addStringToAsset(client, key, val_string, assetName, ...) zclient_addString(client, key, val_string, assetName)

#define zclient_addObject(...) zclient_addObjectToAsset(__VA_ARGS__, NULL)
#define zclient_addObjectToAsset(client, key, val_object, assetName, ...) zclient_addObject(client, key, val_object, assetName)

#define zclient_markDataPointAsError(...) zclient_markDataPointAsErrorInAsset(__VA_ARGS__, NULL)
#define zclient_markDataPointAsErrorInAsset(client, key, assetName, ...) zclient_markDataPointAsError(client, key, assetName)

//TODO: Add provision to have zclient_init for Non-TLS with required arguments alone

#define zclient_init(...) zclient_init_without_logConfig(__VA_ARGS__, NULL)
#define zclient_init_without_logConfig(iot_client, MQTTUserName, MQTTPassword, mode, ca_crt, client_cert, client_key, cert_password, logConfig, ...) zclient_init(iot_client, MQTTUserName, MQTTPassword, mode, ca_crt, client_cert, client_key, cert_password, logConfig)

typedef void (*OTAHandler)(char *, char *, bool, char *);
typedef struct
{
    char *client_id;
    char *hostname;
    char *auth_token;
    char *MqttUserName;
    int retry_limit;
    int payload_size;
    char *mqttBuff;
    char *mqttReadBuff;
} Zconfig;

#if defined(Z_SECURE_CONNECTION)
typedef struct
{
    char *ca_crt;
    char *client_cert;
    char *client_key;
    char *cert_password;
} ZclientCertificates;
#endif

typedef struct
{
    cJSON *cJsonPayload;
    cJSON *data;
} Zpayload;

typedef struct
{
    char *topic;
    cJSON *ackPayload;
} ZfailedACK;

typedef struct
{
    time_t eventPayloadTime;
    cJSON *eventPayload;
} ZfailedEvent;

typedef enum
{
    INITIALIZED = 1,
    CONNECTED = 2,
    DISCONNECTED = 3
} ZclientConnectionState;

typedef struct
{
    MQTTClient mqtt_client;
    Zconfig config;
    ZclientConnectionState current_state;
    Zpayload message;
    int ZretryInterval;
#if defined(Z_SECURE_CONNECTION)
    ZclientCertificates certs;
#endif
} ZohoIOTclient;

typedef enum
{
    SUCCESFULLY_EXECUTED = 1001,
    CONFIG_SUCCESSFULLY_UPDATED = 1003,
    EXECUTION_FAILURE = 4000,
    METHOD_NOT_FOUND = 4001,
    EXECUTING_PREVIOUS_COMMAND = 4002,
    INSUFFICIENT_INPUTS = 4003,
    DEVICE_CONNECTIVITY_ISSUES = 4004,
    PARTIAL_EXECUTION = 4005,
    ALREADY_ON_SAME_STATE = 4006,
    CONFIG_FAILED = 4008
} ZcommandAckResponseCodes;
bool zclient_setAgentNameandVersion(char * name,char * version);
bool zclient_setPlatformName(char * platformName);
int zclient_init(ZohoIOTclient *iot_client, char *MQTTUserName, char *MQTTPassword, certsParseMode mode, char *ca_crt, char *client_cert, char *client_key, char *cert_password, ZlogConfig *logConfig);
int zclient_setMaxPayloadSize(ZohoIOTclient *iot_client,int size);
int zclient_connect(ZohoIOTclient *client);
int zclient_publish(ZohoIOTclient *client, char *payload);
int zclient_disconnect(ZohoIOTclient *client);
int zclient_command_subscribe(ZohoIOTclient *client, messageHandler on_message);
int zclient_config_subscribe(ZohoIOTclient *client, messageHandler on_message);
int zclient_yield(ZohoIOTclient *client, int time_out);
//TODO: to be tested and removed.
int zclient_reconnect(ZohoIOTclient *client);
int zclient_dispatch(ZohoIOTclient *client);

int zclient_dispatchEventFromJSONString(ZohoIOTclient *client, char *eventType, char *eventDescription, char *eventDataJSONString, char *assettName);
int zclient_dispatchEventFromEventDataObject(ZohoIOTclient *client, char *eventType, char *eventDescription, char *assetName);
int zclient_addEventDataNumber(char *key, double val);
int zclient_addEventDataString(char *key, char *val);
int zclient_addEventDataObject(char *key, cJSON* Object);

int zclient_publishCommandAck(ZohoIOTclient *client, char *correlation_id, ZcommandAckResponseCodes status_code, char *responseMessage);
int zclient_publishConfigAck(ZohoIOTclient *client, char *payload, ZcommandAckResponseCodes status_code, char *responseMessage);
void zclient_addConnectionParameter(char *connectionParamKey, char *connectionParamValue);
int zclient_markDataPointAsError(ZohoIOTclient *client, char *key, char *assetName);
// In add String method , assetName parameter as optional.
int zclient_addString(ZohoIOTclient *client, char *key, char *val_string, char *assetName);
// In add Number method , assetName parameter as optional.
int zclient_addNumber(ZohoIOTclient *client, char *key, double val_int, char *assetName);
// In add object method , assetName parameter as optional.
int zclient_addObject(ZohoIOTclient *client, char *key, cJSON* val_object, char *assetName);

cJSON* zclient_FormReceivedACK(char* payload);
#if defined(Z_CLOUD_LOGGING)
void handle_cloud_logging(ZohoIOTclient *client, char *payload);
bool get_cloud_logging_status();
#endif

int zclient_free(ZohoIOTclient *client);
void zclient_enable_paho_debug(bool state);
void zclient_set_tls(bool state);
void zclient_set_client_certs(bool state);
bool get_OTA_status();
bool get_cloud_logging_status();
void handle_cloud_logging(ZohoIOTclient *client, char *payload);
void handle_OTA(ZohoIOTclient *client, char* payload);
int zclient_ota_handler(OTAHandler on_OTA);
int zclient_publishOTAAck(ZohoIOTclient *client, char *correlation_id, ZcommandAckResponseCodes status_code, char *responseMessage);
//int zclient_setRetrycount(ZohoIOTclient *client, int count);
//char *zclient_getpayload();
#endif //# ZOHO_IOT_CLIENT_H_