#include <iostream>

#include <assert.h>
#include <string.h>
#include <algorithm>

#include "glog/logging.h"
#include "config.h"

#include "xdb_core_error.hpp"
#include "xdb_core_application.hpp"
#include "xdb_core_environment.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_application.hpp"

using namespace xdb;

RtApplication::RtApplication(const char* _name)
{
  m_impl = new Application(_name);
}

RtApplication::~RtApplication()
{
  delete m_impl;
}

// TODO: провести инициализацию рантайма с учетом данных начальных условий
const Error& RtApplication::initialize()
{
  m_impl->initialize();
  LOG(INFO) << "RtApplication::initialize()";
#if 0
  // Если инициализация не была выполнена ранее или была ошибка
  if (CONDITION_BAD == getOperationState())
  {
  }
#endif
  return m_impl->getLastError();
}

bool RtApplication::getOption(const std::string& key, int& val)
{
  return m_impl->getOption(key, val);
}

void RtApplication::setOption(const char* key, int val)
{
  m_impl->setOption(key, val);
}

const char* RtApplication::getAppName() const
{
  return m_impl->getAppName();
}

const Error& RtApplication::getLastError() const
{
  return m_impl->getLastError();
}

AppMode_t RtApplication::getOperationMode() const
{
  return m_impl->getOperationMode();
}

AppState_t RtApplication::getOperationState() const
{
  return m_impl->getOperationState();
}

RtEnvironment* RtApplication::getEnvironment(const char* name)
{
  // TODO Найти и вернуть среду по имени
  //return m_impl->getEnvironment(name);
  return NULL;
}

