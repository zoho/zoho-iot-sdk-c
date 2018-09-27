#include <stdio.h>
#include "wiringPi.h"
#include "iot_client.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define trig (int)0
#define echo  (int)2
#define led (int)3

volatile int ctrl_flag = 0;

int distance = 0, status = 0;

void message_handler(MessageData *data)
{
    char payload[data->message->payloadlen];
    char topic[data->topicName->lenstring.len];
    *topic = '\0';
    *payload = '\0';
    strncat(topic, data->topicName->lenstring.data, data->topicName->lenstring.len);
    strncat(payload, data->message->payload, data->message->payloadlen);
    log_debug("Got new message on '%s'\n%s", topic, payload);
}

void interruptHandler(int signo)
{
    log_info("Closing connection...");
    ctrl_flag += 1;

    if (ctrl_flag >= 2)
    {
        log_fatal("Program is force killed");
        exit(0);
    }
}
void setup()
{
    pinMode(trig, OUTPUT);
    pinMode(echo, INPUT);
    pinMode(led, OUTPUT);
    signal(SIGINT, interruptHandler);
    signal(SIGTERM, interruptHandler);

    digitalWrite(trig,LOW);
    delay(2);
}
void formPayload()
{ 
    digitalWrite(trig, HIGH);
    delayMicroseconds(20);
    digitalWrite(trig, LOW);

    while (digitalRead(echo) == LOW);
    long startTime = micros();
    while (digitalRead(echo) == HIGH);
    long travelTime = micros() - startTime;

    distance = travelTime / 58;
    if (distance > 15)
    {
        status = 0;
        digitalWrite(led, HIGH);
    }
    else
    {
        status = 1;
        digitalWrite(led, LOW);
    }
    zclient_addNumber("Distance", distance);
    zclient_addString("status", status == 0? "Free":"Occupied");
}

int main()
{
    int rc = -1;
    IOTclient client;
    if (wiringPiSetup() == -1)
    {
        log_fatal("Program is exited, wiringPi setup failed");
        return 1;
    }
    setup();
    
    char *pRootCACertLocation = "", *pDeviceCertLocation = "", *pDevicePrivateKeyLocation = "",*pDeviceCertParsword = "";

#if defined(SECURE_CONNECTION)
    pRootCACertLocation = ROOTCA_CERT_LOCATION;
    #if defined(USE_CLIENT_CERTS)
        pDeviceCertLocation = CLIENT_CERT_LOCATION;
        pDevicePrivateKeyLocation = CLIENT_KEY_LOCATION;
    #endif
#endif
    rc = zclient_init(&client, "zoho-iot-pi-1", "authToken", NULL, NULL, pRootCACertLocation, pDeviceCertLocation, pDevicePrivateKeyLocation, pDeviceCertParsword);
    if (rc != SUCCESS)
    {
        return 0;
    }
    
    rc = zclient_connect(&client);
    if (rc != SUCCESS)
    {
        return 0;
    }

    rc = zclient_subscribe(&client, message_handler);
    while (ctrl_flag == 0)
    {
        formPayload();
        rc = zclient_dispatch(&client);
        if (rc != SUCCESS)
        {
            log_error("Can't publish\n");
            break;
        }
        zclient_yield(&client, 500);
    }
    zclient_disconnect(&client);

    return 0;
}