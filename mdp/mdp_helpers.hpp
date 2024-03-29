/*  =========================================================================
    mdp_helpers.hpp - ZeroMQ helpers

    =========================================================================
*/


#ifndef __MDP_HELPERS_HPP_INCLUDED__
#define __MDP_HELPERS_HPP_INCLUDED__

#pragma once

//  Include a bunch of headers that we will need in the examples

#include "zmq.hpp"

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
std::string s_recv (zmq::socket_t & socket);

//  Convert string to 0MQ string and send to socket
bool s_send (zmq::socket_t & socket, const std::string & string);

//  Sends string as 0MQ string, as multipart non-terminal
bool s_sendmore (zmq::socket_t & socket, const std::string & string);

//  Receives all message parts from socket, prints neatly
void s_dump (zmq::socket_t & socket);

//  Set simple random printable identity on socket
std::string s_set_id (zmq::socket_t & socket);

//  Report 0MQ version number
void s_version (void);

void s_version_assert (int want_major, int want_minor);

void zclock_log (const char *format, ...);

int64_t zclock_time (void);

//  Return current system clock as milliseconds
int64_t s_clock (void);
//  Return current system clock as milliseconds
int64_t zclock_time (void);
//  Sleep for a number of milliseconds
void s_sleep (int msecs);

#endif
