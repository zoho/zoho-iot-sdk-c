#include "zoho_iot_client.h"
#include "zoho_message_handler.h"
#include "sys/socket.h"
#include "unistd.h"

//TODO: read from config file.
Network n;
certsParseMode parse_mode;
int retryCount = 0;
char dataTopic[100] = "", commandTopic[100] = "", eventTopic[100] = "";
char commandAckTopic[100] = "", connectionStringBuff[256] = "";
cJSON *eventDataObject;
//TODO: Remove all debug statements and use logger.
//TODO: Add logging for all important connection scenarios.
//TODO: Add idle methods when socket is busy as in ssl_client_2.

int zclient_init_config_file(ZohoIOTclient *iot_client, char *MqttConfigFilePath, certsParseMode mode, ZlogConfig *logConfig)
{
    log_initialize(logConfig);
    log_info("\n\n\nSDK Initializing..");

    FILE *MqttConfigFile = fopen(MqttConfigFilePath, "rb");
    if (!MqttConfigFile)
    {
        log_error("Mqtt Config File can't be missing or NULL.");
        return ZFAILURE;
    }
    fseek(MqttConfigFile, 0, SEEK_END);
    long length = ftell(MqttConfigFile);
    fseek(MqttConfigFile, 0, SEEK_SET);
    char buffer[length + 2];
    if (buffer)
    {
        fread(buffer, sizeof(char), length, MqttConfigFile);
    }
    fclose(MqttConfigFile);
    buffer[length] = '\0';
    cJSON *temp = cJSON_Parse(buffer);
    if (temp == NULL)
    {
        log_error("Cannot parse the contents of MQTT config file");
        return ZFAILURE;
    }
    char *mqttUserName = cJSON_GetStringValue(cJSON_GetObjectItem(temp, "MQTTUserName"));
    char *mqttPassword = cJSON_GetObjectItem(temp, "MQTTPassword")->valuestring;
    char *ca_crt = cJSON_GetStringValue(cJSON_GetObjectItem(temp, "RootCACertLocation"));
    char *client_cert = cJSON_GetStringValue(cJSON_GetObjectItem(temp, "DeviceCertLocation"));
    char *client_key = cJSON_GetStringValue(cJSON_GetObjectItem(temp, "DevicePrivateKeyLocation"));
    char *cert_password = cJSON_GetStringValue(cJSON_GetObjectItem(temp, "DeviceCertParsword"));
    return zclient_init(iot_client, mqttUserName, mqttPassword, mode, ca_crt, client_cert, client_key, cert_password);
}

int populateConfigObject(char *MQTTUserName, Zconfig *config)
{
    int len = strlen(MQTTUserName), itr_cnt = 0;
    char *delim = "/", userName[len];
    strcpy(userName, MQTTUserName);
    char *ptr = strtok(userName, delim);
    cloneString(&config->hostname, ptr);
    while (ptr != NULL)
    {
        ptr = strtok(NULL, delim);
        itr_cnt++;
        if (itr_cnt == 3)
        {
            cloneString(&config->client_id, ptr);
        }
    }
    if (itr_cnt != 5)
    {
        return ZFAILURE;
    }
    return ZSUCCESS;
}

