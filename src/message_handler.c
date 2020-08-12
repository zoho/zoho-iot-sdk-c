#include "message_handler.h"

void initMessageHandler(IOTclient *client, char *comm_topic, char *comm_ack_topic)
{
    iot_client = client;
    strcpy(handler_command_topic, comm_topic);
    strcpy(handler_command_ack_topic, comm_ack_topic);
}

void setMessageHandler(messageHandler message_handler)
{
    on_message_handler = message_handler;
}

void onMessageReceived(MessageData *md)
{
    MQTTMessage *message = md->message;
    char payload[md->message->payloadlen];
    char topic[md->topicName->lenstring.len];
    strncat(topic, md->topicName->lenstring.data, md->topicName->lenstring.len);
    strncat(payload, md->message->payload, md->message->payloadlen);
    if (strcmp(topic, handler_command_topic) == 0)
    {
        cJSON *commandMessageArray = cJSON_Parse(payload);
        if (cJSON_IsArray(commandMessageArray) == 1)
        {
            cJSON *commandAckObject = cJSON_CreateObject();
            cJSON *commandMessage, *commandAckObj;
            int len = cJSON_GetArraySize(commandMessageArray);
            for (int iter = 0; iter < len; iter++)
            {
                commandMessage = cJSON_GetArrayItem(commandMessageArray, iter);
                char *correlation_id = cJSON_GetObjectItem(commandMessage, "correlation_id")->valuestring;
                commandAckObj = cJSON_CreateObject();
                cJSON_AddItemToObject(commandAckObject, correlation_id, commandAckObj);
                cJSON_AddNumberToObject(commandAckObj, "status_code", COMMAND_RECIEVED_ACK_CODE);
                cJSON_AddStringToObject(commandAckObj, "response", "");
            }
            MQTTMessage pubmsg;
            pubmsg.id = 1234;
            pubmsg.qos = 1;
            pubmsg.dup = '0';
            pubmsg.retained = '0';
            pubmsg.payload = cJSON_Print(commandAckObject);
            pubmsg.payloadlen = strlen(pubmsg.payload);
            MQTTPublish(&(iot_client->mqtt_client), handler_command_ack_topic, &pubmsg);
            cJSON_Delete(commandMessageArray);
            cJSON_Delete(commandAckObject);
        }
    }
    on_message_handler(md);
}