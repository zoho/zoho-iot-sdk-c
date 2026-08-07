#ifndef ZOHO_MESSAGE_HANDLER_H_
#define ZOHO_MESSAGE_HANDLER_H_

#include "zoho_iot_client.h"

void initMessageHandler(ZohoIOTclient *client);
void setCommandMessageHandler(ZohoIOTclient *client, SubscribeMessageHandler message_handler);
void setConfigMessageHandler(ZohoIOTclient *client, SubscribeMessageHandler message_handler);
#if defined(Z_PAHO_C)
int onMessageReceived(void *context, char *topicName, int topicLen, MQTTClient_message *message);
#else
void onMessageReceived(MessageData *md);
#endif

#endif //# ZOHO_MESSAGE_HANDLER_H_