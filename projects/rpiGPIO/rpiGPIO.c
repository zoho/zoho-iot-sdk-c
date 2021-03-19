#include "zoho_iot_client.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#if defined(Z_SECURE_CONNECTION)
#if defined(EMBED_MODE)
#include "zclient_certificates.h"
#endif
#endif
#include "pigpio.h"

volatile int ctrl_flag = 0;

// Zoho client struct to handle the Communication with the HUB
ZohoIOTclient client;

// Zoho clinet credentials.
#define MQTT_USER_NAME "/domain/v1/devices/client_id/connect"
#define MQTT_PASSWORD ""
#define ROOT_CA_CERT_LOCATION "/path_to/server_ca.crt"
#define DEVICE_CERT_LOCATION "/path_to/client_crt.crt"
#define DEVICE_PRIVATE_KEY_LOCATION "/path_to/client_key.key"
#define DEVICE_CERT_PASSWORD ""

// Intervals in which the telemetry data needs to be dispatched to the hub
#define DISPATCH_INTERVAL 30 // Dispatch data every 30 sec
// Frequency in which the data needs to be fetched
#define POLL_FREQUENCY 5 // poll data every 5 sec

//GPIO settings
#define GPIO_PIN 23 // Pin no 16

//Function to handle the interrupts
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

//Function to retrieves integer data from hex
int64_t retrieveByte(int rc, int size, uint16_t tab_rp_registers[])
{
    int i;
    int64_t val = 0;
    for (i = 0; i < rc; i++)
    {
        val = tab_rp_registers[i] + val;
        if (i != size - 1)
        {
            val = val << 16;
        }
    }
    return val;
}

//Function gets the current time in seconds
unsigned long long getTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long seconds = (unsigned long long)(tv.tv_sec);
    return seconds;
}

int main()
{
    // Initialise the PiGPIO library
    if (gpioInitialise() < 0)
    {
        log_error("pigpio initialisation failed");
        return 1;
    }

    int rc = -1;
    unsigned long long periodic_dispatch = 0;
    unsigned long long poll_time = 0;
    signal(SIGINT, interruptHandler);
    signal(SIGTERM, interruptHandler);

    char *payload;
    char *pRootCACert = "", *pDeviceCert = "", *pDevicePrivateKey = "", *pDeviceCertParsword = "";

#if defined(Z_SECURE_CONNECTION)
    pRootCACert = CA_CRT;
#if defined(Z_USE_CLIENT_CERTS)
    pDeviceCert = CLIENT_CRT;
    pDevicePrivateKey = CLIENT_KEY;
#endif
#endif

    // Initialize the ZohoIoTClient Library
    rc = zclient_init(&client, MQTT_USER_NAME, MQTT_PASSWORD, CRT_PARSE_MODE, pRootCACert, pDeviceCert, pDevicePrivateKey, pDeviceCertParsword);
    if (rc != ZSUCCESS)
    {
        return 0;
    }
    rc = zclient_connect(&client);
    while (rc != ZSUCCESS && ctrl_flag == 0)
    {
        //Endless reconnection on start. No data collection happens until a proper connection is established.
        rc = zclient_reconnect(&client);
    }

    int door_status;     // A variable to hold the data
    int alert_state = 0; // A variable to hold the status of the alert

    // Initialise the pin as input
    gpioSetMode(GPIO_PIN, PI_INPUT);

    while (ctrl_flag == 0)
    {
        // If Connection is already available, then retryring to connect inside function is ignored
        rc = zclient_reconnect(&client);
        if (getTime() > poll_time + POLL_FREQUENCY) //check if it is time for polling the data from the sensor
        {
            log_debug("Polling datapoints");
            // Read the pin status
            door_status = gpioRead(GPIO_PIN);

            // Check for any threshold break and notify the hub as events
            if (door_status == 1 && alert_state == 0)
            {
                alert_state = 1;
                zclient_addEventDataNumber("Door", door_status);
                zclient_dispatchEventFromEventDataObject(&client, "Door Condition", " Room A is Open", "RoomA");
            }
            else if (door_status == 0 && alert_state == 1)
            {
                alert_state = 0;
                zclient_addEventDataNumber("Door", door_status);
                zclient_dispatchEventFromEventDataObject(&client, "Door Condition", " Room A is closed", "RoomA");
            }

            //check if it is time to dispatch data to Hub
            if (getTime() > periodic_dispatch + DISPATCH_INTERVAL)
            {
                rc = zclient_addNumber(&client, "Door", door_status, "RoomA");
                rc = zclient_dispatch(&client);

                periodic_dispatch = getTime();
            }
            poll_time = getTime();
        }
        if (client.current_state == CONNECTED)
        {
            // Yielding only on available connection
            rc = zclient_yield(&client, 300);
        }
    }

    gpioTerminate();
    zclient_disconnect(&client);
    return 0;
}