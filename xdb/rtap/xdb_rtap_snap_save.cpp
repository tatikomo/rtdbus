#include <assert.h>

#include "glog/logging.h"
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_rtap_snap.hpp"
#include "xdb_rtap_environment.hpp"

using namespace xdb;

bool xdb::saveToXML(RtEnvironment* env, const char* fname)
{
  bool status = false;

  assert(env);
  assert(fname);

  LOG(INFO) << "Save '"<< env->getName() <<"' content to "<<fname;
  return status;
}

