#include "iot_client.h"
#include "sys/socket.h"
#include "unistd.h"

//TODO: read from config file.
Network n;
int rc;
cJSON *cJsonPayload, *data;
certsParseMode parse_mode;
int retryCount = 0;
char dataTopic[100] = "", commandTopic[100] = "", eventTopic[100] = "";
//TODO: Remove all debug statements and use logger.
//TODO: Add logging for all important connection scenarios.
//TODO: Add idle methods when socket is busy as in ssl_client_2.

int zclient_init(IOTclient *iot_client, char *device_id, char *auth_token, certsParseMode mode, char *ca_crt, char *client_cert, char *client_key, char *cert_password)
{
    //TODO:1
    // All config.h and device related validations should be done here itself !

    log_initialize();
    log_info("\n\n\nSDK Initializing..");
    if(iot_client == NULL)
    {
        log_error("Client object is NULL");
        return ZFAILURE;
    }
    if(device_id == NULL || auth_token == NULL)
    {
        log_error("Device Credentials can't be NULL");
        return ZFAILURE;
    }

    //Populating dynamic topic names based on its deviceID
    sprintf(dataTopic, "%s/%s%s", topic_pre, device_id, data_topic);
    sprintf(commandTopic, "%s/%s%s", topic_pre, device_id, command_topic);
    sprintf(eventTopic, "%s/%s%s", topic_pre, device_id, event_topic);

    Config config = {NULL, NULL};
    cloneString(&config.device_id, device_id);
    cloneString(&config.auth_token, auth_token);
    iot_client->config = config;
    parse_mode = mode;
#if defined(SECURE_CONNECTION)
    if (ca_crt == NULL || (mode == REFERENCE && access(ca_crt, F_OK) == -1))
    {
        log_error("RootCA file is not found/can't be accessed");
        return ZFAILURE;
    }
    iot_client->certs.ca_crt = ca_crt;
#if defined(USE_CLIENT_CERTS)
    if (client_cert == NULL || client_key == NULL || (mode == REFERENCE && (access(client_cert, F_OK) == -1)) || (mode == REFERENCE && (access(client_key, F_OK) == -1)))
    {
        log_error("Client key or Client certificate is not found/can't be accessed");
        return ZFAILURE;
    }
    iot_client->certs.client_cert = client_cert;
    iot_client->certs.client_key = client_key;
    iot_client->certs.cert_password = cert_password;
#endif
#endif

    //TODO: freeup config.
    cJsonPayload = cJSON_CreateObject();
    data = cJSON_CreateObject();
    if (cJsonPayload == NULL || data == NULL)
    {
        log_error("Can't create cJSON object");
        return ZFAILURE;
    }
    cJSON_AddStringToObject(cJsonPayload, "device_id", device_id);
    iot_client->current_state = Initialized;
    log_info("Client Initialized!");
    return ZSUCCESS;
}

int zclient_connect(IOTclient *client)
{
    if(client == NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }
    //TODO: verify the buff size on real device. and flush the buffer at end of connection.
    if(client->current_state != Initialized || client->current_state != Connected || client->current_state != Disconnected)
    {
        log_error("Client should be initialized before connection");
        return -2; //just to differentiate with network error.
    }
    if(client->current_state == Connected)
    {
        log_info("Client already Connected");
        return ZSUCCESS;
    }
    unsigned const int buff_size = 10000;
    unsigned char buf[buff_size], readbuf[buff_size];

    log_info("Preparing Network..");
    NetworkInit(&n);

#if defined(SECURE_CONNECTION)
    rc = NetworkConnect(&n, hostname, port, parse_mode, client->certs.ca_crt, client->certs.client_cert, client->certs.client_key, client->certs.cert_password);
#else
    rc = NetworkConnect(&n, hostname, port);
#endif
    if (rc != ZSUCCESS)
    {
        log_error("Error Connecting Network.. %d ", rc);
        return ZFAILURE;
    }

    //TODO: Handle the rc of ConnectNetwork().
    log_info("Connecting to \x1b[32m %s : %d \x1b[0m", hostname, port);
    MQTTClientInit(&client->mqtt_client, &n, 1000, buf, buff_size, readbuf, buff_size);
    MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;

    conn_data.MQTTVersion = 4;
    conn_data.cleansession = 1; //TODO: tobe confirmed with Hub
    conn_data.keepAliveInterval = 60;
    conn_data.clientID.cstring = client->config.device_id;
    conn_data.willFlag = 0;

    //TODO:2: to be verified with HUB.
    conn_data.username.cstring = client->config.device_id;
    conn_data.password.cstring = client->config.auth_token;

    rc = MQTTConnect(&client->mqtt_client, &conn_data);
    if (rc == 0)
    {
        log_info("Connected!");
        client->current_state = Connected;
    }
    else
    {
        NetworkDisconnect(client->mqtt_client.ipstack);
        if (rc == 5)
        {
            log_error("Error while establishing connection, due to invalid credentials");
        }
        else
        {
            log_error("Error while establishing connection. Error code: %d", rc);
        }
    }
    return rc;
}

