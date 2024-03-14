#include "zoho_iot_client.h"
#include "zoho_message_handler.h"
#include "sys/socket.h"
#include "unistd.h"
#include <stdbool.h>
#define MQTT_EMB_LOGGING
#define Z_SDK_VERSION "0.0.1"
//TODO: read from config file.
Network n;
certsParseMode parse_mode;
time_t start_time = 0;
int retryCount = 0;
char dataTopic[100] = "", commandTopic[100] = "", eventTopic[100] = "",configTopic[100] = "";
char commandAckTopic[100] = "",configAckTopic[100]="", connectionStringBuff[256] = "";
cJSON *eventDataObject;
bool retryACK;
ZfailedACK failedACK;
bool retryEvent;
ZfailedEvent failedEvent;

bool paho_debug = true;
bool TLS_MODE = true;
bool TLS_CLIENT_CERTS = true;

#if defined(Z_SECURE_CONNECTION)
int ZPORT = 8883;
#else
int ZPORT = 1883;
#endif


//TODO: Remove all debug statements and use logger.
//TODO: Add logging for all important connection scenarios.
//TODO: Add idle methods when socket is busy as in ssl_client_2.

cJSON* generateACKPayload(char* payload,ZcommandAckResponseCodes status_code, char *responseMessage);
cJSON* generateProcessedACK(char* payload,ZcommandAckResponseCodes status_code, char *responseMessage);

void zclient_enable_paho_debug(bool state){
    paho_debug = state;
}
void zclient_set_tls(bool state){
    TLS_MODE = state;
}
void zclient_set_client_certs(bool state){
    TLS_CLIENT_CERTS = state;
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
        log_info("\n\n\nSDK Initializing.. version: %s",Z_SDK_VERSION);
    }
    #if(Z_SECURE_CONNECTION)
        #if(Z_USE_CLIENT_CERTS)
            log_info("Build type: \033[35m TLS build with client certs \033[0m");
        #else
            log_info("Build type: \033[35m TLS build \033[0m");
        #endif
    #else
        log_info("Build type: \033[35m NON_TLS build \033[0m");
    #endif
    if (iot_client == NULL)
    {
        log_error("Client object is NULL");
        return ZFAILURE;
    }
    #if(Z_USE_CLIENT_CERTS)
    if(!TLS_CLIENT_CERTS){
        if (!isStringValid(MQTTUserName) || !isStringValid(MQTTPassword))
        {
            log_error("Device Credentials can't be NULL or Empty");
            return ZFAILURE;
        }
    }
    else
    {
        if(TLS_MODE)
        {
            if (!isStringValid(MQTTUserName))
            {
                log_error("Device UserName can't be NULL or Empty");
                return ZFAILURE;
            }
        }
        else
        {
            if (!isStringValid(MQTTUserName) || !isStringValid(MQTTPassword))
            {
                log_error("Device Credentials can't be NULL or Empty");
                return ZFAILURE;
            }
        }
    }
    #else
    if (!isStringValid(MQTTUserName) || !isStringValid(MQTTPassword))
    {
        log_error("Device Credentials can't be NULL or Empty");
        return ZFAILURE;
    }
    #endif
    Zconfig config = {"", "", "", "", 0};
    if (populateConfigObject(MQTTUserName, &config) == ZFAILURE)
    {
        log_error("MQTTUsername is Malformed.");
        return ZFAILURE;
    }
    iot_client->ZretryInterval = MIN_RETRY_INTERVAL;

    char *trimmedUserName = trim(MQTTUserName);
    cloneString(&config.MqttUserName, trimmedUserName);
    cJSON_free(trimmedUserName);

    #if defined(Z_USE_CLIENT_CERTS)
    if(!TLS_CLIENT_CERTS){
        char *trimmedPassword = trim(MQTTPassword);
        cloneString(&config.auth_token, trimmedPassword);
        cJSON_free(trimmedPassword);
    }
    #else
    char *trimmedPassword = trim(MQTTPassword);
    cloneString(&config.auth_token, trimmedPassword);
    cJSON_free(trimmedPassword);
    #endif
    
    log_debug("client_id:%s", config.client_id);
    log_debug("hostname:%s", config.hostname);
    //Populating dynamic topic names based on its deviceID
    sprintf(dataTopic, "%s/%s%s", TOPIC_PRE, config.client_id, DATA_TOPIC);
    sprintf(commandTopic, "%s/%s%s", TOPIC_PRE, config.client_id, COMMAND_TOPIC);
    sprintf(commandAckTopic, "%s/%s%s", TOPIC_PRE, config.client_id, COMMAND_ACK_TOPIC);
    sprintf(eventTopic, "%s/%s%s", TOPIC_PRE, config.client_id, EVENT_TOPIC);
    sprintf(configTopic, "%s/%s%s", TOPIC_PRE,config.client_id, CONFIG_TOPIC);
    sprintf(configAckTopic, "%s/%s%s", TOPIC_PRE,config.client_id, CONFIG_ACK_TOPIC);

    config.retry_limit = 5;
    config.payload_size = DEFAULT_PAYLOAD_SIZE;
    iot_client->config = config;
    parse_mode = mode;
