#include "zoho_utils.h"
#include <cJSON.h>
void cloneString(char **clone, char *source)
{
  *clone = cJSON_CreateString(source)->valuestring;
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