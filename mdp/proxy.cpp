#include "mdp_common.h"
#include "mdp_broker.hpp"
#include "proxy.hpp"

//  Print formatted string to stdout, prefixed by date/time and
//  terminated with a newline.
void
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
int64_t
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

//  Finally here is the main task. We create a new broker instance and
//  then processes messages on the broker socket.
int main (int argc, char *argv [])
{
    int verbose = (argc > 1 && (0 == strncmp(argv [1], "-v", 3)));
    ustring sender;
    ustring empty;
    ustring header;
    int more;           //  Multipart detection

    try
    {
      s_version_assert (2, 1);
      s_catch_signals ();

      Broker *broker = new Broker(verbose);
      broker->bind ("tcp://*:5555");

      broker->start_brokering();

      delete broker;
    }
    catch (zmq::error_t err)
    {
        std::cout << "E: " << err.what() << std::endl;
    }

    if (s_interrupted)
        printf ("W: interrupt received, shutting down...\n");

    return 0;
}