#if defined(Z_SECURE_CONNECTION)
    if(TLS_MODE){
        if (ca_crt == NULL || (mode == REFERENCE && access(ca_crt, F_OK) == -1))
        {
            log_error("RootCA file is not found/can't be accessed");
            return ZFAILURE;
        }
        iot_client->certs.ca_crt = ca_crt;
#if defined(Z_USE_CLIENT_CERTS)
        if(TLS_CLIENT_CERTS){
            if (client_cert == NULL || client_key == NULL || cert_password == NULL || (mode == REFERENCE && (access(client_cert, F_OK) == -1)) || (mode == REFERENCE && (access(client_key, F_OK) == -1)))
            {
                log_error("Client key or Client certificate is not found/can't be accessed");
                return ZFAILURE;
            }
            iot_client->certs.client_cert = client_cert;
            iot_client->certs.client_key = client_key;
            iot_client->certs.cert_password = cert_password;
        }
#endif
    }
    else{
        log_info("\033[35m TLS_MODE is disabled \033[0m");
        ZPORT = 1883;
    }
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
    initMessageHandler(iot_client, commandTopic, commandAckTopic, configTopic,configAckTopic);
    iot_client->current_state = INITIALIZED;
    log_info("Client Initialized!");
    return ZSUCCESS;
}

int zclient_setMaxPayloadSize(ZohoIOTclient *iot_client,int size)
{
    if(size > MAX_PAYLOAD_SIZE)
    {
        iot_client->config.payload_size = MAX_PAYLOAD_SIZE;
        log_error("Message payload size %d is greater than max size %d ,continuing on max payload size",size,MAX_PAYLOAD_SIZE);
        return -1;
    }
    else if(size<1)
    {
        iot_client->config.payload_size = DEFAULT_PAYLOAD_SIZE;
        log_error("Message payload size %d can't be less than 1 ,continuing on default payload size %d",size,DEFAULT_PAYLOAD_SIZE);
        return -1;
    }
    else
    {
        iot_client->config.payload_size = size;
        log_debug("Message payload size updated to %d",iot_client->config.payload_size);
        return 0;
    }
}

void zclient_addConnectionParameter(char *connectionParamKey, char *connectionParamValue)
{
    strcat(connectionStringBuff, connectionParamKey);
    strcat(connectionStringBuff, "=");
    strcat(connectionStringBuff, connectionParamValue);
    strcat(connectionStringBuff, "&");
}

