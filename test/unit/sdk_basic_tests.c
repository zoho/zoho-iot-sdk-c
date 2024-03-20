#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <limits.h>
#include "zoho_iot_client.h"
#include "zclient_constants.h"
#include "wrap_functions.h"
#include <unistd.h>

#define ACK_SAMPLE_PAYLOAD (char *)"[{\"payload\":[{\"edge_command_key\":\"cname\",\"value\":\"4\"}],\"command_name\":\"cname\",\"correlation_id\":\"5f12bd40-f010-11ed-8a24-5354005d2854\"}]"

int __wrap_MQTTConnect(MQTTClient *c, MQTTPacket_connectData *options)
{
    return mock_type(int);
}

int __wrap_NetworkConnect(Network *n, char *host, int pt, ...)
{
    return mock_type(int);
}
int __wrap_NetworkConnectTLS(Network *n, char *host, int pt, ...)
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

void __wrap_NetworkDisconnect(Network *n)
{
   
}

static int Turn_off_TLS_mode(void **state)
{
    zclient_set_tls(false);
     return 0;
}

static int Turn_on_TLS_mode(void **state)
{
    zclient_set_tls(true);
     return 0;
}

static int Turn_off_TLS_CA_mode(void **state)
{
    zclient_set_tls(false);
    zclient_set_client_certs(false);
     return 0;
}

static int Turn_on_TLS_CA_mode(void **state)
{
    zclient_set_tls(true);
    zclient_set_client_certs(true);
     return 0;
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

    // Init returns Success by initialising client with proper arguments and setting the log config
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, REFERENCE, "", "", "", "", logConfig), ZSUCCESS);

    // Init returns Success by initialising client with proper arguments and setting the log config
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "", logConfig), ZSUCCESS);
}

static void InitMethod_WithTLS_NullSeverCertificates_ShouldFail(void **State)
{

    // Init return failure with TLS enabled as RootCA file is NULL .
    ZohoIOTclient client;
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, REFERENCE, NULL, "", "", ""), ZFAILURE);
}

static void InitMethod_WithTLS_NullClientCertificates_ShouldFail(void **State)
{

    // Init return failure with TLS enabled as Client_Crt /client_key is NULL .
    ZohoIOTclient client;
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, REFERENCE, "/usr/device_certificate.pem", NULL, NULL, ""), ZFAILURE);
}

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


static void ConnectMethod_WithAppropriateTLSServerCertificates_shouldSucceed(void **state)
{
    // With Appropriate TLS Server Certificate and login credentials connect to HUB should succeed .
    will_return(__wrap_NetworkConnectTLS, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);

    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "./ca.crt", "", "", "");
    assert_int_equal(zclient_connect(&client), ZSUCCESS);
}


static void ConnectMethod_WithAppropriateTLSClientCertificates_shouldSucceed(void **state)
{
    // With Appropriate TLS Server and Client Certificates and login credentials connect to HUB should succeed .
    will_return(__wrap_NetworkConnectTLS, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);

    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "./ca.crt", "./client.crt", "./client.key", "");
    assert_int_equal(zclient_connect(&client), ZSUCCESS);
}


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

static void PublishMethod_WithPayloadSizeGreaterThanDefinedPayloadSize_ShouldFail(void **state)
{
    // Publish method with bigger payload than defined should fail
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_setMaxPayloadSize(&client,5);
    zclient_connect(&client);
    assert_int_equal(zclient_publish(&client, "1234567"), ZFAILURE);
}

