#if !defined XDB_DATABASE_HPP
#define XDB_DATABASE_HPP
#pragma once

#define DATABASE_NAME_MAXLEN 10

class XDBDatabaseImpl;

class XDBDatabase
{
  public:
    /* Внутренние состояния базы данных */
    typedef enum {
        INITIALIZED,
        CONNECTED,
        OPENED,
        CLOSED,
        DISCONNECTED,
        DELETED
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
    ~XDBDatabase();

  private:
    XDBDatabaseImpl *impl;
};

#endif

