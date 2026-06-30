#include "zoho_iot_client.h"
#include "sdk_version.h"
#include "zoho_message_handler.h"
#include "sys/socket.h"
#include "unistd.h"
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#define MQTT_EMB_LOGGING

char agentName[100] = "";
char agentVersion[100] = "";
char platform[100] = "";

bool paho_debug = false;
bool TLS_MODE = true;
bool TLS_CLIENT_CERTS = true;
extern Z_log Zlog;


#if defined(Z_HTTP_PUBLISH_ENABLE)
extern int numberOfLinesRead;
extern long sizeOfLogRead;
#endif

#if defined(Z_SECURE_CONNECTION)
int ZPORT = 8883;
#else
int ZPORT = 1883;
#endif

bool get_cloud_logging_status(ZohoIOTclient *client){
    if(client->CLOUD_LOGGING == true && client->cloud_logging_in_processing == false)
    {
        return true;
    }
    return false;
}

//TODO: Add idle methods when socket is busy as in ssl_client_2.

cJSON* generateACKPayload(ZohoIOTclient *client, char* payload, ZcommandAckResponseCodes status_code, char *responseMessage);
cJSON* generateProcessedACK(ZohoIOTclient *client, char* payload, ZcommandAckResponseCodes status_code, char *responseMessage);

void zclient_enable_paho_debug(bool state){
    paho_debug = state;
}
void zclient_set_tls(bool state){
    TLS_MODE = state;
}
void zclient_set_client_certs(bool state){
    TLS_CLIENT_CERTS = state;
}
bool get_OTA_status(ZohoIOTclient *client)
{
    return client->OTA_RECEIVED;
}

#if defined(Z_PAHO_C) && defined(Z_SECURE_CONNECTION)
// Callback invoked by Paho-C for every OpenSSL error during TLS handshake.
// Without this, TLS failures only report error code -1 with no detail.
static int ssl_error_log(const char *str, size_t len, void *u)
{
    (void)u;
    (void)len;
    log_error("SSL error: %s", str);
    return 1;
}
#endif

static void zclient_debug_dump(ZohoIOTclient *client)
{
    /*
    log_debug("=== ZohoIOTclient state dump ===");
    log_debug("  client_id                  : %s", client->config.client_id  ? client->config.client_id  : "(null)");
    log_debug("  hostname                   : %s", client->config.hostname   ? client->config.hostname   : "(null)");
    log_debug("  MqttUserName               : %s", client->config.MqttUserName ? client->config.MqttUserName : "(null)");
    log_debug("  payload_size               : %d", client->config.payload_size);
    log_debug("  retry_limit                : %d", client->config.retry_limit);
    log_debug("  parse_mode                 : %d", (int)client->parse_mode);
    log_debug("  current_state              : %d", (int)client->current_state);
    log_debug("  ZretryInterval             : %d", client->ZretryInterval);
    log_debug("  retryCount                 : %d", client->retryCount);
    log_debug("  start_time                 : %ld", (long)client->start_time);
    log_debug("  dataTopic                  : %s", client->dataTopic);
    log_debug("  commandTopic               : %s", client->commandTopic);
    log_debug("  eventTopic                 : %s", client->eventTopic);
    log_debug("  configTopic                : %s", client->configTopic);
    log_debug("  commandAckTopic            : %s", client->commandAckTopic);
    log_debug("  configAckTopic             : %s", client->configAckTopic);
    log_debug("  retryACK                   : %d", (int)client->retryACK);
    log_debug("  retryEvent                 : %d", (int)client->retryEvent);
    log_debug("  yield_time                 : %llu", client->yield_time);
    log_debug("  yield_interval             : %d", client->yield_interval);
    log_debug("  OTA_RECEIVED               : %d", (int)client->OTA_RECEIVED);
    log_debug("  CLOUD_LOGGING              : %d", (int)client->CLOUD_LOGGING);
    log_debug("  cloud_logging_in_processing: %d", (int)client->cloud_logging_in_processing);
    log_debug("  eventDataObject            : %s", client->eventDataObject ? "(set)" : "(null)");
    log_debug("  on_command_handler         : %s", client->on_command_message_handler ? "(set)" : "(null)");
    log_debug("  on_config_handler          : %s", client->on_config_message_handler  ? "(set)" : "(null)");
    log_debug("  on_OTA_handler             : %s", client->on_OTA_handler ? "(set)" : "(null)");
#if defined(Z_PAHO_C)
    log_debug("  address                    : %s", client->address);
#endif
    log_debug("================================");
    */
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
    static bool sdk_banner_printed = false;

    if (logConfig != NULL)
    {
        log_initialize(logConfig);
    }
    else if (!is_log_initialized())
    {
        log_initialize(NULL);
    }

    if (!sdk_banner_printed)
    {
        log_info("\n\n\tZoho IoT SDK Version:\033[35m %s \033[0m\n", Z_SDK_VERSION);
        #if defined(Z_HTTP_PUBLISH_ENABLE)
            log_info("Cloud_Logging is enabled");
        #endif
        #if defined(Z_SECURE_CONNECTION)
            #if defined(Z_USE_CLIENT_CERTS)
                log_info("Build type: \033[35m TLS build with client certs \033[0m");
            #else
                log_info("Build type: \033[35m TLS build \033[0m");
            #endif
        #else
            log_info("Build type: \033[35m NON_TLS build \033[0m");
        #endif
        #if defined(Z_PAHO_C)
            log_info("Paho type: \033[35m Paho_C \033[0m");
        #else
            log_info("Paho type: \033[35m Embeded Paho \033[0m");
        #endif
        sdk_banner_printed = true;
    }

    #if defined(Z_HTTP_PUBLISH_ENABLE)
        initialize_cloud_log();
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
    sprintf(iot_client->dataTopic, "%s/%s%s", TOPIC_PRE, config.client_id, DATA_TOPIC);
    sprintf(iot_client->commandTopic, "%s/%s%s", TOPIC_PRE, config.client_id, COMMAND_TOPIC);
    sprintf(iot_client->commandAckTopic, "%s/%s%s", TOPIC_PRE, config.client_id, COMMAND_ACK_TOPIC);
    sprintf(iot_client->eventTopic, "%s/%s%s", TOPIC_PRE, config.client_id, EVENT_TOPIC);
    sprintf(iot_client->configTopic, "%s/%s%s", TOPIC_PRE,config.client_id, CONFIG_TOPIC);
    sprintf(iot_client->configAckTopic, "%s/%s%s", TOPIC_PRE,config.client_id, CONFIG_ACK_TOPIC);

    config.retry_limit = 5;
    config.payload_size = DEFAULT_PAYLOAD_SIZE;
    iot_client->config = config;
    iot_client->parse_mode = mode;
#if defined(Z_SECURE_CONNECTION)
    if(TLS_MODE){
        if (ca_crt == NULL || (mode == REFERENCE && access(ca_crt, F_OK) == -1))
        {
            log_error("[%s] RootCA file is not found/can't be accessed", iot_client->config.client_id);
            return ZFAILURE;
        }
        iot_client->certs.ca_crt = strdup(ca_crt);
#if defined(Z_USE_CLIENT_CERTS)
        if(TLS_CLIENT_CERTS){
            if (client_cert == NULL || client_key == NULL || cert_password == NULL || (mode == REFERENCE && (access(client_cert, F_OK) == -1)) || (mode == REFERENCE && (access(client_key, F_OK) == -1)))
            {
                log_error("[%s] Client key or Client certificate is not found/can't be accessed", iot_client->config.client_id);
                return ZFAILURE;
            }
            iot_client->certs.client_cert = strdup(client_cert);
            iot_client->certs.client_key = strdup(client_key);
            iot_client->certs.cert_password = strdup(cert_password);
        }
#endif
    }
    else{
        log_info("[%s]\033[35m TLS_MODE is disabled \033[0m", iot_client->config.client_id);
        ZPORT = 1883;
    }
#endif

    iot_client->message.data = cJSON_CreateObject();
    if (iot_client->message.data == NULL)
    {
        log_error("[%s] Can't create cJSON object", iot_client->config.client_id);
        return ZFAILURE;
    }

    iot_client->eventDataObject = cJSON_CreateObject();
    // Initialize per-client state fields
    iot_client->start_time = 0;
    iot_client->retryCount = 0;
    iot_client->retryACK = false;
    iot_client->retryEvent = false;
    iot_client->yield_time = 0;
    iot_client->yield_interval = 1;
    iot_client->OTA_RECEIVED = false;
    iot_client->on_OTA_handler = NULL;
    iot_client->cloud_logging_in_processing = false;
    iot_client->CLOUD_LOGGING = false;
    iot_client->on_command_message_handler = NULL;
    iot_client->on_config_message_handler = NULL;
    memset(&iot_client->failedACK, 0, sizeof(ZfailedACK));
    memset(&iot_client->failedEvent, 0, sizeof(ZfailedEvent));
    iot_client->connectionStringBuff[0] = '\0';
    initMessageHandler(iot_client);
    if (pthread_mutex_init(&iot_client->lock, NULL) != 0) {
        log_error("[%s] Mutex initialization failed", iot_client->config.client_id);
        return ZFAILURE;
    }
    iot_client->current_state = INITIALIZED;
    zclient_debug_dump(iot_client);
    log_info("[%s] Client Initialized!", iot_client->config.client_id);
    return ZSUCCESS;
}

