#pragma once
#if !defined XDB_CORE_COMMON_H
#define XDB_CORE_COMMON_H

/*
#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"

#ifdef __cplusplus
}
#endif
*/

const char * mco_ret_string(int, int*);
void rc_check(const char*, int);
void show_runtime_info(const char * lead_line);

#endif
