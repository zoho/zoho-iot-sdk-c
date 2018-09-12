#include "iot_client.h"
#include "utils.h"
#include "sys/socket.h"
#include "unistd.h"

//TODO: read from config file.
Network n;
int rc;

//TODO: Remove all debug statements and use logger.
//TODO: Add logging for all important connection scenarios.
//TODO: Add idle methods when socket is busy as in ssl_client_2.

int zclient_init(IOTclient *iot_client, char *device_id, char *auth_token, char *username, char *password)
{
    //TODO:
    // All config.h and device related validations should be done here itself !

    log_initialize();
    log_info("\n\n\nSDK Initializing..");

    Config config = {NULL, NULL, NULL, NULL};
    cloneString(&config.device_id, device_id);
    cloneString(&config.auth_token, auth_token);
    cloneString(&config.username, username);
    cloneString(&config.password, password);
    iot_client->dev_config = config;
    //TODO: freeup config.
    log_info("SDK Initialized!");
    return SUCCESS;
}

int zclient_connect(IOTclient *client, char *host, int port, char *ca_crt, char *client_cert, char *client_key, char *cert_password)
{
    //TODO: verify the buff size on real device. and flush the buffer at end of connection.
    unsigned const int buff_size = 10000;
    unsigned char buf[buff_size], readbuf[buff_size];

    log_info("Preparing Network..");

    NewNetwork(&n);

#if defined(SECURE_CONNECTION)
    rc = ConnectNetwork(&n, host, port, ca_crt, client_cert, client_key, cert_password);
#else
    rc = ConnectNetwork(&n, host, port);
#endif
    if (rc != 0)
    {
        log_error("Error Connecting Network..");
        client->mqtt_client.ipstack->disconnect(client->mqtt_client.ipstack);
        return -1;
    }

    //TODO: Handle the rc of ConnectNetwork().
    log_info("Connecting to \x1b[32m %s : %d \x1b[0m", host, port);
    MQTTClient(&client->mqtt_client, &n, 1000, buf, buff_size, readbuf, buff_size);
    MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;
    //TODO:
    // remove hardcoded values of username and pwd and get from structure copied from config.h;
    conn_data.MQTTVersion = 3;
    conn_data.cleansession = 1; //TODO: tobe confirmed with Hub
    conn_data.keepAliveInterval = 60;
    conn_data.clientID.cstring = client->dev_config.device_id;
    conn_data.willFlag = 0;

    conn_data.username.cstring = client->dev_config.username;
    conn_data.password.cstring = client->dev_config.password;

    rc = MQTTConnect(&client->mqtt_client, &conn_data);
    if (rc == 0)
    {
        log_info("Connected!");
    }
    else
    {
        log_error("Error while establishing connection. Error code: %d", rc);
    }
    return rc;
}

int zclient_publish(IOTclient *client, char *topic, char *payload)
{
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
    rc = MQTTPublish(&(client->mqtt_client), topic, &pubmsg);
    //TODO: check for connection and retry to send the message once the conn got restroed.
    if (rc == 0)
    {
        log_debug("Published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", payload, topic);
    }
    else
    {
        log_error("Error on Pubish. Error code: %d", rc);
    }
    return rc;
}

int zclient_subscribe(IOTclient *client, char *topic, messageHandler on_message)
{
    //TODO: add basic validation & callback method and append it on error logs.
    rc = MQTTSubscribe(&(client->mqtt_client), topic, QOS0, on_message);
    if (rc == 0)
    {
        log_info("Subscribed on \x1b[36m '%s' \x1b[0m", topic);
    }
    else
    {
        log_error("Error on Subscribe. Error code: %d", rc);
    }

    return rc;
}

int zclient_yield(IOTclient *client, int time_out)
{
    return MQTTYield(&client->mqtt_client, time_out);
}

int zclient_disconnect(IOTclient *client)
{
    rc = MQTTDisconnect(&client->mqtt_client);

    log_trace("connected socket ID :%d", client->mqtt_client.ipstack->my_socket);

    rc = shutdown(client->mqtt_client.ipstack->my_socket, SHUT_RDWR);
    if (rc)
    {
        log_trace("Error in shutdown");
        return -1;
    }

    rc = recv(client->mqtt_client.ipstack->my_socket, NULL, (size_t)0, 0);
    if (rc)
    {
        log_trace("Error in recv");
        return -1;
    }

    rc = close(client->mqtt_client.ipstack->my_socket);
    if (rc)
    {
        log_trace("Error in close");
        return -1;
    }

    client->mqtt_client.ipstack->disconnect(client->mqtt_client.ipstack);
    log_debug("Connection Closed with status :%d", client->mqtt_client.isconnected);
    log_free();
    return rc;
}