#if !defined XDB_DATABASE_HPP
#define XDB_DATABASE_HPP
#pragma once

// размер имени базы данных
#define DATABASE_NAME_MAXLEN 10
// максимальное количество Обработчиков для одного Сервиса
#define WAITING_WORKERS_SPOOL_MAXLEN 2

class XDBDatabaseImpl;

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
    const char* DatabaseName();
    bool Connect();
    /* 
     * Каждая реализация базы данных использует 
     * свой собственный словарь, и должна сама
     * реализовывать функцию открытия
     */
    virtual bool Open();
    bool Disconnect();
    const DBState State();
    /* Сменить текущее состояние на новое */
    bool TransitionToState(DBState);
    ~XDBDatabase();

  private:
    XDBDatabaseImpl *impl;

  protected:
    DBState  m_state;
};

#endif

