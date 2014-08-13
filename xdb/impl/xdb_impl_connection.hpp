#pragma once
#ifndef XDB_IMPL_CONNECTION_HPP
#define XDB_IMPL_CONNECTION_HPP

#include <string>
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_impl_attribute.hpp"
#include "xdb_impl_error.hpp"

namespace xdb {

class EnvironmentImpl;
class Point;

class ConnectionImpl
{
  public:
    ConnectionImpl(EnvironmentImpl*);
    ~ConnectionImpl();

    // Копировать точку под новым именем
    const Error& copy(Attribute&, Point&, std::string&);

  private:
    DISALLOW_COPY_AND_ASSIGN(ConnectionImpl);
    EnvironmentImpl* m_environment;
    Error            m_last_error;
};

} // namespace xdb

#endif

