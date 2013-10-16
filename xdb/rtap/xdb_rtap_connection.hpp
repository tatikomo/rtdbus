#if !defined GEV_XDB_RTAP_CONNECTION_H_
#define GEV_XDB_RTAP_CONNECTION_H_
#pragma once

#include <string>
#include "config.h"
#include "xdb_rtap_attribute.hpp"
#include "xdb_rtap_point.hpp"

namespace xdb
{

class RtConnection
{
  public:
    typedef enum {
      ERROR_NONE = 0
    } RtError;

    RtConnection();
    ~RtConnection();

    // Копировать точку под новым именем
    RtError copy(RtAttribute&, RtPoint&, std::string&);

  private:
    DISALLOW_COPY_AND_ASSIGN(RtConnection);
    RtError m_last_error;
};

}
#endif

