#include "iot_client.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>

volatile int ctrl_flag = 0;

void message_handler(MessageData *data)
{
    char payload[data->message->payloadlen];
    char topic[data->topicName->lenstring.len];

    strncpy(topic, data->topicName->lenstring.data, data->topicName->lenstring.len);
    strncpy(payload, data->message->payload, data->message->payloadlen);

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
    IOTclient client;

    signal(SIGINT, interruptHandler);
    signal(SIGTERM, interruptHandler);

    char *pub_topic = "test_topic9876";
    char *sub_topic = "test_topic9877";
    //char *host = "m2m.eclipse.org";
    // char *host = "iot.eclipse.org";
    // char *host = "test.mosquitto.org";
    //TODO: Move host and port into sdk.
    char *host = "172.22.142.33";
    int port = 1883;
    char *pRootCACertLocation = "", *pDeviceCertLocation = "", *pDevicePrivateKeyLocation = "";
#ifdef SECURE_CONNECTION
    port = 8883;
    pRootCACertLocation = "/mymac/Documents/simple_tls/certs/trail/ca.crt";
    pDeviceCertLocation = "/mymac/Documents/simple_tls/certs/trail/client.crt";
    pDevicePrivateKeyLocation = "/mymac/Documents/simple_tls/certs/trail/client.key";
#endif

    //TODO: remove the unused username & password parameters.
    // rc = zclient_init(&client, "deviceID", "authToken","iot","iot");
    rc = zclient_init(&client, "deviceID", "authToken", NULL, NULL);
    if (rc != SUCCESS)
    {
        return 0;
    }

    rc = zclient_connect(&client, host, port, pRootCACertLocation, pDeviceCertLocation, pDevicePrivateKeyLocation, "");
    if (rc != SUCCESS)
    {
        return 0;
    }

    rc = zclient_publish(&client, pub_topic, "hello IOT world!");
    if (rc != SUCCESS)
    {
        return 0;
    }

    rc = zclient_subscribe(&client, sub_topic, message_handler);
    while (ctrl_flag == 0)
    {
        zclient_yield(&client, 300);
    }

    zclient_disconnect(&client);
    return 0;
}