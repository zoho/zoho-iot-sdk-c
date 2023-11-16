#ifndef ZOHO_LOG_H
#define ZOHO_LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#define LOG_PATH "./"
#define LOG_PREFIX "zoho_SDK_logs"
#define LOG_FORMAT ".txt"
#define MAX_LOG_FILE_SIZE  5242880  //file size in Bytes 5MB MAX
#define MIN_LOG_FILE_SIZE  10240 // file size in Bytes 10KB MAX
#define MAX_ROLLING_LOG_FILE 2   // No of rolling log file in addition to the main

FILE *log_file;
typedef void (*log_LockFn)(void *udata, int lock);

static struct
{
    void *udata;
    log_LockFn lock;
    FILE *fp;
    int level;
    int quiet;
    int fileLog;
    char *logPath;
    char *logPrefix;
    int maxLogFileSize;
    int maxRollingLogFile;
} Zlog;

typedef struct
{
    int setQuiet;
    int level;
    int enableFileLog;
    char *logPath;
    char *logPrefix;
    int maxLogFileSize;
    int maxRollingLogFile;
    bool logCompress;
} ZlogConfig;

static const char *level_names[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

// #ifdef LOG_USE_COLOR
static const char *level_colors[] = {
    "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};
// #endif

enum
{
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define log_trace(...) log_log(LOG_TRACE, __FILENAME__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILENAME__, __LINE__, __VA_ARGS__)
#define log_info(...) log_log(LOG_INFO, __FILENAME__, __LINE__, __VA_ARGS__)
#define log_warn(...) log_log(LOG_WARN, __FILENAME__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILENAME__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILENAME__, __LINE__, __VA_ARGS__)

void log_set_udata(void *udata);
void log_set_lock(log_LockFn fn);
void log_set_fp(FILE *fp);
void log_set_level(int level);
void log_set_quiet(int enable);
void log_set_fileLog(int enable);
void log_set_logPath(char *path);
void log_set_logPrefix(char *prefix);
void log_set_maxLogSize(int size);
void log_set_maxRollingLog(int size);
void log_initialize(ZlogConfig *logConfig);
void log_free();
ZlogConfig *getZlogger();

void log_log(int level, const char *file, int line, const char *fmt, ...);

#endif