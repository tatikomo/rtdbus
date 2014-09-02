#pragma once
#ifndef XDB_CORE_COMMON_HPP
#define XDB_CORE_COMMON_HPP

namespace xdb {

    // Внутренние состояния базы данных
    typedef enum {
        DB_STATE_UNINITIALIZED = 1, // первоначальное состояние
        DB_STATE_INITIALIZED   = 2, // инициализирован runtime
        DB_STATE_ATTACHED      = 3, // вызван mco_db_open
        DB_STATE_CONNECTED     = 4, // вызван mco_db_connect
        DB_STATE_DISCONNECTED  = 5, // вызван mco_db_disconnect
        DB_STATE_CLOSED        = 6  // вызван mco_db_close
    } DBState_t;

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

    // Позиции бит в флагах, передаваемых конструктору
    typedef enum
    {
      OF_POS_CREATE   = 1, // создать БД в случае, если это не было сделано ранее
      OF_POS_READONLY = 2, // открыть в режиме "только для чтения"
      OF_POS_RDWR     = 3, // открыть в режиме "чтение|запись" (по умолчанию)
      OF_POS_TRUNCATE = 4, // открыть пустую базу, удалив данные в существующем экземпляре
      OF_POS_LOAD_SNAP= 5  // открыть базу, заполнив ее данными из последнего снапшота
    } FlagPos_t;

}

#endif
