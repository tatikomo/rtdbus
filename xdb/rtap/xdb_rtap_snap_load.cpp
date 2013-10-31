#include <assert.h>

#include "glog/logging.h"
#include "config.h"

#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_snap_main.hpp"

using namespace xdb;

extern char database_name[SERVICE_NAME_MAXLEN + 1];

bool xdb::loadFromXML(xdb::RtEnvironment* env, const char* fname)
{
  bool status = false;
  assert(env);
  assert(fname);
  LOG(INFO) << "Load '"<< database_name <<"' content from "<<fname;
  return status;
}

