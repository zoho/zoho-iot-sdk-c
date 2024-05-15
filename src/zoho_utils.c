#include "zoho_utils.h"
#include <cJSON.h>
#include "zoho_log.h"
#include "sdk_version.h"
void cloneString(char **clone, char *source)
{
    if (source == NULL) {
        *clone = NULL;
        return;
    }

    size_t length = strlen(source);
    *clone = (char *)malloc((length + 1) * sizeof(char)); // +1 for the null terminator
    if (*clone == NULL) {
        // Failed to allocate memory
        log_error("Failed to allocate memory for string clone");
        return;
    }

    strcpy(*clone, source);
}

char *trim(char *s)
{
  int l = strlen(s);

  while (isspace(s[l - 1]))
  {
    --l;
  }
  while (*s && isspace(*s))
  {
    ++s, --l;
  }
  return strndup(s, l);
}

int isStringValid(char *string)
{
  return (string == NULL || strcmp(string, "") == 0) ? 0 : 1;
}

int getRetryInterval(int curr_delay)
{
  if ( curr_delay <= 0)
  {
    return MIN_RETRY_INTERVAL;
  }
  if (curr_delay < MAX_RETRY_INTERVAL)
  {
    curr_delay = curr_delay * 2;
    return (int)curr_delay;
  }
  else
  {
    return MAX_RETRY_INTERVAL;
  }
}

 char *getSdkVersion(){
   return strdup(Z_SDK_VERSION);

 }

 bool getOsnameOsversion(char * osName,char * osVersion){

    FILE *fp;
    char line[1024];
    char *name = NULL;
    char *version = NULL;
    
    fp = fopen("/etc/os-release", "r");
    if (fp == NULL) {
        log_error("Error opening file");
        return false;
    }

    while (fgets(line, 1024, fp) != NULL) {
        if (strstr(line, "NAME=") == line) {
            name = strchr(line, '"') + 1;
            strtok(name, "\"\n");
            strcpy(osName,name);
        } else if (strstr(line, "VERSION=") == line) {
            version = strchr(line, '"') + 1;
            strtok(version, "\"\n"); 
            strcpy(osVersion,version);
        }
        if (name != NULL && version != NULL) {
            break;
        }
    }

    fclose(fp);
    
    if (name == NULL || version == NULL) {
      log_error("Error in finding os name and version");
      return false;
    }
    log_debug("Os_name and version got successfully. \nOs_name = %s \nOs_version = %s",osName,osVersion);
    return true;


 }