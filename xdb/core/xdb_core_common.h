#pragma once
#if !defined XDB_CORE_COMMON_H
#define XDB_CORE_COMMON_H

#ifdef __cplusplus

// C++ часть
#include <bitset>
typedef std::bitset<8> BitSet8;

namespace xdb {

/* Внутренние состояния базы данных */
    typedef enum {
        DB_STATE_UNINITIALIZED = 1, // первоначальное состояние
        DB_STATE_INITIALIZED   = 2, // инициализирован runtime
        DB_STATE_ATTACHED      = 3, // вызван mco_db_open
        DB_STATE_CONNECTED     = 4, // вызван mco_db_connect
        DB_STATE_DISCONNECTED  = 5, // вызван mco_db_disconnect
        DB_STATE_CLOSED        = 6  // вызван mco_db_close
    } DBState;

    typedef enum
    {
      APP_MODE_UNKNOWN    =-1,
      APP_MODE_LOCAL      = 0,
      APP_MODE_REMOTE     = 1
    } AppMode_t;

    typedef enum
    {
      APP_STATE_UNKNOWN =-1,
      APP_STATE_GOOD    = 0,
      APP_STATE_BAD     = 1
    } AppState_t;

    typedef enum
    {
      ENV_STATE_UNKNOWN = 0, // первоначальное состояние
      ENV_STATE_BAD     = 1, // критическая ошибка
      ENV_STATE_INIT    = 2, // среда проинициализирована, БД еще не открыта
      ENV_STATE_DB_OPEN = 3, // среда инициализирована, БД открыта
      ENV_STATE_LOADING = 4,      // среда в процессе запуска
      ENV_STATE_SHUTTINGDOWN = 5  // среда в процессе останова
    } EnvState_t;

    typedef enum
    {
      ENV_SHUTDOWN_SOFT = 0,
      ENV_SHUTDOWN_HARD = 1
    } EnvShutdownOrder_t;

}

extern "C" {
#endif

// C часть
#include "mco.h"

const char * mco_ret_string(int, int*);
void rc_check(const char*, int);
void show_runtime_info(const char * lead_line);

#ifdef __cplusplus
}
#endif

#endif
