#include "iot_client.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#if defined(SECURE_CONNECTION)
#if defined(EMBED_MODE)
#include "certificates.h"
#endif
#endif
volatile int ctrl_flag = 0;

void message_handler(MessageData *data)
{
    char payload[data->message->payloadlen];
    char topic[data->topicName->lenstring.len];
    *topic = '\0';
    *payload = '\0';
    strncat(topic, data->topicName->lenstring.data, data->topicName->lenstring.len);
    strncat(payload, data->message->payload, data->message->payloadlen);
    log_debug("Got new message on '%s'\n%s", topic, payload);
}

void interruptHandler(int signo)
{
    log_info("Closing connection...");
    ctrl_flag += 1;
    if (ctrl_flag >= 2)
    {
        log_fatal("Program is force killed");
        exit(0);
    }
}

int main()
{
    int rc = -1;
    IOTclient client;

    signal(SIGINT, interruptHandler);
    signal(SIGTERM, interruptHandler);

    char *payload;
    int temperature = 23;
    char *pRootCACertLocation = "", *pDeviceCertLocation = "", *pDevicePrivateKeyLocation = "", *pDeviceCertParsword = "";

#if defined(SECURE_CONNECTION)
    pRootCACertLocation = CA_CRT;
#if defined(USE_CLIENT_CERTS)
    pDeviceCertLocation = CLIENT_CRT;
    pDevicePrivateKeyLocation = CLIENT_KEY;
#endif
#endif

    //Update your DEVICE_ID AND AUTH_TOKEN below:
    rc = zclient_init(&client, "636000000059696", "OlyO55SCMB91yZcifsLK2PTmm0z1969ZiCtGPqHm3mKWliG3AB2lHsAFNX+QbZYgxv5J8qJaw6z2SpSNm3Wa5s71ULiorY0KeItwz2yAguFPUT8bBpZbSGcuf7byvJWv", CRT_PARSE_MODE, pRootCACertLocation, pDeviceCertLocation, pDevicePrivateKeyLocation, pDeviceCertParsword);
    if (rc != ZSUCCESS)
    {
        return 0;
    }
    rc = zclient_connect(&client);
    while (rc != ZSUCCESS && ctrl_flag == 0)
    {
        //Endless reconnection on start. No data collection happens during connection error.
        rc = zclient_reconnect(&client);
    }

    // rc = zclient_subscribe(&client, message_handler);
    while (ctrl_flag == 0)
    {
        temperature += 2;
        rc = zclient_addNumber(&client, "temperature", temperature);
        rc = zclient_addString(&client, "status", "OK");

        //payload = zclient_getpayload();
        //rc = zclient_publish(&client, payload);
        rc = zclient_dispatch(&client);
        rc = zclient_yield(&client, 300);
        if (rc == ZCONNECTION_ERROR)
        {
            break;
        }
    }

    zclient_disconnect(&client);
    return 0;
}