static void PublishMethod_ShouldFail_If_Client_is_On_Disconnected_State(void **state)
{
    // Publish method with ClientDisconnect will fail.

    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
     client.current_state = DISCONNECTED;
    assert_int_equal(zclient_publish(&client, "payload"), ZFAILURE);
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
    client.current_state = 0;
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

static void DispatchEventFromJSONString_WithPayloadSizeGreaterThanDefinedPayloadSize_ShouldFail(void **state)
{
    // Dispatch method with bigger payload than defined should fail
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_setMaxPayloadSize(&client,5);
    zclient_connect(&client);
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "key1", 123);
    assert_int_equal(zclient_dispatchEventFromJSONString(&client, "eventType", "eventDescription", cJSON_Print(obj), ""), ZFAILURE);
}
static void DispatchEventFromJSONString_should_fail_when_MQTTPublish_is_not_Succeeded(void **state)
{
    // DispatchEvent from json string with properEventData with and without AssetName should Succeed.
    will_return_always(__wrap_NetworkConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTPublish, ZFAILURE);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "key1", 123);
    assert_int_equal(zclient_dispatchEventFromJSONString(&client, "eventType", "eventDescription", cJSON_Print(obj), ""), ZFAILURE);
    cJSON_AddStringToObject(obj, "key1", "value1");
    assert_int_equal(zclient_dispatchEventFromJSONString(&client, "eventType", "eventDescription", cJSON_Print(obj), "assetName"), ZFAILURE);
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
    assert_int_equal(zclient_publishCommandAck(&client, ACK_SAMPLE_PAYLOAD, 1001, "response message"), ZSUCCESS);
    assert_int_equal(zclient_publishCommandAck(&client, ACK_SAMPLE_PAYLOAD, 1001, ""), ZSUCCESS);
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
    assert_int_equal(zclient_publishCommandAck(&client, "invalid json", 1001, NULL), ZFAILURE);
    assert_int_equal(zclient_publishCommandAck(&client, NULL, 1001, ""), ZFAILURE);
}

static void PublishCommandAck_WithoutMQTTPublish_ShouldFail(void **state)
{
    // PublishCommandAck with proper arguments should succeed.
    will_return_always(__wrap_NetworkConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTConnect, ZSUCCESS);
    will_return_always(__wrap_MQTTPublish, ZFAILURE);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_publishCommandAck(&client, ACK_SAMPLE_PAYLOAD, 1001, "response message"), ZFAILURE);
    assert_int_equal(zclient_publishCommandAck(&client, ACK_SAMPLE_PAYLOAD, 1001, ""), ZFAILURE);
}
// SUBSCRIBE :

static void SubscribeMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // Subscribing with out initializing client would return FAILURE
    ZohoIOTclient client;
    messageHandler msghnd;
    assert_int_equal(zclient_command_subscribe(&client, msghnd), -2);
}

static void SubscribeMethod_OnNullArguments_ShouldFail(void **state)
{
    // Subscribe returns Failure for Null Client .
    messageHandler msghnd;
    assert_int_equal(zclient_command_subscribe(NULL, msghnd), ZFAILURE);
    // Subscribe returns Failure for Null messageHandler .
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_command_subscribe(&client, NULL), ZFAILURE);
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
    assert_int_equal(zclient_command_subscribe(&client, message_handler), ZFAILURE);
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
    assert_int_equal(zclient_command_subscribe(&client, message_handler), ZSUCCESS);
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
    assert_int_equal(zclient_yield(&client,ZSUCCESS), ZFAILURE);
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
     extern cJSON *eventDataObject;
    eventDataObject=NULL;
    // AddEventDataNumber with Proper argument should add data to EventDataObject.
    assert_int_equal(ZSUCCESS, zclient_addEventDataNumber("key1", 123));
}

static void AddEventDataNumber_WithImProperArguments_ShouldFail(void **state)
{
    // AddEventDataNumber with ImProper arguments should Fail to add data to EventDataObject.
    assert_int_equal(ZFAILURE, zclient_addEventDataNumber(NULL, 123));
    assert_int_equal(ZFAILURE, zclient_addEventDataNumber("", 123));
}


static void AddEventDataString_InitializeEventDataObjectIsNull_WithProperArguments_ShouldSucceed(void **state)
{
    extern cJSON *eventDataObject;
    eventDataObject=NULL;
    assert_int_equal(ZSUCCESS, zclient_addEventDataString("key1", "value1"));
}

