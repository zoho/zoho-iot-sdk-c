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

static void dummy_test1(void **state)
{
    assert_int_equal(3, 3);
    assert_int_equal(zclient_addNumber("sdf", 8), -1);
    assert_int_equal(zclient_addString("key","value"), -1);
}

static void dummy_test2(void **state)
{
    assert_int_not_equal(3, 4);
}

static void shouldInitializeBeforeCallConnect(void **state)
{
    IOTclient client;
    int rc = zclient_connect(&client);
    assert_int_equal(rc, -2);
    rc = zclient_dispatch(&client);
    assert_int_equal(rc, -1);
    messageHandler msghandle;
    rc = zclient_subscribe(&client,msghandle);
    assert_int_equal(rc, -1);

}

static void Retrun_Failure_OnNullValues(void **state)
{
    int rc = zclient_init(NULL, "device_id", "token", EMBED, "","","","");
    assert_int_equal(rc, -1);
    IOTclient *client ;
    rc = zclient_init(client, NULL , NULL , EMBED, "","","","");
    assert_int_equal(rc,-1);
    client = NULL;
    rc = zclient_connect(client);
    assert_int_equal(rc,-1);
    rc = zclient_dispatch(client);
    assert_int_equal(rc,-1);
    messageHandler msghandle;
    rc = zclient_subscribe(client,msghandle);
    assert_int_equal(rc,-1);
    rc = zclient_yield(client,1000);
    assert_int_equal(rc,-1);
    rc = zclient_disconnect(client);
    assert_int_equal(rc,-1);
}


static void ReturnFailure_OnAddingNUll_ToAddString(void **state)
{
    IOTclient client;
    int rc = zclient_init(&client,"device_id","token",EMBED,"","","","");
    assert_int_equal(zclient_addString("Key",NULL),-1);
}

static void ReturnTrue_WhenSameKeyAdded_replacing_OldValue(void **state)
{
    IOTclient client;
    zclient_init(&client,"device_id","token",EMBED,"","","","");
    int rc = zclient_addNumber("key1",1);
    rc = zclient_addNumber("key1",2);
    assert_int_equal(rc,0);
    rc = zclient_addString("str_key","str_val1");
    rc = zclient_addString("str_key","str_val2");
    assert_int_equal(rc,0);
}


int main(void)
{
    const struct CMUnitTest sdk_basic_tests[] =
        {
            cmocka_unit_test(dummy_test1),
            cmocka_unit_test(dummy_test2),
            cmocka_unit_test(shouldInitializeBeforeCallConnect),
            cmocka_unit_test(Retrun_Failure_OnNullValues),
            cmocka_unit_test(ReturnFailure_OnAddingNUll_ToAddString),
            cmocka_unit_test(ReturnTrue_WhenSameKeyAdded_replacing_OldValue)};

    cmocka_set_message_output(CM_OUTPUT_XML);
    return cmocka_run_group_tests(sdk_basic_tests, NULL, NULL);
}
