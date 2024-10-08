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
#if defined(Z_LOG_COMPRESS)
#include <zlib.h>
#endif
#include <pthread.h>

FILE *log_file;
Z_log Zlog ={0};

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
// common static logconfig structure that the user can get using the function getZlogger() and configure the logging properties
static ZlogConfig logConfig;

#if defined(Z_CLOUD_LOGGING)
int NUM_LINES_TO_READ  = 100;
int numberOfLinesRead=0;
long sizeOfLogRead = 0;
#endif

#if defined(Z_LOG_COMPRESS)
bool enable_log_compression = true;
#define CHUNK 32768
void compressAndSaveFile(const char *sourceFileName, const char *compressedFileName) {
    gzFile compressedFile = gzopen(compressedFileName, "wb");
    FILE *sourceFile = fopen(sourceFileName, "rb");

    if (compressedFile == NULL || sourceFile == NULL) {
        if (compressedFile)
        {
         gzclose(compressedFile);
         printf("Zoho_log:can't create compress file %s",compressedFileName);
        }
        if (sourceFile)
        {
         fclose(sourceFile);
         printf("Zoho_log:can't able to open source file %s",sourceFileName);
        }
        return;
    }

    char buffer[CHUNK];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0) {
        if (gzwrite(compressedFile, buffer, bytesRead) == 0) {
            printf("Zoho_log:compression error");
            break;
        }
    }

    gzclose(compressedFile);
    fclose(sourceFile);

    if (bytesRead != 0) {
      printf("Zoho_log:corrupted compressed file, removing it");
      remove(compressedFileName); 
      remove(sourceFileName);
    } else {
        remove(sourceFileName); 
    }
}
#endif

static void lock(void)
{
pthread_mutex_lock(&log_mutex);
}

static void unlock(void)
{
  pthread_mutex_unlock(&log_mutex);
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
  log_free();
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
#else
    log_set_quiet(1);
#endif 
  }
  else
  {
    log_set_quiet(logConfig->setQuiet);
    log_set_fileLog(logConfig->enableFileLog);
    (logConfig->logPath == NULL || !isStringValid(logConfig->logPath)) ? log_set_logPath(LOG_PATH) : log_set_logPath(logConfig->logPath);
    (logConfig->logPrefix == NULL || !isStringValid(logConfig->logPrefix)) ? log_set_logPrefix(LOG_PREFIX) : log_set_logPrefix(logConfig->logPrefix);
    log_set_maxRollingLog(logConfig->maxRollingLogFile);
    (logConfig->level >= 0 && logConfig->level <= 5) ? log_set_level(logConfig->level):log_set_level(Z_LOG_LEVEL);
    if(logConfig->logCompress == true)
    {
      #if defined(Z_LOG_COMPRESS)
      log_info("Enabled Log compression");
      if (LOG_INFO <= Zlog.level)
      {
        log_free();
      }
      enable_log_compression = true;
      #else
      log_warn("Log compression lib not included, continuing on without compression");
      if (LOG_WARN <= Zlog.level)
      {
        log_free();
      }
      #endif
    }
    else
    {
      #if defined(Z_LOG_COMPRESS)
      enable_log_compression = false;
      #endif
    }
    
    #if defined(Z_LOG_COMPRESS)
    if(enable_log_compression)
    {
      log_set_maxLogSize(logConfig->maxLogFileSize);
    }
    #endif
    if(logConfig->maxLogFileSize <= MAX_LOG_FILE_SIZE)
    {
      if(logConfig->maxLogFileSize <= MIN_LOG_FILE_SIZE)
      {
          log_set_maxLogSize(MIN_LOG_FILE_SIZE);
          log_warn("Log File size is lesserthan Mn file size \n continuing on default min size %d bytes",MIN_LOG_FILE_SIZE);
          if (LOG_WARN <= Zlog.level)
          {
            log_free();
          }
      }
      else
      {
        log_set_maxLogSize(logConfig->maxLogFileSize);
        log_info("Log File size: %d bytes",logConfig->maxLogFileSize);
        if (Zlog.level < LOG_INFO)
        {
          log_free();
        }
      }
    }
    else
    {
      log_warn("Log File size is greaterthan Max file size \n continuing on default max size %d bytes",MAX_LOG_FILE_SIZE);
      if (LOG_WARN <= Zlog.level)
      {
        log_free();
      }
      log_set_maxLogSize(MAX_LOG_FILE_SIZE);
    }
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
      log_set_fileLog(0);
      log_warn("Error opening log file. Please check the permissions");
      if (LOG_WARN <= Zlog.level)
      {
        log_free();
      }
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
        log_free();
        char *previousLogPath = NULL;
        previousLogPath = strdup(Zlog.logPath);
        Zlog.logPath = "./";
        sprintf(currentLogFile, "%s%s%s", Zlog.logPath, Zlog.logPrefix, LOG_FORMAT);
        log_file = fopen(currentLogFile, "a");
        if (log_file)
        {
          log_set_fp(log_file);
        }
        else
        {
          log_set_fileLog(0);
        }
        free(previousLogPath);
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
            #if defined(Z_LOG_COMPRESS)
            if(enable_log_compression)
            {
              sprintf(currentLogFile, "%s%s%s", Zlog.logPath, Zlog.logPrefix, ".gz");
              char compLogFile[100];
              sprintf(compLogFile, "%s%s%s", Zlog.logPath, Zlog.logPrefix, LOG_FORMAT);
              compressAndSaveFile(compLogFile,currentLogFile);
            }
            else{
              sprintf(currentLogFile, "%s%s%s", Zlog.logPath, Zlog.logPrefix, LOG_FORMAT);
            }
            #else
            sprintf(currentLogFile, "%s%s%s", Zlog.logPath, Zlog.logPrefix, LOG_FORMAT);
            #endif

          }
          else
          {
            #if defined(Z_LOG_COMPRESS)
            if(enable_log_compression)
            {
              sprintf(currentLogFile, "%s%s-%d%s", Zlog.logPath, Zlog.logPrefix, i, ".gz");
            }
            else{
              sprintf(currentLogFile, "%s%s-%d%s", Zlog.logPath, Zlog.logPrefix, i, LOG_FORMAT);
            }
            #else
            sprintf(currentLogFile, "%s%s-%d%s", Zlog.logPath, Zlog.logPrefix, i, LOG_FORMAT);
            #endif
          }
          #if defined(Z_LOG_COMPRESS)
          if(enable_log_compression)
          {
            sprintf(tempFile, "%s%s-%d%s", Zlog.logPath, Zlog.logPrefix, i + 1, ".gz");
          }
          else{
            sprintf(tempFile, "%s%s-%d%s", Zlog.logPath, Zlog.logPrefix, i + 1, LOG_FORMAT);
          }
          #else
          sprintf(tempFile, "%s%s-%d%s", Zlog.logPath, Zlog.logPrefix, i + 1, LOG_FORMAT);
          #endif
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

