#ifndef ZOHO_MESSAGE_HANDLER_H_
#define ZOHO_MESSAGE_HANDLER_H_

#include "zoho_iot_client.h"

ZohoIOTclient *iot_client;
char handler_command_topic[100], handler_command_ack_topic[100];
messageHandler on_message_handler;
void initMessageHandler(ZohoIOTclient *client, char *commandTopic, char *commandAckTopic);
void setMessageHandler(messageHandler message_handler);
void onMessageReceived(MessageData *md);

#endif //# ZOHO_MESSAGE_HANDLER_H_