int zclient_init(ZohoIOTclient *iot_client, char *MQTTUserName, char *MQTTPassword, certsParseMode mode, char *ca_crt, char *client_cert, char *client_key, char *cert_password, ZlogConfig *logConfig)
{
    //TODO:1
    // All config.h and device related validations should be done here itself !

    if (!Zlog.fp)
    {
        log_initialize(logConfig);
        log_info("\n\n\nSDK Initializing..");
    }

    if (iot_client == NULL)
    {
        log_error("Client object is NULL");
        return ZFAILURE;
    }
    if (!isStringValid(MQTTUserName) || !isStringValid(MQTTPassword))
    {
        log_error("Device Credentials can't be NULL or Empty");
        return ZFAILURE;
    }

    Zconfig config = {"", "", "", "", 0};
    if (populateConfigObject(MQTTUserName, &config) == ZFAILURE)
    {
        log_error("MQTTUsername is Malformed.");
        return ZFAILURE;
    }
    char *trimmedPassword = trim(MQTTPassword);
    char *trimmedUserName = trim(MQTTUserName);
    cloneString(&config.auth_token, trimmedPassword);
    cloneString(&config.MqttUserName, trimmedUserName);
    cJSON_free(trimmedPassword);
    cJSON_free(trimmedUserName);
    log_debug("client_id:%s", config.client_id);
    log_debug("hostname:%s", config.hostname);
    //Populating dynamic topic names based on its deviceID
    sprintf(dataTopic, "%s/%s%s", topic_pre, config.client_id, data_topic);
    sprintf(commandTopic, "%s/%s%s", topic_pre, config.client_id, command_topic);
    sprintf(commandAckTopic, "%s/%s%s", topic_pre, config.client_id, command_ack_topic);
    sprintf(eventTopic, "%s/%s%s", topic_pre, config.client_id, event_topic);

    config.retry_limit = 5;
    iot_client->config = config;
    parse_mode = mode;
#if defined(Z_SECURE_CONNECTION)
    if (ca_crt == NULL || (mode == REFERENCE && access(ca_crt, F_OK) == -1))
    {
        log_error("RootCA file is not found/can't be accessed");
        return ZFAILURE;
    }
    iot_client->certs.ca_crt = ca_crt;
#if defined(Z_USE_CLIENT_CERTS)
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
    if (eventDataObject == NULL)
    {
        eventDataObject = cJSON_CreateObject();
    }
    initMessageHandler(iot_client, commandTopic, commandAckTopic);
    iot_client->current_state = INITIALIZED;
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

int validateClientState(ZohoIOTclient *client)
{
    if (client == NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }
    else if (client->current_state != INITIALIZED && client->current_state != CONNECTED && client->current_state != DISCONNECTED)
    {
        log_error("Client should be initialized");
        return -2; //just to differentiate with network error.
    }
    else
    {
        return ZSUCCESS;
    }
}

int zclient_connect(ZohoIOTclient *client)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    //TODO: verify the buff size on real device. and flush the buffer at end of connection.
    if (client->current_state == CONNECTED)
    {
        log_info("Client already Connected");
        return ZSUCCESS;
    }
    unsigned const int buff_size = 10000;
    unsigned char buf[buff_size], readbuf[buff_size];

    log_info("Preparing Network..");
    NetworkInit(&n);

#if defined(Z_SECURE_CONNECTION)
    rc = NetworkConnect(&n, client->config.hostname, zport, parse_mode, client->certs.ca_crt, client->certs.client_cert, client->certs.client_key, client->certs.cert_password);
#else
    rc = NetworkConnect(&n, client->config.hostname, zport);
#endif
    if (rc != ZSUCCESS)
    {
        log_error("Error Connecting Network.. %d ", rc);
        return ZFAILURE;
    }

    //TODO: Handle the rc of ConnectNetwork().
    log_info("Connecting to \x1b[32m %s : %d \x1b[0m", client->config.hostname, zport);
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
        client->current_state = CONNECTED;
    }
    else
    {
        NetworkDisconnect(client->mqtt_client.ipstack);
        if (rc == 1)
        {
            log_error("Error while establishing connection, unacceptable protocol version. Error code: %d", rc);
        }
        else if (rc == 2)
        {
            log_error("Error while establishing connection, Invalid ClientId . Error code: %d", rc);
        }
        else if (rc == 3)
        {
            log_error("Error while establishing connection, Server unavailable . Error code: %d", rc);
        }
        else if (rc == 4)
        {
            log_error("Error while establishing connection, due to invalid credentials. Error code: %d", rc);
        }
        else if (rc == 5)
        {
            log_error("Error while establishing connection, Connection refused,as device is not authorized. Error code: %d", rc);
        }
        else
        {
            log_error("Error while establishing connection. Error code: %d", rc);
        }
    }
    return rc;
}

