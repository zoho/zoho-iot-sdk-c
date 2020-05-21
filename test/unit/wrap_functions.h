int __wrap_MQTTConnect(Client *c, MQTTPacket_connectData *options);
int __wrap_ConnectNetwork(Network *n, char *host, int pt, ...);
int __wrap_MQTTSubscribe(Client *c, const char *topicFilter, enum QoS qos, messageHandler messageHandler);
int __wrap_MQTTPublish(Client *c, const char *topicName, MQTTMessage *message);
int __wrap_MQTTDisconnect(Client *c);
int __wrap_MQTTYield(Client *c, int time_out);