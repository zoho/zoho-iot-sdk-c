#include "zoho_iot_client.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if defined(Z_SECURE_CONNECTION)
#if defined(EMBED_MODE)
#include "zclient_certificates.h"
#endif
#endif

volatile int ctrl_flag = 0;

// Two independent Zoho IoT client instances
ZohoIOTclient client1;
ZohoIOTclient client2;

// --- Client 1 credentials ---
#define MQTT_USER_NAME_1 "<MQTT User Name>"
#define MQTT_PASSWORD_1  "<MQTT Password/Token>"

// --- Client 2 credentials ---
#define MQTT_USER_NAME_2 "<MQTT User Name>"
#define MQTT_PASSWORD_2  "<MQTT Password/Token>"

// Frequency in which the data needs to be fetched
#define POLL_FREQUENCY 5 // poll data every 5 sec

void command_handler_client1(char *topic, char *payload)
{
    log_debug("[client1] Command on '%s': %s", topic, payload);
    log_debug("[client1] Command Ack: %d",
              zclient_publishCommandAck(&client1, payload,
                                        SUCCESSFULLY_EXECUTED,
                                        "Executed by client1."));
}

void command_handler_client2(char *topic, char *payload)
{
    log_debug("[client2] Command on '%s': %s", topic, payload);
    log_debug("[client2] Command Ack: %d",
              zclient_publishCommandAck(&client2, payload,
                                        SUCCESSFULLY_EXECUTED,
                                        "Executed by client2."));
}

void interruptHandler(int signo)
{
    log_info("Closing connections...");
    ctrl_flag += 1;
    if (ctrl_flag >= 2)
    {
        log_fatal("Program is force killed");
        exit(0);
    }
}

// Returns current time in seconds
unsigned long long getTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long)(tv.tv_sec);
}

int main()
{
    int rc1 = -1, rc2 = -1;
    unsigned long long poll_time = 0;
    signal(SIGINT, interruptHandler);
    signal(SIGTERM, interruptHandler);

    int temperature = 23, humidity = 56, pressure = 78;
    char *pRootCACert = "", *pDeviceCert = "", *pDevicePrivateKey = "", *pDeviceCertPassword = "";

#if defined(Z_SECURE_CONNECTION)
    pRootCACert = CA_CRT;
#if defined(Z_USE_CLIENT_CERTS)
    pDeviceCert = CLIENT_CRT;
    pDevicePrivateKey = CLIENT_KEY;
#endif
#endif

    ZlogConfig *logConfig = getZlogger();
    logConfig->enableFileLog = 1;
    logConfig->level = LOG_DEBUG;
    logConfig->logPrefix = "multi_client";
    logConfig->logPath = "./";
    logConfig->maxLogFileSize = 100000;
    logConfig->maxRollingLogFile = 2;
    log_initialize(logConfig);


    rc1 = zclient_init(&client1, MQTT_USER_NAME_1, MQTT_PASSWORD_1, CRT_PARSE_MODE,
                       pRootCACert, pDeviceCert, pDevicePrivateKey, pDeviceCertPassword);
    if (rc1 != ZSUCCESS)
    {
        log_error("client1 init failed");
        return 0;
    }

    rc2 = zclient_init(&client2, MQTT_USER_NAME_2, MQTT_PASSWORD_2, CRT_PARSE_MODE,
                       pRootCACert, pDeviceCert, pDevicePrivateKey, pDeviceCertPassword);
    if (rc2 != ZSUCCESS)
    {
        log_error("client2 init failed");
        return 0;
    }

    zclient_connect(&client1);
    zclient_connect(&client2);

    zclient_command_subscribe(&client1, command_handler_client1);
    zclient_command_subscribe(&client2, command_handler_client2);

    while (ctrl_flag == 0)
    {
    
        zclient_reconnect(&client1);
        zclient_reconnect(&client2);

        if (getTime() - poll_time > POLL_FREQUENCY)
        {
            temperature += 2;
            humidity    *= 2;
            pressure    -= 5;

            if (client1.current_state == CONNECTED)
            {
                log_debug("[client1] Dispatching telemetry");
                zclient_addNumber(&client1, "temperature", temperature);
                zclient_addNumber(&client1, "humidity",    humidity, "room1");
                zclient_addNumber(&client1, "pressure",    pressure, "room2");
                zclient_markDataPointAsError(&client1, "moisture",   "room2");
                zclient_markDataPointAsError(&client1, "air_quality");
                zclient_addString(&client1, "status", "OK");
                zclient_dispatch(&client1);
            }

            if (client2.current_state == CONNECTED)
            {
                log_debug("[client2] Dispatching telemetry");
                zclient_addNumber(&client2, "temperature", temperature);
                zclient_addNumber(&client2, "humidity",    humidity, "room1");
                zclient_addNumber(&client2, "pressure",    pressure, "room2");
                zclient_markDataPointAsError(&client2, "moisture",   "room2");
                zclient_markDataPointAsError(&client2, "air_quality");
                zclient_addString(&client2, "status", "OK");
                zclient_dispatch(&client2);
            }

            poll_time = getTime();
        }

       
        if (client1.current_state == CONNECTED)
            zclient_yield(&client1, 150);

        if (client2.current_state == CONNECTED)
            zclient_yield(&client2, 150);
    }

    zclient_disconnect(&client1);
    zclient_free(&client1);

    zclient_disconnect(&client2);
    zclient_free(&client2);

    return 0;
}