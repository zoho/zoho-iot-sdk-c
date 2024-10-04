#if defined(Z_PAHO_C)
int __wrap_MQTTClient_isConnected(MQTTClient handle);
int __wrap_MQTTClient_create(MQTTClient* handle, const char* serverURI, const char* clientId,
		int persistence_type, void* persistence_context);
int __wrap_MQTTClient_setCallbacks(MQTTClient handle, void* context, MQTTClient_connectionLost* cl,
														MQTTClient_messageArrived* ma, MQTTClient_deliveryComplete* dc);
int __wrap_MQTTClient_connect(MQTTClient handle, MQTTClient_connectOptions* options);
void __wrap_MQTTClient_destroy(MQTTClient* handle);
int __wrap_MQTTClient_disconnect(MQTTClient handle, int timeout);
int __wrap_MQTTClient_subscribe(MQTTClient handle, const char* topic, int qos);
int __wrap_MQTTClient_publishMessage(MQTTClient handle, const char* topicName, MQTTClient_message* message,
															 MQTTClient_deliveryToken* deliveryToken);
int __wrap_MQTTClient_waitForCompletion(MQTTClient handle, MQTTClient_deliveryToken mdt, unsigned long timeout);
#else
int __wrap_MQTTConnect(MQTTClient *c, MQTTPacket_connectData *options);
int __wrap_NetworkConnect(Network *n, char *hostname, int pt, ...);
int __wrap_NetworkConnectTLS(Network *n, char *hostname, int pt, ...);
int __wrap_MQTTSubscribe(MQTTClient *c, const char *topicFilter, enum QoS qos, messageHandler messageHandler);
int __wrap_MQTTPublish(MQTTClient *c, const char *topicName, MQTTMessage *message);
int __wrap_MQTTDisconnect(MQTTClient *c);
int __wrap_MQTTYield(MQTTClient *c, int time_out);
void __wrap_NetworkDisconnect(Network *n);
#endif