#include <assert.h>
#include <string.h>

#include "xdb_broker_service.hpp"

using namespace xdb;

Service::Service() :
  m_id(0),
  m_name(NULL),
  m_endpoint(NULL),
  m_state(DISABLED),
  m_modified(false)
{
}

Service::Service(const int64_t _id, const char *_name) :
  m_id(_id),
  m_name(NULL),
  m_endpoint(NULL),
  m_state(DISABLED),
  m_modified(true)
{
  SetNAME(_name);
}

Service::~Service()
{
  delete []m_name;
  delete []m_endpoint;
}

// TODO: проверить обоснованность присваивания
void Service::SetSTATE(State _state)
{
  m_state = _state;
  m_modified = true;
}

Service::State Service::GetSTATE() const
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

void Service::SetENDPOINT(const char *_endpoint)
{
  assert(_endpoint);

  if (!_endpoint) return;

  /* удалить старое значение названия сервиса */
  delete []_endpoint;

  _endpoint = new char[strlen(_endpoint)+1];
  strcpy(_endpoint, _endpoint);

  m_modified = true;
}

int64_t Service::GetID()
{
  return m_id;
}

const char* Service::GetNAME() const
{
  return m_name;
}

const char* Service::GetENDPOINT() const
{
  return m_endpoint;
}

/* Подтвердить соответствие состояния объекта своему отображению в БД */
void Service::SetVALID()
{
  m_modified = false;
}

/* Соответствует или нет экземпляр своему хранимому в БД состоянию */
bool Service::GetVALID() const
{
  return (m_modified == false);
}

