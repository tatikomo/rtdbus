#pragma once
#ifndef XDB_RTAP_CONNECTION_HPP
#define XDB_RTAP_CONNECTION_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "xdb_common.hpp"   // rtDbCq
#include "xdb_impl_error.hpp"

namespace xdb {

class RtDatabase;
class RtPoint;
class ConnectionImpl;

class RtConnection
{
  public:
    RtConnection(RtDatabase*);
   ~RtConnection();
    // Состояние подключения
    ConnectionState_t state();
    // Создать точку со всеми её атрибутами
    const Error& create(RtPoint*);
    // Изменить значения атрибутов указанной точки
    const Error& write(RtPoint*);
    // Найти точку с указанным тегом и прочитать все её атрибуты
    RtPoint* locate(const char*);
    // Прочитать значение заданного атрибута точки
    const Error& read(AttributeInfo_t*);
    // Прочитать значения заданной группы точек
    const Error& read(std::string&, int*, SubscriptionPoints_t*);
    // Записать значение заданного атрибута точки
    const Error& write(const AttributeInfo_t*);
    // Вернуть ссылку на экземпляр текущей среды
    RtDatabase* database();
    // Интерфейс управления БД
    const Error& ControlDatabase(rtDbCq&);
    const Error& QueryDatabase(rtDbCq&);
    const Error& ConfigDatabase(rtDbCq&);
    // Последняя зафиксированная ошибка
    const Error& getLastError() const { return m_last_error; }

  private:
    DISALLOW_COPY_AND_ASSIGN(RtConnection);
    RtDatabase     *m_database;
    ConnectionImpl *m_impl;
    Error           m_last_error;
};

} // namespace xdb

#endif
