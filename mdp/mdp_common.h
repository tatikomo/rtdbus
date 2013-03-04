//
//  mdp_common.h
//  Majordomo Protocol definitions
//
#ifndef __MDP_H_INCLUDED__
#define __MDP_H_INCLUDED__

#include <stdio.h>      // sprintf
#include <stdarg.h>     // va_args
#include <stdint.h>     // int64_t
#include <string>

static const char * EMPTY_FRAME = "";

//  This is the version of MDP/Client we implement
static const char * MDPC_CLIENT = "MDPC0X";

//  MDP/Client commands, as strings
#define MDPC_REQUEST        "\001"
#define MDPC_REPORT         "\002"
#define MDPC_NAK            "\003"

static char *mdpc_commands [] = {
    '\0', (char*)"REQUEST", (char*)"REPORT", (char*)"NAK",
};

//  This is the version of MDP/Worker we implement
static const char * MDPW_WORKER = "MDPW0X";

//  MDP/Server commands, as strings
#define MDPW_READY          "\001"
#define MDPW_REQUEST        "\002"
#define MDPW_REPORT         "\003"
#define MDPW_HEARTBEAT      "\004"
#define MDPW_DISCONNECT     "\005"

//  We'd normally pull these from config data
#define HEARTBEAT_LIVENESS  4       //  3-5 is reasonable
#define HEARTBEAT_INTERVAL  2500     //  2500 msecs -> 2,5 sec
#define HEARTBEAT_EXPIRY    HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS

static char *mdpw_commands [] = {
    '\0', (char*)"READY", (char*)"REQUEST", (char*)"REPORT", (char*)"HEARTBEAT", (char*)"DISCONNECT"
};

//  Print formatted string to stdout, prefixed by date/time and
//  terminated with a newline.
static void
zclock_log (const char *format, ...)
{
    time_t curtime = time (NULL);
    struct tm *loctime = localtime (&curtime);
    char formatted [20];
    strftime (formatted, 20, "%y-%m-%d %H:%M:%S ", loctime);
    printf ("%s", formatted);

    va_list argptr;
    va_start (argptr, format);
    vprintf (format, argptr);
    va_end (argptr);
    printf ("\n");
    fflush (stdout);
}

//  Return current system clock as milliseconds
static int64_t
zclock_time (void)
{
#if defined (__UNIX__)
    struct timeval tv;
    gettimeofday (&tv, NULL);

    return (int64_t) ((int64_t) tv.tv_sec * 1000 + (int64_t) tv.tv_usec / 1000);
#elif (defined (__WINDOWS__))
    FILETIME ft;
    GetSystemTimeAsFileTime (&ft);
    return s_filetime_to_msec (&ft);
#endif
}

#endif

