#include "iot_client.h"
#include "sys/socket.h"
#include "unistd.h"

//TODO: read from config file.
Network n;
int rc;
cJSON *cJsonPayload,*data;
//TODO: Remove all debug statements and use logger.
//TODO: Add logging for all important connection scenarios.
//TODO: Add idle methods when socket is busy as in ssl_client_2.

int zclient_init(IOTclient *iot_client, char *device_id, char *auth_token, char *username, char *password, char *ca_crt, char *client_cert, char *client_key, char *cert_password)
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
    iot_client->config = config;

#if defined(SECURE_CONNECTION)
    iot_client->certs.ca_crt = ca_crt;
    #if defined(USE_CLIENT_CERTS)
    iot_client->certs.client_cert = client_cert;
    iot_client->certs.client_key = client_key;
    iot_client->certs.cert_password = cert_password;
    #endif
#endif

    //TODO: freeup config.
    cJsonPayload = cJSON_CreateObject();
    data =cJSON_CreateObject();
    if (cJsonPayload == NULL || data == NULL)
    {
        log_error("Can't create cJSON object");
        return FAILURE;
    }
    cJSON_AddStringToObject(cJsonPayload,"ClientID",device_id);
    log_info("SDK Initialized!");
    return SUCCESS;
}

int zclient_connect(IOTclient *client)
{
    //TODO: verify the buff size on real device. and flush the buffer at end of connection.
    unsigned const int buff_size = 10000;
    unsigned char buf[buff_size], readbuf[buff_size];

    log_info("Preparing Network..");

    NewNetwork(&n);

#if defined(SECURE_CONNECTION)
    rc = ConnectNetwork(&n, hostname, port, client->certs.ca_crt, client->certs.client_cert, client->certs.client_key, client->certs.cert_password);
#else
    rc = ConnectNetwork(&n, hostname, port);
#endif
    if (rc != 0)
    {
        log_error("Error Connecting Network..");
        client->mqtt_client.ipstack->disconnect(client->mqtt_client.ipstack);
        return -1;
    }

    //TODO: Handle the rc of ConnectNetwork().
    log_info("Connecting to \x1b[32m %s : %d \x1b[0m", hostname, port);
    MQTTClient(&client->mqtt_client, &n, 1000, buf, buff_size, readbuf, buff_size);
    MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;
    //TODO:remove hardcoded values of username and pwd and get from structure copied from config.h;

    conn_data.MQTTVersion = 3;
    conn_data.cleansession = 1; //TODO: tobe confirmed with Hub
    conn_data.keepAliveInterval = 60;
    conn_data.clientID.cstring = client->config.device_id;
    conn_data.willFlag = 0;

    conn_data.username.cstring = client->config.username;
    conn_data.password.cstring = client->config.password;

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

int zclient_reconnect(IOTclient *client)
{
    int rc = -1, delay = 5;
    while (rc != 0)
    {
        log_info("Trying to reconnect in %d sec ", delay);
        sleep(delay);
        rc = zclient_connect(client);
    }
    return rc;
}

int zclient_publish(IOTclient *client, char *payload)
{
    MQTTMessage pubmsg;
    //TODO:remove hardcoded values of id etc and get from structure copied from config.h
    //TODO: confirm the below parameters with Hub

    pubmsg.id = 1234;
    pubmsg.dup = '0';
    pubmsg.qos = 2;
    pubmsg.retained = '0';

    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    rc = MQTTPublish(&(client->mqtt_client),data_topic, &pubmsg);
    //TODO: check for connection and retry to send the message once the conn got restroed.
    if (rc == 0)
    {
        log_debug("Published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", payload,data_topic);
    }
    else
    {
        log_error("Error on Pubish. Error code: %d", rc);
        rc = zclient_reconnect(client);
    }
    return rc;
}

int zclient_dispatch(IOTclient *client)
{
    //TODO: Add time stamp, Client ID
    time_t curtime;
    time(&curtime);
    char *time_val =strtok(ctime(&curtime),"\n");
    if (!cJSON_HasObjectItem(cJsonPayload, "time"))
    {
        cJSON_AddStringToObject(cJsonPayload,"time",time_val);
    }
    else
    {
        cJSON *temp = cJSON_CreateString(time_val);
        cJSON_ReplaceItemInObject(cJsonPayload,"time",temp);
    }

    if (!cJSON_HasObjectItem(cJsonPayload, "data"))
    {
       cJSON_AddItemToObject(cJsonPayload,"data",data); 
    }
    else
    {
        cJSON_ReplaceItemInObject(cJsonPayload,"data",data);
    }

    char *payload = cJSON_Print(cJsonPayload);
    rc = zclient_publish(client,payload);
    return rc;
}

int zclient_subscribe(IOTclient *client, messageHandler on_message)
{
    //TODO: add basic validation & callback method and append it on error logs.
    rc = MQTTSubscribe(&(client->mqtt_client), command_topic, QOS0, on_message);
    if (rc == 0)
    {
        log_info("Subscribed on \x1b[36m '%s' \x1b[0m", command_topic);
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
    cJSON_free(cJsonPayload);
    return rc;
}


int zclient_addString(char *val_name, char *val_string)
{
    int ret = 0;

    if (!cJSON_HasObjectItem(data, val_name))
    {
        if (cJSON_AddStringToObject(data, val_name, val_string) == NULL)
        {
            log_error("Adding string attribute failed\n");
            ret = -1;
        }
    }
    else
    {
        cJSON *temp = cJSON_CreateString(val_string);
        cJSON_ReplaceItemInObject(data, val_name, temp);
    }
    return ret;
}

int zclient_addNumber(char *val_name, int val_int)
{
    int ret = 0;

    if (!cJSON_HasObjectItem(data, val_name))
    {
        if (cJSON_AddNumberToObject(data, val_name, val_int) == NULL)
        {
            log_error("Adding int attribute failed\n");
            ret = -1;
        }
    }
    else
    {
        cJSON *temp = cJSON_CreateNumber(val_int);
        cJSON_ReplaceItemInObject(data, val_name, temp);
    }
    return ret;
}

/*
char *zclient_getpayload()
{
    return cJSON_Print(cJsonPayload);
}
*/