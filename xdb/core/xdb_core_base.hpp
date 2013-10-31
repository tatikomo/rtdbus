#if !defined XDB_DATABASE_HPP
#define XDB_DATABASE_HPP
#pragma once

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

namespace xdb {

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
    virtual bool Connect();
    virtual bool Disconnect();
    virtual bool Init();

    DBState State() const;
    /* Сменить текущее состояние на новое */
    bool TransitionToState(DBState);

  private:
    char m_name[DBNAME_MAXLEN+1];

  protected:
    DBState  m_state;
};

} //namespace xdb
#endif

