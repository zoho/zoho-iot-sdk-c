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

void processMessageReceived(MessageData *md);
void initMessageHandler(ZohoIOTclient *client, char *comm_topic, char *comm_ack_topic, char *conf_topic, char *conf_ack_topic)
{
    iot_client = client;
    strcpy(handler_COMMAND_TOPIC, comm_topic);
    strcpy(handler_COMMAND_ACK_TOPIC, comm_ack_topic);
    strcpy(handler_CONFIG_TOPIC, conf_topic);
    strcpy(handler_CONFIG_ACK_TOPIC, conf_ack_topic);
}

void setCommandMessageHandler(messageHandler message_handler)
{
    on_command_message_handler = message_handler;
}
void setConfigMessageHandler(messageHandler message_handler)
{
    on_config_message_handler = message_handler;
}

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

    md_copy->message->payload = malloc(md->message->payloadlen);
    if (md_copy->message->payload == NULL) {
        log_error("Failed to allocate memory for payload");
        free(md_copy->message);
        free(md_copy);
        return;
    }
    memcpy(md_copy->message->payload, md->message->payload, md->message->payloadlen);

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
    char payload[message->payloadlen+1];
    char topic[md->topicName->lenstring.len+1];
    memset(topic, '\0', sizeof(topic));
    memset(payload, '\0', sizeof(payload));
    memcpy(topic, md->topicName->lenstring.data, md->topicName->lenstring.len);
    topic[md->topicName->lenstring.len] = '\0';
    memcpy(payload, message->payload, message->payloadlen);
    payload[message->payloadlen] = '\0';

    if (strcmp(topic, handler_COMMAND_TOPIC) == 0)
    {
        
        cJSON* commandAckObject = zclient_FormReceivedACK(payload);
        if (commandAckObject != NULL) {
            char *command_ack_payload = cJSON_Print(commandAckObject);
            PublishPayloadData* data = (PublishPayloadData*)malloc(sizeof(PublishPayloadData));
            data->client = iot_client;
            data->topic = strdup(handler_COMMAND_ACK_TOPIC);
            data->payload = command_ack_payload;
            pthread_t thread;
            int result = pthread_create(&thread, NULL, publishCommandAck, (void*)data);
            if (result != 0) {
                log_error("Failed to create command publish thread");
                free(command_ack_payload);
                free(data);
            }
            pthread_detach(thread);
            cJSON_Delete(commandAckObject);
            if(get_OTA_status())
            {
                //handle OTA
                log_info("Received OTA command. Handling OTA...");
                handle_OTA(iot_client,payload);
                return;
            }
            if(get_cloud_logging_status())
            {
                //handle Cloud logging
                log_info("Received cloud logging command. Handling cloud logging...");
                handle_cloud_logging(iot_client,payload);
                return;
            }
            on_command_message_handler(md);
        }
    }
    else if (strcmp(topic, handler_CONFIG_TOPIC) == 0)
    {
        cJSON* configAckObject = zclient_FormReceivedACK(payload);
        if (configAckObject != NULL) {
            char *config_ack_payload = cJSON_Print(configAckObject);
            PublishPayloadData* data = (PublishPayloadData*)malloc(sizeof(PublishPayloadData));
            data->client = iot_client;
            data->topic = strdup(handler_CONFIG_ACK_TOPIC);
            data->payload = config_ack_payload;
            pthread_t thread;
            int result = pthread_create(&thread, NULL, publishConfigAck, (void*)data);
            if (result != 0) {
                log_error("Failed to create config publish thread");
                free(config_ack_payload);  
                free(data);
            }
            pthread_detach(thread); 
            cJSON_Delete(configAckObject);
            on_config_message_handler(md);
        }
    }

}