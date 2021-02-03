#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <limits.h>
#include "zoho_iot_client.h"
#include "zclient_constants.h"
#include "wrap_functions.h"

int __wrap_MQTTConnect(MQTTClient *c, MQTTPacket_connectData *options)
{
    return mock_type(int);
}

int __wrap_NetworkConnect(Network *n, char *host, int pt, ...)
{
    return mock_type(int);
}

int __wrap_MQTTSubscribe(MQTTClient *c, const char *topicFilter, enum QoS qos, messageHandler messageHandler)
{
    return mock_type(int);
}

int __wrap_MQTTPublish(MQTTClient *c, const char *topicName, MQTTMessage *message)
{
    return mock_type(int);
}

int __wrap_MQTTDisconnect(MQTTClient *client)
{
    return mock_type(int);
}

int __wrap_MQTTYield(MQTTClient *c, int time_out)
{
    return mock_type(int);
}

//TEST CASES:
// INIT :
char *mqttUserName = "/domain_name/v1/devices/client_id/connect";
char *mqttPassword = "mqtt_password";
static void InitMethod_OnNullArguments_ShouldFail(void **state)
{
    // Init returns failure as Client is NULL
    assert_int_equal(zclient_init(NULL, mqttUserName, mqttPassword, EMBED, "", "", "", ""), ZFAILURE);

    // Init returns failure as Device Credentials are NULL
    ZohoIOTclient client;
    assert_int_equal(zclient_init(&client, NULL, NULL, EMBED, "", "", "", ""), ZFAILURE);
}

static void InitMethod_with_LogConfig_OnNullArguments_ShouldFail(void **state)
{
    ZlogConfig *logConfig = getZlogger();
    logConfig->enableFileLog = 1;
    logConfig->logPrefix = "cmocka_test";
    // Init returns failure as Client is NULL
    assert_int_equal(zclient_init(NULL, mqttUserName, mqttPassword, EMBED, "", "", "", "", logConfig), ZFAILURE);

    // Init returns failure as Device Credentials are NULL
    ZohoIOTclient client;
    assert_int_equal(zclient_init(&client, NULL, NULL, EMBED, "", "", "", "", logConfig), ZFAILURE);
}

static void InitMethod_OnProperArguments_ShouldSucceed(void **state)
{
    ZohoIOTclient client;
    // Init returns Success by initialising client with proper arguments
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", ""), ZSUCCESS);
}

static void InitMethod_with_LogConfig_OnProperArguments_ShouldSucceed(void **state)
{
    ZohoIOTclient client;
    ZlogConfig *logConfig = getZlogger();
    logConfig->enableFileLog = 1;
    logConfig->logPrefix = "cmocka_test";

    // Init returns Success by initialising client with proper arguments and setting the log config to NULL
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, REFERENCE, "", "", "", "", logConfig), ZFAILURE);

    // Init returns Success by initialising client with proper arguments and setting the log config
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "", logConfig), ZSUCCESS);
}

static void InitConfigFileMethod_OnNullArguments_ShouldFail(void **state)
{
    ZohoIOTclient client;
    // Init returns failure as FileName is NULL
    assert_int_equal(zclient_init_config_file(&client, NULL, EMBED), ZFAILURE);
    // Init returns failure as FileName is Empty
    assert_int_equal(zclient_init_config_file(&client, "", EMBED), ZFAILURE);
}

static void InitConfigFileMethod_with_LogConfig_OnNullArguments_ShouldFail(void **state)
{

    ZohoIOTclient client;
    ZlogConfig *logConfig = getZlogger();
    logConfig->enableFileLog = 1;
    logConfig->logPrefix = "cmocka_test";
    // Init returns failure as FileName is NULL
    assert_int_equal(zclient_init_config_file(NULL, NULL, EMBED), ZFAILURE);
    // Init returns failure as FileName is Empty
    assert_int_equal(zclient_init_config_file(&client, "", EMBED), ZFAILURE);
}

static void InitConfigFileMethod_OnProperArguments_ShouldSucceed(void **state)
{
    ZohoIOTclient client;
    // Init returns Success by initialising client with proper file contents.
    assert_int_equal(zclient_init_config_file(&client, "../../../test/MqttConfig.json", EMBED), ZSUCCESS);
}

static void InitConfigFileMethod_with_LogConfig_OnProperArguments_ShouldSucceed(void **state)
{
    ZohoIOTclient client;
    ZlogConfig *logConfig = getZlogger();
    logConfig->enableFileLog = 1;
    logConfig->logPrefix = "cmocka_test";
    // Init returns Success by initialising client with proper file contents.
    assert_int_equal(zclient_init_config_file(&client, "../../../test/MqttConfig.json", EMBED, logConfig), ZSUCCESS);
}

