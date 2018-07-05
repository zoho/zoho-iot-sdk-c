#include "utils.h"
void cloneString(char **clone, char *source)
{
    int len = strlen(source);

    if (len > 0)
    {
        *clone = (char *)malloc(sizeof(char) * (len + 1));
        strcpy(*clone, source);
    }
    else
    {
        //TODO: log error
    }
}