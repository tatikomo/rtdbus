#pragma once
#if !defined XDB_RTAP_CONNECTION_HPP
#define XDB_RTAP_CONNECTION_HPP

#include "config.h"

namespace xdb {

class RtEnvironment;
class Connection;

class RtConnection
{
  public:
    RtConnection(RtEnvironment*);
   ~RtConnection();

  private:
    RtEnvironment *m_environment;
    Connection  *m_impl;
};

} // namespace xdb

#endif