static void InitConfigFileMethod_OnProperArguments_withImproperKeys_ShouldFail(void **state)
{
    ZohoIOTclient client;
    // Init returns Fail to initialise client with improper file contents (Empty credentials).
    assert_int_equal(zclient_init_config_file(&client, "../../../test/MqttConfig_Wrong_Params.json", EMBED), ZFAILURE);
    // Init returns Fail to initialise client with proper file format (JSON structure).
    assert_int_equal(zclient_init_config_file(&client, "../../../test/MqttConfig_Wrong_Format.json", EMBED), ZFAILURE);
}

#ifndef Z_SECURE_CONNECTION
#define Z_SECURE_CONNECTION
#endif
static void InitMethod_WithTLS_NullSeverCertificates_ShouldFail(void **State)
{

    // Init return failure with TLS enabled as RootCA file is NULL .
    ZohoIOTclient client;
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, REFERENCE, NULL, "", "", ""), ZFAILURE);
}
#ifndef Z_USE_CLIENT_CERTS
#define Z_USE_CLIENT_CERTS
#endif
static void InitMethod_WithTLS_NullClientCertificates_ShouldFail(void **State)
{

    // Init return failure with TLS enabled as Client_Crt /client_key is NULL .
    ZohoIOTclient client;
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, REFERENCE, "/usr/device_certificate.pem", NULL, NULL, ""), ZFAILURE);
}

#undef Z_USE_CLIENT_CERTS
#undef Z_SECURE_CONNECTION

// CONNECT :

static void ConnectMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // connecting to HUB with out initializing client would return FAILURE
    ZohoIOTclient client;
    assert_int_equal(zclient_connect(&client), -2);
}

static void ConnectMethod_OnNullArguments_ShouldFail(void **state)
{
    // connect returns Failure for Null Client .
    assert_int_equal(zclient_connect(NULL), ZFAILURE);
}

static void ConnectMethod_OnConnectOverExistingConnetion_ShouldSucceed(void **state)
{
    // connect returns SUCCESS for connect called on existing connection.
    ZohoIOTclient client;
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", ""), ZSUCCESS);
    client.current_state = CONNECTED;
    assert_int_equal(zclient_connect(&client), ZSUCCESS);
}

static void ConnectMethod_WithNonNullArguments_ShouldSucceed(void **state)
{
    // Connect method returns SUCCEDD with proper device credentials
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);

    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_connect(&client), ZSUCCESS);
}

static void ConnectMethod_WithLostNetworkConnection_ShouldFail(void **state)
{
    // Connect method returns failure as Network connection is not available.
    will_return(__wrap_NetworkConnect, ZFAILURE);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_connect(&client), ZFAILURE);
}
static void ConnectMethod_WithWrongCredentials_ShouldFail(void **state)
{
    // connect method returns failure as credentials for network connection are incorrect.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, 5);

    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_connect(&client), 5);
}

#ifndef Z_SECURE_CONNECTION
#define Z_SECURE_CONNECTION
#endif
static void ConnectMethod_WithAppropriateTLSServerCertificates_shouldSucceed(void **state)
{
    // With Appropriate TLS Server Certificate and login credentials connect to HUB should succeed .
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);

    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "./ca.crt", "", "", "");
    assert_int_equal(zclient_connect(&client), ZSUCCESS);
}

#ifndef Z_USE_CLIENT_CERTS
#define Z_USE_CLIENT_CERTS
#endif
static void ConnectMethod_WithAppropriateTLSClientCertificates_shouldSucceed(void **state)
{
    // With Appropriate TLS Server and Client Certificates and login credentials connect to HUB should succeed .
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);

    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "./ca.crt", "./client.crt", "./client.key", "");
    assert_int_equal(zclient_connect(&client), ZSUCCESS);
}

#undef Z_USE_CLIENT_CERTS
#undef Z_SECURE_CONNECTION

// PUBLISH :

static void PublishMethod_OnNullArguments_ShouldFail(void **state)
{
    // Publish method returns FAILURE as client is null.
    ZohoIOTclient *client = NULL;
    assert_int_equal(zclient_publish(NULL, "hello"), ZFAILURE);
}

static void PublishMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // publishing to HUB with out initializing client would return FAILURE
    ZohoIOTclient client;
    assert_int_equal(zclient_publish(&client, "hello"), -2);
}

static void PublishMethod_WithLostConnection_ShouldFail(void **state)
{
    // Publishing with lost connection will return failure.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTPublish, ZFAILURE);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_publish(&client, "payload"), ZFAILURE);
}

static void PublishMethod_WithNonNullArguments_ShouldSucceed(void **state)
{
    // Publish method with appropriate arguments should succeed.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTPublish, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_publish(&client, "payload"), ZSUCCESS);
}

// DISPATCH :

static void DispatchMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // Dispatch Payload with out initializing client would return FAILURE
    ZohoIOTclient client;
    assert_int_equal(zclient_dispatch(&client), -2);
}

