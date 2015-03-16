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

#include "dat/rtap_db.hxx"

namespace xdb {

class EnvironmentImpl;

class ConnectionImpl
{
  public:
    ConnectionImpl(EnvironmentImpl *env);
   ~ConnectionImpl();

    mco_db_h handle();

    // Найти точку с указанным тегом
    rtap_db::Point* locate(const char*);

  private:
    // Среда подключения
    EnvironmentImpl *m_env_impl;
    // Хендл экземпляра подключения
    mco_db_h m_database_handle;
};

} // namespace xdb

#endif

