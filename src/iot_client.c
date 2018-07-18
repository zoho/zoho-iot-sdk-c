#include "iot_client.h"
#include "utils.h"

//TODO: read from config file.
Network n;
int rc;

//TODO: Remove all debug statements and use logger.

int init_config(IOTclient *iot_client, char *device_id, char *auth_token)
{
    //TODO:
    // all config.h and device related validations should be done here itself !
    Config config = {NULL, NULL};
    cloneString(&config.device_id, device_id);
    cloneString(&config.auth_token, auth_token);
    iot_client->dev_config = config;
    //TODO: freeup config.
    printf("Initialized!");
    return SUCCESS;
}

int iot_connect(IOTclient *client, char *host, int port)
{
    unsigned const int buff_size = 100;
    unsigned char buf[buff_size], readbuf[buff_size];
    printf("Connecting..");
    NewNetwork(&n);
    ConnectNetwork(&n, host, port);
    MQTTClient(&client->mqtt_client, &n, 1000, buf, buff_size, readbuf, buff_size);
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    //TODO:
    // remove hardcoded values of username and pwd and get from structure copied from config.h;
    data.willFlag = 0;
    data.MQTTVersion = 3;
    data.clientID.cstring = client->dev_config.device_id;
    //TODO: actual values needs tobe passed here.- username,auth token.
    data.username.cstring = "";
    data.password.cstring = "";
    data.keepAliveInterval = 60;
    data.cleansession = 1; //TODO: tobe confirmed with Hub
    printf("Connecting to %s on port : %d\n", host, port);
    rc = MQTTConnect(&client->mqtt_client, &data);
    printf("Connection status: %d\n", rc);
    return rc;
}

int sendTestMessage(IOTclient *client, char *topic, char *payload)
{
    MQTTMessage pubmsg;
    //TODO:
    // remove hardcoded values of id etc and get from structure copied from config.h;

    //TODO: confirm the below parameters with Hub
    pubmsg.qos = 0;
    pubmsg.retained = '0';
    pubmsg.dup = '0';
    pubmsg.id = 1234;
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    rc = MQTTPublish(&(client->mqtt_client), topic, &pubmsg);
    printf("Delivery status:%d\n", rc);
    return rc;
}

int iot_disconnect(IOTclient *client)
{
    rc = MQTTDisconnect(&client->mqtt_client);
    linux_disconnect(&n);
    return rc;
}