static void DispatchMethod_OnNullArguments_ShouldFail(void **state)
{
    // Dispatch Payload returns Failure for Null Client .
    assert_int_equal(zclient_dispatch(NULL), ZFAILURE);
}

static void DispatchMethod_WithNoConnection_ShouldFail(void **state)
{
    // Dispatch with out establishing connection should Fail.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_dispatch(&client), ZFAILURE);
}

static void DispatchMethod_WithProperConnection_ShouldSucceed(void **state)
{
    // Dispatch with Active connection should succeed.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTPublish, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    zclient_addNumber(&client, "key1", 10);
    zclient_addString(&client, "key2", "value");
    assert_int_equal(zclient_dispatch(&client), ZSUCCESS);
}

// DISPATCH EVENT FROM EVENT DATA OBJECT:

static void DispatchEventFromEventDataObject_OnCallingBeforeInitialization_ShouldFail(void **state)
{
    // DispatchEvent from event data object with out initializing should Fail.
    ZohoIOTclient client;
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "key1", 123);
    assert_int_equal(zclient_dispatchEventFromEventDataObject(&client, "eventType", "eventDescription", ""), -2);
    assert_int_equal(zclient_dispatchEventFromEventDataObject(&client, "eventType", "eventDescription", "assetName"), -2);
}

static void DispatchEventEventDataObject_WithNoConnection_ShouldFail(void **state)
{
    // DispatchEvent from event data object with out establishing connection should Fail.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    cJSON *obj = cJSON_CreateObject();
    assert_int_equal(zclient_dispatchEventFromEventDataObject(&client, "eventType", "eventDescription", ""), ZFAILURE);
    assert_int_equal(zclient_dispatchEventFromEventDataObject(&client, "eventType", "eventDescription", "assetName"), ZFAILURE);
}

static void DispatchEventFromEventDataObject_WithProperConnectionWithAndWithOutAssetName_ShouldSucceed(void **state)
{
    // DispatchEvent from event data object with Active connection with and without AssetName should succeed.
    will_return_always(__wrap_NetworkConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTPublish, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    zclient_addEventDataNumber("key1", 10);
    assert_int_equal(zclient_dispatchEventFromEventDataObject(&client, "eventType", "eventDescription", ""), ZSUCCESS);
    zclient_addEventDataNumber("key1", 20);
    assert_int_equal(zclient_dispatchEventFromEventDataObject(&client, "eventType", "eventDescription", "assetName"), ZSUCCESS);
}

static void DispatchEventFromEventDataObject_WithImproperArgumentsOrEventDataWithAndWithOutAssetName_ShouldFail(void **state)
{
    // DispatchEvent from event data object with ImproperEventData with and without AssetName should fail.
    will_return_always(__wrap_NetworkConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTConnect, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_dispatchEventFromEventDataObject(&client, "", NULL, ""), ZFAILURE);
    assert_int_equal(zclient_dispatchEventFromEventDataObject(&client, NULL, "eventDescription", NULL), ZFAILURE);
    assert_int_equal(zclient_dispatchEventFromEventDataObject(&client, "eventType", NULL, "assetName"), ZFAILURE);
    assert_int_equal(zclient_dispatchEventFromEventDataObject(&client, "", NULL, "assetName"), ZFAILURE);
}

// DISPATCH EVENT FROM JSON STRING

static void DispatchEventFromJSONString_OnCallingBeforeInitialization_ShouldFail(void **state)
{
    // DispatchEvent from json string with out initializing should Fail.
    ZohoIOTclient client;
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "key1", 123);
    assert_int_equal(zclient_dispatchEventFromJSONString(&client, "eventType", "eventDescription", cJSON_Print(obj), ""), -2);
}

static void DispatchEventFromJSONString_WithNoConnection_ShouldFail(void **state)
{
    // DispatchEvent from json string with out establishing connection should Fail.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "key1", 123);
    assert_int_equal(zclient_dispatchEventFromJSONString(&client, "eventType", "eventDescription", cJSON_Print(obj), ""), ZFAILURE);
}

static void DispatchEventFromJSONString_WithImproperEventDataWithAndWithOutAssetName_ShouldFail(void **state)
{
    // DispatchEvent from json string with ImproperEventData with and without AssetName should fail.
    will_return_always(__wrap_NetworkConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTConnect, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_dispatchEventFromJSONString(&client, "eventType", "eventDescription", "", ""), ZFAILURE);
    assert_int_equal(zclient_dispatchEventFromJSONString(&client, "eventType", "eventDescription", NULL, "assetName"), ZFAILURE);
    assert_int_equal(zclient_dispatchEventFromJSONString(&client, "eventType", NULL, "", "assetName"), ZFAILURE);
    assert_int_equal(zclient_dispatchEventFromJSONString(&client, "", NULL, "", "assetName"), ZFAILURE);
}