int zclient_setMaxPayloadSize(ZohoIOTclient *iot_client,int size)
{
    if(size > MAX_PAYLOAD_SIZE)
    {
        iot_client->config.payload_size = MAX_PAYLOAD_SIZE;
        log_error("[%s] Message payload size %d is greater than max size %d ,continuing on max payload size",iot_client->config.client_id,size,MAX_PAYLOAD_SIZE);
        return -1;
    }
    else if(size<1)
    {
        iot_client->config.payload_size = DEFAULT_PAYLOAD_SIZE;
        log_error("[%s] Message payload size %d can't be less than 1 ,continuing on default payload size %d",iot_client->config.client_id,size,DEFAULT_PAYLOAD_SIZE);
        return -1;
    }
    else
    {
        iot_client->config.payload_size = size;
        log_debug("[%s] Message payload size updated to %d",iot_client->config.client_id,iot_client->config.payload_size);
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

void zclient_addConnectionParameter(ZohoIOTclient *client, char *connectionParamKey, char *connectionParamValue)
{
    strcat(client->connectionStringBuff, connectionParamKey);
    strcat(client->connectionStringBuff, "=");
    strcat(client->connectionStringBuff, connectionParamValue);
    strcat(client->connectionStringBuff, "&");
}
void addOsdetailstoConnectionParameters(ZohoIOTclient *client){
    char osname[50];
    char osversion[50];
    if(!getOsnameOsversion(osname,osversion)){
        log_error("[%s] failed to get os details", client->config.client_id);
        return;
    }
    zclient_addConnectionParameter(client, "os_name", osname);
    zclient_addConnectionParameter(client, "os_version", osversion);
}

char *formConnectionString(ZohoIOTclient *client, char *username)
{
    client->connectionStringBuff[0] = '\0';
    strcat(client->connectionStringBuff, username);
    strcat(client->connectionStringBuff, "?");
    zclient_addConnectionParameter(client, "sdk_name", "zoho-iot-sdk-c");
    zclient_addConnectionParameter(client, "sdk_version", Z_SDK_VERSION);
    if(isStringValid(agentName) && isStringValid(agentVersion)){
        zclient_addConnectionParameter(client, "agent_name",agentName);
        zclient_addConnectionParameter(client, "agent_version",agentVersion);
    }
    if(isStringValid(platform)){
        zclient_addConnectionParameter(client, "agent_platform",platform);
    }
    addOsdetailstoConnectionParameters(client);
    client->connectionStringBuff[strlen(client->connectionStringBuff) - 1] = '\0';
    return client->connectionStringBuff;
}

int validateClientState(ZohoIOTclient *client)
{
    if (client == NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }

    if (client->current_state == INITIALIZED ||
        client->current_state == CONNECTED   ||
        client->current_state == DISCONNECTED)
    {
        return ZSUCCESS;
    }
    log_error("[%s] Client should be initialized", client->config.client_id ? client->config.client_id : "?");
    return -2;
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
        log_info("[%s] Client already Connected", client->config.client_id);
        return ZSUCCESS;
    }

#if defined(Z_PAHO_C)
    if(client->current_state == INITIALIZED)
    {
        #if defined(Z_SECURE_CONNECTION)
            if(TLS_MODE){
                sprintf(client->address, "ssl://%s:%d", client->config.hostname, ZPORT);
            }
            else{
                sprintf(client->address, "tcp://%s:%d", client->config.hostname, ZPORT);
            }
        #else
            sprintf(client->address, "tcp://%s:%d", client->config.hostname, ZPORT);
        #endif
        if ((rc = MQTTClient_create(&client->mqtt_client,  client->address,  client->config.client_id,
            MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
        {
            log_error("[%s] Failed to create client, return code %d\n", client->config.client_id, rc);
            return ZFAILURE;
        }
        rc = MQTTClient_setCallbacks(client->mqtt_client, client, NULL, onMessageReceived, NULL);
        if (rc != ZSUCCESS)
        {
            log_error("[%s] Error setting callback. Error code: %d", client->config.client_id, rc);
            return rc;
        }
    }

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.MQTTVersion = MQTTVERSION_3_1_1;
    conn_opts.connectTimeout = 30;  // seconds
    conn_opts.cleansession = 1;
    conn_opts.keepAliveInterval = 60;
    conn_opts.username = formConnectionString(client, client->config.MqttUserName);
    conn_opts.password = client->config.auth_token;
    MQTTClient_setCommandTimeout(client->mqtt_client,30);
    #if defined(Z_SECURE_CONNECTION)
    if(TLS_MODE){
        MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
        conn_opts.ssl = &ssl_opts;
        ssl_opts.keyStore = client->certs.client_cert;
        ssl_opts.trustStore = client->certs.ca_crt;
        ssl_opts.privateKey = client->certs.client_key;
        ssl_opts.privateKeyPassword = client->certs.cert_password;
        ssl_opts.ssl_error_cb = ssl_error_log;
        ssl_opts.ssl_error_context = NULL;
    }
    #endif
    zclient_debug_dump(client);
    log_info("[%s] Connection paho connect", client->config.client_id);
    rc = MQTTClient_connect(client->mqtt_client, &conn_opts);

#else
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

    zclient_debug_dump(client);
    log_info("[%s] Preparing Network..", client->config.client_id);
    NetworkInit(&client->network);

#if defined(Z_SECURE_CONNECTION)
    if(TLS_MODE){
        rc = NetworkConnectTLS(&client->network, client->config.hostname, ZPORT, client->parse_mode, client->certs.ca_crt, client->certs.client_cert, client->certs.client_key, client->certs.cert_password);
    }
    else
    {
        rc = NetworkConnect(&client->network, client->config.hostname, ZPORT);
    }
#else
    rc = NetworkConnect(&client->network, client->config.hostname, ZPORT);
#endif
    if (rc != ZSUCCESS)
    {
        log_error("[%s] Error Connecting Network.. %d ", client->config.client_id, rc);
        if(rc == -11)
        {
            log_fatal("[%s] Device time got changed, disconnecting client", client->config.client_id);
            zclient_disconnect(client);
            client->start_time = 0;
        }
        return ZFAILURE;
    }

    //TODO: Handle the rc of ConnectNetwork().
    log_info("[%s] Connecting to \x1b[32m %s : %d \x1b[0m", client->config.client_id, client->config.hostname, ZPORT);

    // Lock the mutex
    log_trace("[%s] Getting client lock for init", client->config.client_id);
    pthread_mutex_lock(&client->lock);
    log_trace("[%s] Got client lock for init", client->config.client_id);
    MQTTClientInit(&client->mqtt_client, &client->network, 30000, client->config.mqttBuff, buff_size, client->config.mqttReadBuff, buff_size);
    // Unlock the mutex
    log_trace("[%s] Releasing client lock for init", client->config.client_id);
    pthread_mutex_unlock(&client->lock);
    log_trace("[%s] Released client lock for init", client->config.client_id);
    MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;

    conn_data.MQTTVersion = 4;
    conn_data.cleansession = 1; 
    conn_data.keepAliveInterval = 60;
    conn_data.clientID.cstring = client->config.client_id;
    conn_data.willFlag = 0;

    conn_data.username.cstring = formConnectionString(client, client->config.MqttUserName);
    conn_data.password.cstring = client->config.auth_token;

    // Lock the mutex
    log_trace("[%s] Getting client lock for connect", client->config.client_id);
    pthread_mutex_lock(&client->lock);
    log_trace("[%s] Got client lock for connect", client->config.client_id);
    rc = MQTTConnect(&client->mqtt_client, &conn_data);
    // Unlock the mutex
    log_trace("[%s] Releasing client lock for connect", client->config.client_id);
    pthread_mutex_unlock(&client->lock);
    log_trace("[%s] Released client lock for connect", client->config.client_id);
#endif
    if (rc == 0)
    {
        log_info("[%s] Connected!", client->config.client_id);
        client->current_state = CONNECTED;
    }
    else
    {
        // Lock the mutex
        log_trace("[%s] Getting client lock for network disconnect", client->config.client_id);
        pthread_mutex_lock(&client->lock);
        log_trace("[%s] Got client lock for network disconnect", client->config.client_id);
        #if defined(Z_PAHO_C)
        if(client->current_state == INITIALIZED)
        {
            MQTTClient_destroy(&client->mqtt_client);
        }
        #else
        NetworkDisconnect(client->mqtt_client.ipstack);
        #endif
        // Unlock the mutex
        log_trace("[%s] Releasing client lock for network disconnect", client->config.client_id);
        pthread_mutex_unlock(&client->lock);
        log_trace("[%s] Released client lock for network disconnect", client->config.client_id);
        if (rc == 1)
        {
            log_error("[%s] Error while establishing connection, unacceptable protocol version. Error code: %d", client->config.client_id, rc);
        }
        else if (rc == 2)
        {
            log_error("[%s] Error while establishing connection, Invalid ClientId . Error code: %d", client->config.client_id, rc);
        }
        else if (rc == 3)
        {
            log_error("[%s] Error while establishing connection, Server unavailable . Error code: %d", client->config.client_id, rc);
        }
        else if (rc == 4)
        {
            log_error("[%s] Error while establishing connection, due to invalid credentials. Error code: %d", client->config.client_id, rc);
        }
        else if (rc == 5)
        {
            log_error("[%s] Error while establishing connection, Connection refused,as device is not authorized. Error code: %d", client->config.client_id, rc);
        }
        else
        {
            log_error("[%s] Error while establishing connection. Error code: %d", client->config.client_id, rc);
        }
    }
    return rc;
}

unsigned long long getCurrentTime()
{

    struct timespec currentTime;
    if (clock_gettime(CLOCK_REALTIME, &currentTime) == -1) {
     return 0;
    }
    return (unsigned long long)(currentTime.tv_sec);

}

int zclient_reconnect(ZohoIOTclient *client)
{
    int rc = validateClientState(client);
    int subscribe_rc = ZSUCCESS;
    if (rc != 0)
    {
        return rc;
    }

    if (client->current_state == CONNECTED)
    {
        return ZSUCCESS;
    }
    if (client->start_time == 0)
    {
        client->start_time = getCurrentTime();
    }
    int delay = client->ZretryInterval;
    if (getCurrentTime() - client->start_time > delay)
    {
        if(client->current_state != INITIALIZED)
        {
            // Lock the mutex
            log_trace("[%s] Getting client lock for network disconnect", client->config.client_id);
            pthread_mutex_lock(&client->lock);
            log_trace("[%s] Got client lock for network disconnect", client->config.client_id);
            #if defined(Z_PAHO_C)
            if(client->current_state == INITIALIZED)
            {
                MQTTClient_destroy(&client->mqtt_client);
            }
            #else
            NetworkDisconnect(client->mqtt_client.ipstack);
            #endif
            // Unlock the mutex
            log_trace("[%s] Releasing client lock for network disconnect", client->config.client_id);
            pthread_mutex_unlock(&client->lock);
            log_trace("[%s] Released client lock for network disconnect", client->config.client_id);
        }
        rc = zclient_connect(client);
        if (rc == ZSUCCESS)
        {
            client->current_state = CONNECTED;
            client->retryCount = 0;
            client->start_time = 0;
            client->ZretryInterval = MIN_RETRY_INTERVAL;
            if(client->on_command_message_handler != NULL)
            {
                subscribe_rc = zclient_command_subscribe(client, client->on_command_message_handler);
                if (subscribe_rc != ZSUCCESS)
                {
                    log_error("[%s] Command subscribe failed during reconnect. Error code: %d", client->config.client_id, subscribe_rc);
                    return ZFAILURE;
                }
            }
            if(client->on_config_message_handler != NULL)
            {
                subscribe_rc = zclient_config_subscribe(client, client->on_config_message_handler);
                if (subscribe_rc != ZSUCCESS)
                {
                    log_error("[%s] Config subscribe failed during reconnect. Error code: %d", client->config.client_id, subscribe_rc);
                    return ZFAILURE;
                }
            }
            if(client->retryACK)
            {
                log_debug("[%s] Attempting to resend the ACK message that previously failed", client->config.client_id);
                char *payload = NULL;
                payload = cJSON_Print(client->failedACK.ackPayload);
                rc = publishMessage(client, client->failedACK.topic, payload);
                if(rc == ZSUCCESS)
                {
                    log_debug("[%s]\x1b[36m Failed ACK published \x1b[0m", client->config.client_id);
                    log_trace("[%s] ACK published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", client->config.client_id, payload, client->failedACK.topic);
                    cJSON_Delete(client->failedACK.ackPayload);
                    client->retryACK = false;
                }
                else{
                    log_error("[%s] Error publishing Ack, Error code: %d", client->config.client_id, rc);
                }
                free(payload);

            }
            if(client->retryEvent)
            {
                if(client->failedEvent.eventPayloadTime + 300 < getCurrentTime())
                {
                    log_debug("[%s] Event is expired, so not attempting to resend the Event message that previously failed", client->config.client_id);
                    cJSON_Delete(client->failedEvent.eventPayload);
                    client->retryEvent = false;
                }
                else
                {
                    log_debug("[%s] Attempting to resend the Event message that previously failed", client->config.client_id);
                    char *payload = NULL;
                    payload = cJSON_Print(client->failedEvent.eventPayload);
                    rc = publishMessage(client, client->eventTopic, payload);
                    if(rc == ZSUCCESS)
                    {
                        log_debug("[%s]\x1b[36m Failed Event published \x1b[0m", client->config.client_id);
                        log_trace("[%s] Event published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", client->config.client_id, payload, client->eventTopic);
                        cJSON_Delete(client->failedEvent.eventPayload);
                        client->retryEvent = false;
                    }
                    else{
                        log_error("[%s] Error publishing Event, Error code: %d", client->config.client_id, rc);
                    }
                    free(payload);
                }
            }
            return ZSUCCESS;
        }
        client->start_time = getCurrentTime();
        client->ZretryInterval = getRetryInterval(client->ZretryInterval);
        client->retryCount++;
        log_info("[%s] retryCount :%d", client->config.client_id, client->retryCount);
        log_info("[%s] Trying to reconnect \x1b[32m %s : %d \x1b[0m in %d sec ", client->config.client_id, client->config.hostname, ZPORT, client->ZretryInterval);
        if (client->current_state != DISCONNECTED && client->current_state != CONNECTED)
        {
            log_info("[%s] Retrying indefinitely", client->config.client_id);
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
        log_debug("[%s] Can not publish, since connection is lost/not established", client->config.client_id);
        return ZFAILURE;
    }
    rc = ZFAILURE;
    rc = publishMessage(client, client->dataTopic, payload);
    //TODO: check for connection and retry to send the message once the conn got restroed.
    if (rc == ZSUCCESS)
    {
        log_debug("[%s]\x1b[36m Telemetry Message Published \x1b[0m", client->config.client_id);
        log_trace("[%s] Published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", client->config.client_id, payload, client->dataTopic);
    }
    #if defined(Z_PAHO_C)
        else if ( MQTTClient_isConnected(client->mqtt_client) == 0)
    #else
        else if (client->mqtt_client.isconnected == 0)
    #endif
    {
        client->current_state = DISCONNECTED;
        log_error("[%s] Error on Publish due to lost connection. Error code: %d", client->config.client_id, rc);
    }
    else
    {
        log_error("[%s] Error on Publish. Error code: %d", client->config.client_id, rc);
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
        log_debug("[%s] Can not dispatch, since connection is lost/not established", client->config.client_id);
        return ZFAILURE;
    }
    
    char *payload = NULL;
    payload = cJSON_Print(client->message.data);
    int status = zclient_publish(client, payload);
    free(payload);
    cJSON_Delete(client->message.data);
    client->message.data= NULL;
    return status;
    }

int zclient_addEventDataNumber(ZohoIOTclient *client, char *key, double val_number)
{
    if (client->eventDataObject == NULL)
    {
        client->eventDataObject = cJSON_CreateObject();
    }

    if (!isStringValid(key))
    {
        log_error("[%s] Key Can't be NULL", client->config.client_id);
        return -1;
    }
    int rc = ZSUCCESS;
    if (!cJSON_HasObjectItem(client->eventDataObject, key))
    {
        if (cJSON_AddNumberToObject(client->eventDataObject, key, val_number) == NULL)
        {
            log_error("[%s] Adding Number attribute failed\n", client->config.client_id);
            rc = ZFAILURE;
        }
    }
    else
    {
        cJSON_ReplaceItemInObject(client->eventDataObject, key, cJSON_CreateNumber(val_number));
    }
    return rc;
}

int zclient_addEventDataObject(ZohoIOTclient *client, char *key, cJSON* Object)
{
    if (client->eventDataObject == NULL)
    {
        client->eventDataObject = cJSON_CreateObject();
    }

    if (!isStringValid(key))
    {
        log_error("[%s] Key Can't be NULL", client->config.client_id);
        return -1;
    }
    int rc = ZSUCCESS;
    cJSON* event_object_copy = cJSON_Duplicate(Object, 1);
    if (!cJSON_HasObjectItem(client->eventDataObject, key))
    {
       cJSON_AddItemToObject(client->eventDataObject, key, event_object_copy);
    }
    else
    {
        cJSON_ReplaceItemInObject(client->eventDataObject, key,event_object_copy);
    }
    return rc;
}

int zclient_addEventDataString(ZohoIOTclient *client, char *key, char *val_string)
{
    if (client->eventDataObject == NULL)
    {
        client->eventDataObject = cJSON_CreateObject();
    }
    if (!isStringValid(key) || !isStringValid(val_string))
    {
        log_error("[%s] Key or Value Can't be NULL", client->config.client_id);
        return -1;
    }
    int rc = ZSUCCESS;
    if (!cJSON_HasObjectItem(client->eventDataObject, key))
    {
        if (cJSON_AddStringToObject(client->eventDataObject, key, val_string) == NULL)
        {
            log_error("[%s] Adding String attribute failed\n", client->config.client_id);
            rc = ZFAILURE;
        }
    }
    else
    {
        cJSON_ReplaceItemInObject(client->eventDataObject, key, cJSON_CreateString(val_string));
    }
    return rc;
}

int zclient_dispatchEventFromEventDataObject(ZohoIOTclient *client, char *eventType, char *eventDescription, char *assetName)
{
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    char *eventDataJSONString = NULL;
    eventDataJSONString = cJSON_Print(client->eventDataObject);
    rc = zclient_dispatchEventFromJSONString(client, eventType, eventDescription, eventDataJSONString, assetName);
    cJSON_Delete(client->eventDataObject);
    free(eventDataJSONString);
    client->eventDataObject = cJSON_CreateObject();
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
        log_debug("[%s] Can not dispatch, since connection is lost/not established", client->config.client_id);
        return ZFAILURE;
    }
    cJSON *dataObject = cJSON_Parse(eventDataJSONString);
    if (dataObject == NULL)
    {
        log_error("[%s] Can not dispatch Event as Event Data JSON string is not parsable.", client->config.client_id);
        return ZFAILURE;
    }
    time_t curtime;
    char *payload = NULL;
    time(&curtime);
    char *time_val = strtok(ctime(&curtime), "\n");
    cJSON *eventObject = cJSON_CreateObject();
    if (!isStringValid(eventType) || eventDescription == NULL)
    {
        log_error("[%s] Can not dispatch Event with Empty EventType or Description.", client->config.client_id);
        return ZFAILURE;
    }
    cJSON_AddStringToObject(eventObject, "event_type", eventType);
    cJSON_AddStringToObject(eventObject, "event_desc", eventDescription);
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
    rc = publishMessage(client, client->eventTopic, payload);
    
    if (rc == ZSUCCESS)
    {
        log_debug("[%s]\x1b[36m Event Message Published \x1b[0m", client->config.client_id);
        log_trace("[%s] Event dispatched \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", client->config.client_id, payload, client->eventTopic);
    }
    #if defined(Z_PAHO_C)
        else if ( MQTTClient_isConnected(client->mqtt_client) == 0)
    #else
        else if (client->mqtt_client.isconnected == 0)
    #endif
    {
        client->current_state = DISCONNECTED;
        log_error("[%s] Error on dispatchEvent due to lost connection. Error code: %d", client->config.client_id, rc);
        client->retryEvent = true;
        if(client->failedEvent.eventPayload != NULL)
        {
            cJSON_Delete(client->failedEvent.eventPayload);
        }
        client->failedEvent.eventPayload = cJSON_Duplicate(eventObject, 1);
        client->failedEvent.eventPayloadTime = getCurrentTime();
    }
    else
    {
        log_error("[%s] Error on dispatchEvent. Error code: %d", client->config.client_id, rc);
        client->retryEvent = true;
        client->failedEvent.eventPayload = cJSON_Duplicate(eventObject, 1);
        client->failedEvent.eventPayloadTime = getCurrentTime();
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
    cJSON* Ack_payload = generateProcessedACK(client, payload, status_code, responseMessage);
    if (Ack_payload == NULL) {
        log_error("[%s] Error on generating command Acknowledgement", client->config.client_id);
        return ZFAILURE;
    }
    char *command_ack_payload = NULL;
    command_ack_payload = cJSON_Print(Ack_payload);
    rc = publishMessage(client, client->commandAckTopic, command_ack_payload);
    if (rc == ZSUCCESS)
    {
        log_debug("[%s]\x1b[36m Command ACK Published \x1b[0m", client->config.client_id);
        log_trace("[%s] Command Ack published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", client->config.client_id, command_ack_payload, client->commandAckTopic);
    }
    #if defined(Z_PAHO_C)
        else if ( MQTTClient_isConnected(client->mqtt_client) == 0)
    #else
        else if (client->mqtt_client.isconnected == 0)
    #endif
    {
        client->current_state = DISCONNECTED;
        log_error("[%s] Error on publishing command ACK due to lost connection. Error code: %d", client->config.client_id, rc);
        client->retryACK = true;
        client->failedACK.ackPayload = cJSON_Duplicate(Ack_payload, 1);
        client->failedACK.topic = client->commandAckTopic;
    }
    else
    {
        log_error("[%s] Error on publishing command Ack. Error code: %d", client->config.client_id, rc);
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
    cJSON* Ack_payload = generateProcessedACK(client, payload, status_code, responseMessage);
    if (Ack_payload == NULL) {
        log_error("[%s] Error on generating config Acknowledgement", client->config.client_id);
        return ZFAILURE;
    }
    char *config_ack_payload = NULL;
    config_ack_payload = cJSON_Print(Ack_payload);
    rc = publishMessage(client, client->configAckTopic, config_ack_payload);
    if (rc == ZSUCCESS)
    {
        log_debug("[%s]\x1b[36m Config ACK Published \x1b[0m", client->config.client_id);
        log_trace("[%s] Config Ack published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", client->config.client_id, config_ack_payload, client->configAckTopic);
    }
    #if defined(Z_PAHO_C)
        else if ( MQTTClient_isConnected(client->mqtt_client) == 0)
    #else
        else if (client->mqtt_client.isconnected == 0)
    #endif
    {
        client->current_state = DISCONNECTED;
        log_error("[%s] Error on publishing config ack due to lost connection. Error code: %d", client->config.client_id, rc);
        client->retryACK = true;
        client->failedACK.ackPayload = cJSON_Duplicate(Ack_payload, 1);
        client->failedACK.topic = client->configAckTopic;
    }
    else
    {
        log_error("[%s] Error on publishing config Ack. Error code: %d", client->config.client_id, rc);
    }
    cJSON_Delete(Ack_payload);
    free(config_ack_payload);
    return rc;
}

int zclient_command_subscribe(ZohoIOTclient *client, SubscribeMessageHandler on_message)
{
    if (client == NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }

    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (on_message == NULL)
    {
        log_error("[%s] Message handler can't be NULL", client->config.client_id);
        return ZFAILURE;
    }
    setCommandMessageHandler(client, on_message);
    if (client->current_state != CONNECTED)
    {
        log_error("[%s] Can not subscribe, since connection is lost/not established", client->config.client_id);
        return ZFAILURE;
    }
    // Lock the mutex
    log_trace("[%s] Getting client lock for subscribe", client->config.client_id);
    pthread_mutex_lock(&client->lock);
    log_trace("[%s] Got client lock for subscribe", client->config.client_id);
    #if defined(Z_PAHO_C)
    rc = MQTTClient_subscribe(client->mqtt_client, client->commandTopic, 0);
    #else
    rc = MQTTSubscribe(&(client->mqtt_client), client->commandTopic, QOS0, onMessageReceived);
    #endif
    // Unlock the mutex
    log_trace("[%s] Releasing client lock for subscribe", client->config.client_id);
    pthread_mutex_unlock(&client->lock);
    log_trace("[%s] Released client lock for subscribe", client->config.client_id);
    if (rc == ZSUCCESS)
    {
        log_info("[%s] Subscribed on \x1b[36m '%s' \x1b[0m", client->config.client_id, client->commandTopic);
    }
    #if defined(Z_PAHO_C)
        else if ( MQTTClient_isConnected(client->mqtt_client) == 0)
    #else
        else if (client->mqtt_client.isconnected == 0)
    #endif
    {
        client->current_state = DISCONNECTED;
        log_error("[%s] Error on Subscribe due to lost connection. Error code: %d", client->config.client_id, rc);
    }
    else
    {
        client->current_state = DISCONNECTED;
        log_error("[%s] Error on Subscribe. Error code: %d", client->config.client_id, rc);
    }

    return rc;
}

int zclient_config_subscribe(ZohoIOTclient *client, SubscribeMessageHandler on_message)
{
    if (client == NULL)
    {
        log_error("Client object can't be NULL");
        return ZFAILURE;
    }
    int rc = validateClientState(client);
    if (rc != 0)
    {
        return rc;
    }
    if (on_message == NULL)
    {
        log_error("[%s] Message handler can't be NULL", client->config.client_id);
        return ZFAILURE;
    }
    setConfigMessageHandler(client, on_message);
    if (client->current_state != CONNECTED)
    {
        log_error("[%s] Can not subscribe, since connection is lost/not established", client->config.client_id);
        return ZFAILURE;
    }
    // Lock the mutex
    log_trace("[%s] Getting client lock for subscribe", client->config.client_id);
    pthread_mutex_lock(&client->lock);
    log_trace("[%s] Got client lock for subscribe", client->config.client_id);
    #if defined(Z_PAHO_C)
        rc = MQTTClient_subscribe(client->mqtt_client, client->configTopic, 0);
    #else
        rc = MQTTSubscribe(&(client->mqtt_client), client->configTopic, QOS0, onMessageReceived);
    #endif
    // Unlock the mutex
    log_trace("[%s] Releasing client lock for subscribe", client->config.client_id);
    pthread_mutex_unlock(&client->lock);
    log_trace("[%s] Released client lock for subscribe", client->config.client_id);
    if (rc == ZSUCCESS)
    {
        log_info("[%s] Subscribed on \x1b[36m '%s' \x1b[0m", client->config.client_id, client->configTopic);
    }
    #if defined(Z_PAHO_C)
        else if ( MQTTClient_isConnected(client->mqtt_client) == 0)
    #else
        else if (client->mqtt_client.isconnected == 0)
    #endif
    {
        client->current_state = DISCONNECTED;
        log_error("[%s] Error on Subscribe due to lost connection. Error code: %d", client->config.client_id, rc);
    }
    else
    {
        client->current_state = DISCONNECTED;
        log_error("[%s] Error on Subscribe. Error code: %d", client->config.client_id, rc);
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
    
    #if defined(Z_PAHO_C)
        if(MQTTClient_isConnected(client->mqtt_client) == 0)
        {
            client->current_state = DISCONNECTED;
            return ZFAILURE;
        }
        sleep(1);
        return 0;
    #else
        if (getCurrentTime() - client->yield_time < (unsigned long long)client->yield_interval){
            return 2;
        }
        client->yield_time = getCurrentTime();
        if (time_out <= 0)
        {
            log_error("[%s] timeout can't be Zero or Negative", client->config.client_id);
            return ZFAILURE;
        }
        // lock the mutex
        log_trace("[%s] Getting client lock for yield", client->config.client_id);
        pthread_mutex_lock(&client->lock);
        log_trace("[%s] Got client lock for yield", client->config.client_id);
        rc = MQTTYield(&client->mqtt_client, time_out);
        // Unlock the mutex
        log_trace("[%s] Releasing client lock for yield", client->config.client_id);
        pthread_mutex_unlock(&client->lock);
        log_trace("[%s] Released client lock for yield", client->config.client_id);
        if (rc == ZSUCCESS)
        {
            return rc;
        }
        else if (rc == ZFAILURE)
        {
            if (client->mqtt_client.isconnected == 0)
            {
                client->current_state = DISCONNECTED;
                return ZFAILURE;
            }
        }
        else
        {
            log_error("[%s] Error on Yield. Error code: %d", client->config.client_id, rc);
            return rc;
        }
        return rc;
    #endif
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
        log_trace("[%s] Getting client lock for mqtt disconnect", client->config.client_id);
        pthread_mutex_lock(&client->lock);
        log_trace("[%s] Got client lock for mqtt disconnect", client->config.client_id);
        #if defined(Z_PAHO_C)
            rc = MQTTClient_disconnect(client->mqtt_client,10000);
        #else
            rc = MQTTDisconnect(&client->mqtt_client);
        #endif
        // Unlock the mutex
        log_trace("[%s] Releasing client lock for mqtt disconnect", client->config.client_id);
        pthread_mutex_unlock(&client->lock);
        log_trace("[%s] Released client lock for mqtt disconnect", client->config.client_id);
    }
    if(client->current_state != INITIALIZED)
    {
        // lock the mutex
        log_trace("[%s] Getting client lock for network disconnect", client->config.client_id);
        pthread_mutex_lock(&client->lock);
        log_trace("[%s] Got client lock for network disconnect", client->config.client_id);
    	#if defined(Z_PAHO_C)
            MQTTClient_destroy(&client->mqtt_client);
        #else
            NetworkDisconnect(client->mqtt_client.ipstack);
        #endif
        // Unlock the mutex
        log_trace("[%s] Releasing client lock for network disconnect", client->config.client_id);
        pthread_mutex_unlock(&client->lock);
        log_trace("[%s] Released client lock for network disconnect", client->config.client_id);
    }
    client->current_state = DISCONNECTED;
    log_info("[%s] Disconnected.", client->config.client_id);
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
        cJSON *current = client->message.data;
        char *assetName_dup = strdup(assetName);
        char *token = strtok(assetName_dup, ".");
        while (token != NULL)
        {
            if (!cJSON_HasObjectItem(current, token))
            {
                cJSON *new_obj = cJSON_CreateObject();
                cJSON_AddItemToObject(current, token, new_obj);
            }
            current = cJSON_GetObjectItem(current, token);
            token = strtok(NULL, ".");
        }
        free(assetName_dup); 
        return current;
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
            log_error("[%s] Adding int attribute failed\n", client->config.client_id);
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
            log_error("[%s] Adding string attribute failed\n", client->config.client_id);
            rc = ZFAILURE;
        }
    }
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
cJSON* zclient_FormReceivedACK(ZohoIOTclient *client, char* payload)
{
    return generateACKPayload(client, payload, RECIEVED_ACK_CODE, "");
}
cJSON* generateProcessedACK(ZohoIOTclient *client, char* payload, ZcommandAckResponseCodes status_code, char *responseMessage)
{
    if (responseMessage == NULL)
    {
        log_error("[%s] Response cannot be Null in Ack object", client->config.client_id);
       return NULL;
    }
    
    switch (status_code) {
        case SUCCESSFULLY_EXECUTED:
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
            log_error("[%s] Status code provided is not a valid in Ack object", client->config.client_id);
            return NULL;
    }
    return generateACKPayload(client, payload, status_code, responseMessage);
}

cJSON* generateACKPayload(ZohoIOTclient *client, char* payload, ZcommandAckResponseCodes status_code, char *responseMessage) {

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
                    client->OTA_RECEIVED = true;
                }
                else
                {
                    client->OTA_RECEIVED = false;
                }

                //check if the command is for cloud logging
                if(strcmp(command_name,"Z_PUBLISH_DEVICE_LOGS")== 0)
                {
                    client->CLOUD_LOGGING = true;
                    #if defined(Z_HTTP_PUBLISH_ENABLE)
                        cJSON *payload_array = cJSON_GetObjectItem(commandMessage, "payload");
                        cJSON *payload_json = cJSON_GetArrayItem(payload_array, 0);
                        char *value = cJSON_GetObjectItem(payload_json, "value")->valuestring;
                        cloud_logging_set_lines(value);
                    #endif
                }
                else
                {
                    client->CLOUD_LOGGING = false;
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
    #if defined(Z_PAHO_C)
        MQTTClient_destroy(&client->mqtt_client); 
    #endif
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
    #ifndef Z_USE_CLIENT_CERTS
        free(client->config.auth_token);
    #endif
    free(client->config.MqttUserName);
    free(client->config.mqttBuff);
    free(client->config.mqttReadBuff);
    pthread_mutex_destroy(&client->lock);
    if(client->message.data != NULL)
    {
        cJSON_Delete(client->message.data);
    }
    if(client->eventDataObject != NULL)
    {
        cJSON_Delete(client->eventDataObject);
    }
    if(client->failedACK.ackPayload != NULL)
    {
        cJSON_Delete(client->failedACK.ackPayload);
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

#if defined(Z_HTTP_PUBLISH_ENABLE)
bool parse_http_response(const char* str) {
    const char* start = strchr(str, '{');
    log_debug("HTTP Server Response : %s\n", start);
    if (start == NULL) {
        log_error("Error: Starting brace not found\n");
        return false;
    }
    const char* end = str + strlen(str) - 1;
    while (end > start) {
        if (*end == '}') {
            break;
        }
        end--;
    }

    if (end <= start) {
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
        log_error("Http Error Message: %s\n", cJSON_GetObjectItem(json, "message")->valuestring);
        cJSON_Delete(json);
        return false;
    }
    log_debug("Http Success Message: %s\n", cJSON_GetObjectItem(json, "message")->valuestring);
    cJSON_Delete(json);
    return true;
}

int http_post(ZohoIOTclient *client, char * publishPayload, char * request_url,char * responseMessage){

    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;
    BIO *bio = NULL;

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
    int url_len = strlen(request_url);
    int len = strlen(publishPayload);

    if(BIO_write(bio, request_url, url_len) <= 0) {
        log_error( "Error sending POST request\n");
        strcpy(responseMessage, "Error sending POST request");
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return ZFAILURE;
    }

    if (BIO_write(bio, publishPayload, len) <= 0) {
        log_error( "Error sending data\n");
        strcpy(responseMessage, "Error sending data");
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return ZFAILURE;
    }
    log_debug("Http_data_writed");

    // Read response
    char buf[100000]={0};
    int bytes_read;
    while ((bytes_read = BIO_read(bio, buf, sizeof(buf))) > 0) {
    }
    BIO_free_all(bio);
    SSL_CTX_free(ctx);

     if(parse_http_response(buf)== false){
        log_debug("The respose is %s",buf);
        strcpy(responseMessage, "Error in parsing the http response");
        return ZFAILURE;
    }
    log_info("Http published successfully");
    char successResponse[150];
    sprintf(successResponse,"Http publish successful");
    strcpy(responseMessage, successResponse);
    return ZSUCCESS;

}

int http_post_cloud_logging(ZohoIOTclient *client, char *payload,char * responseMessage){

    #if defined(Z_USE_CLIENT_CERTS)
    if(TLS_CLIENT_CERTS){
        log_error("[%s] Cloud Logging not supported in Authentication Type -> Client Certificate with TLS", client->config.client_id);
        strcpy(responseMessage, "Cloud Logging not supported in Authentication Type -> Client Certificate with TLS");
        return ZFAILURE;
    }
    #endif
    
    cJSON * cloud_log = get_cloud_log();
    if(cloud_log == NULL){
        log_error("[%s] Error in fetching the log from the file", client->config.client_id);
        strcpy(responseMessage, "Error in fetching the log from the file");
        return ZFAILURE;
    }
    char *cloud_log_string = cJSON_Print(cloud_log);
    cJSON_Delete(cloud_log);
    
    if (cloud_log_string == NULL){
        log_error("[%s] Error in parsing cloudlog array to string", client->config.client_id);
        strcpy(responseMessage, "Error in parsing cloudlog array to string");
        return ZFAILURE;
    }
    int cloud_log_len = strlen(cloud_log_string);
    log_debug("cloud_data: log fetched successfully from the file");
    log_info("Size of Cloud Log data is %f KB\n", cloud_log_len/1000.0);

    char * hostname = client->config.hostname;
    char * client_id = client->config.client_id;
    char * password = client->config.auth_token;
    char request_url[1000];
    snprintf(request_url,sizeof(request_url),"POST /v1/iot/logs/import?device_id=%s&device_token=%s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n", client_id, password, hostname, cloud_log_len);

    if(http_post(client,cloud_log_string,request_url,responseMessage) == ZSUCCESS){
    char successResponse[150];
    log_info("cloud log published successfully");
    sprintf(successResponse,"Log Published Successfully. Number of lines read - %d, Size of log read - %ld KB",numberOfLinesRead,sizeOfLogRead);
    strcpy(responseMessage, successResponse);
    free(cloud_log_string);
    return ZSUCCESS;
    }
    free(cloud_log_string);
    return ZFAILURE;
}


OfflinePublishResponse* publishOfflineData(ZohoIOTclient *client,char *payload){
    OfflinePublishResponse *response = (OfflinePublishResponse *)malloc(sizeof(OfflinePublishResponse));
    response->responseMessage = malloc(500);
    response->status = ZFAILURE;
    
    #if defined(Z_USE_CLIENT_CERTS)
    if(TLS_CLIENT_CERTS){
        log_error("[%s] publishofflineData not supported in Authentication Type -> Client Certificate with TLS", client->config.client_id);
        strcpy(response->responseMessage, "publishofflineData not supported in Authentication Type -> Client Certificate with TLS");
        return response;
    }
    #endif
    if(strlen(payload)>MAX_OFFLINE_DATA_SIZE){
        log_error("[%s] Offline data size exceeds 1 MB", client->config.client_id);
        strcpy(response->responseMessage, "Offline data size exceeds 1 MB");
        return response;
    }
    char * hostname = client->config.hostname;
    char * client_id = client->config.client_id;
    char * password = client->config.auth_token;
    char request_url[1000];
    int ret = snprintf(request_url, sizeof(request_url),
                       "POST /v1/iot/telemetry/import?device_id=%s&device_token=%s&allow_partial_import=true HTTP/1.1\r\n"
                       "Host: %s\r\n"
                       "Content-Type: application/json\r\n"
                       "Content-Length: %zu\r\n\r\n",
                       client_id, password, hostname, strlen(payload));
   
    response->status =  http_post(client,payload,request_url,response->responseMessage);
    return response;

}
#endif

void handle_cloud_logging(ZohoIOTclient *client, char *payload){
    client->cloud_logging_in_processing = true;
    int command_response_code;
    char responseMessage[100];
    #if defined(Z_HTTP_PUBLISH_ENABLE)
        if(http_post_cloud_logging(client, payload,responseMessage) == ZSUCCESS){
            command_response_code = SUCCESSFULLY_EXECUTED;
        }
        else{
            command_response_code = EXECUTION_FAILURE;
        }
    #else
        command_response_code = EXECUTION_FAILURE;
        strcpy(responseMessage, "Http Publish is not enabled for cloud logging");
        log_error("[%s] Http Publish is not enabled for cloud logging", client->config.client_id);
    #endif
    zclient_publishCommandAck(client,payload,command_response_code,responseMessage);
    client->cloud_logging_in_processing = false;
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
        if(client->on_OTA_handler == NULL)
        {
            log_error("[%s] OTA Handler is not set", client->config.client_id);
            zclient_publishOTAAck(client,correlation_id,EXECUTION_FAILURE,"OTA Handler is not set");
            cJSON_Delete(commandMessageArray);
            return;
        }

        //check if the OTA URL is valid
        if(OTA_URL == NULL)
        {
            log_error("[%s] OTA URL is NULL", client->config.client_id);
            zclient_publishOTAAck(client,correlation_id,EXECUTION_FAILURE,"OTA URL is NULL");
            cJSON_Delete(commandMessageArray);
            return;
        }

        //check whether to validate the hash or not
        if(strcmp(edge_command_key,"YES") == 0)
        {
            validity_check = true;
            //check if the hash is valid
            if(hash == NULL)
            {
                log_error("[%s] Hash is NULL", client->config.client_id);
                zclient_publishOTAAck(client,correlation_id,EXECUTION_FAILURE,"Hash is NULL");
                cJSON_Delete(commandMessageArray);
                return;
            }
        }

        //call the user defined OTA handler
        client->on_OTA_handler(OTA_URL,hash,validity_check,correlation_id);
    }
    cJSON_Delete(commandMessageArray);
}

//set the OTA handler
int zclient_ota_handler(ZohoIOTclient *client, OTAHandler on_OTA)
{
    if(on_OTA == NULL)
    {
        log_error("[%s] OTA Handler can't be NULL", client->config.client_id);
        return ZFAILURE;
    }
    client->on_OTA_handler = on_OTA;
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
    rc = publishMessage(client, client->commandAckTopic, command_ack_payload);
    if (rc == ZSUCCESS)
    {
        log_debug("[%s]\x1b[36m OTA ACK Published \x1b[0m", client->config.client_id);
        log_trace("[%s] OTA Ack published \x1b[32m '%s' \x1b[0m on \x1b[36m '%s' \x1b[0m", client->config.client_id, command_ack_payload, client->commandAckTopic);
    }
    #if defined(Z_PAHO_C)
        else if ( MQTTClient_isConnected(client->mqtt_client) == 0)
    #else
        else if (client->mqtt_client.isconnected == 0)
    #endif
    {
        client->current_state = DISCONNECTED;
        log_error("[%s] Error on publishing OTA ACK due to lost connection. Error code: %d", client->config.client_id, rc);
        client->retryACK = true;
        client->failedACK.ackPayload = cJSON_Duplicate(Ack_payload, 1);
        client->failedACK.topic = client->commandAckTopic;
    }
    else
    {
        log_error("[%s] Error on publishing OTA Ack. Error code: %d", client->config.client_id, rc);
    }
    cJSON_Delete(Ack_payload);
    free(command_ack_payload);
    return rc;

}

int publishMessage(ZohoIOTclient *client, const char *topic, char *payload)
{
    #if defined(Z_PAHO_C)
        int rc;
        MQTTClient_message pubmsg = MQTTClient_message_initializer;
        pubmsg.msgid = rand()%10000;
        pubmsg.qos = 1;
        pubmsg.dup = 0;
        pubmsg.retained = 0;
        pubmsg.payload = payload;
        pubmsg.payloadlen = strlen(payload);
        MQTTClient_deliveryToken token;
        // Lock the mutex
        log_trace("[%s] Getting client lock for publish", client->config.client_id);
        pthread_mutex_lock(&client->lock);
        log_trace("[%s] Got client lock for publish", client->config.client_id);
        rc = MQTTClient_publishMessage((client->mqtt_client), topic, &pubmsg,&token);
        if(rc == ZSUCCESS)
        {
            rc = MQTTClient_waitForCompletion((client->mqtt_client), token, 10000);
        }
        // Unlock the mutex
        log_trace("[%s] Releasing client lock for publish", client->config.client_id);
        pthread_mutex_unlock(&client->lock);
        log_trace("[%s] Released client lock for publish", client->config.client_id);
        return rc;
    #else
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
            log_error("[%s] Error on Pubish,payload \x1b[31m(%d)\x1b[0m size is greater than client max payload size \x1b[31m(%d)\x1b[0m", client->config.client_id, pubmsg.payloadlen,client->config.payload_size);
            return ZFAILURE;
        }
        // Lock the mutex
        log_trace("[%s] Getting client lock for publish", client->config.client_id);
        pthread_mutex_lock(&client->lock);
        log_trace("[%s] Got client lock for publish", client->config.client_id);
        
        // Publish the MQTT message
        rc = MQTTPublish(&(client->mqtt_client), topic, &pubmsg);

        // Unlock the mutex
        log_trace("[%s] Releasing client lock for publish", client->config.client_id);
        pthread_mutex_unlock(&client->lock);
        log_trace("[%s] Released client lock for publish", client->config.client_id);

        return rc;
    #endif
}
