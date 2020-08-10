#ifndef MESSAGE_HANDLER_H_
#define MESSAGE_HANDLER_H_

#include "iot_client.h"

IOTclient *iot_client;
char handler_command_topic[100], handler_command_ack_topic[100];
messageHandler on_message_handler;
void initMessageHandler(IOTclient *client, char *commandTopic, char *commandAckTopic);
void setMessageHandler(messageHandler message_handler);
void onMessageReceived(MessageData *md);

#endif //# MESSAGE_HANDLER_H_