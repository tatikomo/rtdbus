#include <assert.h>

#include "glog/logging.h"
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"
#include "xdb_rtap_database.hpp"
#include "xdb_rtap_point.hpp"

using namespace xdb;

RtConnection::RtConnection(RtEnvironment* _env) :
  m_environment(_env),
  m_last_error(rtE_NONE)
{
}

RtConnection::~RtConnection()
{
  LOG(INFO) << "Destroy RtConnection for env " << m_environment->getName();
}

const Error& RtConnection::create(RtPoint* _point)
{
  assert(_point);

  LOG(INFO) << "Create new point '" << _point->getTag()
            << "' for env '" << m_environment->getName() << "'";
  
  // Создание экземпляра
  write(_point);

  return getLastError();
}

// Вернуть ссылку на экземпляр текущей среды
RtEnvironment* RtConnection::environment()
{
  return m_environment;
}

const Error& RtConnection::write(RtPoint* _point)
{
  m_last_error = m_environment->getDatabase()->write(_point->info());
  return getLastError();
}