static void DispatchEventFromJSONString_WithproperEventDataWithAndWithOutAssetName_ShouldSucceed(void **state)
{
    // DispatchEvent from json string with properEventData with and without AssetName should Succeed.
    will_return_always(__wrap_NetworkConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTPublish, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "key1", 123);
    assert_int_equal(zclient_dispatchEventFromJSONString(&client, "eventType", "eventDescription", cJSON_Print(obj), ""), ZSUCCESS);
    cJSON_AddStringToObject(obj, "key1", "value1");
    assert_int_equal(zclient_dispatchEventFromJSONString(&client, "eventType", "eventDescription", cJSON_Print(obj), "assetName"), ZSUCCESS);
}

// PUBLISH COMMAND ACK

static void PublishCommandAck_OnCallingBeforeInitialization_ShouldFail(void **state)
{
    // PublishCommandAck with out initializing should Fail.
    ZohoIOTclient client;
    assert_int_equal(zclient_publishCommandAck(&client, "", 1001, "response message"), -2);
}

static void PublishCommandAck_WithNoConnection_ShouldFail(void **state)
{
    // PublishCommandAck with out establishing connection should Fail.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_publishCommandAck(&client, "", 1001, "response message"), ZFAILURE);
}

static void PublishCommandAck_WithproperArguments_ShouldSucceed(void **state)
{
    // PublishCommandAck with proper arguments should succeed.
    will_return_always(__wrap_NetworkConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTPublish, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_publishCommandAck(&client, "correlation_id", 1001, "response message"), ZSUCCESS);
    assert_int_equal(zclient_publishCommandAck(&client, "correlation_id", 1001, ""), ZSUCCESS);
}

static void PublishCommandAck_WithNullOrEmptyproperArguments_ShouldFail(void **state)
{
    // PublishCommandAck with Improper Null or empty arguments should Fail.
    will_return_always(__wrap_NetworkConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTConnect, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_publishCommandAck(&client, "", 1001, "response message"), ZFAILURE);
    assert_int_equal(zclient_publishCommandAck(&client, "", 1001, "response message"), ZFAILURE);
    assert_int_equal(zclient_publishCommandAck(&client, "correlation_id", 1001, NULL), ZFAILURE);
    assert_int_equal(zclient_publishCommandAck(&client, NULL, 1001, ""), ZFAILURE);
}

// SUBSCRIBE :

static void SubscribeMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // Subscribing with out initializing client would return FAILURE
    ZohoIOTclient client;
    messageHandler msghnd;
    assert_int_equal(zclient_subscribe(&client, msghnd), -2);
}

static void SubscribeMethod_OnNullArguments_ShouldFail(void **state)
{
    // Subscribe returns Failure for Null Client .
    messageHandler msghnd;
    assert_int_equal(zclient_subscribe(NULL, msghnd), ZFAILURE);
    // Subscribe returns Failure for Null messageHandler .
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_subscribe(&client, NULL), -1);
}

void message_handler(MessageData *data) {}

static void SubscribeMethod_WithLostConnection_ShouldFail(void **state)
{
    // Subscribe with lost connection should Fail
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTSubscribe, ZFAILURE);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_subscribe(&client, message_handler), ZFAILURE);
}

static void SubscribeMethod_WithNonNullArguments_ShouldSucceed(void **state)
{
    // Subscribe method returns success with appropriate connection.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTSubscribe, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_subscribe(&client, message_handler), ZSUCCESS);
}

// YIELD :

static void YieldMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // Subscribing with out initializing client would return FAILURE
    ZohoIOTclient client;
    assert_int_equal(zclient_yield(&client, 100), -2);
}

static void YieldMethod_OnNullArguments_ShouldFail(void **state)
{
    //Yield returns Failure for Null Client .
    assert_int_equal(zclient_yield(NULL, 1000), ZFAILURE);

    //Yield returns Failure for non positive timeout.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_yield(&client, 0), ZFAILURE);
}

static void YieldMethod_OnNonNullArguments_ShouldSucceed(void **state)
{
    // yield method with appropriate arguments should succeed.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTYield, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_yield(&client, 300), ZSUCCESS);
}

static void YieldMethod_WithLostConnection_ShouldFail(void **state)
{
    // Yield method wehn connection lost returns failure
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTYield, ZFAILURE);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_yield(&client, 300), ZFAILURE);
}

// DISCONNECT :

static void DisconnectMethod_OnNullArguments_ShouldFail(void **state)
{
    //Disconnect returns Failure for Null Client .
    assert_int_equal(zclient_disconnect(NULL), ZFAILURE);
}

static void DisconnectMethod_OnUnEstablishedConnetion_ShouldSucceed(void **state)
{
    // Disconnect returns SUCCESS for connect called on existing connection.
    ZohoIOTclient client;
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", ""), ZSUCCESS);
    assert_int_equal(zclient_disconnect(&client), ZSUCCESS);
}

