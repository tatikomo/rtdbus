#include <string.h>
#include <bitset>

#include "glog/logging.h"

#include "config.h"
#include "xdb_rtap_application.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"
#include "xdb_rtap_database.hpp"
#include "xdb_rtap_error.hpp"

using namespace xdb;

RtEnvironment::RtEnvironment(RtApplication* _app, const char* _name) :
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

  m_database_impl = new RtCoreDatabase(_name, open_flags);

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
}

RtEnvironment::~RtEnvironment()
{
  delete m_database_impl;
}

const RtError& RtEnvironment::getLastError() const
{
  return m_last_error;
}

// Вернуть имя подключенной БД/среды
const char* RtEnvironment::getName() const
{
  return m_name;
}

// TODO: Создать и вернуть новое подключение к указанной БД/среде
RtDbConnection* RtEnvironment::createDbConnection()
{
  RtDbConnection* conn = NULL;
  LOG(INFO) << "Get connection to env " << m_name;
  conn = new RtDbConnection(this);
  return conn;
}

// TODO: Создать новое сообщение указанного типа
mdp::Letter* RtEnvironment::createMessage(/* msgType */)
{
  m_last_error = rtE_NOT_IMPLEMENTED;
  return NULL;
}

// Отправить сообщение адресату
RtError& RtEnvironment::sendMessage(mdp::Letter* letter)
{
  m_last_error = rtE_NONE;
  assert(letter);
  return m_last_error;
}

RtError& RtEnvironment::MakeSnapshot(const char *filename)
{
  m_last_error = rtE_NONE;

  assert(filename);

  // Допустимым форматом хранения XML-снимков БДРВ является встроенный в eXtremeDB формат.
  // Это снимает зависимость от библиотеки xerces в runtime луркера.
  m_database_impl->StoreSnapshot(filename);
//    m_last_error.set(rtE_XML_NOT_OPENED);

  return m_last_error;
}

// Открыть БД без создания подключений
// Подключения создаются в классе RtDbConection
// TODO: определить, какую словарную функцию использует данная среда
RtError& RtEnvironment::openDB()
{
  bool status;

  m_last_error = rtE_NONE;
  // Открыть БД с помощью mco_db_open(m_name)...
  if (false == (status = m_database_impl->Connect()))
  {
    m_last_error.set(rtE_DB_NOT_OPENED);
  }
  else
  {
    m_state = CONDITION_DB_OPEN;
  }

  return m_last_error;
}
