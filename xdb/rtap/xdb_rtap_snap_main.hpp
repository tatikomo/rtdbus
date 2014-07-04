#pragma once
#if !defined XDB_RTAP_SNAP_MAIN_HPP
#define XDB_RTAP_SNAP_MAIN_HPP

#include "config.h"

namespace xdb {
namespace rtap {

typedef enum 
{
  LOAD_FROM_XML = 1,
  SAVE_TO_XML   = 2
} Commands_t;

class RtEnvironment;

bool loadFromXML(RtEnvironment*, const char*);
bool saveToXML(RtEnvironment*, const char*);
bool translateInstance(const char*);

} // namespace rtap
} // namespace xdb

#endif

