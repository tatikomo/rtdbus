#if OS_TYPE == LINUX
#include <unistd.h>
#include <sys/syscall.h> // gettid()
#endif

#include <pthread.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "helper.hpp"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void LogError(int rc,
            const char *functionName,
            const char *format,
            ...)
{
    const char *empty = "";
    const char *pre_format = "E %s [%d]: ";
    char buffer[255];
    char user_msg[255];
    va_list args;

    pthread_mutex_lock(&mutex);
#if OS_TYPE == LINUX
    printf ("[%ld] ", (long int)syscall(SYS_gettid));
#else
    printf ("[%lu] ", pthread_self());
#endif

    sprintf(buffer, pre_format, 
            functionName? functionName : empty,
            rc);

    va_start (args, format);
    vsprintf (user_msg, format, args);
    strncat(buffer, user_msg, sizeof(buffer)-1);
    fprintf(stderr, "%s\n", buffer);
    fflush(stderr);
    va_end (args);
    pthread_mutex_unlock(&mutex);
}

void LogWarn(
            const char *functionName,
            const char *format,
            ...)
{
    const char *empty = "";
    const char *pre_format = "W %s: ";
    char buffer[255];
    char user_msg[255];
    va_list args;

    pthread_mutex_lock(&mutex);
    sprintf(buffer, pre_format, functionName? functionName : empty);

    va_start (args, format);
    vsprintf (user_msg, format, args);
    strncat(buffer, user_msg, sizeof(buffer)-1);
#if OS_TYPE == LINUX
    fprintf(stdout, "[%ld] %s\n", (long int)syscall(SYS_gettid), buffer);
#else
    fprintf(stdout, "[%lu] %s\n", pthread_self(), buffer);
#endif
    fflush(stdout);
    va_end (args);
    pthread_mutex_unlock(&mutex);
}

void LogInfo(
            const char *functionName,
            const char *format,
            ...)
{
#if defined DEBUG
    pthread_mutex_lock(&mutex);
    const char *empty = "";
    const char *pre_format = "I %s: ";
    char buffer[255];
    char user_msg[255];
    va_list args;

    sprintf(buffer, pre_format, functionName? functionName : empty);

    va_start (args, format);
    vsprintf (user_msg, format, args);
    strncat(buffer, user_msg, sizeof(buffer)-1);
#if OS_TYPE == LINUX
    fprintf(stdout, "[%ld] %s\n", (long int)syscall(SYS_gettid), buffer);
#else
    fprintf(stdout, "[%lu] %s\n", pthread_self(), buffer);
#endif
    fflush(stdout);
    va_end (args);
    pthread_mutex_unlock(&mutex);
#else
  ;
#endif
}

void hex_dump(const std::string& data)
{
   char buf[4000];
   int offset;
   int is_text;
   unsigned int char_nbr;

   // Dump the message as text or binary
   is_text = 1;
   for (char_nbr = 0; char_nbr < data.size(); char_nbr++)
       if (data [char_nbr] < 32 || data [char_nbr] > 127)
           is_text = 0;

   offset = sprintf(buf, "[%03d] ", (int) data.size());
   for (char_nbr = 0; char_nbr < data.size(); char_nbr++)
   {
       if (is_text) 
       {
           strcpy((char*)(&buf[0] + offset++), (const char*) &data [char_nbr]);
       }
       else 
       {
           snprintf(&buf[offset], 3, "%02X", (unsigned char)data[char_nbr]);
           offset += 2;
       }
   }
   buf[offset] = '\0';
   fprintf(stdout, "%s\n", buf); fflush(stdout);
}