char *formConnectionString(char *username)
{
    connectionStringBuff[0] = '\0'; // initialize global variable
    strcat(connectionStringBuff, username);
    strcat(connectionStringBuff, "?");
    zclient_addConnectionParameter("sdk_name", "zoho-iot-sdk-c");
    zclient_addConnectionParameter("sdk_version", Z_SDK_VERSION);
    //zclient_addConnectionParams("sdk_url", "");
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
    unsigned const int buff_size = client->config.payload_size;
    if(client->config.mqttBuff != NULL)
    {
        free(client->config.mqttBuff);
    }
    if(client->config.mqttReadBuff != NULL)
    {
        free(client->config.mqttReadBuff);
    }
    client->config.mqttBuff = (char*)malloc(buff_size);
    client->config.mqttReadBuff = (char*)malloc(buff_size);

    log_info("Preparing Network..");
    NetworkInit(&n);

#if defined(Z_SECURE_CONNECTION)
    if(TLS_MODE){
        rc = NetworkConnectTLS(&n, client->config.hostname, ZPORT, parse_mode, client->certs.ca_crt, client->certs.client_cert, client->certs.client_key, client->certs.cert_password);
    }
    else
    {
        rc = NetworkConnect(&n, client->config.hostname, ZPORT);
    }
#else
    rc = NetworkConnect(&n, client->config.hostname, ZPORT);
#endif
    if (rc != ZSUCCESS)
    {
        log_error("Error Connecting Network.. %d ", rc);
        if(rc == -11)
        {
            log_fatal("Rebooting Because of Time Rest\n\n\n");
            exit(0);
        }
        return ZFAILURE;
    }

    //TODO: Handle the rc of ConnectNetwork().
    log_info("Connecting to \x1b[32m %s : %d \x1b[0m", client->config.hostname, ZPORT);
    MQTTClientInit(&client->mqtt_client, &n, 30000, client->config.mqttBuff, buff_size, client->config.mqttReadBuff, buff_size);
    MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;

    conn_data.MQTTVersion = 4;
    conn_data.cleansession = 1; //TODO: tobe confirmed with Hub
    conn_data.keepAliveInterval = 120;
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

unsigned long long getCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long seconds = (unsigned long long)(tv.tv_sec);
    return seconds;
}

