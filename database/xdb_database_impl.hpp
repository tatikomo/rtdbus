#if !defined XDB_DATABASE_IMPL_HPP
#define XDB_DATABASE_IMPL_HPP
#pragma once

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "xdb_database.hpp"

/* Фактическая реализация функциональности Базы Данных */
class XDBDatabaseImpl
{
  public:
    XDBDatabaseImpl(const XDBDatabase*, const char*);
    ~XDBDatabaseImpl();
    /* Получить символьное имя базы данных */
    const char* DatabaseName();
    /* Подключение к общим структурам базы данных */
    bool Connect();
    /* Непосредственное открытие базы */
    bool Open();
    /* Отключение от базы данных */
    bool Disconnect();
    const XDBDatabase::DBState State();

  private:
    char m_name[DATABASE_NAME_MAXLEN+1];
    const XDBDatabase    *m_self;
    XDBDatabase::DBState  m_state;
};

#endif
