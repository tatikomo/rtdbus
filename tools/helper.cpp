#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "helper.hpp"

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

    sprintf(buffer, pre_format, 
            functionName? functionName : empty,
            rc);

    va_start (args, format);
    vsprintf (user_msg, format, args);
    strncat(buffer, user_msg, sizeof(buffer)-1);
    fprintf(stderr, "%s\n", buffer);
    fflush(stderr);
    va_end (args);
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

    sprintf(buffer, pre_format, functionName? functionName : empty);

    va_start (args, format);
    vsprintf (user_msg, format, args);
    strncat(buffer, user_msg, sizeof(buffer)-1);
    fprintf(stdout, "%s\n", buffer);
    fflush(stdout);
    va_end (args);
}

void LogInfo(
            const char *functionName,
            const char *format,
            ...)
{
#if defined DEBUG
    const char *empty = "";
    const char *pre_format = "I %s: ";
    char buffer[255];
    char user_msg[255];
    va_list args;

    sprintf(buffer, pre_format, functionName? functionName : empty);

    va_start (args, format);
    vsprintf (user_msg, format, args);
    strncat(buffer, user_msg, sizeof(buffer)-1);
    fprintf(stdout, "%s\n", buffer);
    fflush(stdout);
    va_end (args);
#else
  ;
#endif
}


