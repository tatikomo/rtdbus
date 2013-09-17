//
//  mdp_common.h
//  Majordomo Protocol definitions
//
#ifndef __MDP_COMMON_H_INCLUDED__
#define __MDP_COMMON_H_INCLUDED__

#pragma once

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
#define MDPC_REQUEST        "\001"
#define MDPC_REPORT         "\002"
#define MDPC_NAK            "\003"

//  MDP/Server commands, as strings
#define MDPW_READY          "\001"
#define MDPW_REQUEST        "\002"
#define MDPW_REPORT         "\003" // TODO: в версии 0.2 заменен на PARTIAL и FINAL
#define MDPW_PARTIAL        "\003"
#define MDPW_FINAL          "\003"
#define MDPW_HEARTBEAT      "\004"
#define MDPW_DISCONNECT     "\005"

//  We'd normally pull these from config data
#define HEARTBEAT_LIVENESS  4       //  3-5 is reasonable

#endif