int zclient_reconnect(ZohoIOTclient *client)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }

    if (client->current_state == CONNECTED)
    {
        return ZSUCCESS;
    }
    if (start_time == 0)
    {
        start_time = getCurrentTime();
    }
    int delay = client->ZretryInterval;
    if (getCurrentTime() - start_time > delay)
    {
        if(client->current_state != INITIALIZED)
        {
            NetworkDisconnect(client->mqtt_client.ipstack);
        }
        rc = zclient_connect(client);
        if (rc == ZSUCCESS)
        {
            client->current_state = CONNECTED;
            retryCount = 0;
            start_time = 0;
            client->ZretryInterval = MIN_RETRY_INTERVAL;
            if(on_command_message_handler!= NULL)
            {
                zclient_command_subscribe(client, on_command_message_handler);
            }
            if(on_config_message_handler!=NULL)
            {
                zclient_config_subscribe(client, on_config_message_handler);
            }
            if(retryACK)
            {
                log_debug("Attempting to resend the ACK message that previously failed");
                char *payload = NULL;
                payload = cJSON_Print(failedACK.ackPayload);
                MQTTMessage pubmsg;
                pubmsg.id = rand()%10000;
                pubmsg.qos = 1;
                pubmsg.dup = '0';
                pubmsg.retained = '0';
                pubmsg.payload = payload;
                pubmsg.payloadlen = strlen(pubmsg.payload);
                rc = MQTTPublish(&(client->mqtt_client), failedACK.topic, &pubmsg);
                if(rc == ZSUCCESS)
                {
                    log_debug("\x1b[36m Failed ACK published \x1b[0m");
                    log_trace("ACK published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", pubmsg.payload, failedACK.topic);
                    cJSON_Delete(failedACK.ackPayload);
                    retryACK =false;
                }
                else{
                    log_error("Error publishing Ack, Error code: %d", rc);
                }
                free(payload);
                
            }
            if(retryEvent)
            {
                if(failedEvent.eventPayloadTime + 60 < getCurrentTime())
                {
                    log_debug("Event is expired, so not attempting to resend the Event message that previously failed");
                    cJSON_Delete(failedEvent.eventPayload);
                    retryEvent =false;
                }
                else
                {
                    log_debug("Attempting to resend the Event message that previously failed");
                    char *payload = NULL;
                    payload = cJSON_Print(failedEvent.eventPayload);
                    MQTTMessage pubmsg;
                    pubmsg.id = rand()%10000;
                    pubmsg.qos = 1;
                    pubmsg.dup = '0';
                    pubmsg.retained = '0';
                    pubmsg.payload = payload;
                    pubmsg.payloadlen = strlen(pubmsg.payload);
                    rc = MQTTPublish(&(client->mqtt_client), eventTopic, &pubmsg);
                    if(rc == ZSUCCESS)
                    {
                        log_debug("\x1b[36m Failed Event published \x1b[0m");
                        log_trace("Event published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", pubmsg.payload, eventTopic);
                        cJSON_Delete(failedEvent.eventPayload);
                        retryEvent =false;
                    }
                    else{
                        log_error("Error publishing Event, Error code: %d", rc);
                    }
                    free(payload);
                }
            }
            return ZSUCCESS;
        }
        start_time = getCurrentTime();
        client->ZretryInterval = getRetryInterval(client->ZretryInterval);
        retryCount++;
        log_info("retryCount :%d", retryCount);
        log_info("Trying to reconnect \x1b[32m %s : %d \x1b[0m in %d sec ", client->config.hostname, ZPORT, client->ZretryInterval);
        if (client->current_state != DISCONNECTED && client->current_state != CONNECTED)
        {
            log_info("Retrying indefinetely");
            return ZCONNECTION_ERROR;
        }
    }
    usleep(200000);
    return ZFAILURE;
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
    pubmsg.id = rand()%10000;
    pubmsg.qos = 1;
    pubmsg.dup = '0';
    pubmsg.retained = '0';
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    if(pubmsg.payloadlen>client->config.payload_size)
    {
        log_error("Error on Pubish,payload \x1b[31m(%d)\x1b[0m size is greater than client max payload size \x1b[31m(%d)\x1b[0m", pubmsg.payloadlen,client->config.payload_size);
        return ZFAILURE;
    }
    rc = MQTTPublish(&(client->mqtt_client), dataTopic, &pubmsg);
    //TODO: check for connection and retry to send the message once the conn got restroed.
    if (rc == ZSUCCESS)
    {
        log_debug("\x1b[36m Telemetry Message Published \x1b[0m");
        log_trace("Published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", payload, dataTopic);
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
    char *payload = NULL;
    payload = cJSON_Print(client->message.data);
    int status = zclient_publish(client, payload);
    free(payload);
    cJSON_Delete(client->message.data);
    client->message.data= NULL;
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
int zclient_addEventDataObject(char *key, cJSON* Object)
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
    cJSON* event_object_copy = cJSON_Duplicate(Object, 1);
    if (!cJSON_HasObjectItem(eventDataObject, key))
    {
       cJSON_AddItemToObject(eventDataObject, key, event_object_copy);
    }
    else
    {
        cJSON_ReplaceItemInObject(eventDataObject, key,event_object_copy);
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
    char *eventDataJSONString = NULL;
    eventDataJSONString = cJSON_Print(eventDataObject);
    int rc = zclient_dispatchEventFromJSONString(client, eventType, eventDescription, eventDataJSONString, assetName);
    cJSON_Delete(eventDataObject);
    free(eventDataJSONString);
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
    char *payload = NULL;
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
    
    MQTTMessage pubmsg;
    pubmsg.id = rand()%10000;
    pubmsg.qos = 1;
    pubmsg.dup = '0';
    pubmsg.retained = '0';
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    if(pubmsg.payloadlen>client->config.payload_size)
    {
        log_error("Error on dispatchEvent, payload \x1b[31m(%d)\x1b[0m size is greater than client max payload size \x1b[31m(%d)\x1b[0m", pubmsg.payloadlen,client->config.payload_size);
        return ZFAILURE;
    }
    rc = MQTTPublish(&(client->mqtt_client), eventTopic, &pubmsg);
    //TODO: check for connection and retry to send the message once the conn got restroed.
    if (rc == ZSUCCESS)
    {
        log_debug("\x1b[36m Event Message Published \x1b[0m");
        log_trace("Event dispatched \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", payload, eventTopic);
    }
    else if (client->mqtt_client.isconnected == 0)
    {
        client->current_state = DISCONNECTED;
        log_error("Error on dispatchEvent due to lost connection. Error code: %d", rc);
        retryEvent = true;
        if(failedEvent.eventPayload != NULL)
        {
            cJSON_Delete(failedEvent.eventPayload);
        }
        failedEvent.eventPayload = cJSON_Duplicate(eventObject, 1);
        failedEvent.eventPayloadTime = getCurrentTime();
    }
    else
    {
        log_error("Error on dispatchEvent. Error code: %d", rc);
        retryEvent = true;
        failedEvent.eventPayload = cJSON_Duplicate(eventObject, 1);
        failedEvent.eventPayloadTime = getCurrentTime();
    }
    free(payload);
    cJSON_Delete(eventObject);
    return rc;
}

int zclient_publishCommandAck(ZohoIOTclient *client, char *payload, ZcommandAckResponseCodes status_code, char *responseMessage)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    cJSON* Ack_payload = generateProcessedACK(payload,status_code, responseMessage);
    if (Ack_payload == NULL) {
        return ZFAILURE;
    }
    char *command_ack_payload = NULL;
    command_ack_payload = cJSON_Print(Ack_payload);
    MQTTMessage pubmsg;
    pubmsg.id = rand()%10000;
    pubmsg.qos = 1;
    pubmsg.dup = '0';
    pubmsg.retained = '0';
    pubmsg.payload = command_ack_payload;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    rc = MQTTPublish(&(client->mqtt_client), commandAckTopic, &pubmsg);
    if (rc == ZSUCCESS)
    {
        log_debug("\x1b[36m Command ACK Published \x1b[0m");
        log_trace("Command Ack published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", pubmsg.payload, commandAckTopic);
    }
    else if (client->mqtt_client.isconnected == 0)
    {
        client->current_state = DISCONNECTED;
        log_error("Error on publishing command ACK due to lost connection. Error code: %d", rc);
        retryACK = true;
        failedACK.ackPayload = cJSON_Duplicate(Ack_payload, 1);
        failedACK.topic = commandAckTopic;
    }
    else
    {
        log_error("Error on publishing command Ack. Error code: %d", rc);
    }
    cJSON_Delete(Ack_payload);
    free(command_ack_payload);
    return rc;
}
int zclient_publishConfigAck(ZohoIOTclient *client, char *payload, ZcommandAckResponseCodes status_code, char *responseMessage)
{
    
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    cJSON* Ack_payload = generateProcessedACK(payload,status_code, responseMessage);
    if (Ack_payload == NULL) {
        return ZFAILURE;
    }
    char *config_ack_payload = NULL;
    config_ack_payload = cJSON_Print(Ack_payload);
    MQTTMessage pubmsg;
    pubmsg.id = rand()%10000;
    pubmsg.qos = 1;
    pubmsg.dup = '0';
    pubmsg.retained = '0';
    pubmsg.payload = config_ack_payload;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    rc = MQTTPublish(&(client->mqtt_client), configAckTopic, &pubmsg);
    if (rc == ZSUCCESS)
    {
        log_debug("\x1b[36m Config ACK Published \x1b[0m");
        log_trace("Config Ack published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", pubmsg.payload, configAckTopic);
    }
    else if (client->mqtt_client.isconnected == 0)
    {
        client->current_state = DISCONNECTED;
        log_error("Error on publishing config ack due to lost connection. Error code: %d", rc);
        retryACK = true;
        failedACK.ackPayload = cJSON_Duplicate(Ack_payload, 1);
        failedACK.topic = configAckTopic;
    }
    else
    {
        log_error("Error on publishing config Ack. Error code: %d", rc);
    }
    cJSON_Delete(Ack_payload);
    free(config_ack_payload);
    return rc;
}

