#include "glog/logging.h"

#include "config.h"

#include "xdb_core_environment.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"
#include "xdb_rtap_database.hpp"
#include "mdp_letter.hpp"

using namespace xdb;

RtEnvironment::RtEnvironment(Application* _app, const char* _name)
{
  m_impl = new Environment(_app, _name);
  m_database = NULL;
  m_conn = NULL;
}

RtEnvironment::~RtEnvironment()
{
  delete m_conn;
  if (m_database)
  {
    m_database->Disconnect();
    delete m_database;
  }
  delete m_impl;
}

// Вернуть имя подключенной БД/среды
const char* RtEnvironment::getName() const
{
  return m_impl->getName();
}

// вернуть подключение к указанной БД/среде
RtConnection* RtEnvironment::getConnection()
{
  // TODO реализация
  if (!m_conn)
  {
    m_conn = new RtConnection(this);
  }
  return m_conn;
}

// Загрузить ранее сохраненный снимок для указанного Приложения
const Error& RtEnvironment::LoadSnapshot(const char *filename)
{
  if (!m_database)
  {
    m_database = new RtCoreDatabase();
    LOG(INFO) << "Lazy creating database " << m_database->getName();
    m_database->Connect();
    LOG(INFO) << "Lazy connection to database " << m_database->getName();
  }
  return m_database->LoadSnapshot(/*m_impl->getAppName(),*/ filename);
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

// Изменить состояние Среды
void RtEnvironment::setEnvState(EnvState_t _new_state)
{
  m_impl->setEnvState(_new_state);
}

EnvState_t RtEnvironment::getEnvState() const
{
  return m_impl->getEnvState();
}

const Error& RtEnvironment::getLastError() const
{
  return m_impl->getLastError();
}

// Запустить исполнение
const Error& RtEnvironment::Start()
{
  // TODO Создать экземпляр базы данных на основе ранее загруженных словарей
  m_impl->setLastError(rtE_NOT_IMPLEMENTED);

  if (m_impl->getLastError().Ok())
  {
    m_impl->setEnvState(ENV_STATE_DB_OPEN);
  }
  return m_impl->getLastError();
}

// Загрузить справочники
const Error& RtEnvironment::LoadDictionary()
{
  m_impl->setLastError(rtE_NOT_IMPLEMENTED);

  if (m_impl->getLastError().Ok())
  {
    m_impl->setEnvState(ENV_STATE_INIT);
  }
  return m_impl->getLastError();
}

// Завершить исполнение
const Error& RtEnvironment::Shutdown(EnvShutdownOrder_t order)
{
  switch(order)
  {
    case ENV_SHUTDOWN_SOFT:
      LOG(INFO) << "Soft shuttingdown";
      break;

    case ENV_SHUTDOWN_HARD:
      LOG(INFO) << "Hard shuttingdown";
      break;
  }

  m_impl->setLastError(rtE_NOT_IMPLEMENTED);
  return m_impl->getLastError();
}
