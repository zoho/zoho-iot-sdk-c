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

static void dummy_test1(void **state)
{
    assert_int_equal(3, 3);
    assert_int_equal(zclient_addNumber("sdf", 8), -1);
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
}

int main(void)
{
    const struct CMUnitTest sdk_basic_tests[] =
        {
            cmocka_unit_test(dummy_test1),
            cmocka_unit_test(dummy_test2),
            cmocka_unit_test(shouldInitializeBeforeCallConnect)};

    return cmocka_run_group_tests(sdk_basic_tests, NULL, NULL);
}
