#pragma once
#ifndef XDB_RTAP_DATABASE_HPP
#define XDB_RTAP_DATABASE_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_impl_error.hpp"
#include "xdb_impl_common.hpp"

namespace xdb {

class DatabaseImpl;

class RtDatabase
{
  public:
    RtDatabase(const char*, const Options& /*BitSet8*/);
    ~RtDatabase();

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
    const char* getName() const;
    // Вернуть последнюю ошибку
    const Error& getLastError() const;
    // Вернуть признак отсутствия ошибки
    bool  ifErrorOccured() const;
    // Сбросить ошибку
    void  clearError();
    // Установить новое состояние ошибки
    void  setError(ErrorType_t);
    // Вернуть текущее состояние БД
    DBState_t State() const;

  private:
    DatabaseImpl *m_impl;
};

} //namespace xdb

#endif

