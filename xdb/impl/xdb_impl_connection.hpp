#pragma once
#ifndef XDB_IMPL_CONNECTION_IMPL_HPP
#define XDB_IMPL_CONNECTION_IMPL_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "xdb_common.hpp"

#include "xdb_impl_error.hpp"
#include "dat/rtap_db.hxx"

namespace xdb {

class DatabaseRtapImpl;

class ConnectionImpl
{
  public:
    ConnectionImpl(DatabaseRtapImpl*);
   ~ConnectionImpl();

    // Найти точку с указанным тегом
    rtap_db::Point* locate(const char*);
    // Прочитать значение конкретного атрибута
    const Error& read(AttributeInfo_t*);
    // Прочитать значения атрибутов указанной группы подписки
    const Error& read(std::string&, int*, SubscriptionPoints_t*);
    // Записать значение конкретного атрибута
    const Error& write(const AttributeInfo_t*);
    // Изменить значения атрибутов указанной точки
    const Error& write(rtap_db::Point&);

    const Error& Control(rtDbCq&);
    const Error& Query(rtDbCq&);
    const Error& Config(rtDbCq&);
    // Создание Точки
    const Error& create(rtap_db::Point*);
    // Удаление Точки
    const Error& erase(rtap_db::Point*);

  private:
    DISALLOW_COPY_AND_ASSIGN(ConnectionImpl);
    // Имплементация Базы данных
    DatabaseRtapImpl *m_rtap_db_impl;
    // Хендл экземпляра подключения
    mco_db_h m_database_handle;
    // Состояние подключения.
    // NB: Состояния объектов базы и подключения могут не совпадать,
    // поскольку база имеет ограничения на количество подключений. В этом
    // случае старые подключения могут работать, а новые подключения - нет.
    ConnectionState_t m_state;
    Error m_last_error;
};

} // namespace xdb

#endif

