#ifndef ZOHO_MESSAGE_HANDLER_H_
#define ZOHO_MESSAGE_HANDLER_H_

#include "zoho_iot_client.h"

ZohoIOTclient *iot_client;
char handler_COMMAND_TOPIC[100], handler_COMMAND_ACK_TOPIC[100],handler_CONFIG_TOPIC[100], handler_CONFIG_ACK_TOPIC[100];
SubscribeMessageHandler on_command_message_handler;
SubscribeMessageHandler on_config_message_handler;
void initMessageHandler(ZohoIOTclient *client, char *commandTopic, char *commandAckTopic,char *configTopic,char *configAckTopic);
void setCommandMessageHandler(SubscribeMessageHandler message_handler);
void setConfigMessageHandler(SubscribeMessageHandler message_handler);
#if defined(Z_PAHO_C)
int onMessageReceived(void *context, char *topicName, int topicLen, MQTTClient_message *message);
#else
void onMessageReceived(MessageData *md);
#endif

#endif //# ZOHO_MESSAGE_HANDLER_H_