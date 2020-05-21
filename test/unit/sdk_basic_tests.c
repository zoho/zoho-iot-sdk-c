#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <limits.h>
#include "iot_client.h"
#include "generic.h"
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
    IOTclient *client;
    assert_int_equal(zclient_init(client, NULL, NULL, EMBED, "", "", "", ""), ZFAILURE);
}

#ifndef SECURE_CONNECTION
#define SECURE_CONNECTION
#endif
static void InitMethod_WithTLS_NullSeverCertificates_ShouldFail(void **State)
{

    // Init return failure with TLS enabled as RootCA file is NULL .
    IOTclient client;
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, REFERENCE, NULL, "", "", ""), ZFAILURE);
}
#ifndef USE_CLIENT_CERTS
#define USE_CLIENT_CERTS
#endif
static void InitMethod_WithTLS_NullClientCertificates_ShouldFail(void **State)
{

    // Init return failure with TLS enabled as Client_Crt /client_key is NULL .
    IOTclient client;
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, REFERENCE, "/usr/device_certificate.pem", NULL, NULL, ""), ZFAILURE);
}

#undef USE_CLIENT_CERTS
#undef SECURE_CONNECTION

// CONNECT :

static void ConnectMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // connecting to HUB with out initializing client would return FAILURE
    IOTclient client;
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
    IOTclient client;
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", ""), ZSUCCESS);
    client.current_state = Connected;
    assert_int_equal(zclient_connect(&client), ZSUCCESS);
}

static void ConnectMethod_WithNonNullArguments_ShouldSucceed(void **state)
{
    // Connect method returns SUCCEDD with proper device credentials
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);

    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_connect(&client), ZSUCCESS);
}

static void ConnectMethod_WithLostNetworkConnection_ShouldFail(void **state)
{
    // Connect method returns failure as Network connection is not available.
    will_return(__wrap_NetworkConnect, ZFAILURE);
    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_connect(&client), ZFAILURE);
}
static void ConnectMethod_WithWrongCredentials_ShouldFail(void **state)
{
    // connect method returns failure as credentials for network connection are incorrect.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, 5);

    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_connect(&client), 5);
}

#ifndef SECURE_CONNECTION
#define SECURE_CONNECTION
#endif
static void ConnectMethod_WithAppropriateTLSServerCertificates_shouldSucceed(void **state)
{
    // With Appropriate TLS Server Certificate and login credentials connect to HUB should succeed .
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);

    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "./ca.crt", "", "", "");
    assert_int_equal(zclient_connect(&client), ZSUCCESS);
}

#ifndef USE_CLIENT_CERTS
#define USE_CLIENT_CERTS
#endif
static void ConnectMethod_WithAppropriateTLSClientCertificates_shouldSucceed(void **state)
{
    // With Appropriate TLS Server and Client Certificates and login credentials connect to HUB should succeed .
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);

    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "./ca.crt", "./client.crt", "./client.key", "");
    assert_int_equal(zclient_connect(&client), ZSUCCESS);
}

#undef USE_CLIENT_CERTS
#undef SECURE_CONNECTION

// PUBLISH :

static void PublishMethod_OnNUllArguments_ShouldFail(void **state)
{
    // Publish method returns FAILURE as client is null.
    IOTclient *client = NULL;
    assert_int_equal(zclient_publish(NULL, "hello"), ZFAILURE);
}

static void PublishMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // publishing to HUB with out initializing client would return FAILURE
    IOTclient client;
    assert_int_equal(zclient_publish(&client, "hello"), ZFAILURE);
}

static void PublishMethod_WithLostConnection_ShouldFail(void **state)
{
    // Publishing with lost connection will return failure.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTPublish, ZFAILURE);
    IOTclient client;
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
    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_publish(&client, "payload"), ZSUCCESS);
}

// DISPATCH :

static void DispatchMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // Dispatch Payload with out initializing client would return FAILURE
    IOTclient client;
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
    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_dispatch(&client), ZFAILURE);
}

static void DispatchMethod_WithProperConnection_ShouldSucceed(void **state)
{
    // Dispatch with Active connection should succeed.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTPublish, ZSUCCESS);
    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    zclient_addNumber(&client, "key1", 10);
    zclient_addString(&client, "key2", "value");
    assert_int_equal(zclient_dispatch(&client), ZSUCCESS);
}

