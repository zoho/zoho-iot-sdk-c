#include "zoho_message_handler.h"

void initMessageHandler(ZohoIOTclient *client, char *comm_topic, char *comm_ack_topic, char *conf_topic, char *conf_ack_topic)
{
    iot_client = client;
    strcpy(handler_command_topic, comm_topic);
    strcpy(handler_command_ack_topic, comm_ack_topic);
    strcpy(handler_config_topic, conf_topic);
    strcpy(handler_config_ack_topic, conf_ack_topic);
}

void setCommandMessageHandler(messageHandler message_handler)
{
    on_command_message_handler = message_handler;
}
void setConfigMessageHandler(messageHandler message_handler)
{
    on_config_message_handler = message_handler;
}

void onMessageReceived(MessageData *md)
{
    MQTTMessage *message = md->message;
    char payload[md->message->payloadlen];
    char topic[md->topicName->lenstring.len];
    memset(topic, '\0', sizeof(topic));
    memset(payload, '\0', sizeof(payload));
    strncat(topic, md->topicName->lenstring.data, md->topicName->lenstring.len);
    strncat(payload, md->message->payload, md->message->payloadlen);

    if (strcmp(topic, handler_command_topic) == 0)
    {
        
        cJSON* commandAckObject = zclient_FormReceivedACK(payload);
        if (commandAckObject != NULL) {
            MQTTMessage pubmsg;
            pubmsg.id = rand()%10000;
            pubmsg.qos = 1;
            pubmsg.dup = '0';
            pubmsg.retained = '0';
            pubmsg.payload = cJSON_Print(commandAckObject);
            pubmsg.payloadlen = strlen(pubmsg.payload);
            MQTTPublish(&(iot_client->mqtt_client), handler_command_ack_topic, &pubmsg);
            cJSON_Delete(commandAckObject);
            on_command_message_handler(md);
        }
    }
    else if (strcmp(topic, handler_config_topic) == 0)
    {
        cJSON* configAckObject = zclient_FormReceivedACK(payload);
        if (configAckObject != NULL) {
            MQTTMessage pubmsg;
            pubmsg.id = rand()%10000;
            pubmsg.qos = 1;
            pubmsg.dup = '0';
            pubmsg.retained = '0';
            pubmsg.payload = cJSON_Print(configAckObject);
            pubmsg.payloadlen = strlen(pubmsg.payload);
            MQTTPublish(&(iot_client->mqtt_client), handler_config_ack_topic, &pubmsg);
            cJSON_Delete(configAckObject);
            on_config_message_handler(md);
        }
    }

}