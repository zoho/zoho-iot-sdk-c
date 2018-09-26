#include "iot_client.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>

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
    char *pRootCACertLocation = "", *pDeviceCertLocation = "", *pDevicePrivateKeyLocation = "" , *pDeviceCertParsword = "";
    
#if defined(SECURE_CONNECTION)
    pRootCACertLocation = ROOTCA_CERT_LOCATION;
    #if defined(USE_CLIENT_CERTS)
        pDeviceCertLocation = CLIENT_CERT_LOCATION;
        pDevicePrivateKeyLocation = CLIENT_KEY_LOCATION;
    #endif
#endif

    //TODO: remove the unused username & password parameters.
    // rc = zclient_init(&client, "deviceID", "authToken","iot","iot");
    rc = zclient_init(&client, "deviceID", "authToken", NULL, NULL, pRootCACertLocation, pDeviceCertLocation, pDevicePrivateKeyLocation, pDeviceCertParsword);
    if (rc != SUCCESS)
    {
        return 0;
    }

    rc = zclient_connect(&client);
    if (rc != SUCCESS)
    {
        return 0;
    }

    rc = zclient_subscribe(&client, message_handler);
    while (ctrl_flag == 0)
    {
        temperature += 2;
        rc = zclient_addNumber("temperature", temperature);
        rc = zclient_addString("status", "OK");

//        payload = zclient_getpayload();
//        rc = zclient_publish(&client, payload);
        rc = zclient_dispatch(&client);
        if (rc != SUCCESS)
        {
            log_error("Can't publish\n");
            break;
        }
        zclient_yield(&client, 300);
    }

    zclient_disconnect(&client);
    return 0;
}