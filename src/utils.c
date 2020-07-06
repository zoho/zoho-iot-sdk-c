#include "utils.h"
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
