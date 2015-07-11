#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_common.hpp"
#include "xdb_impl_environment.hpp"
#include "xdb_impl_application.hpp"
#include "xdb_rtap_application.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"
#include "xdb_rtap_database.hpp"
//#include "mdp_letter.hpp"

using namespace xdb;

RtEnvironment::RtEnvironment(RtApplication* _app, const char* _name) :
  m_impl(NULL),
  m_appli(_app),
  m_conn(NULL),
  m_database(NULL)
{
  m_impl = new EnvironmentImpl(_app->getImpl(), _name);
  m_database = new RtDatabase(_name, _app->getOptions());
  // NB: Подключение к БД происходит при создании экземпляра RtConnection
  LOG(INFO) << "Creating database " << m_database->getName();
}

RtEnvironment::~RtEnvironment()
{
// Подключения должны удаляться в вызывающем контексте
//1  delete m_conn;
  delete m_impl;
//  if (m_database)
//  {
    delete m_database; // Disconnect при удалении автоматически
//  }
}

// Вернуть имя подключенной БД/среды
const char* RtEnvironment::getName() const
{
  return m_impl->getName();
}

// вернуть подключение к указанной БД/среде
// TODO: каждая нить, требующая подключения к БД, должна выполнить mco_db_connection(),
// чтобы получить свой mco_db_h.
RtConnection* RtEnvironment::getConnection()
{
  switch(m_database->State())
  {
    case DB_STATE_UNINITIALIZED:
        m_database->Init();
        // NB: break пропущен специально
    case DB_STATE_INITIALIZED:
    case DB_STATE_ATTACHED:
        // Если мы еще не подключились к БД, самое время сделать это сейчас
        // Connect внутри, если состояние БД = UNINITIALIZED, выполнит инициализацию
        m_database->Connect();
        LOG(INFO) << "Connection to database " << m_database->getName();
        // NB: break пропущен специально

    case DB_STATE_CONNECTED:
//1        if (!m_conn)
        {
          // Каждый раз возвращать новое подключение, поскольку вызовы могут быть из разных нитей
          // Каждая нить владеет одним подключением, и явно закрывает его при завершении работы.
          m_conn = new RtConnection(m_database);
          LOG(INFO) << "Created new RtConnection " << m_conn;
        }
    break;

//    case DB_STATE_UNINITIALIZED:
    case DB_STATE_DISCONNECTED:
        LOG(ERROR) << "Unable to get connection to disconnected or uninitialized database";

    default:
        LOG(WARNING) << "GEV: add state " << m_database->State() << " to processing";
  }

  return m_conn;
}

// Загрузить ранее сохраненный снимок для указанного Приложения
const Error& RtEnvironment::LoadSnapshot(const char *filename)
{
/*  if (!m_database)
  {
    m_database = new RtDatabase(m_impl->getName(), m_appli->getOptions());
    LOG(INFO) << "Lazy creating database " << m_database->getName();
    m_database->Connect();
    LOG(INFO) << "Lazy connection to database " << m_database->getName();
  }*/

  if (m_impl->getLastError().Ok())
  {
    m_database->LoadSnapshot(filename);
  }

  return m_impl->getLastError();
}

const Error& RtEnvironment::MakeSnapshot(const char *filename)
{
  return m_database->MakeSnapshot(filename);
}

/*mdp::Letter* RtEnvironment::createMessage(int)
{
  m_impl->setLastError(rtE_NOT_IMPLEMENTED);
  return NULL;
}

const Error& RtEnvironment::sendMessage(mdp::Letter* _letter)
{
  assert(_letter);

  m_impl->setLastError(rtE_NOT_IMPLEMENTED);
  return m_impl->getLastError();
}*/

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

RtDatabase* RtEnvironment::getDatabase()
{
  return m_database;
}
