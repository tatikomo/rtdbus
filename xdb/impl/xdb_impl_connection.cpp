#include "glog/logging.h"
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_impl_error.hpp"
#include "xdb_impl_environment.hpp"
#include "xdb_impl_connection.hpp"
#include "xdb_impl_attribute.hpp" // Attribute & Point 

using namespace xdb;

ConnectionImpl::ConnectionImpl(EnvironmentImpl* _env) :
  m_environment(_env),
  m_last_error(rtE_NONE)
{
}

ConnectionImpl::~ConnectionImpl()
{
  LOG(INFO) << "Destroy Connection for env " << m_environment->getName();
}

// Копировать точку под новым именем
const Error& ConnectionImpl::copy(Attribute&, Point&, std::string&)
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return m_last_error;
}
