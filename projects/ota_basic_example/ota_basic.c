#include "zoho_iot_client.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#if defined(Z_SECURE_CONNECTION)
#if defined(EMBED_MODE)
#include "zclient_certificates.h"
#endif
#endif

//version will be sent in the telemetry as dummy data
#define VERSION "1.0.0"
volatile int ctrl_flag = 0;

// Zoho client struct to handle the Communication with the HUB
ZohoIOTclient client;

#define MQTT_USER_NAME "<MQTT User Name>"
#define MQTT_PASSWORD "<MQTT Password/Token>"

// Frequency in which the data needs to be published
#define POLL_FREQUENCY 5 // publish data every 5 sec

#define SHELL_MAX_BUFFER_SIZE 1024

// executing shell command from c
int executeShellCommand(char *command, char *response)
{

    FILE *fp;
    fp = popen(command, "r");
    if (fp == NULL)
    {
        log_error("Error in opening shell");
        strcpy(response, "Error in opening shell");
        return -1;
    }
    char shellBuffer[SHELL_MAX_BUFFER_SIZE] = "";
    size_t bytesRead = fread(shellBuffer, 1, SHELL_MAX_BUFFER_SIZE - 1, fp); // last byte for null
    if (bytesRead == 0 && !feof(fp))
    {
        strcpy(response, "Error reading from pipe");
        pclose(fp);
        return -1;
    }
    shellBuffer[bytesRead] = '\0';
    strcpy(response, shellBuffer);
    int status = pclose(fp);
    if (status != 0)
    {
        return -1;
    }

    return 0;
}

// OTA will be executed in the following sequence
// 1. Download the package from the given url
// 2. Validate the hash of the downloaded package(optional)
// 3. Extract the package
// 4. Execute the zparent.sh
// 5. Publish the OTA acknowledgment message

// Downloading the package from the given url
int downloadPackage(char *url, char *response)
{

    if (url == NULL || strlen(url) <= 0)
    {
        log_error("OTA url null");
        strcpy(response, "OTA url is null");
        return -1;
    }

    char command[1000] = "wget -O /tmp/zoho_iot_ota.tar.gz ";
    strcat(command, url);
    strcat(command, " 2>&1");
    log_debug("Downloading : %s\n", command);
    return executeShellCommand(command, response);
}

// Validating the hash of the downloaded package
int validateHash(const char *expectedHash, char *response)
{
    log_debug("Validating hash");
    char hash[SHELL_MAX_BUFFER_SIZE] = "";
    int rc = executeShellCommand("openssl sha256 /tmp/zoho_iot_ota.tar.gz | awk '{print $2}' | tr -d '\n' 2>&1", hash);
    if (rc != 0)
    {
        strcpy(response, "Error calculating hash");
        return -1;
    }
    if (strcmp(hash, expectedHash) != 0)
    {
        sprintf(response, "Hash mismatch: expected %s, got %s", expectedHash, hash);
        return -1;
    }
    return 0;
}

// Extracting the package in root directory
int extractPackage(char *response)
{
    log_debug("Extracting package");
    return executeShellCommand("tar -xzf /tmp/zoho_iot_ota.tar.gz -C /tmp/ 2>&1", response);
}

// Executing the zparent.sh for OTA
int executeOTA(char *correlation_id, char *response)
{
    log_debug("Executing OTA");
    char command[1000] = "/tmp/zoho_iot_ota/zparent.sh ";
    strcat(command, correlation_id);
    strcat(command, " 2>&1");
    return executeShellCommand(command, response);
}

// OTA Callback function
void message_OTA_handler(char *url, char *hash, bool validity_check, char *correlation_id)
{
    //OTA will kill the existing process and start the new process, 
    //so take necessary action before starting the OTA process

    log_info("OTA url : %s", url);
    log_info("OTA hash : %s", hash);
    log_info("OTA validity check : %d", validity_check);
    log_info("OTA correlation_id : %s", correlation_id);
    char response[1000];
    int rc = downloadPackage(url, response);
    if (rc != 0)
    {
        log_error("OTA download failed : %s", response);
        // publish OTA acknowledgment message
        zclient_publishOTAAck(&client, correlation_id, EXECUTION_FAILURE, response);
        return;
    }
    // clear response for next command
    memset(response, 0, sizeof(response));
    if (validity_check)
    {
        // validate hash
        rc = validateHash(hash, response);
        if (rc != 0)
        {
            log_error("OTA hash validation failed : %s", response);
            // publish OTA acknowledgment message
            zclient_publishOTAAck(&client, correlation_id, EXECUTION_FAILURE, response);
            // remove the downloaded package
            executeShellCommand("rm -f /tmp/zoho_iot_ota.tar.gz", response);
            return;
        }
    }
    // clear response for next command
    memset(response, 0, sizeof(response));
    rc = extractPackage(response);
    if (rc != 0)
    {
        log_error("OTA extraction failed : %s", response);
        // publish OTA acknowledgment message
        zclient_publishOTAAck(&client, correlation_id, EXECUTION_FAILURE, response);
        // remove the downloaded package
        executeShellCommand("rm -f /tmp/zoho_iot_ota.tar.gz", response);
        return;
    }
    // remove the source package since it is extracted
    executeShellCommand("rm -f /tmp/zoho_iot_ota.tar.gz", response);
    // clear response for next command
    memset(response, 0, sizeof(response));
    // check zparent.sh exists
    if (access("/tmp/zoho_iot_ota/zparent.sh", F_OK) != -1)
    {
        // execute zparent.sh
        rc = executeOTA(correlation_id, response);
        if (rc != 0)
        {
            log_error("OTA zparent.sh execution failed : %s", response);
            // publish OTA acknowledgment message
            zclient_publishOTAAck(&client, correlation_id, EXECUTION_FAILURE, response);
            // remove the package directory
            executeShellCommand("rm -rf /tmp/zoho_iot_ota", response);
            return;
        }
    }
    else
    {
        log_error("zparent.sh not found");
        // publish OTA acknowledgment message
        zclient_publishOTAAck(&client, correlation_id, EXECUTION_FAILURE, "zparent.sh not found in OTA package");
        // remove the package directory
        executeShellCommand("rm -rf /tmp/zoho_iot_ota", response);
        return;
    }
}

