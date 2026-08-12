#include "zoho_message_handler.h"
#include <pthread.h>
#include <stdlib.h>
typedef struct {
    ZohoIOTclient* client;
    char* topic;
    char* payload;
} PublishPayloadData;

void* publishCommandAck(void* arg) {
    PublishPayloadData* data = (PublishPayloadData*)arg;
    publishMessage(data->client, data->topic, data->payload);
    free(data->payload);
    free(data->topic);
    free(data);
    return NULL;
}

void* publishConfigAck(void* arg) {
    PublishPayloadData* data = (PublishPayloadData*)arg;
    publishMessage(data->client, data->topic, data->payload);
    free(data->payload);
    free(data->topic);
    free(data);
    return NULL;
}

struct SubscribeThreadArgs {
    ZohoIOTclient *client;
    char* topic;
    char* payload;
};

#define MAX_CLIENT_REGISTRY 16
static ZohoIOTclient *client_registry[MAX_CLIENT_REGISTRY];
static int client_registry_count = 0;

static ZohoIOTclient *findClientByTopic(const char *topic)
{
    for (int i = 0; i < client_registry_count; i++) {
        ZohoIOTclient *c = client_registry[i];
        if (strcmp(topic, c->commandTopic) == 0 || strcmp(topic, c->configTopic) == 0)
            return c;
    }
    return NULL;
}

#ifndef Z_PAHO_C
void processMessageReceived(MessageData *md);
#endif
void initMessageHandler(ZohoIOTclient *client)
{
    for (int i = 0; i < client_registry_count; i++) {
        if (client_registry[i] == client) return;
    }
    if (client_registry_count < MAX_CLIENT_REGISTRY)
        client_registry[client_registry_count++] = client;
}

void setCommandMessageHandler(ZohoIOTclient *client, SubscribeMessageHandler message_handler)
{
    client->on_command_message_handler = message_handler;
}
void setConfigMessageHandler(ZohoIOTclient *client, SubscribeMessageHandler message_handler)
{
    client->on_config_message_handler = message_handler;
}


#if defined(Z_PAHO_C)
void* subscribe_ack_function(void* arg) {
    struct SubscribeThreadArgs* args = (struct SubscribeThreadArgs*)arg;
    ZohoIOTclient *iot_client = args->client;
    log_debug("[%s] Thread received strings: %s and %s\n", iot_client->config.client_id, args->topic, args->payload);

    if (strcmp(args->topic, iot_client->commandTopic) == 0)
    {

        cJSON* commandAckObject = zclient_FormReceivedACK(iot_client, (char*)args->payload);
        if (commandAckObject != NULL) {
            char *command_ack_payload = cJSON_Print(commandAckObject);
            PublishPayloadData* data = (PublishPayloadData*)malloc(sizeof(PublishPayloadData));
            data->client = iot_client;
            data->topic = strdup(iot_client->commandAckTopic);
            data->payload = command_ack_payload;
            pthread_t thread;
            int result = pthread_create(&thread, NULL, publishCommandAck, (void*)data);
            if (result != 0) {
                log_error("[%s] Failed to create command publish thread", iot_client->config.client_id);
                free(command_ack_payload);
                free(data);
            }
            else{
                pthread_detach(thread);
            }
            cJSON_Delete(commandAckObject);
            if(get_OTA_status(iot_client))
            {
                //handle OTA
                log_info("[%s] Received OTA command. Handling OTA...", iot_client->config.client_id);
                handle_OTA(iot_client, args->payload);
                return NULL;
            }
            if(get_cloud_logging_status(iot_client))
            {
                //handle Cloud logging
                log_info("[%s] Received cloud logging command. Handling cloud logging...", iot_client->config.client_id);
                handle_cloud_logging(iot_client, args->payload);
                return NULL;
            }
            if (iot_client->on_command_message_handler != NULL) {
                iot_client->on_command_message_handler(args->topic, args->payload);
            } else {
                log_error("[%s] Command handler not available", iot_client->config.client_id);
            }
        }
    }
    else if (strcmp(args->topic, iot_client->configTopic) == 0)
    {
        cJSON* configAckObject = zclient_FormReceivedACK(iot_client, (char*)args->payload);
        if (configAckObject != NULL) {
            char *config_ack_payload = cJSON_Print(configAckObject);
            PublishPayloadData* data = (PublishPayloadData*)malloc(sizeof(PublishPayloadData));
            data->client = iot_client;
            data->topic = strdup(iot_client->configAckTopic);
            data->payload = config_ack_payload;
            pthread_t thread;
            int result = pthread_create(&thread, NULL, publishConfigAck, (void*)data);
            if (result != 0) {
                log_error("[%s] Failed to create config publish thread", iot_client->config.client_id);
                free(config_ack_payload);
                free(data);
            } else {
                pthread_detach(thread);
            }
            cJSON_Delete(configAckObject);
            if (iot_client->on_config_message_handler != NULL) {
                iot_client->on_config_message_handler(args->topic, args->payload);
            }
            else{
                log_error("[%s] Config handler not available", iot_client->config.client_id);
            }
        }
    }
    free(args->topic);
    free(args->payload);
    free(args);

    pthread_exit(NULL);
    return 0;
}


