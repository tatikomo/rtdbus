//
//  mdp_common.h
//  Majordomo Protocol definitions
//
#pragma once
#ifndef __MDP_COMMON_H_INCLUDED__
#define __MDP_COMMON_H_INCLUDED__


#include <stdio.h>      // sprintf
#include <stdarg.h>     // va_args
#include <stdint.h>     // int64_t
#include <string>

extern const char * EMPTY_FRAME;
//  This is the version of MDP/Client we implement
extern const char * MDPC_CLIENT;
extern char *mdpc_commands [];
//  This is the version of MDP/Worker we implement
extern const char * MDPW_WORKER;
extern char *mdpw_commands [];

//  MDP/Client commands, as strings
#define MDPC_REQUEST        (const char*)"\001"
#define MDPC_REPORT         (const char*)"\002"
#define MDPC_NAK            (const char*)"\003"

//  MDP/Server commands, as strings
#define MDPW_READY          (const char*)"\001"
#define MDPW_REQUEST        (const char*)"\002"
#define MDPW_REPORT         (const char*)"\003" // TODO: в версии 0.2 заменен на PARTIAL и FINAL
#define MDPW_PARTIAL        (const char*)"\003"
#define MDPW_FINAL          (const char*)"\003"
#define MDPW_HEARTBEAT      (const char*)"\004"
#define MDPW_DISCONNECT     (const char*)"\005"

#endif

