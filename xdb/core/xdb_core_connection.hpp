#if !defined GEV_XDB_RTAP_CONNECTION_H_
#define GEV_XDB_RTAP_CONNECTION_H_
#pragma once

#include <string>
#include "config.h"
#include "xdb_rtap_attribute.hpp"
#include "xdb_rtap_point.hpp"
#include "xdb_rtap_error.hpp"

namespace xdb
{

class RtEnvironment;

class RtDbConnection
{
  public:
    RtDbConnection(RtEnvironment*);
    ~RtDbConnection();

    // Копировать точку под новым именем
    RtError copy(RtAttribute&, RtPoint&, std::string&);

  private:
    DISALLOW_COPY_AND_ASSIGN(RtDbConnection);
    RtEnvironment* m_environment;
    RtError        m_last_error;
};

}
#endif

