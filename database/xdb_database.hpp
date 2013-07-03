#if !defined XDB_DATABASE_HPP
#define XDB_DATABASE_HPP
#pragma once

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

class XDBDatabase
{
  public:
    /* Внутренние состояния базы данных */
    typedef enum {
        INITIALIZED = 1,
        CONNECTED   = 2,
        OPENED      = 3,
        CLOSED      = 4,
        DISCONNECTED= 5,
        DELETED     = 6
    } DBState;

    XDBDatabase(const char*);
    virtual ~XDBDatabase();

    const char* DatabaseName();
    /* 
     * Каждая реализация базы данных использует 
     * свой собственный словарь, и должна сама
     * реализовывать функцию открытия
     */
    virtual bool Connect();
    virtual bool Open();
    virtual bool Disconnect();

    const DBState State();
    /* Сменить текущее состояние на новое */
    bool TransitionToState(DBState);

  private:
    char m_name[DBNAME_MAXLEN+1];

  protected:
    DBState  m_state;
};

#endif

