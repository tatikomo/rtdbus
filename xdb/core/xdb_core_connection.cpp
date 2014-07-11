#include <assert.h>

#include "glog/logging.h"

#include "config.h"
#include "xdb_core_error.hpp"
#include "xdb_core_environment.hpp"
#include "xdb_core_connection.hpp"
#include "xdb_core_attribute.hpp" // Attribute & Point 

using namespace xdb;

Connection::Connection(Environment* _env) :
  m_last_error(rtE_NONE)
{
  assert(_env);
  m_environment = _env;
}

Connection::~Connection()
{
  LOG(INFO) << "Destroy connection for env " << m_environment->getName();
}

// Копировать точку под новым именем
const Error& Connection::copy(Attribute&, Point&, std::string&)
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return m_last_error;
}
