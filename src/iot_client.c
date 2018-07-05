//#include "MQTTClient.h"
#include "iot_client.h"
#include "utils.h"

int init_config(IOTclient *iot_client, char *device_id, char *auth_token)
{
    Config config = {NULL, NULL};
    //TODO: Add config validations

    cloneString(&config.device_id, device_id);
    cloneString(&config.auth_token, auth_token);

    iot_client->dev_config = config;

    //TODO: return values based on the config status.
    return 1;
}

int iot_connect(IOTclient *client)
{
    /* Connection Manual:
    https://github.com/eclipse/paho.mqtt.c/blob/master/src/samples/MQTTClient_publish.c
    */
    //TODO: move these conn options to header file
    /*
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    //TODO: token needs to be passed.
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    */
    //TODO: return values based on the conn status.
    return 1;
}
/*
int sendTestMessage(IOTclient *client, unsigned char *payload)
{
    long megTimeout = 10000L;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    pubmsg.payload = payload;
    pubmsg.payloadlen = (int)strlen(payload);
    pubmsg.qos = QOS0;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(client, "TEST_TOPIC", &pubmsg, &token);

    printf("Waiting for up to %d seconds for publication of %s\n"
           "on topic %s\n",
           (int)(megTimeout / 1000), payload, "TEST_TOPIC");
    rc = MQTTClient_waitForCompletion(client, token, megTimeout);
    printf("Message with delivery token %d delivered\n", token);
    return rc;
}
*/
/*
int iot_disconnect(IOTclient *client)
{
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}
*/