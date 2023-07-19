#include "zoho_utils.h"
#include <cJSON.h>
#include "zoho_log.h"
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