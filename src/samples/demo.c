#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "wiringPi.h"
#include "iot_client.h"

#define trig (int)0
#define echo (int)2
#define led (int)3

volatile int ctrl_flag = 0;

int distance = 0, status = 0;

void interruptHandler(int signo)
{
    // Set ctrl_flag on interrupt to stop program.
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
    // Setting up pin configuration.
    pinMode(trig, OUTPUT);
    pinMode(echo, INPUT);
    pinMode(led, OUTPUT);
    signal(SIGINT, interruptHandler);
    signal(SIGTERM, interruptHandler);

    // Setting the default state OFF.
    digitalWrite(trig,LOW);
    delay(2);
}

void pollData()
{ 
    // Trigger the Ultrasonic sensor.
    digitalWrite(trig, HIGH);
    delayMicroseconds(20);
    digitalWrite(trig, LOW);

    // Read data from sensor and caliculating distance .
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
}

void generateJSONPayload()
{
    // Add datapoint to json payload
    zclient_addNumber("Distance", distance);
    zclient_addString("status", status == 0? "Free":"Occupied");
}

int main()
{
    int rc = -1;
    IOTclient client;
    char *pRootCACertLocation = "", *pDeviceCertLocation = "", *pDevicePrivateKeyLocation = "",*pDeviceCertParsword = "";

    // Check if wiringPi setup is succesful.
    if (wiringPiSetup() == -1)
    {
        log_fatal("Program is exited, wiringPi setup failed");
        return 1;
    }
    setup();
    
    //Initialize TLS if Secure connection is Enabled. 
#if defined(SECURE_CONNECTION)
    pRootCACertLocation = ROOTCA_CERT_LOCATION;
    #if defined(USE_CLIENT_CERTS)
        pDeviceCertLocation = CLIENT_CERT_LOCATION;
        pDevicePrivateKeyLocation = CLIENT_KEY_LOCATION;
    #endif
#endif

    /// Initialize Zoho client with device and certificate details. 
    rc = zclient_init(&client, "zoho-iot-pi-1", "authToken", NULL, NULL, pRootCACertLocation, pDeviceCertLocation, pDevicePrivateKeyLocation, pDeviceCertParsword);
    if (rc != SUCCESS)
    {
        return 0;
    }
    
    // Establishing connection to  Zoho client.
    rc = zclient_connect(&client);
    if (rc != SUCCESS)
    {
        return 0;
    }

    // Poll for data from sensors and dispatch.
    while (ctrl_flag == 0)
    {
        pollData();
        generateJSONPayload();
        rc = zclient_dispatch(&client);

        if (rc != SUCCESS)
        {
            log_error("Can't publish\n");
            break;
        }

        //Configuring Polling interval as 500msec
        zclient_yield(&client, 500);
    }

    // Disconnect Zoho client
    zclient_disconnect(&client);

    return 0;
}