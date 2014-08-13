#pragma once
#ifndef XDB_RTAP_ANALOG_HPP
#define XDB_RTAP_ANALOG_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
//#include "xdb_impl_attribute.hpp"
//#include "xdb_rtap_connection.hpp"

namespace xdb {

typedef struct
{
  uint64_t      id;       /* oid */
  int16_t       objclass;
  univname_t    univname;
  univname_t    parent;
  code_t        code;
  longlabel_t   label;
  shortlabel_t  shortlabel;
  uint64_t      id_SA;
  uint64_t      id_parent;
  float         val;
  float         valacq;
  float         valmanual;
  float         mnvalphy;
  float         mxvalphy;
  unsigned int  date_mark;
  unsigned int  time_mark;
  int8_t        valid;
  int8_t        validacq;
  uint64_t      id_unity_display;
  uint64_t      id_unity_origin;
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
