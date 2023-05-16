#ifndef ZOHO_MESSAGE_HANDLER_H_
#define ZOHO_MESSAGE_HANDLER_H_

#include "zoho_iot_client.h"

ZohoIOTclient *iot_client;
char handler_COMMAND_TOPIC[100], handler_COMMAND_ACK_TOPIC[100],handler_CONFIG_TOPIC[100], handler_CONFIG_ACK_TOPIC[100];
messageHandler on_command_message_handler;
messageHandler on_config_message_handler;
void initMessageHandler(ZohoIOTclient *client, char *commandTopic, char *commandAckTopic,char *configTopic,char *configAckTopic);
void setCommandMessageHandler(messageHandler message_handler);
void setConfigMessageHandler(messageHandler message_handler);
void onMessageReceived(MessageData *md);

#endif //# ZOHO_MESSAGE_HANDLER_H_