#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "zoho_log.h"
#include "zoho_utils.h"

// common static logconfig structure that the user can get using the function getZlogger() and configure the logging properties
static ZlogConfig logConfig;

static void lock(void)
{
  if (Zlog.lock)
  {
    Zlog.lock(Zlog.udata, 1);
  }
}

static void unlock(void)
{
  if (Zlog.lock)
  {
    Zlog.lock(Zlog.udata, 0);
  }
}

void log_set_udata(void *udata)
{
  Zlog.udata = udata;
}

void log_set_lock(log_LockFn fn)
{
  Zlog.lock = fn;
}

void log_set_fp(FILE *fp)
{
  Zlog.fp = fp;
}

void log_set_level(int level)
{
  Zlog.level = level;
}

void log_set_quiet(int enable)
{
  Zlog.quiet = enable ? 1 : 0;
}

void log_set_fileLog(int enable)
{
  Zlog.fileLog = enable ? 1 : 0;
}

void log_set_logPath(char *path)
{
  Zlog.logPath = path;
}

void log_set_logPrefix(char *prefix)
{
  Zlog.logPrefix = prefix;
}

void log_set_maxLogSize(int size)
{
  Zlog.maxLogFileSize = size;
}

void log_set_maxRollingLog(int size)
{
  Zlog.maxRollingLogFile = size;
}

void log_initialize(ZlogConfig *logConfig)
{
  //TODO: make ERROR as default level
  log_set_level(Z_LOG_LEVEL);
  if (logConfig == NULL)
  {
#if defined(Z_LOGGING)
    log_set_fileLog(1);
    log_set_logPath(LOG_PATH);
    log_set_logPrefix(LOG_PREFIX);
    log_set_maxLogSize(MAX_LOG_FILE_SIZE);
    log_set_maxRollingLog(MAX_ROLLING_LOG_FILE);
#endif 
  }
  else
  {
    log_free();
    log_set_quiet(logConfig->setQuiet);
    log_set_fileLog(logConfig->enableFileLog);
    (logConfig->logPath == NULL || !isStringValid(logConfig->logPath)) ? log_set_logPath(LOG_PATH) : log_set_logPath(logConfig->logPath);
    (logConfig->logPrefix == NULL || !isStringValid(logConfig->logPrefix)) ? log_set_logPrefix(LOG_PREFIX) : log_set_logPrefix(logConfig->logPrefix);
    (logConfig->maxLogFileSize <= MAX_LOG_FILE_SIZE) ? log_set_maxLogSize(MAX_LOG_FILE_SIZE) : log_set_maxLogSize(logConfig->maxLogFileSize);
    log_set_maxRollingLog(logConfig->maxRollingLogFile);
    (logConfig->level >= 0 && logConfig->level <= 5) ? log_set_level(logConfig->level):log_set_level(Z_LOG_LEVEL);
  }
  if (Zlog.fileLog)
  {
    char currentLogFile[100];
    sprintf(currentLogFile, "%s%s%s", Zlog.logPath, Zlog.logPrefix, LOG_FORMAT);
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
  Zlog.fp = NULL;
}

void log_log(int level, const char *file, int line, const char *fmt, ...)
{
  if (level < Zlog.level)
  {
    return;
  }

  /* Acquire lock */
  lock();

  /* Get current time */
  time_t t = time(NULL);
  struct tm *lt = localtime(&t);

  /* Log to stderr */
  if (!Zlog.quiet)
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
  if (Zlog.fileLog)
  {
    va_list args;
    char buf[32];
    buf[strftime(buf, sizeof(buf), "%d %b %Y %X", lt)] = '\0';
    // buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';

    char currentLogFile[100], tempFile[100];
    int size = 0;
    if (Zlog.fp)
    {
      struct stat buf;
      fstat(fileno(Zlog.fp), &buf);
      size = buf.st_size;
      // Implementation to check if the file is currently available 
      if (buf.st_nlink == 0)
      {
        log_free();
      }
    }

    if (!Zlog.fp)
    {
      sprintf(currentLogFile, "%s%s%s", Zlog.logPath, Zlog.logPrefix, LOG_FORMAT);
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

    if (size > Zlog.maxLogFileSize)
    {
      log_free();

      if (Zlog.maxRollingLogFile == 0)
      {
        sprintf(currentLogFile, "%s%s%s", Zlog.logPath, Zlog.logPrefix, LOG_FORMAT);
        remove(currentLogFile);
      }
      else
      {
        int i;
        for (i = (Zlog.maxRollingLogFile - 1); i >= 0; i--)
        {
          if (i == 0)
          {
            sprintf(currentLogFile, "%s%s%s", Zlog.logPath, Zlog.logPrefix, LOG_FORMAT);
          }
          else
          {
            sprintf(currentLogFile, "%s%s-%d%s", Zlog.logPath, Zlog.logPrefix, i, LOG_FORMAT);
          }
          sprintf(tempFile, "%s%s-%d%s", Zlog.logPath, Zlog.logPrefix, i + 1, LOG_FORMAT);
          rename(currentLogFile, tempFile);
        }
      }
      sprintf(currentLogFile, "%s%s%s", Zlog.logPath, Zlog.logPrefix, LOG_FORMAT);
      log_file = fopen(currentLogFile, "a");
      log_set_fp(log_file);
    }

    fprintf(Zlog.fp, "%s [%-5s] %s:%d: ", buf, level_names[level], file, line);
    va_start(args, fmt);
    vfprintf(Zlog.fp, fmt, args);
    va_end(args);
    fprintf(Zlog.fp, "\n");
    fflush(Zlog.fp);
  }

  /* Release lock */
  unlock();
}

ZlogConfig *getZlogger()
{
  return &logConfig;
}
