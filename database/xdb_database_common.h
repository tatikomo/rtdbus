#if !defined GEV_XDB_DATABASE_COMMON_H
#define GEV_XDB_DATABASE_COMMON_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"

#ifdef __cplusplus
}
#endif

void errhandler(MCO_RET);
void extended_errhandler(MCO_RET errcode, const char* file, int line);
const char * mco_ret_string(MCO_RET, int*);
void rc_check(const char*, MCO_RET);

void show_runtime_info(const char * lead_line);
#if (EXTREMEDB_VERSION >=41)
void show_device_info(const char * lead_line, mco_device_t dev[], int nDev);
#endif

#endif
