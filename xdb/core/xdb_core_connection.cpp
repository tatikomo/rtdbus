#include <assert.h>

#include "config.h"

#include "xdb_core_environment.hpp"
#include "xdb_core_connection.hpp"

using namespace xdb::core;

Connection::Connection(Environment* _env) :
  m_last_error(rtE_NONE)
{
  assert(_env);
  m_environment = _env;
}

Connection::~Connection()
{
}