static void AddEventDataString_WithProperArguments_ShouldSucceed(void **state)
{
    // AddEventDataString with Proper argument should add data to EventDataObject.
    assert_int_equal(ZSUCCESS, zclient_addEventDataString("key1", "value1"));
}

static void AddEventDataString_WithImProperArguments_ShouldFail(void **state)
{
    // AddEventDataString with ImProper arguments should Fail to add data to EventDataObject.
    assert_int_equal(ZFAILURE, zclient_addEventDataString("", "value1"));
    assert_int_equal(ZFAILURE, zclient_addEventDataString(NULL, "value1"));
    assert_int_equal(ZFAILURE, zclient_addEventDataString(NULL, NULL));
    assert_int_equal(ZFAILURE, zclient_addEventDataString(NULL, ""));
}

static void AddEventDataString_OnAddingSamekey_ShouldSucceed_ReplacingOldValue(void **state)
{
    zclient_addEventDataString("key1", "value1");
    assert_int_equal(ZSUCCESS, zclient_addEventDataString("key1", "value2"));
}

static void AddEventDataObject_InitializeEventDataObjectIsNull_WithProperArguments_ShouldSucceed(void ** states){
     extern cJSON *eventDataObject;
    eventDataObject=NULL;
     cJSON* Object=cJSON_CreateObject();
     zclient_addEventDataObject("key1", Object);
    assert_int_equal(ZSUCCESS, zclient_addEventDataObject("key1", Object));
    assert_int_equal(ZFAILURE, zclient_addEventDataObject("", Object));
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
    assert_int_equal(zclient_addString(NULL, "Key", "value"), ZFAILURE);
    assert_int_equal(zclient_addString(&client, NULL,"value"), ZFAILURE);
    assert_int_equal(zclient_addString(&client, "Key", NULL), ZFAILURE);
}

static void AddStringMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue(void **state)
{
    // AddString with same key returns SUCCESS , old value gets replaced.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addString(&client, "str_key", "str_val1");
    zclient_addString(&client, "str_key", "str_val2");
    assert_int_equal(strcmp("str_val2", cJSON_GetObjectItem(client.message.data, "str_key")->valuestring),ZSUCCESS);
}

static void AddStringMethod_withNonNullAssetNameArgument_ShouldAddKeyToAssetObject_InData(void **state)
{
    // AddString with non-null asset name argument should add value to asset name in data.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addString(&client, "key1", "str_val1", "asset1");
    cJSON *assetObj = cJSON_GetObjectItem(client.message.data, "asset1");
    assert_int_equal(strcmp("str_val1", cJSON_GetObjectItem(assetObj, "key1")->valuestring), ZSUCCESS);
}

static void AddStringMethod_withNonNullAssetNameArgument_OnAddingSamekey_ShouldSucceed_ReplacingOldValue(void **state)
{
    // AddString with non-null asset name argument on adding key should replace old value in asset object of data.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addString(&client, "key1", "str_val1", "asset1");
    zclient_addString(&client, "key1", "str_val2", "asset1");
    cJSON *assetObj = cJSON_GetObjectItem(client.message.data, "asset1");
    assert_int_equal(strcmp("str_val2", cJSON_GetObjectItem(assetObj, "key1")->valuestring), ZSUCCESS);
}

static void AddStringMethod_withEmptyAssetNameArgument_ShouldAddKeyToData(void **state)
{
    // AddString with empty asset name argument on adding key should add key to data.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addString(&client, "key1", "str_val1", "");
    assert_int_equal(strcmp("str_val1", cJSON_GetObjectItem(client.message.data, "key1")->valuestring), ZSUCCESS);
}

// MARK DATAPOINT ERROR :

