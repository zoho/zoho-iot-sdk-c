#include "iot_client.h"
#include "utils.h"

//TODO: read from config file.
Network n;
int rc;

//TODO: Remove all debug statements and use logger.

int zclient_init(IOTclient *iot_client, char *device_id, char *auth_token)
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

int zclient_connect(IOTclient *client, char *host, int port)
{
    unsigned const int buff_size = 1000;
    unsigned char buf[buff_size], readbuf[buff_size];
    printf("Preparing Network..");
    NewNetwork(&n);
    ConnectNetwork(&n, host, port);
    printf("Connecting..");
    MQTTClient(&client->mqtt_client, &n, 1000, buf, buff_size, readbuf, buff_size);
    MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;
    //TODO:
    // remove hardcoded values of username and pwd and get from structure copied from config.h;
    conn_data.MQTTVersion = 3;
    conn_data.cleansession = 1; //TODO: tobe confirmed with Hub
    conn_data.keepAliveInterval = 60;
    conn_data.clientID.cstring = client->dev_config.device_id;
    conn_data.willFlag = 0;

    //TODO: actual values needs tobe passed here.- username,auth token.
    conn_data.username.cstring = "";
    conn_data.password.cstring = "";

    printf("Connecting to %s on port : %d\n", host, port);
    rc = MQTTConnect(&client->mqtt_client, &conn_data);
    printf("Connection status: %d\n", rc);
    return rc;
}

int zclient_publish(IOTclient *client, char *topic, char *payload)
{
    int status;
    MQTTMessage pubmsg;
    //TODO:
    // remove hardcoded values of id etc and get from structure copied from config.h;

    //TODO: confirm the below parameters with Hub
    pubmsg.id = 1234;
    pubmsg.dup = '0';
    pubmsg.qos = 0;
    pubmsg.retained = '0';

    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    status = MQTTPublish(&(client->mqtt_client), topic, &pubmsg);
    //TODO: check for connection and retry to send the message once the conn got restroed.
    printf("Delivery status:%d\n", status);
    return status;
}

int zclient_disconnect(IOTclient *client)
{
    rc = MQTTDisconnect(&client->mqtt_client);
    linux_disconnect(&n);
    return rc;
}