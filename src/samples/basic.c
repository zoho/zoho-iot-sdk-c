#include "iot_client.h"

int main()
{
    int rc = -1;
    IOTclient client;
    char *topic = "test_topic9876";
    char *host = "m2m.eclipse.org";
    const int port = 1883;

    rc = zclient_init(&client, "deviceID", "authToken");
    if (rc != SUCCESS)
    {
        printf("Initialize failed and returned. rc = %d.\n Quitting..", rc);
        return 0;
    }
    rc = zclient_connect(&client, host, port);

    if (rc != SUCCESS)
    {
        printf("Connection failed and returned. rc = %d.\n Quitting..\n", rc);
        return 0;
    }

    rc = zclient_publish(&client, topic, "test message");
    if (rc != SUCCESS)
    {
        printf("Unable to send message ..%d\n", rc);
        return 0;
    }

    zclient_disconnect(&client);
    return 0;
}