// SUBSCRIBE :

static void SubscribeMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // Subscribing with out initializing client would return FAILURE
    IOTclient client;
    messageHandler msghnd;
    assert_int_equal(zclient_subscribe(&client, msghnd), -2);
}

static void SubscribeMethod_OnNullArguments_ShouldFail(void **state)
{
    // Subscribe returns Failure for Null Client .
    messageHandler msghnd;
    assert_int_equal(zclient_subscribe(NULL, msghnd), ZFAILURE);
    // Subscribe returns Failure for Null messageHandler .
    //     IOTclient client;
    //     zclient_init(&client, mqttUserName , mqttPassword , EMBED, "","","","");
    //     assert_int_equal(zclient_subscribe(&client,NULL),-1);
}

void message_handler(MessageData *data) {}

static void SubscribeMethod_WithLostConnection_ShouldFail(void **state)
{
    // Subscribe with lost connection should Fail
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTSubscribe, ZFAILURE);
    IOTclient client;
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
    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_subscribe(&client, message_handler), ZSUCCESS);
}

// YIELD :

static void YieldMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // Subscribing with out initializing client would return FAILURE
    IOTclient client;
    assert_int_equal(zclient_yield(&client, 100), -2);
}

static void YieldMethod_OnNullArguments_ShouldFail(void **state)
{
    //Yield returns Failure for Null Client .
    assert_int_equal(zclient_yield(NULL, 1000), ZFAILURE);

    //Yield returns Failure for non positive timeout.
    IOTclient client;
    assert_int_equal(zclient_yield(&client, 0), ZFAILURE);
}

