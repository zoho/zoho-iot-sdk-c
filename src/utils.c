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

char *copyWord(char *str, char *dest, char separator)
{
  for (; *str != separator && *str; str++)
  {
    *dest = *str;
    dest++;
  }
  *dest = '\0';
  return str;
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

char **stringSplit(const char *str, char separator,int *size)
{
  int i = 0;
  int count = 0;
  /*counts total number of words in the string and reserves space for them*/
  char *copy = (char *)str;
  char **split;
  if (*str != separator)
  {
    count++;
  }
  for (; *str; str++)
  {
    if (*str == separator)
    {
      for (; *str == separator; str++)
        ;
      if (*str != '\0')
        count++;
    }
  }
  split = malloc(sizeof(char *) * (count + 1));
  split[count] = NULL;

  /*while copy is not equal to \0*/
  while (*copy)
  {
    if (*copy == separator)
    {
      /*jumps over all consecutive seperator and returns pointer of non-seperator character*/
      for (; *copy == separator; copy++)
        ;
    }
    else
    {
      /*counts the number of letters in a word and allocates space for it*/
      char *dup = copy;
      int count;
      for (count = 0; *dup != separator && *dup; dup++)
      {
        /*count number of letters in the word*/
        count++;
      }
      split[i] = malloc(sizeof(char) * (count + 1));

      /*copies word into the newly allocated space and returns a pointer to the location where word ends*/
      copy = copyWord(copy, split[i], separator);
      i++;
    }
  }
  *size = count;
  return split;
}