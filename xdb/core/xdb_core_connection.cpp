#include "config.h"

#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"

using namespace xdb;

RtDbConnection::RtDbConnection(RtEnvironment* _env) :
  m_last_error(rtE_NONE)
{
  assert(_env);
  m_environment = _env;
}

RtDbConnection::~RtDbConnection()
{
}

