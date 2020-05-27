#include "iot_client.h"
#include "sys/socket.h"
#include "unistd.h"

//TODO: read from config file.
Network n;
certsParseMode parse_mode;
int retryCount = 0;
char dataTopic[100] = "", commandTopic[100] = "", eventTopic[100] = "", connectionStringBuff[256] = "";
//TODO: Remove all debug statements and use logger.
//TODO: Add logging for all important connection scenarios.
//TODO: Add idle methods when socket is busy as in ssl_client_2.

int zclient_init(IOTclient *iot_client, char *MQTTUserName, char *MQTTPassword, certsParseMode mode, char *ca_crt, char *client_cert, char *client_key, char *cert_password)
{
    //TODO:1
    // All config.h and device related validations should be done here itself !

    log_initialize();
    log_info("\n\n\nSDK Initializing..");
    if (iot_client == NULL)
    {
        log_error("Client object is NULL");
        return ZFAILURE;
    }
    if (MQTTUserName == NULL || MQTTPassword == NULL)
    {
        log_error("Device Credentials can't be NULL");
        return ZFAILURE;
    }

    Config config = {NULL, NULL, 0};
    int str_array_size = 0;
    char **string_array = stringSplit(MQTTUserName, '/', &str_array_size);
    if (str_array_size != 5)
    {
        log_error("MQTTUsername is Malformed.");
        return ZFAILURE;
    }
    cloneString(&config.hostname, trim(string_array[0]));
    cloneString(&config.client_id, trim(string_array[3]));
    cloneString(&config.auth_token, trim(MQTTPassword));
    cloneString(&config.MqttUserName, trim(MQTTUserName));
    log_error("client_id:%s", config.client_id);
    log_error("hostname:%s", config.hostname);
    //Populating dynamic topic names based on its deviceID
    sprintf(dataTopic, "%s/%s%s", topic_pre, config.client_id, data_topic);
    sprintf(commandTopic, "%s/%s%s", topic_pre, config.client_id, command_topic);
    sprintf(eventTopic, "%s/%s%s", topic_pre, config.client_id, event_topic);

    config.retry_limit = 5;
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
    if (client_cert == NULL || client_key == NULL || cert_password == NULL || (mode == REFERENCE && (access(client_cert, F_OK) == -1)) || (mode == REFERENCE && (access(client_key, F_OK) == -1)))
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
    iot_client->message.data = cJSON_CreateObject();
    if (iot_client->message.data == NULL)
    {
        log_error("Can't create cJSON object");
        return ZFAILURE;
    }
    iot_client->current_state = Initialized;
    log_info("Client Initialized!");
    return ZSUCCESS;
}

void zclient_addConnectionParameter(char *connectionParamKey, char *connectionParamValue)
{
    sprintf(connectionStringBuff, "%s%s%s%s%s", connectionStringBuff, connectionParamKey, "=", connectionParamValue, "&");
}

char *formConnectionString(char *username)
{
    sprintf(connectionStringBuff, "%s%s", username, "?");
    zclient_addConnectionParameter("sdk_name", "zoho-iot-sdk-c");
    zclient_addConnectionParameter("sdk_version", "0.0.1");
    //    zclient_addConnectionParams("sdk_url", "");
    connectionStringBuff[strlen(connectionStringBuff) - 1] = '\0';
    return connectionStringBuff;
}

int validateClientState(IOTclient *client)
{
    if (client == NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }
    else if (client->current_state != Initialized && client->current_state != Connected && client->current_state != Disconnected)
    {
        log_error("Client should be initialized");
        return -2; //just to differentiate with network error.
    }
    else
    {
        return ZSUCCESS;
    }
}

int zclient_connect(IOTclient *client)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    //TODO: verify the buff size on real device. and flush the buffer at end of connection.
    if (client->current_state == Connected)
    {
        log_info("Client already Connected");
        return ZSUCCESS;
    }
    unsigned const int buff_size = 10000;
    unsigned char buf[buff_size], readbuf[buff_size];

    log_info("Preparing Network..");
    NetworkInit(&n);

#if defined(SECURE_CONNECTION)
    rc = NetworkConnect(&n, client->config.hostname, port, parse_mode, client->certs.ca_crt, client->certs.client_cert, client->certs.client_key, client->certs.cert_password);
#else
    rc = NetworkConnect(&n, client->config.hostname, port);
#endif
    if (rc != ZSUCCESS)
    {
        log_error("Error Connecting Network.. %d ", rc);
        return ZFAILURE;
    }

    //TODO: Handle the rc of ConnectNetwork().
    log_info("Connecting to \x1b[32m %s : %d \x1b[0m", client->config.hostname, port);
    MQTTClientInit(&client->mqtt_client, &n, 1000, buf, buff_size, readbuf, buff_size);
    MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;

    conn_data.MQTTVersion = 4;
    conn_data.cleansession = 1; //TODO: tobe confirmed with Hub
    conn_data.keepAliveInterval = 60;
    conn_data.clientID.cstring = client->config.client_id;
    conn_data.willFlag = 0;

    //TODO:2: to be verified with HUB.
    conn_data.username.cstring = formConnectionString(client->config.MqttUserName);
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
    int delay = 5;
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }

    if (client->current_state == Connected)
    {
        log_info("Client already Connected");
        return ZSUCCESS;
    }

    log_info("Trying to reconnect \x1b[32m %s : %d \x1b[0m in %d sec ", client->config.hostname, port, delay);
    sleep(delay);
    rc = zclient_connect(client);
    if (rc == ZSUCCESS)
    {
        client->current_state = Connected;
        retryCount = 0;
        return ZSUCCESS;
    }
    retryCount++;
    log_info("retryCount :%d", retryCount);
    if (client->current_state != Disconnected && client->current_state != Connected)
    {
        log_info("Retrying indefinetely");
        return ZCONNECTION_ERROR;
    }

    if (retryCount > client->config.retry_limit)
    {
        log_info("Retry limit Exceeded");
        return ZCONNECTION_ERROR;
    }
    return rc;
}

