#include "zoho_iot_client.h"
#include <stdio.h>
#include <strings.h>
#include <gpiod.h>
#if defined(Z_SECURE_CONNECTION)
#if defined(EMBED_MODE)
#include "zclient_certificates.h"
#endif
#endif

int ctrl_flag = 0;
struct gpiod_chip *chip;
struct gpiod_line *input_line;
struct gpiod_line *output_line;
// Zoho client struct to handle the Communication with the HUB
ZohoIOTclient client;

// Zoho client credentials.
#define MQTT_USER_NAME "<MQTT User Name>"
#define MQTT_PASSWORD "<MQTT Password/Token>"

#define CHIPNAME "gpiochip4"  //get the correct chip name using command "gpiodetect" where rp-1 chip for  general gpio pins

// Frequency in which the data needs to be fetched and published
#define POLL_FREQUENCY 10 // poll data every 10 sec
unsigned long long poll_time = 0;

#define INPUT_PIN 26
#define OUTPUT_PIN 4




void message_command_handler(MessageData *data)
{
    char payload[data->message->payloadlen];
    char topic[data->topicName->lenstring.len];
    *topic = '\0';
    *payload = '\0';
    strncat(topic, data->topicName->lenstring.data, data->topicName->lenstring.len);
    strncat(payload, data->message->payload, data->message->payloadlen);
    log_debug("\n\n Got new command message on '%s'\n%s \n\n", topic, payload);
    cJSON * commandMessageArray = cJSON_Parse(payload);
 
    if(cJSON_IsArray(commandMessageArray)!=1){
        log_debug("Second level Command Ack status : %d", zclient_publishCommandAck(&client,payload, EXECUTION_FAILURE, "Command based task Failed."));
        return;
    }
    cJSON * commandMessage = cJSON_GetArrayItem(commandMessageArray,0);
       
    cJSON * payloadValue = cJSON_GetObjectItem(commandMessage,"payload");
    cJSON * array = cJSON_GetArrayItem(payloadValue,0);
    log_debug(cJSON_Print(array));
    char* commandValue = cJSON_GetObjectItem(array,"value")->valuestring;
    if(strcmp(commandValue,"on")==0){
        if(gpiod_line_set_value(output_line, 1)<0){
            log_error("failed to change the state of pin %d",OUTPUT_PIN);
             zclient_publishCommandAck(&client,payload, EXECUTION_FAILURE, "Command based task Failed.");
            return ;
        }
        log_debug("command : pin -%d state changed successfully to ON",OUTPUT_PIN);
    }
    else if (strcmp(commandValue,"off")==0)
    {
        if(gpiod_line_set_value(output_line, 0)<0){
            log_error("failed to change the state of pin %d",OUTPUT_PIN);
            zclient_publishCommandAck(&client,payload, EXECUTION_FAILURE, "Command based task Failed.");
            return ;
        }
        log_debug("command : pin -%d state changed successfully to OFF",OUTPUT_PIN);
    }
    else{
         zclient_publishCommandAck(&client,payload, EXECUTION_FAILURE, "Command based task Failed.");
        log_error("unsupported command value");
    }
    log_debug("Second level Command Ack status : %d", zclient_publishCommandAck(&client,payload, SUCCESSFULLY_EXECUTED, "Command based task Executed."));
}

void interruptHandler(int signo)
{
    log_info("Closing connection...");
    ctrl_flag += 1;
    if (ctrl_flag >= 2)
    {
        log_fatal("Program is force killed");
        gpiod_chip_close(chip);
        exit(0);
    }
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
    ZlogConfig *logConfig = getZlogger();
    logConfig->enableFileLog = 1;
    logConfig->level = LOG_DEBUG;
    logConfig->logPrefix = "rpi5_gpio";
    logConfig->logPath = "./";
    logConfig->maxLogFileSize = 5000000; // File size in bytes
    logConfig->maxRollingLogFile = 2;
    // Initialise the PiGPIO library

    const char *chipname = CHIPNAME;
    unsigned int input_line_num = INPUT_PIN; // GPIO pin 5
    unsigned int output_line_num = OUTPUT_PIN; // GPIO pin 5

    int value;

    chip = gpiod_chip_open_by_name(chipname);
    if (!chip) {
        log_error("Open chip failed\n");
        return -1;
    }

    input_line = gpiod_chip_get_line(chip, input_line_num);
    if (!input_line) {
        log_error("Get line failed\n");
        gpiod_chip_close(chip);
        return -1;
    }

    struct gpiod_line_request_config config;
    config.consumer = "gpio_read_example";
    config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
    config.flags = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;

    if (gpiod_line_request(input_line, &config, 0) < 0) {
        log_error("Request line as input with pull-down failed\n");
        gpiod_chip_close(chip);
        return -1;
    }

    output_line = gpiod_chip_get_line(chip, output_line_num);
    if (!output_line) {
        log_error("Get line failed\n");
        gpiod_chip_close(chip);
        return -1;
    }

    // Request the line as an output
    if (gpiod_line_request_output(output_line, "gpio_write_example", 0) < 0) {
        log_error("Request line as output failed\n");
        gpiod_chip_close(chip);
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
    rc = zclient_init(&client, MQTT_USER_NAME, MQTT_PASSWORD, CRT_PARSE_MODE, pRootCACert, pDeviceCert, pDevicePrivateKey, pDeviceCertParsword,logConfig);

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

    rc = zclient_command_subscribe(&client, message_command_handler);
    while (ctrl_flag == 0)
    {
        // If Connection is already available, then retryring to connect inside function is ignored
        rc = zclient_reconnect(&client);
        if (getTime() > poll_time + POLL_FREQUENCY) //check if it is time for polling the data from the sensor
        {
            int inputValue = gpiod_line_get_value(input_line);
            if (inputValue < 0) {
                log_error("Read line input failed\n");
                gpiod_chip_close(chip);
                return -1;
            }
            log_debug("Pin %d value - %d", INPUT_PIN,inputValue);
            char key[50] ;
            sprintf(key,"input_pin_%d_value",INPUT_PIN);
            rc = zclient_addNumber(&client, key, inputValue);
            rc = zclient_dispatch(&client);

            poll_time = getTime();
        }
        if (client.current_state == CONNECTED)
        {
            // Yielding only on available connection
            rc = zclient_yield(&client, 300);
        }
    }

    zclient_disconnect(&client);
    gpiod_chip_close(chip);
    return 0;
}
