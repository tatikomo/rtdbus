#include <assert.h>
#include <string.h>

#include "config.h"

#include "xdb_rtap_error.hpp"
#include "xdb_rtap_application.hpp"

using namespace xdb;

RtApplication::RtApplication(const char* _name) :
  m_mode(MODE_UNKNOWN),
  m_state(CONDITION_UNKNOWN)
{
  m_error.set(rtE_NONE);
  m_options[0] = '\0';
  m_environment_name[0] = '\0';
  strncpy(m_appli_name, _name, IDENTITY_MAXLEN);
  m_appli_name[IDENTITY_MAXLEN] = '\0';
}

const char* RtApplication::getAppName() const
{
  return m_appli_name;
}

const RtError& RtApplication::setAppName(const char* _name)
{
  m_error.set(rtE_NONE);
  assert(_name);

  if (_name)
  {
    if (strlen(_name) > IDENTITY_MAXLEN)
    {
      m_error.set(rtE_STRING_TOO_LONG);
    }
    else
    {
      strncpy(m_appli_name, _name, IDENTITY_MAXLEN);
      m_appli_name[IDENTITY_MAXLEN] = '\0';
    }
  }
  else
  {
    m_error.set(rtE_STRING_IS_EMPTY);
  }

  return m_error;
}

const char* RtApplication::getEnvName() const
{
  return m_environment_name;
}

const RtError& RtApplication::setEnvName(const char* _env_name)
{
  m_error.set(rtE_NONE);
  strncpy(m_environment_name, _env_name, IDENTITY_MAXLEN);
  return m_error;
}

// TODO: провести инициализацию рантайма с учетом данных начальных условий
const RtError& RtApplication::initialize()
{
  m_error.set(rtE_NONE);
  m_state = CONDITION_GOOD;
  return m_error;
}

RtApplication::RtAppMode RtApplication::getOperationMode() const
{
  return m_mode;
}

RtApplication::RtAppState RtApplication::getOperationState() const
{
  return m_state;
}

const char* RtApplication::getOptions() const
{
  return m_options;
}

const RtError& RtApplication::setOptions(const char* _options)
{
  m_error.set(rtE_NONE);
  // TODO установить массив опций
  strncpy(m_options, _options, IDENTITY_MAXLEN);
  return m_error;
}

const RtError& RtApplication::getLastError() const
{
  return m_error;
}

