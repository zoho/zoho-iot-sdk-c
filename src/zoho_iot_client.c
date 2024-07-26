#include "zoho_iot_client.h"
#include "sdk_version.h"
#include "zoho_message_handler.h"
#include "sys/socket.h"
#include "unistd.h"
#include <stdbool.h>
#include <pthread.h>
#define MQTT_EMB_LOGGING
//TODO: read from config file.
Network n;
certsParseMode parse_mode;
time_t start_time = 0;
int retryCount = 0;
char dataTopic[100] = "", commandTopic[100] = "", eventTopic[100] = "",configTopic[100] = "";
char commandAckTopic[100] = "",configAckTopic[100]="", connectionStringBuff[256] = "",agentName[100]="",agentVersion[100]="",platform[100]="";
cJSON *eventDataObject;
bool retryACK;
ZfailedACK failedACK;
bool retryEvent;
ZfailedEvent failedEvent;

bool paho_debug = false;
bool TLS_MODE = true;
bool TLS_CLIENT_CERTS = true;
extern Z_log Zlog;

//OTA related variables
bool OTA_RECEIVED = false;
OTAHandler on_OTA_handler = NULL;

#if defined(Z_CLOUD_LOGGING)
extern int numberOfLinesRead;
extern long sizeOfLogRead;
#endif

#if defined(Z_SECURE_CONNECTION)
int ZPORT = 8883;
#else
int ZPORT = 1883;
#endif

bool cloud_logging_in_processing = false;
bool CLOUD_LOGGING = false;
bool get_cloud_logging_status(){
    log_debug("Cloud logging status : %d",CLOUD_LOGGING);
    log_debug("Cloud logging in processing status : %d",cloud_logging_in_processing);
    if(CLOUD_LOGGING == true && cloud_logging_in_processing == false)
    {
        return true;
    }
    return false;
}

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
bool get_OTA_status()
{
    return OTA_RECEIVED;
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
        log_error("populating config object failed");
        return ZFAILURE;
    }
    return ZSUCCESS;
}