static void MarkDataPointAsError_withNonNullAssetNameArgument_ShouldAddErrorValueToDataPointInAssetObject(void **state)
{
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_markDataPointAsError(&client, "key1", "asset1");
    cJSON *assetObj = cJSON_GetObjectItem(client.message.data, "asset1");
    assert_int_equal(strcmp("<ERROR>", cJSON_GetObjectItem(assetObj, "key1")->valuestring), ZSUCCESS);
}

static void MarkDataPointAsError_withNoOrNullAssetNameArgument_ShouldAddErrorValueToDataPoint(void **state)
{
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_markDataPointAsError(&client, "key1");
    assert_int_equal(strcmp("<ERROR>", cJSON_GetObjectItem(client.message.data, "key1")->valuestring),ZSUCCESS);
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

static void GetRetryInterval_withNegativeValues_ShouldReturnDefaultValue(void **state)
{
    // Get Retry Interval with Negative arguments should return default min retry interval
    assert_int_equal(getRetryInterval(-1), MIN_RETRY_INTERVAL);
}

static void GetRetryInterval_withValuesGreaterthenMaxRetryInterval_ShouldReturnDefaultValue(void **state)
{
    // Get Retry Interval with interval > MAX_RETRY_INTERVAL should return default MAX_RETRY_INTERVAL
    assert_int_equal(getRetryInterval(2000), MAX_RETRY_INTERVAL);
}

static void ReconnectMethod_OnLostConnection_ShouldRetryAndSucceed(void **state)
{
    // Reconnect method with lost connection can Retry connection and succeed.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTSubscribe, ZSUCCESS);
    will_return_always(__wrap_MQTTPublish, ZSUCCESS);
    
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    client.current_state = DISCONNECTED;
    zclient_reconnect(&client);
    sleep(3);
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
    sleep(5);
    zclient_reconnect(&client);
    assert_int_equal(client.ZretryInterval, 4);
}

static void ReconnectMethod_WithMQTTPublishFailure_OnLostConnection_ShouldRetryAndFails(void **state)
{
    // Reconnect method with lost connection can Retry connection and succeed.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTSubscribe, ZSUCCESS);
    will_return_always(__wrap_MQTTPublish, ZFAILURE);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    client.current_state = DISCONNECTED;
    zclient_reconnect(&client);
    sleep(3);
    assert_int_equal(zclient_reconnect(&client), ZSUCCESS);
}

//Config Publish
static void zclient_publishConfigAck_should_MQTT_NotConnected_ShouldFail(void ** state)
{
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTPublish, ZFAILURE);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_publishConfigAck(&client, ACK_SAMPLE_PAYLOAD, 1001, "response message"),ZFAILURE);
}

static void zclient_publishConfigAck_withProperArguments_shouldSuccess(void ** state)
{
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTPublish, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_publishConfigAck(&client, ACK_SAMPLE_PAYLOAD, 1001, "response message"),ZSUCCESS);
}

static void zclient_publishConfigAck_withNoConnection_shouldFail(void ** state){
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_publishConfigAck(&client, NULL, 1001, "response message"),ZFAILURE);
}

//Config Subscribe
static void  config_subscribe_WithNonNullArguments_ShouldSucceed(void **state)
{
    // Subscribe method returns success with appropriate connection.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTSubscribe, ZSUCCESS);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal( zclient_config_subscribe(&client, message_handler), ZSUCCESS);
}

static void  config_subscribe_WithLostConnection_ShouldFail(void **state)
{
    // Subscribe method returns success with appropriate connection.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTSubscribe, ZFAILURE);
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal( zclient_config_subscribe(&client, message_handler), ZFAILURE);
}

static void ConfigSubscribeMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // Subscribing with out initializing client would return FAILURE
    ZohoIOTclient client;
    messageHandler msghnd;
    assert_int_equal(zclient_config_subscribe(&client, msghnd), -2);
}