int zclient_reconnect(ZohoIOTclient *client)
{
    int delay = 5;
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }

    if (client->current_state == CONNECTED)
    {
        log_info("Client already Connected");
        return ZSUCCESS;
    }

    log_info("Trying to reconnect \x1b[32m %s : %d \x1b[0m in %d sec ", client->config.hostname, zport, delay);
    sleep(delay);
    rc = zclient_connect(client);
    if (rc == ZSUCCESS)
    {
        client->current_state = CONNECTED;
        retryCount = 0;
        return ZSUCCESS;
    }
    retryCount++;
    log_info("retryCount :%d", retryCount);
    if (client->current_state != DISCONNECTED && client->current_state != CONNECTED)
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

int zclient_publish(ZohoIOTclient *client, char *payload)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (client->current_state != CONNECTED)
    {
        log_debug("Can not publish, since connection is lost/not established");
        return ZFAILURE;
    }
    rc = ZFAILURE;

    //TODO:remove hardcoded values of id etc and get from structure copied from config.h
    //TODO: confirm the below parameters with Hub. Especially the pubmessageID
    MQTTMessage pubmsg;
    pubmsg.id = 1234;
    pubmsg.qos = 1;
    pubmsg.dup = '0';
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
        client->current_state = DISCONNECTED;
        log_error("Error on Pubish due to lost connection. Error code: %d", rc);
    }
    else
    {
        log_error("Error on Pubish. Error code: %d", rc);
    }
    return rc;
}

int zclient_dispatch(ZohoIOTclient *client)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (client->current_state != CONNECTED)
    {
        log_debug("Can not dispatch, since connection is lost/not established");
        return ZFAILURE;
    }
    //TODO: Add time stamp, Client ID
    char *payload = cJSON_Print(client->message.data);
    int status = zclient_publish(client, payload);
    cJSON_free(payload);
    return status;
}

int zclient_addEventDataNumber(char *key, double val_number)
{
    if (eventDataObject == NULL)
    {
        eventDataObject = cJSON_CreateObject();
    }

    if (!isStringValid(key))
    {
        log_error("Key Can't be NULL");
        return -1;
    }
    int rc = ZSUCCESS;
    if (!cJSON_HasObjectItem(eventDataObject, key))
    {
        if (cJSON_AddNumberToObject(eventDataObject, key, val_number) == NULL)
        {
            log_error("Adding Number attribute failed\n");
            rc = ZFAILURE;
        }
    }
    else
    {
        cJSON_ReplaceItemInObject(eventDataObject, key, cJSON_CreateNumber(val_number));
    }
    return rc;
}

int zclient_addEventDataString(char *key, char *val_string)
{
    if (eventDataObject == NULL)
    {
        eventDataObject = cJSON_CreateObject();
    }
    if (!isStringValid(key) || !isStringValid(val_string))
    {
        log_error("Key or Value Can't be NULL");
        return -1;
    }
    int rc = ZSUCCESS;
    if (!cJSON_HasObjectItem(eventDataObject, key))
    {
        if (cJSON_AddStringToObject(eventDataObject, key, val_string) == NULL)
        {
            log_error("Adding String attribute failed\n");
            rc = ZFAILURE;
        }
    }
    else
    {
        cJSON_ReplaceItemInObject(eventDataObject, key, cJSON_CreateString(val_string));
    }
    return rc;
}

int zclient_dispatchEventFromEventDataObject(ZohoIOTclient *client, char *eventType, char *eventDescription, char *assetName)
{
    char *eventDataJSONString = cJSON_Print(eventDataObject);
    int rc = zclient_dispatchEventFromJSONString(client, eventType, eventDescription, eventDataJSONString, assetName);
    cJSON_Delete(eventDataObject);
    cJSON_free(eventDataJSONString);
    eventDataObject = cJSON_CreateObject();
    return rc;
}

