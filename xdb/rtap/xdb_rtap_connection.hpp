#pragma once
#ifndef XDB_RTAP_CONNECTION_HPP
#define XDB_RTAP_CONNECTION_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "xdb_impl_error.hpp"
#include "xdb_impl_common.hpp" // rtDbCq

namespace xdb {

class RtEnvironment;
class RtPoint;
class ConnectionImpl;

class RtConnection
{
  public:
    RtConnection(RtEnvironment*);
   ~RtConnection();
    // Создать точку со всеми её атрибутами
    const Error& create(RtPoint*);
    // Изменить значения атрибутов указанной точки
    const Error& write(RtPoint*);
    // Найти точку с указанными атрибутами (UNIVNAME?)
    RtPoint* locate(const char*);
    // Вернуть ссылку на экземпляр текущей среды
    RtEnvironment* environment();
    // Интерфейс управления БД
    const Error& ControlDatabase(rtDbCq&);
    const Error& QueryDatabase(rtDbCq&);
    const Error& ConfigDatabase(rtDbCq&);
    // Последняя зафиксированная ошибка
    const Error& getLastError() const { return m_last_error; }

  private:
    DISALLOW_COPY_AND_ASSIGN(RtConnection);
    RtEnvironment  *m_environment;
    ConnectionImpl *m_impl;
    Error           m_last_error;
};

} // namespace xdb

#endif