static void ConfigSubscribeMethod_OnNullArguments_ShouldFail(void **state)
{
    // Subscribe returns Failure for Null Client .
    messageHandler msghnd;
    assert_int_equal(zclient_config_subscribe(NULL, msghnd), ZFAILURE);
    // Subscribe returns Failure for Null messageHandler .
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_config_subscribe(&client, NULL), ZFAILURE);
}

//add Object
static void AddObjectMethod_CalledWithoutInitialization_ShouldFail(void **state)
{   
    cJSON *obj = cJSON_CreateObject();
    // Client must be initialized to add string to cjson payload.
    ZohoIOTclient client;
    assert_int_equal(zclient_addObject(&client, "key1", obj), -2);
}

static void AddObjectMethod_OnNullArguments_ShouldFail(void **state)
{   
    cJSON *obj = cJSON_CreateObject();
    // AddObject with null key/value returns FAILURE
    ZohoIOTclient client;
    int rc = zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_addObject(&client, "", obj), ZFAILURE);
}

static void AddObjectMethod_withNonNullAssetNameArgument_ShouldSucceed(void **state)
{   
    cJSON *obj = cJSON_CreateObject();
    // AddObject with non-null asset name argument should add value to asset name in data.
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_addObject(&client, "key1", obj, "asset1"), ZSUCCESS);
}

static void AddObjectMethod_withEmptyAssetNameArgument_ShouldSucceed(void **state)
{   
    cJSON *obj = cJSON_CreateObject();
    // AddObject with empty asset name argument on adding key should add key to data.
    ZohoIOTclient client;
    zclient_init(&client,mqttUserName, " mqttPassword ", EMBED, "", "", "", "");
    assert_int_equal(zclient_addObject(&client, "key1", obj, ""), ZSUCCESS);
}

