#include "utils.h"
void cloneString(char **clone, char *source)
{
    if (source != NULL)
    {
        int len = strlen(source);
        *clone = (char *)malloc(sizeof(char) * (len + 1));
        strcpy(*clone, source);
    }
    else
    {
        //TODO: log error
    }
}