static void DisconnectMethod_WithActiveConnection_ShouldDisconnectAndReturnSuccess(void **state)
{
    // Disconnect method with active connection get disconnected properly from HUB and return success.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTDisconnect, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_disconnect(&client), ZSUCCESS);
}

// ADD NUMBER :

static void AddNumberMethod_WithNullArguments_ShouldFail(void **state)
{
    // Adding Number will null key  or client would fail.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addNumber(NULL, "key1", 2);
    assert_int_equal(zclient_addNumber(&client, NULL, 10), ZFAILURE);
}

static void AddNumberMethod_CalledWithoutInitialization_ShouldFail(void **state)
{
    // Client must be initialized to add number to cjson payload.
    ZohoIOTclient client;
    assert_int_equal(zclient_addNumber(&client, "key1", 2), -2);
}

static void AddNumberMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue(void **state)
{
    // AddNumber with same key returns SUCCESS , old value gets replaced.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addNumber(&client, "key1", 1);
    zclient_addNumber(&client, "key1", 2);
    assert_int_equal(2, cJSON_GetObjectItem(client.message.data, "key1")->valueint);
}

static void AddNumberMethod_withNonNullAssetNameArgument_ShouldAddKeyToAssetObject_InData(void **state)
{
    // AddNumber with non-null asset name argument should add value to asset object in data.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addNumber(&client, "key1", 1, "asset1");
    cJSON *assetObj = cJSON_GetObjectItem(client.message.data, "asset1");
    assert_int_equal(1, cJSON_GetObjectItem(assetObj, "key1")->valueint);
}

static void AddNumberMethod_withNonNullAssetNameArgument_OnAddingSamekey_ShouldSucceed_ReplacingOldValue(void **state)
{
    // AddNumber with non-null asset name argument on adding key should replace old value in asset object of data.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addNumber(&client, "key1", 1, "asset1");
    zclient_addNumber(&client, "key1", 30, "asset1");
    cJSON *assetObj = cJSON_GetObjectItem(client.message.data, "asset1");
    assert_int_equal(30, cJSON_GetObjectItem(assetObj, "key1")->valueint);
}

static void AddNumberMethod_withEmptyAssetNameArgument_ShouldAddKeyToData(void **state)
{
    // AddNumber with empty asset name argument on adding key should add key to data.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addNumber(&client, "key1", 123, "");
    assert_int_equal(123, cJSON_GetObjectItem(client.message.data, "key1")->valueint);
}

// ADD EVENTDATA NUMBER :
static void AddEventDataNumber_WithProperArguments_ShouldSucceed(void **state)
{
    // AddEventDataNumber with Proper argument should add data to EventDataObject.
    assert_int_equal(0, zclient_addEventDataNumber("key1", 123));
}

static void AddEventDataNumber_WithImProperArguments_ShouldFail(void **state)
{
    // AddEventDataNumber with ImProper arguments should Fail to add data to EventDataObject.
    assert_int_equal(-1, zclient_addEventDataNumber(NULL, 123));
    assert_int_equal(-1, zclient_addEventDataNumber("", 123));
}

// ADD EVENTDATA STRING :
static void AddEventDataString_WithProperArguments_ShouldSucceed(void **state)
{
    // AddEventDataString with Proper argument should add data to EventDataObject.
    assert_int_equal(0, zclient_addEventDataString("key1", "value1"));
}

static void AddEventDataString_WithImProperArguments_ShouldFail(void **state)
{
    // AddEventDataString with ImProper arguments should Fail to add data to EventDataObject.
    assert_int_equal(-1, zclient_addEventDataString("", "value1"));
    assert_int_equal(-1, zclient_addEventDataString(NULL, "value1"));
    assert_int_equal(-1, zclient_addEventDataString(NULL, NULL));
    assert_int_equal(-1, zclient_addEventDataString(NULL, ""));
}

static void AddEventDataString_OnAddingSamekey_ShouldSucceed_ReplacingOldValue(void **state)
{
    zclient_addEventDataString("key1", "value1");
    assert_int_equal(0, zclient_addEventDataString("key1", "value2"));
}
// ADD STRING :

static void AddStringMethod_CalledWithoutInitialization_ShouldFail(void **state)
{
    // Client must be initialized to add string to cjson payload.
    ZohoIOTclient client;
    assert_int_equal(zclient_addString(&client, "key1", "value1"), -2);
}

static void AddStringMethod_OnNullArguments_ShouldFail(void **state)
{
    // AddString with null key/value returns FAILURE
    ZohoIOTclient client;
    int rc = zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addString(NULL, "Key", "value");
    assert_int_equal(zclient_addString(&client, "Key", NULL), ZFAILURE);
}

