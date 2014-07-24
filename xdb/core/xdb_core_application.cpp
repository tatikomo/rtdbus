#include <iostream>

#include <assert.h>
#include <string.h>
#include <algorithm>

#include "glog/logging.h"
#include "config.h"

#include "xdb_core_error.hpp"
#include "xdb_core_application.hpp"
#include "xdb_core_environment.hpp"

using namespace xdb;

Application::Application(const char* _name) :
  m_mode(APP_MODE_UNKNOWN),
  m_state(APP_STATE_UNKNOWN),
  m_last_error(rtE_NONE)
{
  strncpy(m_appli_name, _name, IDENTITY_MAXLEN);
  m_appli_name[IDENTITY_MAXLEN] = '\0';
}

Application::~Application()
{
}

const char* Application::getAppName() const
{
  return m_appli_name;
}

const Error& Application::setAppName(const char* _name)
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
const Error& Application::initialize()
{
  LOG(INFO) << "Application::initialize()";
  m_last_error.clear();

  // Если инициализация не была выполнена ранее или была ошибка
  if (APP_STATE_BAD == getOperationState())
  {
    LOG(ERROR) << "Unable to initialize Application";
  }
  return m_last_error;
}

AppMode_t Application::getOperationMode() const
{
  return m_mode;
}

AppState_t Application::getOperationState() const
{
  return m_state;
}

bool Application::getOption(const std::string& key, int& val)
{
  bool status = false;
  OptionIterator p = m_map_options.find(key);

  if (p != m_map_options.end())
  {
    val = p->second;
    status = true;
    std::cout << "Found '"<<key<<"' option=" << p->second << std::endl;
  }
  return status;
}

void Application::setOption(const char* key, int val)
{
  m_map_options.insert(Pair(std::string(key), val));
  LOG(INFO)<<"Parameter '"<<key<<"': "<<val;
}

const Error& Application::getLastError() const
{
  return m_last_error;
}

void Application::setLastError(const Error& _new_error)
{
  m_last_error.set(_new_error);
}

void Application::clearError()
{
  m_last_error.clear();
}

