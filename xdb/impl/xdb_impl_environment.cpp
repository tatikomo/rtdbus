#include <assert.h>
#include <string.h>

#include "glog/logging.h"
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_impl_application.hpp"
#include "xdb_impl_environment.hpp"
//#include "xdb_impl_connection.hpp"
#include "xdb_impl_database.hpp"
#include "xdb_impl_error.hpp"

using namespace xdb;

EnvironmentImpl::EnvironmentImpl(ApplicationImpl* _app, const char* _name) :
  m_state(ENV_STATE_UNKNOWN),
  m_last_error(rtE_NONE),
  m_appli(_app),
  m_conn(NULL)
{
  BitSet8 open_flags;
  int cheched_val;

  if (m_appli->getOption("OF_CREATE", cheched_val) && cheched_val)
  {
    open_flags.set(OF_POS_CREATE);
    cheched_val = 0;
  }
  if (m_appli->getOption("OF_TRUNCATE", cheched_val) && cheched_val)
  {
    open_flags.set(OF_POS_TRUNCATE);
    cheched_val = 0;
  }
  if (m_appli->getOption("OF_LOAD_SNAP", cheched_val) && cheched_val)
  {
    open_flags.set(OF_POS_LOAD_SNAP);
    cheched_val = 0;
  }
  if (m_appli->getOption("OF_RDWR", cheched_val) && cheched_val)
  {
    open_flags.set(OF_POS_RDWR);
    cheched_val = 0;
  }
  if (m_appli->getOption("OF_READONLY", cheched_val) && cheched_val)
  {
    open_flags.set(OF_POS_READONLY);
    cheched_val = 0;
  }

  strncpy(m_name, _name, IDENTITY_MAXLEN);
  m_name[IDENTITY_MAXLEN] = '\0';

  m_state = ENV_STATE_BAD;
}

EnvironmentImpl::~EnvironmentImpl()
{
  LOG(INFO) << "EnvironmentImpl '" << m_name << "' destructor";
}

void EnvironmentImpl::setEnvState(EnvState_t _new_state)
{
  LOG(INFO) << "Change environment '" << m_name
            << "' state from " << m_state << " to " << _new_state;
  m_state = _new_state;
}

const Error& EnvironmentImpl::getLastError() const
{
  return m_last_error;
}

void EnvironmentImpl::setLastError(const Error& _new_error)
{
  m_last_error.set(_new_error);
}

void EnvironmentImpl::clearError()
{
  m_last_error.clear();
}

// Вернуть имя подключенной БД/среды
const char* EnvironmentImpl::getName() const
{
  return m_name;
}

// Вернуть имя Приложения
const char* EnvironmentImpl::getAppName() const
{
  return m_appli->getAppName();
}

#if 0
// TODO: Создать и вернуть новое подключение к указанной БД/среде
ConnectionImpl* EnvironmentImpl::getConnection()
{
  if (!m_conn)
  {
    LOG(INFO) << "Creates new connection to env " << m_name;
    m_conn = new ConnectionImpl(this);
  }
  return m_conn;
}

// TODO: Создать новое сообщение указанного типа
mdp::Letter* EnvironmentImpl::createMessage(/* msgType */)
{
  m_last_error = rtE_NOT_IMPLEMENTED;
  return NULL;
}

// Отправить сообщение адресату
Error& EnvironmentImpl::sendMessage(mdp::Letter* letter)
{
  m_last_error = rtE_NONE;
  assert(letter);
  return m_last_error;
}
#endif

const Error& EnvironmentImpl::LoadSnapshot(const char *app_name, const char *filename)
{
  LOG(INFO) << "Load " << m_name 
            << " current snapshot for '" << app_name 
            << "' from '" << filename << "'";
  m_last_error = rtE_NOT_IMPLEMENTED;
  return m_last_error;
}

const Error& EnvironmentImpl::MakeSnapshot(const char *filename)
{
  m_last_error = rtE_NONE;

  assert(filename);
  m_last_error.set(rtE_XML_OPEN);
  return m_last_error;
}

