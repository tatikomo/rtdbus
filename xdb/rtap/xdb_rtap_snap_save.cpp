#include <assert.h>

#include "glog/logging.h"
#include "config.h"

#include "xdb_rtap_snap_main.hpp"
#include "xdb_rtap_environment.hpp"

using namespace xdb::rtap;

extern char database_name[SERVICE_NAME_MAXLEN + 1];

bool xdb::rtap::saveToXML(RtEnvironment* env, const char* fname)
{
  bool status = false;

  assert(env);
  assert(fname);

  LOG(INFO) << "Save '"<< database_name <<"' content to "<<fname;
  return status;
}

