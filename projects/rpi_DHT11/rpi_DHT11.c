#include "zoho_iot_client.h"
#include <stdio.h>
#include <strings.h>
#include <pigpio.h>
#if defined(Z_SECURE_CONNECTION)
#if defined(EMBED_MODE)
#include "zclient_certificates.h"
#endif
#endif

// Zoho client struct to handle the Communication with the HUB
ZohoIOTclient client;

// Zoho clinet credentials.
#define MQTT_USER_NAME "<MQTT User Name>"
#define MQTT_PASSWORD "<MQTT Password/Token>"

// Frequency in which the data needs to be fetched and published
#define POLL_FREQUENCY 10 // poll data every 10 sec
unsigned long long poll_time = 0;

#define INPUT_GPIO_PIN 4
#define DHT11 0
#define DHT22 1
#define DEFAULT_DEVICE DHT11

int bitCount;
int data[5];
int startTick;

//Function gets the current time in seconds
unsigned long long getTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long seconds = (unsigned long long)(tv.tv_sec);
    return seconds;
}

void callback(int gpio, int level, uint32_t tick);

int main()
{
    // Initialise the PiGPIO library
    if (gpioInitialise() < 0)
    {
        log_error("pigpio initialisation failed");
        return -1;
    }

    // set callback function on gpio transition
    if (gpioSetAlertFunc(INPUT_GPIO_PIN, callback) != 0)
    {
        puts("PI_BAD_USER_GPIO");
        return -1;
    }

    int rc = -1;

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
    while (rc != ZSUCCESS)
    {
        //Endless reconnection on start. No data collection happens until a proper connection is established.
        rc = zclient_reconnect(&client);
    }

    while (1)
    {
        // If Connection is already available, then retryring to connect inside function is ignored
        rc = zclient_reconnect(&client);
        if (getTime() > poll_time + POLL_FREQUENCY) //check if it is time for polling the data from the sensor
        {

            // number of attempts to query sensor
            int tries = 3;

            // attempt to read sensor tries times or loop forever
            do
            {
                // start bit count less 3 non-data bits
                bitCount = -3;

                // zero the data array
                for (int i = 0; i < 5; i++)
                    data[i] = 0;

                // set start time of first low signal
                startTick = gpioTick();

                // set pin as output and make high for 50ms so we can detect first low
                gpioSetMode(INPUT_GPIO_PIN, PI_OUTPUT);
                gpioWrite(INPUT_GPIO_PIN, 1);
                gpioDelay(50000);

                // send start signal
                gpioWrite(INPUT_GPIO_PIN, 0);
                // wait for 18ms
                gpioDelay(18000);
                // return bus to high for 20us
                gpioWrite(INPUT_GPIO_PIN, 1);
                gpioDelay(20);
                // change to input mode
                gpioSetMode(INPUT_GPIO_PIN, PI_INPUT);

                // wait 50ms for data input
                gpioDelay(50000);

                // if we received 40 data bits and the checksum is valid
                if (bitCount == 40 &&
                    data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xff))
                {
                    float tempC;
                    float humidity;

                    if (DEFAULT_DEVICE == DHT11)
                    {
                        humidity = data[0] + data[1] / 10.0f;
                        tempC = data[2] + data[3] / 10.0f;
                    }
                    else if (DEFAULT_DEVICE == DHT22)
                    {
                        humidity = (data[0] * 256 + data[1]) / 10.0f;
                        tempC = ((data[2] & 0x7f) * 256 + data[3]) / 10.0f;
                        if (data[2] & 0x80)
                            tempC *= -1.0f;
                    }

                    printf("\nTemperature: %.1fC Humidity: %.1f%%\n", tempC, humidity);
                    tries = 0;

                    if (tempC > 0 && humidity > 0)
                    {
                        rc = zclient_addNumber(&client, "temperature_C", tempC);
                        rc = zclient_addNumber(&client, "humidity", humidity);
                        rc = zclient_dispatch(&client);
                    }
                }
                else
                {
                    puts("Data Invalid!");
                    --tries;
                }

                // minimum device reset time, 2 seconds
                gpioSleep(PI_TIME_RELATIVE, 2, 0);
            } while (tries);

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

void callback(int gpio, int level, uint32_t tick)
{
    // if the level has gone low
    if (level == 0)
    {
        // duration is the elapsed time between lows
        int duration = tick - startTick;
        // set the timer start point to this low
        startTick = tick;

        // if we have seen the first three lows which aren't data
        if (++bitCount > 0)
        {
            // point into data structure, eight bits per array element
            // shift the data one bit left
            data[(bitCount - 1) / 8] <<= 1;
            // set data bit high if elapsed time greater than 100us
            data[(bitCount - 1) / 8] |= (duration > 100 ? 1 : 0);
        }
    }
}