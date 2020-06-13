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
    int temperature = 23, humidity = 56, pressure = 78;
    char *pRootCACertLocation = "", *pDeviceCertLocation = "", *pDevicePrivateKeyLocation = "", *pDeviceCertParsword = "";

#if defined(SECURE_CONNECTION)
    pRootCACertLocation = CA_CRT;
#if defined(USE_CLIENT_CERTS)
    pDeviceCertLocation = CLIENT_CRT;
    pDevicePrivateKeyLocation = CLIENT_KEY;
#endif
#endif

    //Update your DEVICE_ID AND AUTH_TOKEN below:
    rc = zclient_init(&client, "/domain_name/v1/devices/client_id/connect", "mqtt_password", CRT_PARSE_MODE, pRootCACertLocation, pDeviceCertLocation, pDevicePrivateKeyLocation, pDeviceCertParsword);
    // rc = zclient_init_config_file(&client, "MQTTConfigFileName", CRT_PARSE_MODE);
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
        humidity *= 2;
        pressure -= 5;
        rc = zclient_addNumber(&client, "temperature", temperature);
        rc = zclient_addNumber(&client, "humidity", humidity, "room1");
        rc = zclient_addNumber(&client, "pressure", pressure, "room2");
        rc = zclient_markDataPointAsError(&client, "mositure", "room2");
        rc = zclient_markDataPointAsError(&client, "air_quality");
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