static void AddObjectMethod_OnAddingSamekey_ShouldSucceed(void **state)
{
    cJSON *obj = cJSON_CreateObject();
    // AddObject with same key returns SUCCESS , old value gets replaced.
    ZohoIOTclient client;
    cJSON *obj_2 = cJSON_CreateObject();
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addObject(&client, "str_key", obj);
    assert_int_equal(zclient_addObject(&client, "str_key", obj_2),ZSUCCESS);
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

static void SetMaxPayloadSize_withValueGreaterthenMaxPayloadSize_ShouldTakeMaxPayloadSize(void **state)
{
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    //client->config->payload_size set to MAX_PAYLOAD_SIZE if the value given is greater than max payload
    zclient_setMaxPayloadSize(&client,150000);
    assert_int_equal(client.config.payload_size, MAX_PAYLOAD_SIZE);

}

static void SetMaxPayloadSize_withValuelesserthenOne_ShouldTakeDefaultPayloadSize(void **state)
{
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    //client->config->payload_size set to DEFAULT_PAYLOAD_SIZE if the value given is lesser than one
    zclient_setMaxPayloadSize(&client,0);
    assert_int_equal(client.config.payload_size, DEFAULT_PAYLOAD_SIZE);

}

static void SetMaxPayloadSize_withProperValue_ShouldTakeProperValue(void **state)
{
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    //client->config->payload_size set to given value if the given value is in range 1 to max payload
    int size = (rand() % (MAX_PAYLOAD_SIZE - 1 + 1)) + 1;
    zclient_setMaxPayloadSize(&client,size);
    assert_int_equal(client.config.payload_size, size);

}

static void WithoutCalling_SetMaxPayloadSize_ShouldTakeDefaultPayloadSize(void **state)
{
    ZohoIOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    //client->config->payload_size set to DEFAULT_PAYLOAD_SIZE if the set payload size is not called
    assert_int_equal(client.config.payload_size, DEFAULT_PAYLOAD_SIZE);
}

int main(void)
{
    const struct CMUnitTest sdk_basic_tests[] = {
        cmocka_unit_test_setup(InitMethod_OnNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(InitMethod_with_LogConfig_OnNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(InitMethod_OnProperArguments_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(InitMethod_with_LogConfig_OnProperArguments_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ConnectMethod_OnCallingBeforeInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ConnectMethod_OnNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ConnectMethod_OnConnectOverExistingConnetion_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ConnectMethod_WithNonNullArguments_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ConnectMethod_WithLostNetworkConnection_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ConnectMethod_WithWrongCredentials_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(PublishMethod_OnNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(PublishMethod_OnCallingBeforeInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(PublishMethod_WithLostConnection_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(PublishMethod_WithNonNullArguments_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(PublishMethod_ShouldFail_If_Client_is_On_Disconnected_State,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchMethod_OnCallingBeforeInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchMethod_OnNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchMethod_WithNoConnection_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchMethod_WithProperConnection_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddEventDataNumber_WithImProperArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddEventDataNumber_WithProperArguments_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddEventDataString_WithImProperArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddEventDataString_WithProperArguments_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddEventDataString_InitializeEventDataObjectIsNull_WithProperArguments_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddEventDataObject_InitializeEventDataObjectIsNull_WithProperArguments_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchEventFromEventDataObject_WithProperConnectionWithAndWithOutAssetName_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchEventFromJSONString_WithImproperEventDataWithAndWithOutAssetName_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchEventFromJSONString_WithproperEventDataWithAndWithOutAssetName_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchEventFromJSONString_OnCallingBeforeInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchEventFromJSONString_WithNoConnection_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchEventFromEventDataObject_OnCallingBeforeInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchEventEventDataObject_WithNoConnection_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchEventFromEventDataObject_WithImproperArgumentsOrEventDataWithAndWithOutAssetName_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchEventFromJSONString_should_fail_when_MQTTPublish_is_not_Succeeded,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddEventDataString_OnAddingSamekey_ShouldSucceed_ReplacingOldValue,Turn_off_TLS_mode),
        cmocka_unit_test_setup(PublishCommandAck_OnCallingBeforeInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(PublishCommandAck_WithNoConnection_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(PublishCommandAck_WithproperArguments_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(PublishCommandAck_WithNullOrEmptyproperArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(PublishCommandAck_WithoutMQTTPublish_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(SubscribeMethod_OnCallingBeforeInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(SubscribeMethod_OnNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(SubscribeMethod_WithNonNullArguments_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(SubscribeMethod_WithLostConnection_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(YieldMethod_OnCallingBeforeInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(YieldMethod_OnNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(YieldMethod_OnNonNullArguments_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(YieldMethod_WithLostConnection_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DisconnectMethod_OnNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DisconnectMethod_OnUnEstablishedConnetion_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DisconnectMethod_WithActiveConnection_ShouldDisconnectAndReturnSuccess,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddNumberMethod_WithNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddNumberMethod_CalledWithoutInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddNumberMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddNumberMethod_withNonNullAssetNameArgument_ShouldAddKeyToAssetObject_InData,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddNumberMethod_withNonNullAssetNameArgument_OnAddingSamekey_ShouldSucceed_ReplacingOldValue,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddNumberMethod_withEmptyAssetNameArgument_ShouldAddKeyToData,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddStringMethod_CalledWithoutInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddStringMethod_OnNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddStringMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddStringMethod_withNonNullAssetNameArgument_ShouldAddKeyToAssetObject_InData,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddStringMethod_withNonNullAssetNameArgument_OnAddingSamekey_ShouldSucceed_ReplacingOldValue,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddStringMethod_withEmptyAssetNameArgument_ShouldAddKeyToData,Turn_off_TLS_mode),
        cmocka_unit_test_setup(MarkDataPointAsError_withNonNullAssetNameArgument_ShouldAddErrorValueToDataPointInAssetObject,Turn_off_TLS_mode),
        cmocka_unit_test_setup(MarkDataPointAsError_withNoOrNullAssetNameArgument_ShouldAddErrorValueToDataPoint,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ReconnectMethod_WithExistingConnection_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ReconnectMethod_OnNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ReconnectMethod_OnCallingBeforeInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ReconnectMethod_WithMQTTPublishFailure_OnLostConnection_ShouldRetryAndFails,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ReconnectMethod_OnLostConnection_ShouldRetryAndSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ReconnectMethod_OnLostConnection_ShouldExponentiallyIncrease,Turn_off_TLS_mode),
        cmocka_unit_test_setup(GetRetryInterval_withNegativeValues_ShouldReturnDefaultValue,Turn_off_TLS_mode),
        cmocka_unit_test_setup(GetRetryInterval_withValuesGreaterthenMaxRetryInterval_ShouldReturnDefaultValue,Turn_off_TLS_mode),
        cmocka_unit_test_setup(SetMaxPayloadSize_withValueGreaterthenMaxPayloadSize_ShouldTakeMaxPayloadSize,Turn_off_TLS_mode),
        cmocka_unit_test_setup(SetMaxPayloadSize_withValuelesserthenOne_ShouldTakeDefaultPayloadSize,Turn_off_TLS_mode),
        cmocka_unit_test_setup(SetMaxPayloadSize_withProperValue_ShouldTakeProperValue,Turn_off_TLS_mode),
        cmocka_unit_test_setup(WithoutCalling_SetMaxPayloadSize_ShouldTakeDefaultPayloadSize,Turn_off_TLS_mode),
        cmocka_unit_test_setup(PublishMethod_WithPayloadSizeGreaterThanDefinedPayloadSize_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(DispatchEventFromJSONString_WithPayloadSizeGreaterThanDefinedPayloadSize_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(zclient_publishConfigAck_should_MQTT_NotConnected_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(zclient_publishConfigAck_withProperArguments_shouldSuccess,Turn_off_TLS_mode),
        cmocka_unit_test_setup(zclient_publishConfigAck_withNoConnection_shouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(config_subscribe_WithNonNullArguments_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(config_subscribe_WithLostConnection_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ConfigSubscribeMethod_OnCallingBeforeInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(ConfigSubscribeMethod_OnNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddObjectMethod_CalledWithoutInitialization_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddObjectMethod_OnNullArguments_ShouldFail,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddObjectMethod_withNonNullAssetNameArgument_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddObjectMethod_withEmptyAssetNameArgument_ShouldSucceed,Turn_off_TLS_mode),
        cmocka_unit_test_setup(AddObjectMethod_OnAddingSamekey_ShouldSucceed,Turn_off_TLS_mode),

#ifdef Z_SECURE_CONNECTION
        cmocka_unit_test_setup(ConnectMethod_WithAppropriateTLSServerCertificates_shouldSucceed,Turn_on_TLS_mode),
        cmocka_unit_test_setup(InitMethod_WithTLS_NullSeverCertificates_ShouldFail,Turn_on_TLS_mode),

#ifdef Z_USE_CLIENT_CERTS
        cmocka_unit_test_setup(InitMethod_WithTLS_NullClientCertificates_ShouldFail,Turn_on_TLS_CA_mode),
        cmocka_unit_test_setup(ConnectMethod_WithAppropriateTLSClientCertificates_shouldSucceed,Turn_on_TLS_CA_mode),

#endif
#endif
//        cmocka_unit_test(SetRetryCountMethod_CalledWithoutInitializingClient_ShouldFail),
//        cmocka_unit_test(SetRetryCountMethod_WithNullArguments_ShouldFail),
//        cmocka_unit_test(SetRetryCountMethod_WithNegativeCount_ShouldFail_DefaultValueIsUnchanged),
//        cmocka_unit_test(SetRetryCountMethod_WithAppropriateArguments_ShouldSucceed)
    };
    cmocka_set_message_output(CM_OUTPUT_XML);
    return cmocka_run_group_tests(sdk_basic_tests, NULL, NULL);
}