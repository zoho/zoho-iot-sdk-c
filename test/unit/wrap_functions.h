int __wrap_MQTTConnect(MQTTClient *c, MQTTPacket_connectData *options);
int __wrap_NetworkConnect(Network *n, char *hostname, int pt, ...);
int __wrap_NetworkConnectTLS(Network *n, char *hostname, int pt, ...);
int __wrap_MQTTSubscribe(MQTTClient *c, const char *topicFilter, enum QoS qos, messageHandler messageHandler);
int __wrap_MQTTPublish(MQTTClient *c, const char *topicName, MQTTMessage *message);
int __wrap_MQTTDisconnect(MQTTClient *c);
int __wrap_MQTTYield(MQTTClient *c, int time_out);
void __wrap_NetworkDisconnect(Network *n);