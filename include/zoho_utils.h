#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_RETRY_INTERVAL 1800
#define MIN_RETRY_INTERVAL 2

void cloneString(char **clone, char *source);
char *trim(char *s);
int isStringValid(char *value);
int getRetryInterval(int curr_delay);