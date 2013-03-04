//
//  mdp.h
//  Majordomo Protocol definitions
//
#ifndef __MDP_H_INCLUDED__
#define __MDP_H_INCLUDED__

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

#include <string>

static char *mdpw_commands [] = {
    '\0', (char*)"READY", (char*)"REQUEST", (char*)"REPORT", (char*)"HEARTBEAT", (char*)"DISCONNECT"
};

#endif

