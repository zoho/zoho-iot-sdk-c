#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "zoho_log.h"

static void lock(void)
{
  if (L.lock)
  {
    L.lock(L.udata, 1);
  }
}

static void unlock(void)
{
  if (L.lock)
  {
    L.lock(L.udata, 0);
  }
}

void log_set_udata(void *udata)
{
  L.udata = udata;
}

void log_set_lock(log_LockFn fn)
{
  L.lock = fn;
}

void log_set_fp(FILE *fp)
{
  L.fp = fp;
}

void log_set_level(int level)
{
  L.level = level;
}

void log_set_quiet(int enable)
{
  L.quiet = enable ? 1 : 0;
}

void log_set_fileLog(int enable)
{
  L.fileLog = enable ? 1 : 0;
}

void log_set_logPath(char *path)
{
  L.logPath = path;
}

void log_set_logPrefix(char *prefix)
{
  L.logPrefix = prefix;
}

void log_set_maxLogSize(int size)
{
  L.maxLogFileSize = size;
}

void log_set_maxRollingLog(int size)
{
  L.maxRollingLogFile = size;
}

void log_initialize()
{
  //TODO: make ERROR as default level
#if defined(Z_LOGGING)
  log_set_fileLog(1);
  log_set_logPath(LOG_PATH);
  log_set_logPrefix(LOG_PREFIX);
  log_set_maxLogSize(MAX_LOG_FILE_SIZE);
  log_set_maxRollingLog(MAX_ROLLING_LOG_FILE);
  log_set_level(Z_LOG_LEVEL);
#endif 

  if (L.fileLog)
  {
    char currentLogFile[100];
    sprintf(currentLogFile, "%s%s%s", L.logPath, L.logPrefix, LOG_FORMAT);
    log_file = fopen(currentLogFile, "a");
    if (log_file)
    {
      log_set_fp(log_file);
    }
    else
    {
      log_warn("Error opening log file. Please check the permissions");
      log_set_fileLog(0);
    }
  }
}

void log_free()
{
  if (log_file)
  {
    fclose(log_file);
  }
  L.fp = NULL;
}

void log_log(int level, const char *file, int line, const char *fmt, ...)
{
  if (level < L.level)
  {
    return;
  }

  /* Acquire lock */
  lock();

  /* Get current time */
  time_t t = time(NULL);
  struct tm *lt = localtime(&t);

  /* Log to stderr */
  if (!L.quiet)
  {
    va_list args;
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';
    // #ifdef LOG_USE_COLOR
    fprintf(
        stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
        buf, level_colors[level], level_names[level], file, line);
    // #else
    //     fprintf(stderr, "%s %-5s %s:%d: ", buf, level_names[level], file, line);
    // #endif
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
  }

  /* Log to file */
  if (L.fileLog)
  {
    va_list args;
    char buf[32];
    buf[strftime(buf, sizeof(buf), "%d %b %Y %X", lt)] = '\0';
    // buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';

    char currentLogFile[100], tempFile[100];
    int size = 0;
    if (L.fp)
    {
      struct stat buf;
      fstat(fileno(L.fp), &buf);
      size = buf.st_size;
      // Implementation to check if the file is currently available
      if (buf.st_nlink == 0)
      {
        log_free();
      }
    }

    if (!L.fp)
    {
      sprintf(currentLogFile, "%s%s%s", L.logPath, L.logPrefix, LOG_FORMAT);
      log_file = fopen(currentLogFile, "a");
      if (log_file)
      {
        log_set_fp(log_file);
      }
      else
      {
        log_warn("Error opening log file. Please check the permissions");
        log_set_fileLog(1);
      }
    }

    if (size > L.maxLogFileSize)
    {
      log_free();

      if (L.maxRollingLogFile == 0)
      {
        sprintf(currentLogFile, "%s%s%s", L.logPath, L.logPrefix, LOG_FORMAT);
        remove(currentLogFile);
      }
      else
      {
        int i;
        for (i = (L.maxRollingLogFile - 1); i >= 0; i--)
        {
          if (i == 0)
          {
            sprintf(currentLogFile, "%s%s%s", L.logPath, L.logPrefix, LOG_FORMAT);
          }
          else
          {
            sprintf(currentLogFile, "%s%s-%d%s", L.logPath, L.logPrefix, i, LOG_FORMAT);
          }
          sprintf(tempFile, "%s%s-%d%s", L.logPath, L.logPrefix, i + 1, LOG_FORMAT);
          rename(currentLogFile, tempFile);
        }
      }
      sprintf(currentLogFile, "%s%s%s", L.logPath, L.logPrefix, LOG_FORMAT);
      log_file = fopen(currentLogFile, "a");
      log_set_fp(log_file);
    }

    fprintf(L.fp, "%s [%-5s] %s:%d: ", buf, level_names[level], file, line);
    va_start(args, fmt);
    vfprintf(L.fp, fmt, args);
    va_end(args);
    fprintf(L.fp, "\n");
    fflush(L.fp);
  }

  /* Release lock */
  unlock();
}