static void AddStringMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue(void **state)
{
    // AddString with same key returns SUCCESS , old value gets replaced.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addString(&client, "str_key", "str_val1");
    zclient_addString(&client, "str_key", "str_val2");
    assert_int_equal(strcmp("str_val2", cJSON_GetObjectItem(client.message.data, "str_key")->valuestring), 0);
}

static void AddStringMethod_withNonNullAssetNameArgument_ShouldAddKeyToAssetObject_InData(void **state)
{
    // AddString with non-null asset name argument should add value to asset name in data.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addString(&client, "key1", "str_val1", "asset1");
    cJSON *assetObj = cJSON_GetObjectItem(client.message.data, "asset1");
    assert_int_equal(strcmp("str_val1", cJSON_GetObjectItem(assetObj, "key1")->valuestring), 0);
}

static void AddStringMethod_withNonNullAssetNameArgument_OnAddingSamekey_ShouldSucceed_ReplacingOldValue(void **state)
{
    // AddString with non-null asset name argument on adding key should replace old value in asset object of data.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addString(&client, "key1", "str_val1", "asset1");
    zclient_addString(&client, "key1", "str_val2", "asset1");
    cJSON *assetObj = cJSON_GetObjectItem(client.message.data, "asset1");
    assert_int_equal(strcmp("str_val2", cJSON_GetObjectItem(assetObj, "key1")->valuestring), 0);
}

static void AddStringMethod_withEmptyAssetNameArgument_ShouldAddKeyToData(void **state)
{
    // AddString with empty asset name argument on adding key should add key to data.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addString(&client, "key1", "str_val1", "");
    assert_int_equal(strcmp("str_val1", cJSON_GetObjectItem(client.message.data, "key1")->valuestring), 0);
}

// MARK DATAPOINT ERROR :

static void MarkDataPointAsError_withNonNullAssetNameArgument_ShouldAddErrorValueToDataPointInAssetObject(void **state)
{
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_markDataPointAsError(&client, "key1", "asset1");
    cJSON *assetObj = cJSON_GetObjectItem(client.message.data, "asset1");
    assert_int_equal(strcmp("<ERROR>", cJSON_GetObjectItem(assetObj, "key1")->valuestring), 0);
}

static void MarkDataPointAsError_withNoOrNullAssetNameArgument_ShouldAddErrorValueToDataPoint(void **state)
{
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_markDataPointAsError(&client, "key1");
    assert_int_equal(strcmp("<ERROR>", cJSON_GetObjectItem(client.message.data, "key1")->valuestring), 0);
}

// RECONNECT :

static void ReconnectMethod_WithExistingConnection_ShouldSucceed(void **state)
{
    //  Reconnecting over the existing connection returns SUCCESS.
    ZohoIOTclient client;
    client.current_state = CONNECTED;
    assert_int_equal(zclient_reconnect(&client), ZSUCCESS);
}

static void ReconnectMethod_OnNullArguments_ShouldFail(void **state)
{
    //Reconnect returns Failure on NULL client .
    assert_int_equal(zclient_reconnect(NULL), ZFAILURE);
}

static void ReconnectMethod_OnCallingBeforeInitialization_ShouldFail()
{
    //  Reconnecting with out initializing client would return FAILURE .
    ZohoIOTclient client;
    assert_int_equal(zclient_reconnect(&client), -2);
}

static void ReconnectMethod_OnLostConnection_ShouldRetryAndSucceed(void **state)
{
    // Reconnect method with lost connection can Retry connection and succeed.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    client.current_state = DISCONNECTED;
    assert_int_equal(zclient_reconnect(&client), ZSUCCESS);
}

static void ReconnectMethod_OnLostConnection_ShouldExponentiallyIncrease(void **state)
{
    // Reconnect method with lost connection can Retry with increased delay.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    client.current_state = DISCONNECTED;
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZFAILURE);
    zclient_reconnect(&client);

    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZFAILURE);
    zclient_reconnect(&client);
    assert_int_equal(client.ZretryInterval, 7);

    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    zclient_reconnect(&client);
}

static void GetRetryInterval_withNegativeValues_ShouldReturnDefaultValue(void **state)
{
    // Get Retry Interval with Negative arguments should return default min retry interval
    assert_int_equal(getRetryInterval(-1),MIN_RETRY_INTERVAL);
}

static void GetRetryInterval_withValuesGreaterthenMaxRetryInterval_ShouldReturnDefaultValue(void **state)
{
    // Get Retry Interval with interval > MAX_RETRY_INTERVAL should return default MAX_RETRY_INTERVAL
    assert_int_equal(getRetryInterval(2000),MAX_RETRY_INTERVAL);
}


