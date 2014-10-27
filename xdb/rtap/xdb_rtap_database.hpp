#pragma once
#ifndef XDB_RTAP_DATABASE_HPP
#define XDB_RTAP_DATABASE_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_impl_error.hpp"
#include "xdb_impl_common.hpp"
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
    // Создание Точки
    const Error& create(rtap_db::Point&);
    // Удаление Точки
    const Error& erase(rtap_db::Point&);
    // Чтение данных Точки
    const Error& read(rtap_db::Point&);
    // Изменение данных Точки
    const Error& write(rtap_db::Point&);
    // Блокировка данных Точки от изменения в течение заданного времени
    const Error& lock(rtap_db::Point&, int);
    const Error& unlock(rtap_db::Point&);

  private:
    DatabaseRtapImpl *m_impl;
    const Options    *m_options;
};

} //namespace xdb

#endif

