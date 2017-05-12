#pragma once
#ifndef XDB_RTAP_DATABASE_HPP
#define XDB_RTAP_DATABASE_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_common.hpp"
#include "xdb_impl_error.hpp"
#include "helper.hpp"
#include "dat/rtap_db.hxx"

namespace xdb {

class DatabaseRtapImpl;

class RtDatabase
{
  public:
    RtDatabase(const char*, const ::Options*);
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
    const Error& MakeSnapshot(const char* = 0);
    const char* getName() const;
    // Вернуть последнюю ошибку
    const Error& getLastError() const;
    // Вернуть признак отсутствия ошибки
    bool  ifErrorOccured() const;
    // Сбросить ошибку
    void  clearError();
    // Установить новое состояние ошибки
    void  setError(ErrorCode_t);
    // Вернуть текущее состояние БД
    DBState_t State() const;

    // Функции изменения содержимого БД
    // Разименованный указатель на данные используется с целью
    // отвязки от включения специфичного для XDB 
    // ====================================================
    const Error& Control(rtDbCq&);
    const Error& Query(rtDbCq&);
    const Error& Config(rtDbCq&);

    // NB: Функции манипуляции данными находятся в RtConnection
    // Нужен для передачи в ConnectionImpl
    DatabaseRtapImpl *getImpl() { return m_impl; };

  protected:
    DatabaseRtapImpl *m_impl;

  private:
    DISALLOW_COPY_AND_ASSIGN(RtDatabase);
    const Options    *m_options;
};

} //namespace xdb

#endif

