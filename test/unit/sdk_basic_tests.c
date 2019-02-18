#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <setjmp.h>
#include <cmocka.h>
#include "iot_client.h"
#include "generic.h"

static void InitMethod_OnNullArguments_ShouldFail(void **state)
{
    // Init returns failure as Client object is NULL
    assert_int_equal(zclient_init(NULL, "device_id", "token", EMBED, "", "", "", ""), ZFAILURE);

    // Init returns failure as Device Credentials are NULL
    IOTclient *client;
    assert_int_equal(zclient_init(client, NULL, NULL, EMBED, "", "", "", ""), ZFAILURE);
}

static void InitMethod_WithTls_NullCertificates_ShouldFail(void **State)
{
#ifdef SECURE_CONNECTION
    // Init return failure with TLS enabled as RootCA file is NULL .
    IOTclient client;
    assert_int_equal(zclient_init(&client, "device_id", "token", REFERENCE, NULL, "", "", ""), ZFAILURE);
// Init return failure with TLS enabled as Client_Crt /client_key is NULL .
#ifdef USE_CLIENT_CERTS
    assert_int_equal(zclient_init(&client, "device_id", "token", REFERENCE, "/usr/device_certificate.pem", NULL, NULL, ""), ZFAILURE);
#endif
#endif
}

static void ConnectMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // connecting to HUB with out initializing client would return FAILURE
    IOTclient client;
    assert_int_equal(zclient_connect(&client), -2);
}

static void ConnectMethod_OnNullArguments_ShouldFail(void **state)
{
    // connect returns Failure for Null Client object.
    assert_int_equal(zclient_connect(NULL), ZFAILURE);
}

static void ConnectMethod_OnConnectOverExistingConnetion_ShouldSucceed(void **state)
{
    // connect returns SUCCESS for connect called on existing connection.
    IOTclient client;
    assert_int_equal(zclient_init(&client, "device_id", "token", EMBED, "", "", "", ""),ZSUCCESS);
    client.current_state = Connected;
    assert_int_equal(zclient_connect(&client), ZSUCCESS);
}

static void DispacthMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // Dispatch Payload with out initializing client would return FAILURE
    IOTclient client;
    assert_int_equal(zclient_dispatch(&client), -2);
}

static void DispatchMethod_OnNullArguments_ShouldFail(void **state)
{
    // Dispatch Payload returns Failure for Null Client object.
    assert_int_equal(zclient_dispatch(NULL), ZFAILURE);
}

static void SubscribeMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // Subscribing with out initializing client would return FAILURE
    IOTclient client;
    messageHandler msghnd;
    assert_int_equal(zclient_subscribe(&client, msghnd), -2);
}

static void SubscribeMethod_OnNullArguments_ShouldFail(void **state)
{
    // Subscribe returns Failure for Null Client object.
    messageHandler msghnd;
    assert_int_equal(zclient_subscribe(NULL, msghnd), ZFAILURE);
    // Subscribe returns Failure for Null messageHandler object.
    //     IOTclient client;
    //     zclient_init(&client, "device_id" , "token" , EMBED, "","","","");
    //     assert_int_equal(zclient_subscribe(&client,NULL),-1);
}

static void YieldMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // Subscribing with out initializing client would return FAILURE
    IOTclient client;
    assert_int_equal(zclient_yield(&client, 100), -2);
}

static void YieldMethod_OnNullArguments_ShouldFail(void **state)
{
    //Yield returns Failure for Null Client object.
    assert_int_equal(zclient_yield(NULL, 1000), ZFAILURE);

    //Yield returns Failure for non positive timeout.
    IOTclient client;
    assert_int_equal(zclient_yield(&client, 0), ZFAILURE);
}

static void DisconnectMethod_OnCallingBeforeInitialization_ShouldFail(void **state)
{
    // Subscribing with out initializing client would return FAILURE
    IOTclient client;
    messageHandler msghnd;
    assert_int_equal(zclient_subscribe(&client, msghnd), ZFAILURE);
}

static void DisconnectMethod_OnNullArguments_ShouldFail(void **state)
{
    //Disconnect returns Failure for Null Client object.
    assert_int_equal(zclient_disconnect(NULL), ZFAILURE);
}