int zclient_dispatchEventFromJSONString(ZohoIOTclient *client, char *eventType, char *eventDescription, char *eventDataJSONString, char *assetName)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (client->current_state != CONNECTED)
    {
        log_debug("Can not dispatch, since connection is lost/not established");
        return ZFAILURE;
    }
    cJSON *dataObject = cJSON_Parse(eventDataJSONString);
    if (dataObject == NULL)
    {
        log_error("Can not dispatch Event as Event Data JSON string is not parsable.");
        return ZFAILURE;
    }
    time_t curtime;
    char *payload;
    time(&curtime);
    char *time_val = strtok(ctime(&curtime), "\n");
    cJSON *eventObject = cJSON_CreateObject();
    if (!isStringValid(eventType) || eventDescription == NULL)
    {
        log_error("Can not dispatch Event with Empty EventType or Description.");
        return ZFAILURE;
    }
    cJSON_AddStringToObject(eventObject, "event_type", eventType);
    cJSON_AddStringToObject(eventObject, "event_descr", eventDescription);
    cJSON_AddStringToObject(eventObject, "event_timestamp", time_val);
    cJSON_AddItemToObject(eventObject, "event_data", dataObject);
    if (!isStringValid(assetName))
    {
        payload = cJSON_Print(eventObject);
    }
    else
    {
        cJSON *eventDispatchObject = cJSON_CreateObject();
        cJSON_AddItemReferenceToObject(eventDispatchObject, assetName, eventObject);
        payload = cJSON_Print(eventDispatchObject);
        cJSON_Delete(eventDispatchObject);
    }
    cJSON_Delete(eventObject);
    MQTTMessage pubmsg;
    pubmsg.id = 1234;
    pubmsg.qos = 1;
    pubmsg.dup = '0';
    pubmsg.retained = '0';
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    rc = MQTTPublish(&(client->mqtt_client), eventTopic, &pubmsg);
    //TODO: check for connection and retry to send the message once the conn got restroed.
    if (rc == ZSUCCESS)
    {
        log_debug("Event dispatched \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", payload, eventTopic);
    }
    else if (client->mqtt_client.isconnected == 0)
    {
        client->current_state = DISCONNECTED;
        log_error("Error on dispatchEvent due to lost connection. Error code: %d", rc);
    }
    else
    {
        log_error("Error on dispatchEvent. Error code: %d", rc);
    }
    cJSON_free(payload);
    return rc;
}

int zclient_publishCommandAck(ZohoIOTclient *client, char *correlation_id, ZcommandAckResponseCodes status_code, char *responseMessage)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (!isStringValid(correlation_id))
    {
        log_error("Correlation_id cannot be Null or empty in command Ack object");
        return ZFAILURE;
    }
    if (responseMessage == NULL)
    {
        log_error("Response cannot be Null in command Ack object");
        return ZFAILURE;
    }
    if (status_code != SUCCESFULLY_EXECUTED && (status_code < EXECUTION_FAILURE || status_code > ALREADY_ON_SAME_STATE))
    {
        log_error("Status code provided is not a valid in command Ack object");
        return ZFAILURE;
    }
    cJSON *commandAckResponseObject = cJSON_CreateObject();
    cJSON *commandAckObject = cJSON_AddObjectToObject(commandAckResponseObject, correlation_id);
    cJSON_AddNumberToObject(commandAckObject, "status_code", status_code);
    cJSON_AddStringToObject(commandAckObject, "response", responseMessage);
    char *payload = cJSON_Print(commandAckResponseObject);
    MQTTMessage pubmsg;
    pubmsg.id = 1234;
    pubmsg.qos = 1;
    pubmsg.dup = '0';
    pubmsg.retained = '0';
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    rc = MQTTPublish(&(client->mqtt_client), commandAckTopic, &pubmsg);
    if (rc == ZSUCCESS)
    {
        log_debug("Command Ack published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", pubmsg.payload, eventTopic);
    }
    else
    {
        log_error("Error on publishing command Ack. Error code: %d", rc);
    }
    return rc;
}