int zclient_command_subscribe(ZohoIOTclient *client, messageHandler on_message)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (client->current_state != CONNECTED)
    {
        log_error("Can not subscribe, since connection is lost/not established");
        return ZFAILURE;
    }
    //TODO: add basic validation & callback method and append it on error logs.
    rc = MQTTSubscribe(&(client->mqtt_client), commandTopic, QOS0, onMessageReceived);
    setCommandMessageHandler(on_message);
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
int zclient_config_subscribe(ZohoIOTclient *client, messageHandler on_message)
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
    rc = MQTTSubscribe(&(client->mqtt_client), configTopic, QOS0, onMessageReceived);
    setConfigMessageHandler(on_message);
    if (rc == ZSUCCESS)
    {
        log_info("Subscribed on \x1b[36m '%s' \x1b[0m", configTopic);
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
    // if (client->current_state == DISCONNECTED)
    // {
    //     rc = zclient_connect(client);
    // }

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
            // log_error("Error on Yielding due to lost connection. Error code: %d", rc);
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
    }
    if(client->current_state != INITIALIZED)
    {
    	NetworkDisconnect(client->mqtt_client.ipstack);
    }
    client->current_state = DISCONNECTED;
    log_info("Disconnected.");
    return rc;
}

//int zclient_setRetrycount(ZohoIOTclient *client, int count)
//{
//    int rc = validateClientState(client);
//    if (rc != 0)
//    {
//        return rc;
//    }
//
//    if (count < 0)
//    {
//        log_info("Retry limit value given is < 0 , so set to default value :%d", client->config.retry_limit);
//        return ZFAILURE;
//    }
//
//    client->config.retry_limit = count;
//    return ZSUCCESS;
//}