int zclient_init(ZohoIOTclient *iot_client, char *MQTTUserName, char *MQTTPassword, certsParseMode mode, char *ca_crt, char *client_cert, char *client_key, char *cert_password, ZlogConfig *logConfig)
{
    //TODO:1
    // All config.h and device related validations should be done here itself !

    
    log_initialize(logConfig);
    #if defined(Z_CLOUD_LOGGING)
        log_info("Cloud_Logging is enabled");
        intitialize_cloud_log();
    #endif
    log_info("\n\n\nSDK Initializing.. version: %s",Z_SDK_VERSION);

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
        iot_client->certs.ca_crt = strdup(ca_crt);
#if defined(Z_USE_CLIENT_CERTS)
        if(TLS_CLIENT_CERTS){
            if (client_cert == NULL || client_key == NULL || cert_password == NULL || (mode == REFERENCE && (access(client_cert, F_OK) == -1)) || (mode == REFERENCE && (access(client_key, F_OK) == -1)))
            {
                log_error("Client key or Client certificate is not found/can't be accessed");
                return ZFAILURE;
            }
            iot_client->certs.client_cert = strdup(client_cert);
            iot_client->certs.client_key = strdup(client_key);
            iot_client->certs.cert_password = strdup(cert_password);
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
    if (pthread_mutex_init(&iot_client->lock, NULL) != 0) {
        log_error("Mutex initialization failed");
        exit(EXIT_FAILURE);
    }
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
 bool zclient_setAgentNameandVersion(char * name,char * version){
    if(!isStringValid(name) || !isStringValid(version)){
        log_error("Agent name or version is invalid");
        return false;
    }
    strcpy(agentName,name);
    strcpy(agentVersion,version);
    return true;
}

bool zclient_setPlatformName(char * platformName){
    if(!isStringValid(platformName)){
        log_error("Platform name is invalid");
        return false;
    }
    strcpy(platform,platformName);
    return true;
}

void zclient_addConnectionParameter(char *connectionParamKey, char *connectionParamValue)
{
    strcat(connectionStringBuff, connectionParamKey);
    strcat(connectionStringBuff, "=");
    strcat(connectionStringBuff, connectionParamValue);
    strcat(connectionStringBuff, "&");
}
void addOsdetailstoConnectionParameters(){
    char osname[50];
    char osversion[50];
    if(!getOsnameOsversion(osname,osversion)){
        log_error("failed to get os details");
        return;
    }
    zclient_addConnectionParameter("os_name", osname);
    zclient_addConnectionParameter("os_version", osversion);
}

char *formConnectionString(char *username)
{
    connectionStringBuff[0] = '\0'; // initialize global variable
    strcat(connectionStringBuff, username);
    strcat(connectionStringBuff, "?");
    zclient_addConnectionParameter("sdk_name", "zoho-iot-sdk-c");
    zclient_addConnectionParameter("sdk_version", Z_SDK_VERSION);
    if(isStringValid(agentName) && isStringValid(agentVersion)){
        zclient_addConnectionParameter("agent_name",agentName);
        zclient_addConnectionParameter("agent_version",agentVersion);
    }
    if(isStringValid(platform)){
        zclient_addConnectionParameter("agent_platform",platform);
    }
    addOsdetailstoConnectionParameters();
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

    // Lock the mutex
    log_trace("Getting client lock for init");
    pthread_mutex_lock(&client->lock);
    log_trace("Got client lock for init");
    MQTTClientInit(&client->mqtt_client, &n, 30000, client->config.mqttBuff, buff_size, client->config.mqttReadBuff, buff_size);
    // Unlock the mutex
    log_trace("Releasing client lock for init");
    pthread_mutex_unlock(&client->lock);
    log_trace("Released client lock for init");
    MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;

    conn_data.MQTTVersion = 4;
    conn_data.cleansession = 1; //TODO: tobe confirmed with Hub
    conn_data.keepAliveInterval = 120;
    conn_data.clientID.cstring = client->config.client_id;
    conn_data.willFlag = 0;

    //TODO:2: to be verified with HUB.
    conn_data.username.cstring = formConnectionString(client->config.MqttUserName);
    conn_data.password.cstring = client->config.auth_token;

    // Lock the mutex
    log_trace("Getting client lock for connect");
    pthread_mutex_lock(&client->lock);
    log_trace("Got client lock for connect");
    rc = MQTTConnect(&client->mqtt_client, &conn_data);
    // Unlock the mutex
    log_trace("Releasing client lock for connect");
    pthread_mutex_unlock(&client->lock);
    log_trace("Released client lock for connect");
    if (rc == 0)
    {
        log_info("Connected!");
        client->current_state = CONNECTED;
    }
    else
    {
        // Lock the mutex
        log_trace("Getting client lock for network disconnect");
        pthread_mutex_lock(&client->lock);
        log_trace("Got client lock for network disconnect");
        NetworkDisconnect(client->mqtt_client.ipstack);
        // Unlock the mutex
        log_trace("Releasing client lock for network disconnect");
        pthread_mutex_unlock(&client->lock);
        log_trace("Released client lock for network disconnect");
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
            // Lock the mutex
            log_trace("Getting client lock for network disconnect");
            pthread_mutex_lock(&client->lock);
            log_trace("Got client lock for network disconnect");
            NetworkDisconnect(client->mqtt_client.ipstack);
            // Unlock the mutex
            log_trace("Releasing client lock for network disconnect");
            pthread_mutex_unlock(&client->lock);
            log_trace("Released client lock for network disconnect");
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
                rc = publishMessage(client, failedACK.topic, payload);
                if(rc == ZSUCCESS)
                {
                    log_debug("\x1b[36m Failed ACK published \x1b[0m");
                    log_trace("ACK published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", payload, failedACK.topic);
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
                if(failedEvent.eventPayloadTime + 300 < getCurrentTime())
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
                    rc = publishMessage(client, eventTopic, payload);
                    if(rc == ZSUCCESS)
                    {
                        log_debug("\x1b[36m Failed Event published \x1b[0m");
                        log_trace("Event published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", payload, eventTopic);
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
    rc = publishMessage(client, dataTopic, payload);
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
    rc = publishMessage(client, eventTopic, payload);
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
        log_error("Error on generating command Acknowledgement");
        return ZFAILURE;
    }
    char *command_ack_payload = NULL;
    command_ack_payload = cJSON_Print(Ack_payload);
    rc = publishMessage(client, commandAckTopic, command_ack_payload);
    if (rc == ZSUCCESS)
    {
        log_debug("\x1b[36m Command ACK Published \x1b[0m");
        log_trace("Command Ack published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", command_ack_payload, commandAckTopic);
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
        log_error("Error on generating config Acknowledgement");
        return ZFAILURE;
    }
    char *config_ack_payload = NULL;
    config_ack_payload = cJSON_Print(Ack_payload);
    rc = publishMessage(client, configAckTopic, config_ack_payload);
    if (rc == ZSUCCESS)
    {
        log_debug("\x1b[36m Config ACK Published \x1b[0m");
        log_trace("Config Ack published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", config_ack_payload, configAckTopic);
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
    // Lock the mutex
    log_trace("Getting client lock for subscribe");
    pthread_mutex_lock(&client->lock);
    log_trace("Got client lock for subscribe");
    //TODO: add basic validation & callback method and append it on error logs.
    rc = MQTTSubscribe(&(client->mqtt_client), commandTopic, QOS0, onMessageReceived);
    // Unlock the mutex
    log_trace("Releasing client lock for subscribe");
    pthread_mutex_unlock(&client->lock);
    log_trace("Released client lock for subscribe");
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
        log_error("Can not subscribe, since connection is lost/not established");
        return ZFAILURE;
    }
    // Lock the mutex
    log_trace("Getting client lock for subscribe");
    pthread_mutex_lock(&client->lock);
    log_trace("Got client lock for subscribe");
    //TODO: add basic validation & callback method and append it on error logs.
    rc = MQTTSubscribe(&(client->mqtt_client), configTopic, QOS0, onMessageReceived);
    // Unlock the mutex
    log_trace("Releasing client lock for subscribe");
    pthread_mutex_unlock(&client->lock);
    log_trace("Released client lock for subscribe");
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
    // lock the mutex
    log_trace("Getting client lock for yield");
    pthread_mutex_lock(&client->lock);
    log_trace("Got client lock for yield");
    rc = MQTTYield(&client->mqtt_client, time_out);
    // Unlock the mutex
    log_trace("Releasing client lock for yield");
    pthread_mutex_unlock(&client->lock);
    log_trace("Released client lock for yield");
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
        // lock the mutex
        log_trace("Getting client lock for mqtt disconnect");
        pthread_mutex_lock(&client->lock);
        log_trace("Got client lock for mqtt disconnect");
        rc = MQTTDisconnect(&client->mqtt_client);
        // Unlock the mutex
        log_trace("Releasing client lock for mqtt disconnect");
        pthread_mutex_unlock(&client->lock);
        log_trace("Released client lock for mqtt disconnect");
    }
    if(client->current_state != INITIALIZED)
    {
        // lock the mutex
        log_trace("Getting client lock for network disconnect");
        pthread_mutex_lock(&client->lock);
        log_trace("Got client lock for network disconnect");
    	NetworkDisconnect(client->mqtt_client.ipstack);
        // Unlock the mutex
        log_trace("Releasing client lock for network disconnect");
        pthread_mutex_unlock(&client->lock);
        log_trace("Released client lock for network disconnect");
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
    
    switch (status_code) {
        case SUCCESFULLY_EXECUTED:
        case CONFIG_SUCCESSFULLY_UPDATED:
        case EXECUTION_FAILURE:
        case METHOD_NOT_FOUND:
        case EXECUTING_PREVIOUS_COMMAND:
        case INSUFFICIENT_INPUTS:
        case DEVICE_CONNECTIVITY_ISSUES:
        case PARTIAL_EXECUTION:
        case ALREADY_ON_SAME_STATE:
        case CONFIG_FAILED:
            break;
        default:
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

            //check if the command is OTA
            cJSON *command_name_json = cJSON_GetObjectItem(commandMessage, "command_name");
            if(command_name_json != NULL)
            {
                char *command_name = command_name_json->valuestring;
                if(strcmp(command_name,"Z_OTA")== 0)
                {
                    OTA_RECEIVED = true;
                }
                else
                {
                    OTA_RECEIVED = false;
                }

                //check if the command is for cloud logging
                if(strcmp(command_name,"Z_PUBLISH_DEVICE_LOGS")== 0)
                {
                    CLOUD_LOGGING = true;
                    #if defined(Z_CLOUD_LOGGING)
                        cJSON *payload_array = cJSON_GetObjectItem(commandMessage, "payload");
                        cJSON *payload_json = cJSON_GetArrayItem(payload_array, 0);
                        char *value = cJSON_GetObjectItem(payload_json, "value")->valuestring;
                        cloud_logging_set_lines(value);
                    #endif
                }
                else
                {
                    CLOUD_LOGGING = false;
                }
            }

            commandAckObj = cJSON_CreateObject();
            cJSON_AddItemToObject(commandAckObject, correlation_id, commandAckObj);
            cJSON_AddNumberToObject(commandAckObj, "status_code", status_code);
            cJSON_AddStringToObject(commandAckObj, "response",responseMessage);
            //check is_new_config key is present in the payload
            cJSON *new_config = cJSON_GetObjectItem(commandMessage, "is_new_config");
            if(new_config != NULL)
            {
                if(new_config->type == cJSON_True)
                {
                    cJSON_AddBoolToObject(commandAckObj, "is_new_config", true);
                }
                else
                {
                    cJSON_AddBoolToObject(commandAckObj, "is_new_config", false);
                }
            }
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
    pthread_mutex_destroy(&client->lock);
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
            if(client->certs.client_cert != NULL)
            { 
                free(client->certs.client_cert);
            }
            if(client->certs.client_key != NULL)
            { 
                free(client->certs.client_key);
            }
            if(client->certs.cert_password != NULL)
            { 
                free(client->certs.cert_password);
            }
        }
        #endif
        if(client->certs.ca_crt != NULL)
        {   
            free(client->certs.ca_crt);
        }
    }
    #endif
    log_free();
    return ZSUCCESS;
}

#if defined(Z_CLOUD_LOGGING)
bool parse_http_response(const char* str) {
    const char* start = strchr(str, '{');
    log_debug("HTTP Server Response : %s\n", start);
    if (start == NULL) {
        log_error("Error: Starting brace not found\n");
        return false;
    }
    const char* end = strchr(start, '}');
    if (end == NULL) {
        log_error("Error: Ending brace not found\n");
        return false;
    }
    size_t length = end - start + 1;
    char json_str[length + 1];
    strncpy(json_str, start, length);
    json_str[length] = '\0';
    cJSON* json = cJSON_Parse(json_str);
    if (json == NULL) {
        log_error("Error: Failed to parse JSON object\n");
        return false;
    }
    char *status = cJSON_GetObjectItem(json, "status")->valuestring;

    if(strcasecmp(status, "success") != 0) {
        log_error("Cloud logging Error Message: %s\n", cJSON_GetObjectItem(json, "message")->valuestring);
        return false;
    }
    log_debug("Cloue logging Success Message: %s\n", cJSON_GetObjectItem(json, "message")->valuestring);
    return true;
}

int http_post_cloud_logging(ZohoIOTclient *client, char *payload,char * responseMessage)
{
    SSL_CTX *ctx;
    SSL *ssl;
    BIO *bio;

    // Initialize OpenSSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    char * hostname = client->config.hostname;
    char * client_id = client->config.client_id;
    char * password = client->config.auth_token;
    #if defined(Z_SECURE_CONNECTION)
        const char *server_root_cert = client->certs.ca_crt;
    #endif

    char host_with_port[100];
    snprintf(host_with_port, sizeof(host_with_port), "%s:443", hostname);
    // Create SSL context
    ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == NULL) {
        log_error( "Error creating SSL context\n");
        strcpy(responseMessage, "Error creating SSL context");
        return ZFAILURE;
    }

       // Enable verbose output
    // SSL_CTX_set_options(ctx, SSL_OP_ALL);

    #if defined(Z_SECURE_CONNECTION)
        if (!SSL_CTX_load_verify_locations(ctx, server_root_cert, NULL)) {
            log_error( "Error loading CA certificate\n, so setting to no verification\n");
            SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
        }
        else{
            SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
        }

    #else
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    #endif

    // Create BIO connection
    bio = BIO_new_ssl_connect(ctx);
    if (bio == NULL) {
        log_error( "Error creating BIO\n");
        strcpy(responseMessage, "Error creating BIO");
        SSL_CTX_free(ctx);
        return ZFAILURE;
    }
    BIO_set_conn_hostname(bio, host_with_port);

    // Perform the connection
    if (BIO_do_connect(bio) <= 0) {
        log_error( "Error connecting to server\n");
        strcpy(responseMessage, "Error connecting to server");
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return ZFAILURE;
    }

    // Get SSL connection
    BIO_get_ssl(bio, &ssl);
    if (!ssl) {
        log_error( "Error establishing SSL connection\n");
        strcpy(responseMessage, "Error establishing SSL connection");
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return ZFAILURE;
    }

    // Perform SSL handshake
    if (SSL_do_handshake(ssl) <= 0) {
        log_error( "Error performing SSL handshake\n");
        strcpy(responseMessage, "Error performing SSL handshake");
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return ZFAILURE;
    }

    cJSON * cloud_log = get_cloud_log();
    if(cloud_log == NULL){
        log_error("Error in fetching the log from the file");
        strcpy(responseMessage, "Error in fetching the log from the file");
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return ZFAILURE;
    }
    char *cloud_log_string = cJSON_Print(cloud_log);
    if (cloud_log_string == NULL){
        log_error("Error in parsing cloudlog array to string");
        strcpy(responseMessage, "Error in parsing cloudlog array to string");
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return ZFAILURE;
    }
    int cloud_log_len = strlen(cloud_log_string);
    log_debug("cloud_data: log fetched successfully from the file");

    char request_url[1000];
    int len = snprintf(request_url,sizeof(request_url),"POST /v1/iot/logs/import?device_id=%s&device_token=%s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n", client_id, password, hostname, cloud_log_len);
    log_info("Size of Cloud Log data is %f KB\n", cloud_log_len/1000.0);

    if(BIO_write(bio, request_url, len) <= 0) {
        log_error( "Error sending POST request\n");
        strcpy(responseMessage, "Error sending POST request");
        free(cloud_log_string);
        cJSON_Delete(cloud_log);
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return ZFAILURE;
    }

    if (BIO_write(bio, cloud_log_string, cloud_log_len) <= 0) {
        log_error( "Error sending data\n");
        strcpy(responseMessage, "Error sending data");
        free(cloud_log_string);
        cJSON_Delete(cloud_log);
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return ZFAILURE;
    }
    log_debug("Cloud_data_writed");

    // Read response
    char buf[1000]={0};
    int bytes_read;
    while ((bytes_read = BIO_read(bio, buf, sizeof(buf))) > 0) {
    }
    BIO_free_all(bio);
    SSL_CTX_free(ctx);
    free(cloud_log_string);
    cJSON_Delete(cloud_log);

     if(parse_http_response(buf)== false){
        log_debug("The respose is %s",buf);
        strcpy(responseMessage, "Error in parsing the http response");
        return ZFAILURE;
    }
    log_info("Cloud logging is Successfull");
    char succesResponse[150];
    sprintf(succesResponse,"Log Published Successfully. Number of lines read - %d, Size of log read - %lld KB",numberOfLinesRead,sizeOfLogRead);
    strcpy(responseMessage, succesResponse);
    return ZSUCCESS;

}

#endif

void handle_cloud_logging(ZohoIOTclient *client, char *payload){
    cloud_logging_in_processing = true;
    int command_response_code;
    char responseMessage[100];
    #if defined(Z_CLOUD_LOGGING)
        if(http_post_cloud_logging(client, payload,responseMessage) == ZSUCCESS){
            command_response_code = SUCCESFULLY_EXECUTED;
        }
        else{
            command_response_code = EXECUTION_FAILURE;
        }
    #else
        command_response_code = EXECUTION_FAILURE;
        strcpy(responseMessage, "Cloud logging is not enabled");
    #endif
    zclient_publishCommandAck(client,payload,command_response_code,responseMessage);
    cloud_logging_in_processing = false;
}

void handle_OTA(ZohoIOTclient *client,char* payload)
{
    char *OTA_URL = NULL;
    char *hash = NULL;
    char *correlation_id = NULL;
    char *edge_command_key = NULL;
    bool validity_check = false;

    //get required fields from payload
    cJSON *commandMessageArray = cJSON_Parse(payload);
    if (cJSON_IsArray(commandMessageArray) == 1) {
        cJSON *commandMessage, *commandAckObj;
        int len = cJSON_GetArraySize(commandMessageArray);
        for (int iter = 0; iter < len; iter++) {
            commandMessage = cJSON_GetArrayItem(commandMessageArray, iter);
            correlation_id = cJSON_GetObjectItem(commandMessage, "correlation_id")->valuestring;
            cJSON *payload_array = cJSON_GetObjectItem(commandMessage, "payload");
            cJSON *payload_message = cJSON_GetArrayItem(payload_array,0);
            edge_command_key = cJSON_GetObjectItem(payload_message, "edge_command_key")->valuestring;
            cJSON *attributes = cJSON_GetObjectItem(payload_message, "attributes");
            for(int i = 0; i < cJSON_GetArraySize(attributes); i++)
            {
                cJSON *attribute = cJSON_GetArrayItem(attributes, i);
                char *attribute_json = cJSON_Print(attribute);
                log_debug("attribute : %s",attribute_json);
                free(attribute_json);
                if(strcmp(cJSON_GetObjectItem(attribute, "field")->valuestring,"OTA_URL") == 0)
                {
                    OTA_URL = cJSON_GetObjectItem(attribute, "value")->valuestring;
                }
                else if (strcmp(cJSON_GetObjectItem(attribute, "field")->valuestring,"HASH") == 0)
                {
                    hash = cJSON_GetObjectItem(attribute, "value")->valuestring;
                }
            }
        }

        //check if the user has set the OTA handler
        if(on_OTA_handler == NULL)
        {
            log_error("OTA Handler is not set");
            zclient_publishOTAAck(client,correlation_id,EXECUTION_FAILURE,"OTA Handler is not set");
            cJSON_Delete(commandMessageArray);
            return;
        }

        //check if the OTA URL is valid
        if(OTA_URL == NULL)
        {
            log_error("OTA URL is NULL");
            zclient_publishOTAAck(client,correlation_id,EXECUTION_FAILURE,"OTA URL is NULL");
            cJSON_Delete(commandMessageArray);
            return;
        }

        //check whether to validate the hash or not
        if(strcmp(edge_command_key,"${YES}") == 0)
        {
            validity_check = true;
            //check if the hash is valid
            if(hash == NULL)
            {
                log_error("Hash is NULL");
                zclient_publishOTAAck(client,correlation_id,EXECUTION_FAILURE,"Hash is NULL");
                cJSON_Delete(commandMessageArray);
                return;
            }
        }

        //call the user defined OTA handler
        on_OTA_handler(OTA_URL,hash,validity_check,correlation_id);
    }
    cJSON_Delete(commandMessageArray);
}

//set the OTA handler
int zclient_ota_handler(OTAHandler on_OTA)
{
    if(on_OTA == NULL)
    {
        log_error("OTA Handler can't be NULL");
        return ZFAILURE;
    }
    on_OTA_handler = on_OTA;
    return ZSUCCESS;
}


int zclient_publishOTAAck(ZohoIOTclient *client, char *correlation_id, ZcommandAckResponseCodes status_code, char *responseMessage){

    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }

    //create the OTA Ack payload
    cJSON* Ack_payload = cJSON_CreateObject();
    cJSON* OTAAckObj = cJSON_CreateObject();
    cJSON_AddNumberToObject(OTAAckObj, "status_code", status_code);
    cJSON_AddStringToObject(OTAAckObj, "response",responseMessage);
    cJSON_AddItemToObject(Ack_payload, correlation_id, OTAAckObj);

    if (Ack_payload == NULL) {
        return ZFAILURE;
    }

    char *command_ack_payload = NULL;
    command_ack_payload = cJSON_Print(Ack_payload);
    rc = publishMessage(client, commandAckTopic, command_ack_payload);
    if (rc == ZSUCCESS)
    {
        log_debug("\x1b[36m OTA ACK Published \x1b[0m");
        log_trace("OTA Ack published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", command_ack_payload, commandAckTopic);
    }
    else if (client->mqtt_client.isconnected == 0)
    {
        client->current_state = DISCONNECTED;
        log_error("Error on publishing OTA ACK due to lost connection. Error code: %d", rc);
        retryACK = true;
        failedACK.ackPayload = cJSON_Duplicate(Ack_payload, 1);
        failedACK.topic = commandAckTopic;
    }
    else
    {
        log_error("Error on publishing OTA Ack. Error code: %d", rc);
    }
    cJSON_Delete(Ack_payload);
    free(command_ack_payload);
    return rc;

}

int publishMessage(ZohoIOTclient *client, const char *topic, char *payload)
{
    int rc;
    MQTTMessage pubmsg;

    // Prepare the MQTT message
    pubmsg.id = rand() % 10000;
    pubmsg.qos = 1;
    pubmsg.dup = 0;
    pubmsg.retained = 0;
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    if(pubmsg.payloadlen>client->config.payload_size)
    {
        log_error("Error on Pubish,payload \x1b[31m(%d)\x1b[0m size is greater than client max payload size \x1b[31m(%d)\x1b[0m", pubmsg.payloadlen,client->config.payload_size);
        return ZFAILURE;
    }
    // Lock the mutex
    log_trace("Getting client lock for publish");
    pthread_mutex_lock(&client->lock);
    log_trace("Got client lock for publish");
    
    // Publish the MQTT message
    rc = MQTTPublish(&(client->mqtt_client), topic, &pubmsg);

    // Unlock the mutex
    log_trace("Releasing client lock for publish");
    pthread_mutex_unlock(&client->lock);
    log_trace("Released client lock for publish");

    return rc;
}