int zclient_subscribe(ZohoIOTclient *client, messageHandler on_message)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (client->current_state != CONNECTED)
    {
        log_debug("Can not subscribe, since connection is lost/not established");
        return ZFAILURE;
    }
    //TODO: add basic validation & callback method and append it on error logs.
    rc = MQTTSubscribe(&(client->mqtt_client), commandTopic, QOS0, onMessageReceived);
    setMessageHandler(on_message);
    if (rc == ZSUCCESS)
    {
        log_info("Subscribed on \x1b[36m '%s' \x1b[0m", commandTopic);
    }
    else if (client->mqtt_client.isconnected == 0)
    {
        client->current_state = DISCONNECTED;
        log_error("Error on Subscribe due to lost connection. Error code: %d", rc);
    }
    else
    {
        log_error("Error on Subscribe. Error code: %d", rc);
    }

    return rc;
}

int zclient_yield(ZohoIOTclient *client, int time_out)
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
    if (client->current_state == DISCONNECTED)
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
            client->current_state = DISCONNECTED;
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

int zclient_disconnect(ZohoIOTclient *client)
{
    int rc = ZSUCCESS;
    if (client == NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }
    if (client->current_state == CONNECTED)
    {
        rc = MQTTDisconnect(&client->mqtt_client);
        NetworkDisconnect(client->mqtt_client.ipstack);
    }
    client->current_state = DISCONNECTED;
    log_info("Disconnected.");
    log_free();
    return rc;
}

int zclient_setRetrycount(ZohoIOTclient *client, int count)
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

cJSON *addAssetNameTopayload(ZohoIOTclient *client, char *assetName)
{
    if (isStringValid(assetName))
    {
        if (!cJSON_HasObjectItem(client->message.data, assetName))
        {
            cJSON_AddObjectToObject(client->message.data, assetName);
        }
        return cJSON_GetObjectItem(client->message.data, assetName);
    }
    else
    {
        return client->message.data;
    }
}

int zclient_addNumber(ZohoIOTclient *client, char *key, double val, char *assetName)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (!isStringValid(key))
    {
        return -1;
    }

    cJSON *obj = addAssetNameTopayload(client, assetName);
    rc = ZSUCCESS;

    if (!cJSON_HasObjectItem(obj, key))
    {
        if (cJSON_AddNumberToObject(obj, key, val) == NULL)
        {
            log_error("Adding int attribute failed\n");
            rc = ZFAILURE;
        }
    }
    else
    {
        cJSON_ReplaceItemInObject(obj, key, cJSON_CreateNumber(val));
    }
    return rc;
}

int zclient_addString(ZohoIOTclient *client, char *key, char *val_string, char *assetName)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (!isStringValid(key))
    {
        return -1;
    }

    cJSON *obj = addAssetNameTopayload(client, assetName);
    rc = ZSUCCESS;

    if (!cJSON_HasObjectItem(obj, key))
    {
        if (cJSON_AddStringToObject(obj, key, val_string) == NULL)
        {
            log_error("Adding string attribute failed\n");
            rc = ZFAILURE;
        }
    }
    else
    {
        cJSON_ReplaceItemInObject(obj, key, cJSON_CreateString(val_string));
    }
    return rc;
}

int zclient_markDataPointAsError(ZohoIOTclient *client, char *key, char *assetName)
{
    return zclient_addString(client, key, "<ERROR>", assetName);
}

/*
char *zclient_getpayload()
{
    return cJSON_Print(cJsonPayload);
}
*/