static void DisconnectMethod_OnUnEstablishedConnetion_ShouldSucceed(void **state)
{
    // connect returns SUCCESS for connect called on existing connection.
    IOTclient client;
    assert_int_equal(zclient_init(&client, "device_id", "token", EMBED, "", "", "", ""),ZSUCCESS);
    assert_int_equal(zclient_disconnect(&client), ZSUCCESS);
}


static void ReconnectMethod_WithExistingConnection_ShouldSucceed(void **state)
{
    //  Reconnecting over the existing connection returns SUCCESS.
    IOTclient client;
    client.current_state = Connected;
    assert_int_equal(zclient_reconnect(&client), ZSUCCESS);
}
static void ReconnectMethod_OnNullArguments_ShouldFail(void **state)
{
    //Reconnect returns Failure on NULL client object 
    assert_int_equal(zclient_reconnect(NULL), ZFAILURE);
}

static void AddString_OnNullArguments_ShouldFail(void **state)
{
    // AddString with null key/value returns FAILURE
    IOTclient client;
    int rc = zclient_init(&client, "device_id", "token", EMBED, "", "", "", "");
    assert_int_equal(zclient_addString("Key", NULL), ZFAILURE);
}

static void AddStringMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue(void **state)
{
    // AddString with same key returns SUCCESS , old value gets replaced.
    IOTclient client;
    zclient_init(&client, "device_id", "token", EMBED, "", "", "", "");
    zclient_addString("str_key", "str_val1");
    assert_int_equal(zclient_addString("str_key", "str_val2"), 0);
}

static void AddNumberMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue(void **state)
{
    // AddNumber with same key returns SUCCESS , old value gets replaced.
    IOTclient client;
    zclient_init(&client, "device_id", "token", EMBED, "", "", "", "");
    zclient_addNumber("key1", 1);
    assert_int_equal(zclient_addNumber("key1", 2), 0);
}


static void PublishMethod_OnCallingBeforeInitialization_ShouldFail()
{
    // connecting to HUB with out initializing client would return FAILURE
    IOTclient client;
    assert_int_equal(zclient_publish(&client,"hello"), ZFAILURE);
}

static void PublishMethod_OnNUllArguments_ShouldFail(void **state)
{
    // Publish method returns FAILURE as client object is null.
    IOTclient *client = NULL;
    assert_int_equal(zclient_publish(NULL, "hello"), ZFAILURE);
}



int main(void)
{
    const struct CMUnitTest sdk_basic_tests[] =
        {
            cmocka_unit_test(InitMethod_OnNullArguments_ShouldFail),
            cmocka_unit_test(ConnectMethod_OnCallingBeforeInitialization_ShouldFail),
            cmocka_unit_test(ConnectMethod_OnNullArguments_ShouldFail),
            cmocka_unit_test(ConnectMethod_OnConnectOverExistingConnetion_ShouldSucceed),
            cmocka_unit_test(DispacthMethod_OnCallingBeforeInitialization_ShouldFail),
            cmocka_unit_test(DispatchMethod_OnNullArguments_ShouldFail),
            cmocka_unit_test(SubscribeMethod_OnCallingBeforeInitialization_ShouldFail),
            cmocka_unit_test(SubscribeMethod_OnNullArguments_ShouldFail),
            cmocka_unit_test(YieldMethod_OnCallingBeforeInitialization_ShouldFail),
            cmocka_unit_test(YieldMethod_OnNullArguments_ShouldFail),
            cmocka_unit_test(DisconnectMethod_OnNullArguments_ShouldFail),
            cmocka_unit_test(DisconnectMethod_OnUnEstablishedConnetion_ShouldSucceed),
            cmocka_unit_test(AddString_OnNullArguments_ShouldFail),
            cmocka_unit_test(ReconnectMethod_WithExistingConnection_ShouldSucceed),
            cmocka_unit_test(ReconnectMethod_OnNullArguments_ShouldFail),
            cmocka_unit_test(AddStringMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue),
            cmocka_unit_test(AddNumberMethod_OnAddingSamekey_ShouldSucceed_ReplacingOldValue),
            cmocka_unit_test(InitMethod_WithTls_NullCertificates_ShouldFail),
            cmocka_unit_test(PublishMethod_OnNUllArguments_ShouldFail),
            cmocka_unit_test(PublishMethod_OnCallingBeforeInitialization_ShouldFail)
            };

    cmocka_set_message_output(CM_OUTPUT_XML);
    return cmocka_run_group_tests(sdk_basic_tests, NULL, NULL);
}
