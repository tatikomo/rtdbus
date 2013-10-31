#if !defined XDB_RTAP_SNAP_MAIN_H_
#define XDB_RTAP_SNAP_MAIN_H_
#pragma once

#include "config.h"

namespace xdb
{

typedef enum 
{
  LOAD_FROM_XML = 1,
  SAVE_TO_XML   = 2
} Commands_t;

class RtEnvironment;

bool loadFromXML(xdb::RtEnvironment*, const char*);
bool saveToXML(xdb::RtEnvironment*, const char*);
bool translateInstance(const char*);

}

#endif

