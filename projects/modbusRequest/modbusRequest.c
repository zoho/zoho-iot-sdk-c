#include "zoho_iot_client.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#if defined(Z_SECURE_CONNECTION)
#if defined(EMBED_MODE)
#include "zclient_certificates.h"
#endif
#endif
#include <modbus/modbus.h>
#include <sys/time.h>
volatile int ctrl_flag = 0;

// Zoho client struct to handle the Communication with the HUB
ZohoIOTclient client;

// Zoho clinet credentials.
#define MQTT_USER_NAME "<MQTT User Name>"
#define MQTT_PASSWORD "<MQTT Password/Token>"

// Modbus TCP server location
#define MODBUS_SERVER_ADDRESS "127.0.0.1"
// Modbus Port
#define MODBUS_SERVER_PORT 502
// Modbus Debug
#define MODBUS_DEBUG FALSE // Set TRUE if debug is required
// Modbus data address for an Input register
#define MODBUS_DATA_ADDRESS 0
// Modbus data size
#define MODBUS_DATA_SIZE 1
// Alert threshold value
#define THRESHOLD_VALUE 20

// Intervals in which the telemetry data needs to be dispatched to the hub
#define DISPATCH_INTERVAL 30 // Dispatch data every 30 sec
// Frequency in which the data needs to be fetched
#define POLL_FREQUENCY 5 // poll data every 5 sec

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
    int rc = -1;
    // Variable to check for the Data dispatch time
    unsigned long long periodic_dispatch = 0;
    // Variable to check for the Data polling time
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

    int temp;            // A variable to hold the data
    int alert_state = 0; // A variable to hold the status of the alert

    while (ctrl_flag == 0)
    {
        // If Connection is already available, then retryring to connect inside function is ignored
        rc = zclient_reconnect(&client);
        if (getTime() > poll_time + POLL_FREQUENCY) //check if it is time for polling the data from the sensor
        {
            //Modbus implementation
            log_debug("Polling datapoints");
            modbus_t *ctx;
            uint16_t tab_rp_registers[MODBUS_DATA_SIZE];
            int64_t value;

            ctx = modbus_new_tcp(MODBUS_SERVER_ADDRESS, MODBUS_SERVER_PORT);
            modbus_set_debug(ctx, MODBUS_DEBUG);

            int mc = -1, status = -1;
            if (modbus_connect(ctx) != -1)
            {
                mc = modbus_set_slave(ctx, 1);
            }

            if (mc != -1)
            {
                status = modbus_read_input_registers(ctx, MODBUS_DATA_ADDRESS, MODBUS_DATA_SIZE, tab_rp_registers);

                if (status != -1)
                {
                    value = retrieveByte(status, MODBUS_DATA_SIZE, tab_rp_registers);
                    temp = (int16_t)value;
                }
            }

            modbus_close(ctx);
            modbus_free(ctx);

            // Check for any threshold break and notify the hub when the modbus response is received properly
            if (mc != -1 && status != -1)
            {
                if (temp > THRESHOLD_VALUE && alert_state == 0)
                {
                    alert_state = 1;
                    zclient_addEventDataNumber("temperature", temp);
                    zclient_dispatchEventFromEventDataObject(&client, "High Temperature", " Storage Temperature is high", "StorageRoom");
                }
                else if (temp <= THRESHOLD_VALUE && alert_state == 1)
                {
                    alert_state = 0;
                    zclient_addEventDataNumber("temperature", temp);
                    zclient_dispatchEventFromEventDataObject(&client, "Normal Temperature", " Storage Temperature is normal", "StorageRoom");
                }
            }

            if (getTime() > periodic_dispatch + DISPATCH_INTERVAL) //check if it is time to dispatch data to Hub
            {
                if (mc != -1 && status != -1)
                {
                    rc = zclient_addNumber(&client, "temperature", temp, "StorageRoom");
                    rc = zclient_dispatch(&client);
                }
                else
                {
                    rc = zclient_markDataPointAsError(&client, "temperature", "StorageRoom");
                    rc = zclient_dispatch(&client);
                }

                while (rc != ZSUCCESS && ctrl_flag == 0)
                {
                    rc = zclient_reconnect(&client);
                }

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
    zclient_disconnect(&client);
    return 0;
}