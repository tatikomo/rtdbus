#include <iostream>

#include <assert.h>
#include <string.h>
#include <algorithm>

#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "xdb_impl_error.hpp"
#include "xdb_impl_application.hpp"

using namespace xdb;

ApplicationImpl::ApplicationImpl(const char* _name) :
  m_mode(APP_MODE_UNKNOWN),
  m_state(APP_STATE_UNKNOWN),
  m_last_error(rtE_NONE)
{
  strncpy(m_appli_name, _name, IDENTITY_MAXLEN);
  m_appli_name[IDENTITY_MAXLEN] = '\0';
}

ApplicationImpl::~ApplicationImpl()
{
}

const char* ApplicationImpl::getAppName() const
{
  return m_appli_name;
}

const Error& ApplicationImpl::setAppName(const char* _name)
{
  m_last_error.clear();
  assert(_name);

  if (_name)
  {
    if (strlen(_name) > IDENTITY_MAXLEN)
    {
      m_last_error.set(rtE_STRING_TOO_LONG);
    }
    else
    {
      strncpy(m_appli_name, _name, IDENTITY_MAXLEN);
      m_appli_name[IDENTITY_MAXLEN] = '\0';
    }
  }
  else
  {
    m_last_error.set(rtE_STRING_IS_EMPTY);
  }

  return m_last_error;
}

// TODO: провести инициализацию рантайма с учетом данных начальных условий
const Error& ApplicationImpl::initialize()
{
  LOG(INFO) << "ApplicationImpl::initialize()";
  m_last_error.clear();

  // Если инициализация не была выполнена ранее или была ошибка
  if (APP_STATE_BAD == getOperationState())
  {
    LOG(ERROR) << "Unable to initialize ApplicationImpl";
  }
  return m_last_error;
}

AppMode_t ApplicationImpl::getOperationMode() const
{
  return m_mode;
}

AppState_t ApplicationImpl::getOperationState() const
{
  return m_state;
}

bool ApplicationImpl::getOption(const std::string& key, int& val)
{
  bool status = ::getOption(m_map_options, key, val);
  return status;
}

Options& ApplicationImpl::getOptions()
{
  return m_map_options;
}

void ApplicationImpl::setOption(const char* key, int val)
{
  m_map_options.insert(Pair(std::string(key), val));
  LOG(INFO)<<"Parameter '"<<key<<"': "<<val;
}

const Error& ApplicationImpl::getLastError() const
{
  return m_last_error;
}

void ApplicationImpl::setLastError(const Error& _new_error)
{
  m_last_error.set(_new_error);
}

void ApplicationImpl::clearError()
{
  m_last_error.clear();
}

