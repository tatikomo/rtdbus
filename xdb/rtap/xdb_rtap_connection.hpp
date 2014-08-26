#pragma once
#if !defined XDB_RTAP_CONNECTION_HPP
#define XDB_RTAP_CONNECTION_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

namespace xdb {

class RtEnvironment;
class ConnectionImpl;

class RtConnection
{
  public:
    RtConnection(RtEnvironment*);
   ~RtConnection();

  private:
    DISALLOW_COPY_AND_ASSIGN(RtConnection);
    RtEnvironment  *m_environment;
    ConnectionImpl *m_impl;
};

} // namespace xdb

#endif