int zclient_reconnect(IOTclient *client)
{
    if(client==NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }

    if(client->current_state == Connected)
    {
        log_info("Client already Connected");
        return ZSUCCESS;
    }

    int rc = -1, delay = 5;
    log_info("Trying to reconnect \x1b[32m %s : %d \x1b[0m in %d sec ", hostname, port, delay);
    sleep(delay);
    rc = zclient_connect(client);
    if (rc == ZSUCCESS)
    {
        client->current_state = Connected;
        retryCount = 0;
        return ZSUCCESS;
    }
    retryCount++;
    log_info("retryCount :%d",retryCount);
    if(retryCount > retry_limit)
    {
        log_info("Retry limit Exceeded");
        return ZCONNECTION_ERROR;
    }
    return rc;
}

int zclient_publish(IOTclient *client, char *payload)
{
    if(client == NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }
    if (client->current_state != Connected)
    {
        log_debug("Failed to publish, since connection is lost/not established");
        return ZFAILURE;
    }
    MQTTMessage pubmsg;
    //TODO:remove hardcoded values of id etc and get from structure copied from config.h
    //TODO: confirm the below parameters with Hub. Especially the pubmessageID

    pubmsg.id = 1234;
    pubmsg.dup = '0';
    pubmsg.qos = 1;
    pubmsg.retained = '0';

    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    rc = MQTTPublish(&(client->mqtt_client), dataTopic, &pubmsg);
    //TODO: check for connection and retry to send the message once the conn got restroed.
    if (rc == ZSUCCESS)
    {
        log_debug("Published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", payload, dataTopic);
    }
    else
    {
        client->current_state = Disconnected;
        log_error("Error on Pubish. Error code: %d", rc);
    }
    return rc;
}

int zclient_dispatch(IOTclient *client)
{
    if(client == NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }
    if (client->current_state != Initialized || client->current_state != Connected || client->current_state != Disconnected)
    {
        log_error("Client should be initialized");
        return -2; 
    }
    //TODO: Add time stamp, Client ID
    time_t curtime;
    time(&curtime);
    char *time_val = strtok(ctime(&curtime), "\n");
    if (!cJSON_HasObjectItem(cJsonPayload, "time"))
    {
        cJSON_AddStringToObject(cJsonPayload, "time", time_val);
    }
    else
    {
        cJSON *temp = cJSON_CreateString(time_val);
        cJSON_ReplaceItemInObject(cJsonPayload, "time", temp);
    }

    if (!cJSON_HasObjectItem(cJsonPayload, "data"))
    {
        cJSON_AddItemToObject(cJsonPayload, "data", data);
    }
    else
    {
        cJSON_ReplaceItemInObject(cJsonPayload, "data", data);
    }

    char *payload = cJSON_Print(cJsonPayload);
    rc = zclient_publish(client, payload);
    return rc;
}

int zclient_subscribe(IOTclient *client, messageHandler on_message)
{
    if(client == NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }
    if (client->current_state != Initialized || client->current_state != Connected || client->current_state != Disconnected)
    {
        log_error("Client should be initialized");
        return -2; 
    }
    //TODO: add basic validation & callback method and append it on error logs.
    rc = MQTTSubscribe(&(client->mqtt_client), commandTopic, QOS0, on_message);
    if (rc == ZSUCCESS)
    {
        log_info("Subscribed on \x1b[36m '%s' \x1b[0m", commandTopic);
    }
    else
    {
        client->current_state = Disconnected;
        log_error("Error on Subscribe. Error code: %d", rc);
    }

    return rc;
}

int zclient_yield(IOTclient *client, int time_out)
{
    if(client == NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }
    if(time_out<=0)
    {
        log_error("timeout can't be Zero or Negative");
        return ZFAILURE;
    }
    if (client->current_state != Initialized || client->current_state != Connected || client->current_state != Disconnected)
    {
        log_error("Client should be initialized");
        return -2; 
    }
    if (client->current_state == Disconnected)
    {
        rc = zclient_reconnect(client);
        return rc;
    }

    rc = MQTTYield(&client->mqtt_client, time_out);

    if (rc == ZFAILURE)
    {
        client->current_state = Disconnected;
        return ZFAILURE;
    }
    return rc;
}

int zclient_disconnect(IOTclient *client)
{
    if(client == NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }
    if (client->current_state == Connected)
    {
        rc = MQTTDisconnect(&client->mqtt_client);
        NetworkDisconnect(client->mqtt_client.ipstack);
    }
    client->current_state = Disconnected;
    log_info("Disconnected.");
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
            ret = ZFAILURE;
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
            ret = ZFAILURE;
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