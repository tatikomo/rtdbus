#pragma once
#ifndef XDB_CORE_COMMON_H
#define XDB_CORE_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

// C часть

const char * mco_ret_string(int, int*);
void rc_check(const char*, int);
void show_runtime_info(const char * lead_line);

#ifdef __cplusplus
}
#endif

#endif