int zclient_publish(IOTclient *client, char *payload)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (client->current_state != Connected)
    {
        log_debug("Can not publish, since connection is lost/not established");
        return ZFAILURE;
    }
    rc = ZFAILURE;
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
    else if (client->mqtt_client.isconnected == 0)
    {
        client->current_state = Disconnected;
        log_error("Error on Pubish due to lost connection. Error code: %d", rc);
    }
    else
    {
        log_error("Error on Pubish. Error code: %d", rc);
    }
    return rc;
}

int zclient_dispatch(IOTclient *client)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (client->current_state != Connected)
    {
        log_debug("Can not dispatch, since connection is lost/not established");
        return ZFAILURE;
    }
    //TODO: Add time stamp, Client ID
    char *payload = cJSON_Print(client->message.data);
    return zclient_publish(client, payload);
}

int zclient_subscribe(IOTclient *client, messageHandler on_message)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (client->current_state != Connected)
    {
        log_debug("Can not subscribe, since connection is lost/not established");
        return ZFAILURE;
    }
    //TODO: add basic validation & callback method and append it on error logs.
    rc = MQTTSubscribe(&(client->mqtt_client), commandTopic, QOS0, on_message);
    if (rc == ZSUCCESS)
    {
        log_info("Subscribed on \x1b[36m '%s' \x1b[0m", commandTopic);
    }
    else if (client->mqtt_client.isconnected == 0)
    {
        client->current_state = Disconnected;
        log_error("Error on Subscribe due to lost connection. Error code: %d", rc);
    }
    else
    {
        log_error("Error on Subscribe. Error code: %d", rc);
    }

    return rc;
}

int zclient_yield(IOTclient *client, int time_out)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (time_out <= 0)
    {
        log_error("timeout can't be Zero or Negative");
        return ZFAILURE;
    }
    if (client->current_state == Disconnected)
    {
        rc = zclient_reconnect(client);
        return rc;
    }

    rc = MQTTYield(&client->mqtt_client, time_out);
    if (rc == ZSUCCESS)
    {
        return rc;
    }
    else if (rc == ZFAILURE)
    {
        if (client->mqtt_client.isconnected == 0)
        {
            client->current_state = Disconnected;
            log_error("Error on Yielding due to lost connection. Error code: %d", rc);
            return ZFAILURE;
        }
    }
    else
    {
        log_error("Error on Yield. Error code: %d", rc);
        return rc;
    }
    return rc;
}

int zclient_disconnect(IOTclient *client)
{
    int rc = ZSUCCESS;
    if (client == NULL)
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
    return rc;
}

int zclient_setRetrycount(IOTclient *client, int count)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }

    if (count < 0)
    {
        log_info("Retry limit value given is < 0 , so set to default value :%d", client->config.retry_limit);
        return ZFAILURE;
    }

    client->config.retry_limit = count;
    return ZSUCCESS;
}

cJSON *addAssetNameTopayload(IOTclient *client, char *assetName)
{
    if (assetName != NULL)
    {
        if (!cJSON_HasObjectItem(client->message.data, assetName))
        {
            cJSON_AddItemReferenceToObject(client->message.data, assetName, cJSON_CreateObject());
        }
        return cJSON_GetObjectItem(client->message.data, assetName);
    }
    else
    {
        return client->message.data;
    }
}

int zclient_addNumber(IOTclient *client, char *val_name, int val_int, char *assetName)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (val_name == NULL)
    {
        return -1;
    }

    cJSON *obj = addAssetNameTopayload(client, assetName);
    rc = ZSUCCESS;

    if (!cJSON_HasObjectItem(obj, val_name))
    {
        if (cJSON_AddNumberToObject(obj, val_name, val_int) == NULL)
        {
            log_error("Adding int attribute failed\n");
            rc = ZFAILURE;
        }
    }
    else
    {
        cJSON *temp = cJSON_CreateNumber(val_int);
        cJSON_ReplaceItemInObject(obj, val_name, temp);
    }
    return rc;
}

int zclient_addString(IOTclient *client, char *val_name, char *val_string, char *assetName)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (val_name == NULL)
    {
        return -1;
    }

    cJSON *obj = addAssetNameTopayload(client, assetName);
    rc = ZSUCCESS;

    if (!cJSON_HasObjectItem(obj, val_name))
    {
        if (cJSON_AddStringToObject(obj, val_name, val_string) == NULL)
        {
            log_error("Adding string attribute failed\n");
            rc = ZFAILURE;
        }
    }
    else
    {
        cJSON *temp = cJSON_CreateString(val_string);
        cJSON_ReplaceItemInObject(obj, val_name, temp);
    }
    return rc;
}

int zclient_markDataPointAsError(IOTclient *client, char *val_name, char *assetName)
{
    return zclient_addString(client, val_name, "<ERROR>", assetName);
}

/*
char *zclient_getpayload()
{
    return cJSON_Print(cJsonPayload);
}
*/