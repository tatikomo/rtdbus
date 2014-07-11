#pragma once
#if !defined XDB_CORE_DATABASE_HPP
#define XDB_CORE_DATABASE_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "xdb_core_error.hpp"
#include "xdb_core_common.h"

namespace xdb {

// Позиции бит в флагах, передаваемых конструктору
typedef enum
{
  OF_POS_CREATE   = 1, // создать БД в случае, если это не было сделано ранее
  OF_POS_READONLY = 2, // открыть в режиме "только для чтения"
  OF_POS_RDWR     = 3, // открыть в режиме "чтение|запись" (по умолчанию)
  OF_POS_TRUNCATE = 4, // открыть пустую базу, удалив данные в существующем экземпляре
  OF_POS_LOAD_SNAP= 5  // открыть базу, заполнив ее данными из последнего снапшота
} FlagPos_t;

class Database
{
  public:
    Database(const char*);
    ~Database();

    const char* DatabaseName() const;
    /* 
     * Каждая реализация базы данных использует 
     * свой собственный словарь, и должна сама
     * реализовывать функцию открытия
     */
    const Error& Init();
    const Error& Create();
    const Error& Connect();
    const Error& Disconnect();
    const Error& LoadSnapshot(const char* = 0);
    const Error& StoreSnapshot(const char* = 0);

    DBState State() const;

    /* Сменить текущее состояние на новое */
    const Error& TransitionToState(DBState);
    const Error& getLastError() { return m_last_error; };
    bool  ifErrorOccured() { return (m_last_error.getCode() != rtE_NONE); };
    void  setError(ErrorType_t);
    void  clearError() { m_last_error.clear(); };

  private:
    char m_name[DBNAME_MAXLEN+1];
    Error m_last_error;
    DBState  m_state;
};

} //namespace xdb

#endif

