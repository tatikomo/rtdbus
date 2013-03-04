/*  =========================================================================
    mdp_helpers.hpp - ZeroMQ helpers

    =========================================================================
*/


#ifndef __MDP_HELPERS_HPP_INCLUDED__
#define __MDP_HELPERS_HPP_INCLUDED__

//  Include a bunch of headers that we will need in the examples

#include <zmq.hpp>

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>        // random()  RAND_MAX
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

//  Bring Windows MSVC up to C99 scratch
#if (defined (__WINDOWS__))
    typedef unsigned long ulong;
    typedef unsigned int  uint;
    typedef __int64 int64_t;
#endif

//  Provide random number from 0..(num-1)
#define within(num) (int) ((float) (num) * random () / (RAND_MAX + 1.0))

//  Receive 0MQ string from socket and convert into string
static std::string
s_recv (zmq::socket_t & socket) {

    zmq::message_t message;
    socket.recv(&message);

    return std::string(static_cast<char*>(message.data()), message.size());
}

//  Convert string to 0MQ string and send to socket
static bool
s_send (zmq::socket_t & socket, const std::string & string) {

    zmq::message_t message(string.size());
    memcpy(message.data(), string.data(), string.size());

    bool rc = socket.send(message);
    return (rc);
}

//  Sends string as 0MQ string, as multipart non-terminal
static bool
s_sendmore (zmq::socket_t & socket, const std::string & string) {

    zmq::message_t message(string.size());
    memcpy(message.data(), string.data(), string.size());

    bool rc = socket.send(message, ZMQ_SNDMORE);
    return (rc);
}

//  Receives all message parts from socket, prints neatly
//
static void
s_dump (zmq::socket_t & socket)
{
    std::cout << "----------------------------------------" << std::endl;

    while (1) {
        //  Process all parts of the message

        zmq::message_t message;
        socket.recv(&message);

        //  Dump the message as text or binary
        int size = message.size();
        std::string data(static_cast<char*>(message.data()), size);

        bool is_text = true;

        int char_nbr;
        unsigned char byte;
        for (char_nbr = 0; char_nbr < size; char_nbr++) {
            byte = data [char_nbr];
            if (byte < 32 || byte > 127)
              is_text = false;
        }

        std::cout << "[" << std::setfill('0') << std::setw(3) << size << "]";

        for (char_nbr = 0; char_nbr < size; char_nbr++) {
            if (is_text) {
                std::cout << (char)data [char_nbr];
            } else {
                std::cout << std::setfill('0') << std::setw(2)
                   << std::hex << (unsigned int) data [char_nbr];
            }
        }
        std::cout << std::endl;

        int more;           //  Multipart detection
        size_t more_size = sizeof (more);
        socket.getsockopt(ZMQ_RCVMORE, &more, &more_size);

        if (!more)
            break;      //  Last message part
    }
}

//  Set simple random printable identity on socket
//
inline std::string
s_set_id (zmq::socket_t & socket)
{
    std::stringstream ss;
    ss << std::hex << std::uppercase
          << std::setw(4) << std::setfill('0') << within (0x10000) << "-"
          << std::setw(4) << std::setfill('0') << within (0x10000);
    socket.setsockopt(ZMQ_IDENTITY, ss.str().c_str(), ss.str().length());
    return ss.str();
}

//  Report 0MQ version number
//
static void
s_version (void)
{
    int major, minor, patch;
    zmq_version (&major, &minor, &patch);
    std::cout << "Current 0MQ version is " << major << "." << minor << "." << patch << std::endl;
}

static void
s_version_assert (int want_major, int want_minor)
{
    int major, minor, patch;
    zmq_version (&major, &minor, &patch);
    if (major < want_major
    || (major == want_major && minor < want_minor)) {
        std::cout << "Current 0MQ version is " << major << "." << minor << std::endl;
        std::cout << "Application needs at least " << want_major << "." << want_minor
              << " - cannot continue" << std::endl;
        exit (EXIT_FAILURE);
    }
}

//  Return current system clock as milliseconds
static int64_t
s_clock (void)
{
#if (defined (__WINDOWS__))
    SYSTEMTIME st;
    GetSystemTime (&st);
    return (int64_t) st.wSecond * 1000 + st.wMilliseconds;
#else
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) ((int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000);
#endif
}

// Log out to stdout
static void
s_console (const char *format, ...)
{
    time_t curtime = time (NULL);
    struct tm *loctime = localtime (&curtime);
    char *formatted = new char[20];
    strftime (formatted, 20, "%y-%m-%d %H:%M:%S ", loctime);
    printf ("%s", formatted);
    delete[] formatted;

    va_list argptr;
    va_start (argptr, format);
    vprintf (format, argptr);
    va_end (argptr);
    printf ("\n");
    fflush(stdout);
}

//  Sleep for a number of milliseconds
static void
s_sleep (int msecs)
{
#if (defined (__WINDOWS__))
    Sleep (msecs);
#else
    struct timespec t;
    t.tv_sec = msecs / 1000;
    t.tv_nsec = (msecs % 1000) * 1000000;
    nanosleep (&t, NULL);
#endif
}

#endif
