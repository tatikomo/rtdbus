#pragma once
#if !defined XDB_RTAP_CONNECTION_HPP
#define XDB_RTAP_CONNECTION_HPP

#include "config.h"

namespace xdb {

class Environment;
class Connection;

class RtConnection
{
  public:
    RtConnection(Environment*);
   ~RtConnection();

  private:
    Environment *m_environment;
    Connection  *m_impl;
};

} // namespace xdb

#endif
