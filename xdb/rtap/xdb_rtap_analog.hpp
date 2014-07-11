#pragma once
#ifndef XDB_RTAP_ANALOG_HPP
#define XDB_RTAP_ANALOG_HPP

#include "config.h"
//#include "xdb_core_attribute.hpp"
//#include "xdb_rtap_connection.hpp"

namespace xdb {

typedef struct
{
  autoid_t      id;       /* oid */
  int16_t       objclass;
  univname_t    univname;
  univname_t    parent;
  code_t        code;
  longlabel_t   label;
  shortlabel_t  shortlabel;
  autoid_t      id_SA;
  autoid_t      id_parent;
  float         val;
  float         valacq;
  float         valmanual;
  float         mnvalphy;
  float         mxvalphy;
  mco_date      date_mark;
  mco_time      time_mark;
  int8_t        valid;
  int8_t        validacq;
  autoid_t      id_unity_display;
  autoid_t      id_unity_origin;
} Analog_t;

class RtAnalog
{
  public:
    RtAnalog(Analog_t&);
    RtAnalog(Analog_t*);
    ~RtAnalog();
//    friend class RtConnection;

  private:
    Analog_t* m_instance;
};

} // namespace xdb

#endif