int onMessageReceived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    ZohoIOTclient *client = (ZohoIOTclient *)context;
    pthread_t thread;
    struct SubscribeThreadArgs* args = malloc(sizeof(struct SubscribeThreadArgs));
    args->client = client;
    int tlen = (topicLen > 0) ? topicLen : (int)strlen(topicName);
    args->topic = (char *)malloc(tlen + 1);
    memcpy(args->topic, topicName, tlen);
    args->topic[tlen] = '\0';
    int plen = message->payloadlen;
    args->payload = (char *)malloc(plen + 1);
    memcpy(args->payload, message->payload, plen);
    args->payload[plen] = '\0';
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    pthread_create(&thread, NULL, subscribe_ack_function, args);
    pthread_detach(thread);
    return 1;
}
#else
void* onMessageReceivedThread(void* arg) {
    MessageData* md = (MessageData*)arg;
    processMessageReceived(md);
    free(md->message->payload);
    free(md->message);
    free(md->topicName->lenstring.data);
    free(md->topicName);
    free(md);
    return NULL;
}
void onMessageReceived(MessageData *md)
{

    MessageData* md_copy = (MessageData*)malloc(sizeof(MessageData));
    if (md_copy == NULL) {
        log_error("Failed to create copy command");
        return;
    }


    md_copy->message = (MQTTMessage*)malloc(sizeof(MQTTMessage));
    if (md_copy->message == NULL) {
        log_error("Failed to allocate memory for message");
        free(md_copy);
        return;
    }
    *md_copy->message = *md->message;

    md_copy->message->payload = malloc(md->message->payloadlen + 1);
    if (md_copy->message->payload == NULL) {
        log_error("Failed to allocate memory for payload");
        free(md_copy->message);
        free(md_copy);
        return;
    }
    memcpy(md_copy->message->payload, md->message->payload, md->message->payloadlen);
    ((char *)md_copy->message->payload)[md->message->payloadlen] = '\0';

    md_copy->topicName = (MQTTString*)malloc(sizeof(MQTTString));
    if (md_copy->topicName == NULL) {
        log_error("Failed to allocate memory for topicName");
        free(md_copy->message->payload);
        free(md_copy->message);
        free(md_copy);
        return;
    }
    *md_copy->topicName = *md->topicName;

    md_copy->topicName->lenstring.data = (char*)malloc(md->topicName->lenstring.len);
    if (md_copy->topicName->lenstring.data == NULL) {
        log_error("Failed to allocate memory for topicName data");
        free(md_copy->topicName);
        free(md_copy->message->payload);
        free(md_copy->message);
        free(md_copy);
        return;
    }
    memcpy(md_copy->topicName->lenstring.data, md->topicName->lenstring.data, md->topicName->lenstring.len);

    pthread_t thread;
    int result = pthread_create(&thread, NULL, onMessageReceivedThread, (void*)md_copy);
    if (result != 0) {
        log_error("Command thread creation failed");
        free(md_copy->topicName->lenstring.data);
        free(md_copy->topicName);
        free(md_copy->message->payload);
        free(md_copy->message);
        free(md_copy);
        return;
    }

    pthread_detach(thread); 
}
void processMessageReceived(MessageData *md)
{
    MQTTMessage *message = md->message;

    size_t plen = message->payloadlen;
    size_t tlen = md->topicName->lenstring.len;

    char *payload = malloc(plen + 1);
    if (payload == NULL) {
        log_error("Failed to allocate payload buffer");
        return;
    }
    memcpy(payload, message->payload, plen);
    payload[plen] = '\0';

    char *topic = malloc(tlen + 1);
    if (topic == NULL) {
        log_error("Failed to allocate topic buffer");
        free(payload);
        return;
    }
    memcpy(topic, md->topicName->lenstring.data, tlen);
    topic[tlen] = '\0';

    ZohoIOTclient *iot_client = findClientByTopic(topic);
    if (iot_client == NULL) {
        log_error("No registered client found for topic: %s", topic);
        free(topic);
        free(payload);
        return;
    }

    if (strcmp(topic, iot_client->commandTopic) == 0)
    {
        
        cJSON* commandAckObject = zclient_FormReceivedACK(iot_client, payload);
        if (commandAckObject != NULL) {
            char *command_ack_payload = cJSON_Print(commandAckObject);
            PublishPayloadData* data = (PublishPayloadData*)malloc(sizeof(PublishPayloadData));
            data->client = iot_client;
            data->topic = strdup(iot_client->commandAckTopic);
            data->payload = command_ack_payload;
            pthread_t thread;
            int result = pthread_create(&thread, NULL, publishCommandAck, (void*)data);
            if (result != 0) {
                log_error("[%s] Failed to create command publish thread", iot_client->config.client_id);
                free(command_ack_payload);
                free(data);
            }
            else{
                pthread_detach(thread);
            }
            
            cJSON_Delete(commandAckObject);
            if(get_OTA_status(iot_client))
            {
                //handle OTA
                log_info("[%s] Received OTA command. Handling OTA...", iot_client->config.client_id);
                handle_OTA(iot_client,payload);
                free(topic);
                free(payload);
                return;
            }
            if(get_cloud_logging_status(iot_client))
            {
                //handle Cloud logging
                log_info("[%s] Received cloud logging command. Handling cloud logging...", iot_client->config.client_id);
                handle_cloud_logging(iot_client,payload);
                free(topic);
                free(payload);
                return;
            }
            if (iot_client->on_command_message_handler != NULL) {
                iot_client->on_command_message_handler(topic, payload);
            } else {
                log_error("[%s] Command handler not available", iot_client->config.client_id);
            }
        }
    }
    else if (strcmp(topic, iot_client->configTopic) == 0)
    {
        cJSON* configAckObject = zclient_FormReceivedACK(iot_client, payload);
        if (configAckObject != NULL) {
            char *config_ack_payload = cJSON_Print(configAckObject);
            PublishPayloadData* data = (PublishPayloadData*)malloc(sizeof(PublishPayloadData));
            data->client = iot_client;
            data->topic = strdup(iot_client->configAckTopic);
            data->payload = config_ack_payload;
            pthread_t thread;
            int result = pthread_create(&thread, NULL, publishConfigAck, (void*)data);
            if (result != 0) {
                log_error("[%s] Failed to create config publish thread", iot_client->config.client_id);
                free(config_ack_payload);
                free(data);
            } else {
                pthread_detach(thread);
            }
            cJSON_Delete(configAckObject);
            if (iot_client->on_config_message_handler != NULL) {
                iot_client->on_config_message_handler(topic, payload);
            }
            else{
                log_error("[%s] Command handler not available", iot_client->config.client_id);
            }
        }
    }

    free(topic);
    free(payload);
}
#endif