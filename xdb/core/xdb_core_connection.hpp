#pragma once
#if !defined XDB_CORE_CONNECTION_HPP
#define XDB_CORE_CONNECTION_HPP

#include <string>
#include "config.h"
#include "xdb_core_attribute.hpp"
#include "xdb_core_error.hpp"

namespace xdb {
namespace core {

class Environment;
class Point;

class Connection
{
  public:
    Connection(Environment*);
    ~Connection();

    // Копировать точку под новым именем
    Error copy(Attribute&, Point&, std::string&);

  private:
    DISALLOW_COPY_AND_ASSIGN(Connection);
    Environment* m_environment;
    Error        m_last_error;
};

} // namespace core
} // namespace xdb
#endif

