#pragma once
#if !defined XDB_CORE_DATABASE_HPP
#define XDB_CORE_DATABASE_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <bitset>

#include "xdb_core_error.hpp"

namespace xdb {
namespace core {

typedef std::bitset<8> BitSet8;

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
    /* Внутренние состояния базы данных */
    typedef enum {
        UNINITIALIZED = 1, // первоначальное состояние
        INITIALIZED   = 2, // инициализирован runtime
        ATTACHED      = 3, // вызван mco_db_open
        CONNECTED     = 4, // вызван mco_db_connect
        DISCONNECTED  = 5, // вызван mco_db_disconnect
        CLOSED        = 6  // вызван mco_db_close
    } DBState;

    Database(const char*);
    virtual ~Database();

    const char* DatabaseName() const;
    /* 
     * Каждая реализация базы данных использует 
     * свой собственный словарь, и должна сама
     * реализовывать функцию открытия
     */
    virtual const xdb::core::Error& Init();
    virtual const xdb::core::Error& Create();
    virtual const xdb::core::Error& Connect();
    virtual const xdb::core::Error& Disconnect();
    virtual const xdb::core::Error& LoadSnapshot(const char* = NULL);
    virtual const xdb::core::Error& StoreSnapshot(const char* = NULL);

    DBState State() const;

    /* Сменить текущее состояние на новое */
    const xdb::core::Error& TransitionToState(DBState);
    const xdb::core::Error& getLastError() { return m_last_error; };
    virtual bool ifErrorOccured() { return (m_last_error.getCode() != rtE_NONE); };

  private:
    char m_name[DBNAME_MAXLEN+1];
    xdb::core::Error m_last_error;

  protected:
    void  setError(xdb::core::ErrorType_t);
    void  clearError() { m_last_error.clear(); };
    DBState  m_state;
};

} //namespace core
} //namespace xdb
#endif

