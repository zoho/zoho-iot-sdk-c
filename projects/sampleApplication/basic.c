#include "zoho_iot_client.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#if defined(Z_SECURE_CONNECTION)
#if defined(EMBED_MODE)
#include "zclient_certificates.h"
#endif
#endif
volatile int ctrl_flag = 0;

ZohoIOTclient client;

void message_handler(MessageData *data)
{
    char payload[data->message->payloadlen];
    char topic[data->topicName->lenstring.len];
    *topic = '\0';
    *payload = '\0';
    strncat(topic, data->topicName->lenstring.data, data->topicName->lenstring.len);
    strncat(payload, data->message->payload, data->message->payloadlen);
    log_debug("\n\n Got new message on '%s'\n%s \n\n", topic, payload);
    log_debug("Second level Command Ack status : %d", zclient_publishCommandAck(&client, "123456tui-orpm123", SUCCESFULLY_EXECUTED, "Command based task Executed."));
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

int main()
{
    int rc = -1;
    signal(SIGINT, interruptHandler);
    signal(SIGTERM, interruptHandler);

    char *payload;
    int temperature = 23, humidity = 56, pressure = 78;
    char *pRootCACert = "", *pDeviceCert = "", *pDevicePrivateKey = "", *pDeviceCertParsword = "";

#if defined(Z_SECURE_CONNECTION)
    pRootCACert = CA_CRT;
#if defined(Z_USE_CLIENT_CERTS)
    pDeviceCert = CLIENT_CRT;
    pDevicePrivateKey = CLIENT_KEY;
#endif
#endif

    //Update your DEVICE_ID AND AUTH_TOKEN below:
    rc = zclient_init(&client, "/domain_name/v1/devices/client_id/connect", "mqtt_password", CRT_PARSE_MODE, pRootCACert, pDeviceCert, pDevicePrivateKey, pDeviceCertParsword);
    // rc = zclient_init_config_file(&client, "MQTTConfigFileName", CRT_PARSE_MODE);

    /*
    * If you want to override the default SDK logging configuration use the ZlogConfig object and 
    * set the required configuration and pass it to the zclient_init (or) zclient_init_config_file function as follows 
    * 
    * ZlogConfig *logConfig = getZlogger();
    * logConfig->enableFileLog = 1;
    * logConfig->setQuiet = 1;
    * logConfig ->logPath = "./"
    * logConfig->logPrefix = "logFile_name";
    * logConfig->maxLogFileSize = 100000;
    * logConfig->maxRollingLogFile = 1;
    * 
    * zclient_init(&client, "/domain_name/v1/devices/client_id/connect", "mqtt_password", CRT_PARSE_MODE, pRootCACert, pDeviceCert, pDevicePrivateKey, pDeviceCertParsword,logConfig);
    *                                                                           (or)
    * rc = zclient_init_config_file(&client, "MQTTConfigFileName", CRT_PARSE_MODE,logConfig);
    * 
    * enableFileLog     -> enables the file logging for the application
    * setQuiet          -> disables the console print 
    * logPath           -> location where the log to be created
    * logPrefix         -> log file name
    * maxLogFileSize    -> size of the log file in bytes
    * maxRollingLogFile -> No of rolling log file to be created in addition to the main log file
    * 
    */

    if (rc != ZSUCCESS)
    {
        return 0;
    }
    rc = zclient_connect(&client);
    while (rc != ZSUCCESS && ctrl_flag == 0)
    {
        //Endless reconnection on start. No data collection happens during connection error.
        rc = zclient_reconnect(&client);
    }

    rc = zclient_subscribe(&client, message_handler);

    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "Ac_speed", 1);
    cJSON_AddFalseToObject(obj, "Fan_Status");
    char *eventMsg = cJSON_Print(obj);
    zclient_dispatchEventFromJSONString(&client, "High Temperature", "Room+3 temperature is above normal", eventMsg, "Room_3");
    cJSON_free(eventMsg);
    cJSON_Delete(obj);
    zclient_addEventDataNumber("Room1_temperature", 15);
    zclient_addEventDataString("Exhuast_speed", "Full");
    zclient_dispatchEventFromEventDataObject(&client, "Low Temperature", "Room+1 Temperature is below normal", "Floor_2");

    while (ctrl_flag == 0)
    {
        temperature += 2;
        humidity *= 2;
        pressure -= 5;
        rc = zclient_addNumber(&client, "temperature", temperature);
        rc = zclient_addNumber(&client, "humidity", humidity, "room1");
        rc = zclient_addNumber(&client, "pressure", pressure, "room2");
        rc = zclient_markDataPointAsError(&client, "mositure", "room2");
        rc = zclient_markDataPointAsError(&client, "air_quality");
        rc = zclient_addString(&client, "status", "OK");

        //payload = zclient_getpayload();
        //rc = zclient_publish(&client, payload);
        rc = zclient_dispatch(&client);
        rc = zclient_yield(&client, 300);
        if (rc == ZCONNECTION_ERROR)
        {
            break;
        }
    }

    zclient_disconnect(&client);
    return 0;
}