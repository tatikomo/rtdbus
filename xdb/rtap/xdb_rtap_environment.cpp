#include <string.h>

#include "config.h"
#include "xdb_rtap_environment.hpp"

using namespace xdb;

RtEnvironment::RtEnvironment(const char* _name) :
  m_last_error(ERROR_NONE)
{
  strncpy(m_name, _name, IDENTITY_MAXLEN);
  m_name[IDENTITY_MAXLEN] = '\0';
}

RtEnvironment::~RtEnvironment()
{
}

RtEnvironment::RtError RtEnvironment::getLastError() const
{
  return m_last_error;
}

// Вернуть имя подключенной ДБ/среды
const char* RtEnvironment::getName() const
{
  return m_name;
}

// TODO: Создать и вернуть новое подключение к указанной БД/среде
RtDbConnection* RtEnvironment::createDbConnection()
{
  return NULL;
}

// TODO: Создать новое сообщение указанного типа
mdp::Letter* RtEnvironment::createMessage(/* msgType */)
{
  return NULL;
}

// Отправить сообщение адресату
RtEnvironment::RtError RtEnvironment::sendMessage(mdp::Letter* letter)
{
  m_last_error = ERROR_NONE;
  assert(letter);
  return m_last_error;
}