#if defined(Z_CLOUD_LOGGING)
cJSON * get_last_lines(FILE *fp, int num_lines,long long file_old_ending_position) {

    int lines_read = 0;
    char buffer[LINE_SIZE];
    int garbage_line_flag =0;
    fseek(fp, 0, SEEK_END);
    long long pointer_position = ftell(fp);
    long long file_new_ending_position = ftell(fp);

    cJSON * log_json_array = cJSON_CreateArray();
    if (log_json_array == NULL) {
        log_error("Failed to create JSON array\n");
        return NULL;
    }
    log_debug("Lines requested to read by command: %d", num_lines);
    log_debug("Maximum bytes in a file allowed to read : %f KB", MAXIMUM_READ/1000.0);

    while (pointer_position > 0 && lines_read < num_lines) {
        fseek(fp, --pointer_position, SEEK_SET);

        if((file_new_ending_position - pointer_position > MAXIMUM_READ) || (pointer_position < file_old_ending_position)){
            break;
        }
        if (fgetc(fp) == '\n') {
            // fgets(buffer, LINE_SIZE, fp);  //To read in reverse order
            // if(garbage_line_flag == 0){
            //     garbage_line_flag = 1;
            // }
            // else{
            //     cJSON *string_json = cJSON_CreateString(buffer);
            //     cJSON_AddItemToArray(log_json_array, string_json);
            //     log_error("%s", buffer); // Print the line
            // }
            lines_read++;
        }
    }
    numberOfLinesRead = lines_read;
    sizeOfLogRead = (file_new_ending_position - pointer_position)/1000;
    log_info("Cloud logging : size of file readed : %lld", file_new_ending_position - pointer_position);
    log_info("Cloud Logging: Actual number of lines_readed : %d", lines_read);
    while((fgets(buffer, LINE_SIZE, fp) != NULL) && lines_read > 0){
        cJSON *string_json = cJSON_CreateString(buffer);
        cJSON_AddItemToArray(log_json_array, string_json);
        lines_read--;
    }
    char sizeLengthString [100];
    sprintf(sizeLengthString,"Number of lines read: %d Size of log read: %lld KB",numberOfLinesRead,sizeOfLogRead);
    cJSON_AddItemToArray(log_json_array,cJSON_CreateString(sizeLengthString));
    return log_json_array;

}

void initialize_cloud_log()
{
  sprintf(ZcloudLog.currentLogFile, "%s%s%s", Zlog.logPath, Zlog.logPrefix, LOG_FORMAT);
  ZcloudLog.file = fopen(ZcloudLog.currentLogFile, "r");
  if (ZcloudLog.file == NULL) {
      log_error("Error opening file");
      return ;
  }
  ZcloudLog.file_old_ending_position = 0;
}

cJSON* get_cloud_log(){

    if(ZcloudLog.file == NULL){
        log_error("Error opening file");
        return NULL;
    }
    cJSON * log_json_array = NULL;
    fseek(ZcloudLog.file, 0, SEEK_END);
    ZcloudLog.file_new_ending_position = ftell(ZcloudLog.file);
    if ((ZcloudLog.file_new_ending_position>(Zlog.maxLogFileSize-5000)))
    {
        fclose(ZcloudLog.file);
        log_debug("Old File closed\n");
        ZcloudLog.file = fopen(ZcloudLog.currentLogFile, "r");
        if (ZcloudLog.file == NULL)
        {
            log_error("Error opening file");
            return NULL;
        }
        fseek(ZcloudLog.file, 0, SEEK_END);
        ZcloudLog.file_old_ending_position = 0;
        ZcloudLog.file_new_ending_position = ftell(ZcloudLog.file);
    }
    log_json_array =get_last_lines(ZcloudLog.file, NUM_LINES_TO_READ,ZcloudLog.file_old_ending_position);
    ZcloudLog.file_old_ending_position = ZcloudLog.file_new_ending_position;
    return log_json_array;
}

int cloud_logging_set_lines(char * line){
  int lines = atoi(line);
  if(lines<1){
    log_error("cloud logging: number of lines is invalid");
    return -1;
  }
  NUM_LINES_TO_READ = lines;
  return 0;
}

#endif