cJSON *addAssetNameTopayload(ZohoIOTclient *client, char *assetName)
{
    if(client->message.data == NULL)
    {
      client->message.data =cJSON_CreateObject();  
    }

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
    //TODO: null check for object required
    else
    {
        cJSON_ReplaceItemInObject(obj, key, cJSON_CreateString(val_string));
    }
    return rc;
}

int zclient_addObject(ZohoIOTclient *client, char *key, cJSON* val_object, char *assetName)
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
    //TODO: null check for object required
    cJSON* val_object_copy = cJSON_Duplicate(val_object, 1);  
    if (!cJSON_HasObjectItem(obj, key))
    {
        cJSON_AddItemToObject(obj, key, val_object_copy);
    }
    else
    {
        cJSON_ReplaceItemInObject(obj, key,val_object_copy);
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
cJSON* zclient_FormReceivedACK(char* payload)
{
    return generateACKPayload(payload,RECIEVED_ACK_CODE ,"");
}
cJSON* generateProcessedACK(char* payload,ZcommandAckResponseCodes status_code, char *responseMessage)
{
    if (responseMessage == NULL)
    {
        log_error("Response cannot be Null in Ack object");
       return NULL;
    }
    if (status_code != SUCCESFULLY_EXECUTED && (status_code < EXECUTION_FAILURE || status_code > ALREADY_ON_SAME_STATE))
    {
        log_error("Status code provided is not a valid in Ack object");
        return NULL;
    }
    return generateACKPayload(payload,status_code ,responseMessage);
}

cJSON* generateACKPayload(char* payload,ZcommandAckResponseCodes status_code, char *responseMessage) {
    
    cJSON *commandMessageArray = cJSON_Parse(payload);
    if (cJSON_IsArray(commandMessageArray) == 1) {
        cJSON *commandAckObject = cJSON_CreateObject();
        cJSON *commandMessage, *commandAckObj;
        int len = cJSON_GetArraySize(commandMessageArray);
        for (int iter = 0; iter < len; iter++) {
            commandMessage = cJSON_GetArrayItem(commandMessageArray, iter);
            char *correlation_id = cJSON_GetObjectItem(commandMessage, "correlation_id")->valuestring;
            commandAckObj = cJSON_CreateObject();
            cJSON_AddItemToObject(commandAckObject, correlation_id, commandAckObj);
            cJSON_AddNumberToObject(commandAckObj, "status_code", status_code);
            cJSON_AddStringToObject(commandAckObj, "response",responseMessage);
        }
        cJSON_Delete(commandMessageArray);
        return commandAckObject;
    }
    cJSON_Delete(commandMessageArray);
    return NULL;
}
int zclient_free(ZohoIOTclient *client)
{
    if (client == NULL)
    {
        log_error("Client object is NULL");
        return ZFAILURE;
    }
    if (client->current_state == CONNECTED)
    {
        zclient_disconnect(client);
    }
    free(client->config.hostname);
    free(client->config.client_id);
    free(client->config.auth_token);
    free(client->config.MqttUserName);
    free(client->config.mqttBuff);
    free(client->config.mqttReadBuff);
    if(client->message.data != NULL)
    {
        cJSON_Delete(client->message.data);
    }
    if(eventDataObject != NULL)
    {
        cJSON_Delete(eventDataObject);
    }
    if(failedACK.ackPayload != NULL)
    {
        cJSON_Delete(failedACK.ackPayload);
    }
    if(failedACK.topic != NULL)
    {
        free(failedACK.topic);
    }
    #if defined(Z_SECURE_CONNECTION)
    if(TLS_MODE){
        #if defined(Z_USE_CLIENT_CERTS)
        if(TLS_CLIENT_CERTS){
            free(client->certs.client_cert);
            free(client->certs.client_key);
            free(client->certs.cert_password);
        }
        #endif
        free(client->certs.ca_crt);
    }
    #endif
    log_free();
    return ZSUCCESS;
}
