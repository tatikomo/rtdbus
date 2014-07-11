#include "glog/logging.h"

#include "config.h"

#include "xdb_core_connection.hpp"
#include "xdb_core_environment.hpp"
#include "xdb_rtap_connection.hpp"

using namespace xdb;

class Environment;

RtConnection::RtConnection(Environment* _env)
{
  m_impl = new Connection(_env);
  m_environment = _env;
}

RtConnection::~RtConnection()
{
  LOG(INFO) << "Destroy RtConnection for env " << m_environment->getName();
  delete m_impl;
}

