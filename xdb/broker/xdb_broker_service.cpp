#include <assert.h>
#include <string.h>

#include "xdb_broker_service.hpp"

using namespace xdb;

Service::Service() :
  m_id(0),
  m_name(NULL),
  m_state(DISABLED),
  m_modified(false)
{
}

Service::Service(const int64_t _id, const char *_name) :
  m_name(NULL)
{
  SetID(_id);
  SetSTATE(DISABLED);
  SetNAME(_name);
}

Service::~Service()
{
  delete []m_name;
}

// TODO: проверить обоснованность присваивания
void Service::SetSTATE(State _state)
{
  m_state = _state;
  m_modified = true;
}

Service::State Service::GetSTATE()
{
  return m_state;
}

void Service::SetID(int64_t _id)
{
  m_id = _id;
  m_modified = true;
}

void Service::SetNAME(const char *_name)
{
  assert(_name);

  if (!_name) return;

  /* удалить старое значение названия сервиса */
  delete []m_name;

  m_name = new char[strlen(_name)+1];
  strcpy(m_name, _name);

  m_modified = true;
}

int64_t Service::GetID()
{
  return m_id;
}

const char* Service::GetNAME()
{
  return m_name;
}

/* Подтвердить соответствие состояния объекта своему отображению в БД */
void Service::SetVALID()
{
  m_modified = false;
}

/* Соответствует или нет экземпляр своему хранимому в БД состоянию */
bool Service::GetVALID()
{
  return (m_modified == false);
}

