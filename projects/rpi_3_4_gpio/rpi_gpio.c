#include "zoho_iot_client.h"
#include <stdio.h>
#include <strings.h>
#include <pigpio.h>
#if defined(Z_SECURE_CONNECTION)
#if defined(EMBED_MODE)
#include "zclient_certificates.h"
#endif
#endif

int ctrl_flag = 0;
// Zoho client struct to handle the Communication with the HUB
ZohoIOTclient client;

// Zoho client credentials.
#define MQTT_USER_NAME "<MQTT User Name>"
#define MQTT_PASSWORD "<MQTT Password/Token>"

// Frequency in which the data needs to be fetched and published
#define POLL_FREQUENCY 10 // poll data every 10 sec
unsigned long long poll_time = 0;

#define INPUT_PIN 4
#define OUTPUT_PIN 26
#define INTERRUPT_PIN 17
#define DEBOUNCE_TIME 200 // Debounce time in milliseconds
int lastInterruptTime = 0;
bool interrupt_occured = false;

void gpiointerrupt_handling(int pin, int level, uint32_t tick,void * arguments){
    int currentTime = gpioTick() / 1000;
    int timeDiff = currentTime - lastInterruptTime;
    if (timeDiff > DEBOUNCE_TIME){
            interrupt_occured= true;
            log_debug("Interrupt Occured in pin - %d, Level - %d",pin,level);
            char key[50];
            sprintf(key,"interrupt_pin_%d_value",pin);
            zclient_addNumber(&client,key,level);
            lastInterruptTime= currentTime;
    }
}


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
        if(gpioWrite(OUTPUT_PIN,1)<0){
            log_error("failed to change the state of pin %d",OUTPUT_PIN);
             zclient_publishCommandAck(&client,payload, EXECUTION_FAILURE, "Command based task Failed.");
            return ;
        }
        log_debug("command : pin -%d state changed successfully to ON",OUTPUT_PIN);
    }
    else if (strcmp(commandValue,"off")==0)
    {
        if(gpioWrite(OUTPUT_PIN,0)<0){
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
    logConfig->logPrefix = "rpi_gpio";
    logConfig->logPath = "./";
    logConfig->maxLogFileSize = 5000000; // File size in bytes
    logConfig->maxRollingLogFile = 2;
    // Initialise the PiGPIO library
    if (gpioInitialise() < 0)
    {
        log_error("pigpio initialisation failed");
        return -1;
    }

    if (gpioSetMode(INPUT_PIN, PI_INPUT) != 0) {
        log_error("Failed to set pin input mode for pin %d\n",INPUT_PIN);
        return -1;
    }

   if(gpioSetMode(OUTPUT_PIN,PI_OUTPUT)!=0){
        log_error("Fail to set pin output mode for pin %d\n",OUTPUT_PIN);
        return -1;
    }

    if (gpioSetPullUpDown(INPUT_PIN, PI_PUD_DOWN) != 0) {
        log_error("Failed to set pullUpDown for input pin -%d\n",INPUT_PIN);
        return -1;
    }

    //set output pin to default off
    if(gpioWrite(OUTPUT_PIN,0)<0){
            log_error("failed to set default state OFF for pin %d",OUTPUT_PIN);
            return -1;
        }

    if (gpioSetMode(INTERRUPT_PIN, PI_INPUT) != 0) {
        log_error("Failed to set pin mode for interrupt pin %d\n",INTERRUPT_PIN);
        return -1;
    }
    if (gpioSetPullUpDown(INTERRUPT_PIN, PI_PUD_DOWN) != 0) {
        log_error("Failed to set pullUpDown interrupt pin %d\n",INTERRUPT_PIN);
        return -1;
    }
    if (gpioSetAlertFuncEx(INTERRUPT_PIN,gpiointerrupt_handling,NULL) != 0) {
        log_error("Failed to set interrupt handler for pin %d\n",INTERRUPT_PIN);
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
        if(interrupt_occured){
            zclient_dispatch(&client);
            interrupt_occured = false;
        }
        // If Connection is already available, then retryring to connect inside function is ignored
        rc = zclient_reconnect(&client);
        if (getTime() > poll_time + POLL_FREQUENCY) //check if it is time for polling the data from the sensor
        {
            int inputValue = gpioRead(INPUT_PIN);
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
    return 0;
}