static void YieldMethod_OnNonNullArguments_ShouldSucceed(void **state)
{
    // yield method with appropriate arguments should succeed.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTYield, ZSUCCESS);
    IOTclient client;
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
    IOTclient client;
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
    IOTclient client;
    assert_int_equal(zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", ""), ZSUCCESS);
    assert_int_equal(zclient_disconnect(&client), ZSUCCESS);
}

static void DisconnectMethod_WithActiveConnection_ShouldDisconnectAndReturnSuccess(void **state)
{
    // Disconnect method with active connection get disconnected properly from HUB and return success.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    will_return(__wrap_MQTTDisconnect, ZSUCCESS);
    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_connect(&client);
    assert_int_equal(zclient_disconnect(&client), ZSUCCESS);
}

// ADD NUMBER :

static void AddNumberMethod_WithNullArguments_ShouldFail(void **state)
{
    // Adding Number will null key  or client would fail.
    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addNumber(NULL, "key1", 2);
    assert_int_equal(zclient_addNumber(&client, NULL, 10), ZFAILURE);
}

static void AddNumberMethod_CalledWithoutInitialization_ShouldFail(void **state)
{
    // Client must be initialized to add number to cjson payload.
    IOTclient client;
    assert_int_equal(zclient_addNumber(&client, "key1", 2), -2);
}

static void AddNumberMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue(void **state)
{
    // AddNumber with same key returns SUCCESS , old value gets replaced.
    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addNumber(&client, "key1", 1);
    zclient_addNumber(&client, "key1", 2);
    assert_int_equal(2, cJSON_GetObjectItem(client.message.data, "key1")->valueint);
}

// ADD STRING :

static void AddStringMethod_CalledWithoutInitialization_ShouldFail(void **state)
{
    // Client must be initialized to add string to cjson payload.
    IOTclient client;
    assert_int_equal(zclient_addString(&client, "key1", "value1"), -2);
}

static void AddStringMethod_OnNullArguments_ShouldFail(void **state)
{
    // AddString with null key/value returns FAILURE
    IOTclient client;
    int rc = zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addString(NULL, "Key", "value");
    assert_int_equal(zclient_addString(&client, "Key", NULL), ZFAILURE);
}

static void AddStringMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue(void **state)
{
    // AddString with same key returns SUCCESS , old value gets replaced.
    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_addString(&client, "str_key", "str_val1");
    zclient_addString(&client, "str_key", "str_val2");
    assert_int_equal(strcmp("str_val2", cJSON_GetObjectItem(client.message.data, "str_key")->valuestring), 0);
}

// RECONNECT :

static void ReconnectMethod_WithExistingConnection_ShouldSucceed(void **state)
{
    //  Reconnecting over the existing connection returns SUCCESS.
    IOTclient client;
    client.current_state = Connected;
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
    IOTclient client;
    assert_int_equal(zclient_reconnect(&client), -2);
}

static void ReconnectMethod_OnLostConnection_ShouldRetryAndSucceed(void **state)
{
    // Reconnect method with lost connection can Retry connection and succeed.
    will_return(__wrap_NetworkConnect, ZSUCCESS);
    will_return(__wrap_MQTTConnect, ZSUCCESS);
    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    client.current_state = Disconnected;
    assert_int_equal(zclient_reconnect(&client), ZSUCCESS);
}

// SET RETRY COUNT :

static void SetRetryCountMethod_CalledWithoutInitializingClient_ShouldFail(void **state)
{
    // retry count method returns failure as client must be initialized .
    IOTclient client;
    assert_int_equal(zclient_setRetrycount(&client, 10), -2);
}

static void SetRetryCountMethod_WithNullArguments_ShouldFail(void **state)
{
    // set Retry count method fails since client is NULL.
    assert_int_equal(zclient_setRetrycount(NULL, 10), ZFAILURE);
}

static void SetRetryCountMethod_WithNegativeCount_ShouldFail_DefaultValueIsUnchanged(void **state)
{
    // Retry count is set to default when negative value is set.
    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    assert_int_equal(zclient_setRetrycount(&client, -10), ZFAILURE);
}

static void SetRetryCountMethod_WithAppropriateArguments_ShouldSucceed(void **state)
{
    // Setting retry count with appropriate arguments succced.
    IOTclient client;
    zclient_init(&client, mqttUserName, mqttPassword, EMBED, "", "", "", "");
    zclient_setRetrycount(&client, 10);
    assert_int_equal(client.config.retry_limit, 10);
}

int main(void)
{
    const struct CMUnitTest sdk_basic_tests[] =
        {
            cmocka_unit_test(InitMethod_OnNullArguments_ShouldFail),
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
            cmocka_unit_test(PublishMethod_OnNUllArguments_ShouldFail),
            cmocka_unit_test(PublishMethod_OnCallingBeforeInitialization_ShouldFail),
            cmocka_unit_test(PublishMethod_WithLostConnection_ShouldFail),
            cmocka_unit_test(PublishMethod_WithNonNullArguments_ShouldSucceed),
            cmocka_unit_test(DispatchMethod_OnCallingBeforeInitialization_ShouldFail),
            cmocka_unit_test(DispatchMethod_OnNullArguments_ShouldFail),
            cmocka_unit_test(DispatchMethod_WithNoConnection_ShouldFail),
            cmocka_unit_test(DispatchMethod_WithProperConnection_ShouldSucceed),
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
            cmocka_unit_test(AddStringMethod_CalledWithoutInitialization_ShouldFail),
            cmocka_unit_test(AddStringMethod_OnNullArguments_ShouldFail),
            cmocka_unit_test(AddStringMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue),
            cmocka_unit_test(ReconnectMethod_WithExistingConnection_ShouldSucceed),
            cmocka_unit_test(ReconnectMethod_OnNullArguments_ShouldFail),
            cmocka_unit_test(ReconnectMethod_OnCallingBeforeInitialization_ShouldFail),
            cmocka_unit_test(ReconnectMethod_OnLostConnection_ShouldRetryAndSucceed),
            cmocka_unit_test(SetRetryCountMethod_CalledWithoutInitializingClient_ShouldFail),
            cmocka_unit_test(SetRetryCountMethod_WithNullArguments_ShouldFail),
            cmocka_unit_test(SetRetryCountMethod_WithNegativeCount_ShouldFail_DefaultValueIsUnchanged),
            cmocka_unit_test(SetRetryCountMethod_WithAppropriateArguments_ShouldSucceed)};
    cmocka_set_message_output(CM_OUTPUT_XML);
    return cmocka_run_group_tests(sdk_basic_tests, NULL, NULL);
}
