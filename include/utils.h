#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

void cloneString(char **clone, char *source);
char **stringSplit(const char *str, char separator,int *size);
char *copyWord(char *str, char *dest, char separator);
char *trim(char * s);