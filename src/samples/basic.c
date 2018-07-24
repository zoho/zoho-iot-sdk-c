#include "iot_client.h"
#include <signal.h>

volatile int ctrl_flag = 0;

void message_handler(MessageData *data)
{
    printf("\nMessage Received!\n");

    printf("Topic :%.*s \n", (int)data->topicName->lenstring.len, (char *)data->topicName->lenstring.data);
    printf("Payload : %.*s \n", (int)data->message->payloadlen, (char *)data->message->payload);
}

void interruptHandler(int signo)
{
    printf("\n Interrupt observed!.\n");
    ctrl_flag += 1;

    if (ctrl_flag >= 2)
    {
        printf("Program is force killed\n");
        exit(0);
    }
}

int main()
{
    int rc = -1;
    IOTclient client;
    char *pub_topic = "test_topic9876";
    char *sub_topic = "test_topic9877";
    //char *host = "m2m.eclipse.org";
    // char *host = "iot.eclipse.org";
    char *host = "test.mosquitto.org";
    const int port = 1883;

    signal(SIGINT, interruptHandler);
    signal(SIGTERM, interruptHandler);

    //rc = zclient_init(&client, "deviceID", "authToken","iot","iot");
    rc = zclient_init(&client, "deviceID", "authToken",NULL,NULL);
    if (rc != SUCCESS)
    {
        printf("Initialize failed and returned. rc = %d.\n Quitting..", rc);
        return 0;
    }

    rc = zclient_connect(&client, host, port);
    if (rc != SUCCESS)
    {
        printf("Connection failed and returned. rc = %d.\n Quitting..\n", rc);
        return 0;
    }

    rc = zclient_publish(&client, pub_topic, "hello IOT world!");
    if (rc != SUCCESS)
    {
        printf("Unable to send message ..%d\n", rc);
        return 0;
    }

    rc = zclient_subscribe(&client, sub_topic, message_handler);
    printf("Subscribed!\n");
    while (ctrl_flag == 0)
    {
        zclient_yield(&client, 500);
    }

    zclient_disconnect(&client);
    return 0;
}