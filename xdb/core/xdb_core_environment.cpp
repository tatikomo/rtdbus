#include <assert.h>
#include <string.h>

#include "glog/logging.h"

#include "config.h"
#include "xdb_core_application.hpp"
#include "xdb_core_environment.hpp"
#include "xdb_core_connection.hpp"
#include "xdb_core_base.hpp"
#include "xdb_core_error.hpp"

using namespace xdb::core;

Environment::Environment(Application* _app, const char* _name) :
  m_state(CONDITION_UNKNOWN),
  m_last_error(rtE_NONE),
  m_appli(_app)
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

#if 1
  /* NB: Конкретный экземпляр базы данных создается в дочернем классе */
  m_database_impl = NULL; /* new CoreDatabase(_name, open_flags);*/
  m_state = CONDITION_BAD;
#else
  // Открыть базу данных с именем m_name
  if (true == m_database_impl->Init())
  {
    if (openDB().getCode() == rtE_NONE)
      m_state = CONDITION_INIT;
    LOG(INFO) << "Init is successful";
  }
  else
  {
    m_state = CONDITION_BAD;
    LOG(INFO) << "Init not successful";
  }
#endif
}

Environment::~Environment()
{
#if 0
  delete m_database_impl;
#endif
}

const Error& Environment::getLastError() const
{
  return m_last_error;
}

void Environment::setLastError(const Error& _new_error)
{
  m_last_error.set(_new_error);
}

void Environment::clearError()
{
  m_last_error.clear();
}

// Вернуть имя подключенной БД/среды
const char* Environment::getName() const
{
  return m_name;
}

// TODO: Создать и вернуть новое подключение к указанной БД/среде
Connection* Environment::createConnection()
{
  Connection* conn = NULL;
  LOG(INFO) << "Get connection to env " << m_name;
  conn = new Connection(this);
  return conn;
}

#if 0
// TODO: Создать новое сообщение указанного типа
mdp::Letter* Environment::createMessage(/* msgType */)
{
  m_last_error = rtE_NOT_IMPLEMENTED;
  return NULL;
}

// Отправить сообщение адресату
Error& Environment::sendMessage(mdp::Letter* letter)
{
  m_last_error = rtE_NONE;
  assert(letter);
  return m_last_error;
}
#endif

Error& Environment::MakeSnapshot(const char *filename)
{
  m_last_error = rtE_NONE;

  assert(filename);

#if 0
  // Допустимым форматом хранения XML-снимков БДРВ является встроенный в eXtremeDB формат.
  // Это снимает зависимость от библиотеки xerces в runtime луркера.
  m_database_impl->StoreSnapshot(filename);
//    m_last_error.set(rtE_XML_NOT_OPENED);
#endif
  return m_last_error;
}

// Открыть БД без создания подключений
// Подключения создаются в классе DbConection
// TODO: определить, какую словарную функцию использует данная среда
Error& Environment::openDB()
{
  m_last_error = rtE_NONE;

  // Действительное открытие БД происходит в дочернем классе
#if 1
    m_last_error.set(rtE_DB_NOT_OPENED);
#else
  // Открыть БД с помощью mco_db_open(m_name)...
  if (false == m_database_impl->Connect())
  {
    m_last_error.set(rtE_DB_NOT_OPENED);
  }
#endif

  return m_last_error;
}