void check_OTA_state()
{
    if (access("/tmp/zoho_iot_ota_status.txt", F_OK) != -1)
    {
        // Wait for 30 seconds to get the status file
        sleep(30);
        FILE *fp = fopen("/tmp/zoho_iot_ota_status.txt", "r");
        if (fp == NULL)
        {
            log_error("Error in opening file");
            return;
        }

        char buffer[100];
        // Read first line of the file for correlation_id
        if (fgets(buffer, sizeof(buffer), fp) != NULL)
        {

            log_info("OTA correlation_id: %s", buffer);
            char correlation_id[100];
            strncpy(correlation_id, buffer, sizeof(correlation_id));
            // Remove newline character if present
            size_t len = strlen(correlation_id);
            if (len > 0 && correlation_id[len - 1] == '\n')
            {
                correlation_id[len - 1] = '\0';
            }

            // Read second line of the file for status
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                log_info("OTA status: %s", buffer);
                // Publish OTA acknowledgment message based on status
                if (strcmp(buffer, "success\n") == 0)
                {
                    zclient_publishOTAAck(&client, correlation_id, SUCCESFULLY_EXECUTED, "OTA update success");
                }
                else
                {
                    //Read third line of the file for response
                    if (fgets(buffer, sizeof(buffer), fp) != NULL)
                    {
                        zclient_publishOTAAck(&client, correlation_id, EXECUTION_FAILURE, buffer);
                    }
                    else
                    {
                        zclient_publishOTAAck(&client, correlation_id, EXECUTION_FAILURE, "OTA update failed");
                    }
                }
            }
            else
            {
                log_error("OTA status not found");
            }
        }
        fclose(fp);
        // Remove the status file
        remove("/tmp/zoho_iot_ota_status.txt");
        //remove log file if required
        //remove("/tmp/zoho_ota_parent.log");
        //remove("/tmp/zoho_ota_update.log");
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
    log_debug("Second level Command Ack status : %d", zclient_publishCommandAck(&client, payload, SUCCESFULLY_EXECUTED, "Command based task Executed."));
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

// Function gets the current time in seconds
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
    unsigned long long publish_time = 0;
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

    ZlogConfig *logConfig = getZlogger();
    logConfig->enableFileLog = 1;
    logConfig->logPrefix = "ota_basic";
    logConfig->logPath = "./";
    logConfig->maxLogFileSize = 100000; // File size in bytes
    logConfig->maxRollingLogFile = 2;

    // Update your DEVICE_ID AND AUTH_TOKEN below:
    //  IF the application is used in non-TLS mode then replace all the following variables pRootCACert, pDeviceCert, pDevicePrivateKey, pDeviceCertParsword as emtpy string ;
    rc = zclient_init(&client, MQTT_USER_NAME, MQTT_PASSWORD, CRT_PARSE_MODE, pRootCACert, pDeviceCert, pDevicePrivateKey, pDeviceCertParsword, logConfig);

    if (rc != ZSUCCESS)
    {
        return 0;
    }
    rc = zclient_connect(&client);
    while (rc != ZSUCCESS && ctrl_flag == 0)
    {
        // Endless reconnection on start. No data collection happens during connection error. 
        rc = zclient_reconnect(&client);
    }

    rc = zclient_command_subscribe(&client, message_command_handler);
    // Registering the OTA callback function
    rc = zclient_ota_handler(message_OTA_handler);
    // Check for OTA status file exist or not and publish the OTA acknowledgment message
    check_OTA_state();

    while (ctrl_flag == 0)
    {
        // If Connection is already available, then retryring to connect inside function is ignored
        rc = zclient_reconnect(&client);
        if (getTime() - publish_time > POLL_FREQUENCY)
        {
            if (client.current_state == CONNECTED)
            {
                // publish the version as dummy data
                rc = zclient_addString(&client, "version", VERSION);
                rc = zclient_dispatch(&client);
            }
            publish_time = getTime();
        }
        if (client.current_state == CONNECTED)
        {
            rc = zclient_yield(&client, 300);
        }
    }

    zclient_disconnect(&client);
    return 0;
}