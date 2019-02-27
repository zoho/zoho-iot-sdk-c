int __wrap_MQTTConnect(MQTTClient* c, MQTTPacket_connectData* options);
int __wrap_NetworkConnect(Network* n, char* host, int pt);
int __wrap_MQTTSubscribe(MQTTClient *c, const char* topicFilter, enum QoS qos,messageHandler messageHandler);