//
//// SET RETRY COUNT : Retry mechanism changed to indefinite exponential retry.
//
//static void SetRetryCountMethod_CalledWithoutInitializingClient_ShouldFail(void **state)
//{
//    // retry count method returns failure as client must be initialized .
//    ZohoIOTclient client;
//    assert_int_equal(zclient_setRetrycount(&client, 10), -2);
//}
//
//static void SetRetryCountMethod_WithNullArguments_ShouldFail(void **state)
//{
//    // set Retry count method fails since client is NULL.
//    assert_int_equal(zclient_setRetrycount(NULL, 10), ZFAILURE);
//}
//
//static void SetRetryCountMethod_WithNegativeCount_ShouldFail_DefaultValueIsUnchanged(void **state)
//{
//    // Retry count is set to default when negative value is set.
//    ZohoIOTclient client;
//    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
//    assert_int_equal(zclient_setRetrycount(&client, -10), ZFAILURE);
//}
//
//static void SetRetryCountMethod_WithAppropriateArguments_ShouldSucceed(void **state)
//{
//    // Setting retry count with appropriate arguments succced.
//    ZohoIOTclient client;
//    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
//    zclient_setRetrycount(&client, 10);
//    assert_int_equal(client.config.retry_limit, 10);
//}

int main(void)
{
    const struct CMUnitTest sdk_basic_tests[] = {
        cmocka_unit_test(InitMethod_OnNullArguments_ShouldFail),
        cmocka_unit_test(InitMethod_with_LogConfig_OnNullArguments_ShouldFail),
        cmocka_unit_test(InitMethod_OnProperArguments_ShouldSucceed),
        cmocka_unit_test(InitMethod_with_LogConfig_OnProperArguments_ShouldSucceed),
        cmocka_unit_test(InitConfigFileMethod_OnNullArguments_ShouldFail),
        cmocka_unit_test(InitConfigFileMethod_OnProperArguments_ShouldSucceed),
        cmocka_unit_test(InitConfigFileMethod_OnProperArguments_withImproperKeys_ShouldFail),
        cmocka_unit_test(InitConfigFileMethod_with_LogConfig_OnNullArguments_ShouldFail),
        cmocka_unit_test(InitConfigFileMethod_with_LogConfig_OnProperArguments_ShouldSucceed),
        cmocka_unit_test(InitMethod_WithTLS_NullSeverCertificates_ShouldFail),
        cmocka_unit_test(InitMethod_WithTLS_NullClientCertificates_ShouldFail),
        cmocka_unit_test(ConnectMethod_OnCallingBeforeInitialization_ShouldFail),
        cmocka_unit_test(ConnectMethod_OnNullArguments_ShouldFail),
        cmocka_unit_test(ConnectMethod_OnConnectOverExistingConnetion_ShouldSucceed),
        cmocka_unit_test(ConnectMethod_WithNonNullArguments_ShouldSucceed),
        cmocka_unit_test(ConnectMethod_WithLostNetworkConnection_ShouldFail),
        cmocka_unit_test(ConnectMethod_WithWrongCredentials_ShouldFail),
        cmocka_unit_test(ConnectMethod_WithAppropriateTLSServerCertificates_shouldSucceed),
        cmocka_unit_test(ConnectMethod_WithAppropriateTLSClientCertificates_shouldSucceed),
        cmocka_unit_test(PublishMethod_OnNullArguments_ShouldFail),
        cmocka_unit_test(PublishMethod_OnCallingBeforeInitialization_ShouldFail),
        cmocka_unit_test(PublishMethod_WithLostConnection_ShouldFail),
        cmocka_unit_test(PublishMethod_WithNonNullArguments_ShouldSucceed),
        cmocka_unit_test(DispatchMethod_OnCallingBeforeInitialization_ShouldFail),
        cmocka_unit_test(DispatchMethod_OnNullArguments_ShouldFail),
        cmocka_unit_test(DispatchMethod_WithNoConnection_ShouldFail),
        cmocka_unit_test(DispatchMethod_WithProperConnection_ShouldSucceed),
        cmocka_unit_test(AddEventDataNumber_WithImProperArguments_ShouldFail),
        cmocka_unit_test(AddEventDataNumber_WithProperArguments_ShouldSucceed),
        cmocka_unit_test(AddEventDataString_WithImProperArguments_ShouldFail),
        cmocka_unit_test(AddEventDataString_WithProperArguments_ShouldSucceed),
        cmocka_unit_test(DispatchEventFromEventDataObject_WithProperConnectionWithAndWithOutAssetName_ShouldSucceed),
        cmocka_unit_test(DispatchEventFromJSONString_WithImproperEventDataWithAndWithOutAssetName_ShouldFail),
        cmocka_unit_test(DispatchEventFromJSONString_WithproperEventDataWithAndWithOutAssetName_ShouldSucceed),
        cmocka_unit_test(DispatchEventFromJSONString_OnCallingBeforeInitialization_ShouldFail),
        cmocka_unit_test(DispatchEventFromJSONString_WithNoConnection_ShouldFail),
        cmocka_unit_test(DispatchEventFromEventDataObject_OnCallingBeforeInitialization_ShouldFail),
        cmocka_unit_test(DispatchEventEventDataObject_WithNoConnection_ShouldFail),
        cmocka_unit_test(DispatchEventFromEventDataObject_WithImproperArgumentsOrEventDataWithAndWithOutAssetName_ShouldFail),
        cmocka_unit_test(AddEventDataString_OnAddingSamekey_ShouldSucceed_ReplacingOldValue),
        cmocka_unit_test(PublishCommandAck_OnCallingBeforeInitialization_ShouldFail),
        cmocka_unit_test(PublishCommandAck_WithNoConnection_ShouldFail),
        cmocka_unit_test(PublishCommandAck_WithproperArguments_ShouldSucceed),
        cmocka_unit_test(PublishCommandAck_WithNullOrEmptyproperArguments_ShouldFail),
        cmocka_unit_test(SubscribeMethod_OnCallingBeforeInitialization_ShouldFail),
        cmocka_unit_test(SubscribeMethod_OnNullArguments_ShouldFail),
        cmocka_unit_test(SubscribeMethod_WithNonNullArguments_ShouldSucceed),
        cmocka_unit_test(SubscribeMethod_WithLostConnection_ShouldFail),
        cmocka_unit_test(YieldMethod_OnCallingBeforeInitialization_ShouldFail),
        cmocka_unit_test(YieldMethod_OnNullArguments_ShouldFail),
        cmocka_unit_test(YieldMethod_OnNonNullArguments_ShouldSucceed),
        cmocka_unit_test(YieldMethod_WithLostConnection_ShouldFail),
        cmocka_unit_test(DisconnectMethod_OnNullArguments_ShouldFail),
        cmocka_unit_test(DisconnectMethod_OnUnEstablishedConnetion_ShouldSucceed),
        cmocka_unit_test(DisconnectMethod_WithActiveConnection_ShouldDisconnectAndReturnSuccess),
        cmocka_unit_test(AddNumberMethod_WithNullArguments_ShouldFail),
        cmocka_unit_test(AddNumberMethod_CalledWithoutInitialization_ShouldFail),
        cmocka_unit_test(AddNumberMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue),
        cmocka_unit_test(AddNumberMethod_withNonNullAssetNameArgument_ShouldAddKeyToAssetObject_InData),
        cmocka_unit_test(AddNumberMethod_withNonNullAssetNameArgument_OnAddingSamekey_ShouldSucceed_ReplacingOldValue),
        cmocka_unit_test(AddNumberMethod_withEmptyAssetNameArgument_ShouldAddKeyToData),
        cmocka_unit_test(AddStringMethod_CalledWithoutInitialization_ShouldFail),
        cmocka_unit_test(AddStringMethod_OnNullArguments_ShouldFail),
        cmocka_unit_test(AddStringMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue),
        cmocka_unit_test(AddStringMethod_withNonNullAssetNameArgument_ShouldAddKeyToAssetObject_InData),
        cmocka_unit_test(AddStringMethod_withNonNullAssetNameArgument_OnAddingSamekey_ShouldSucceed_ReplacingOldValue),
        cmocka_unit_test(AddStringMethod_withEmptyAssetNameArgument_ShouldAddKeyToData),
        cmocka_unit_test(MarkDataPointAsError_withNonNullAssetNameArgument_ShouldAddErrorValueToDataPointInAssetObject),
        cmocka_unit_test(MarkDataPointAsError_withNoOrNullAssetNameArgument_ShouldAddErrorValueToDataPoint),
        cmocka_unit_test(ReconnectMethod_WithExistingConnection_ShouldSucceed),
        cmocka_unit_test(ReconnectMethod_OnNullArguments_ShouldFail),
        cmocka_unit_test(ReconnectMethod_OnCallingBeforeInitialization_ShouldFail),
        cmocka_unit_test(ReconnectMethod_OnLostConnection_ShouldRetryAndSucceed),
        cmocka_unit_test(ReconnectMethod_OnLostConnection_ShouldExponentiallyIncrease),
        cmocka_unit_test(GetRetryInterval_withNegativeValues_ShouldReturnDefaultValue),
        cmocka_unit_test(GetRetryInterval_withValuesGreaterthenMaxRetryInterval_ShouldReturnDefaultValue),
//        cmocka_unit_test(SetRetryCountMethod_CalledWithoutInitializingClient_ShouldFail),
//        cmocka_unit_test(SetRetryCountMethod_WithNullArguments_ShouldFail),
//        cmocka_unit_test(SetRetryCountMethod_WithNegativeCount_ShouldFail_DefaultValueIsUnchanged),
//        cmocka_unit_test(SetRetryCountMethod_WithAppropriateArguments_ShouldSucceed)
};
    cmocka_set_message_output(CM_OUTPUT_XML);
    return cmocka_run_group_tests(sdk_basic_tests, NULL, NULL);
}
