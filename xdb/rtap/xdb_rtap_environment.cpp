#include "config.h"

#include "xdb_core_environment.hpp"
#include "xdb_rtap_environment.hpp"
#include "mdp_letter.hpp"

using namespace xdb;

RtEnvironment::RtEnvironment(Application* _app, const char* _name)
{
  m_impl = new Environment(_app, _name);
}

RtEnvironment::~RtEnvironment()
{
  delete m_impl;
}

// Вернуть имя подключенной БД/среды
const char* RtEnvironment::getName() const
{
  return m_impl->getName();
}

// Создать и вернуть новое подключение к указанной БД/среде
RtConnection* RtEnvironment::createConnection()
{
  // TODO реализация
  return 0;
}

const Error& RtEnvironment::LoadSnapshot(const char *filename)
{
  return m_impl->LoadSnapshot(filename);
}

const Error& RtEnvironment::MakeSnapshot(const char *filename)
{
  return m_impl->MakeSnapshot(filename);
}

mdp::Letter* RtEnvironment::createMessage(int)
{
  m_impl->setLastError(rtE_NOT_IMPLEMENTED);
  return NULL;
}

const Error& RtEnvironment::sendMessage(mdp::Letter* _letter)
{
  assert(_letter);

  m_impl->setLastError(rtE_NOT_IMPLEMENTED);
  return m_impl->getLastError();
}

EnvState_t RtEnvironment::getEnvState() const
{
  return m_impl->getEnvState();
}

const Error& RtEnvironment::getLastError() const
{
  return m_impl->getLastError();
}
