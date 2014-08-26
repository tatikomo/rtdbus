#include "glog/logging.h"
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_impl_connection.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"

using namespace xdb;

RtConnection::RtConnection(RtEnvironment* _env)
{
  m_impl = new ConnectionImpl(_env->m_impl);
  m_environment = _env;
}

RtConnection::~RtConnection()
{
  LOG(INFO) << "Destroy RtConnection for env " << m_environment->getName();
  delete m_impl;
}

