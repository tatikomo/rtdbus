#include <assert.h>
#include <string.h>
#include "xdb_database_service.hpp"

Service::Service()
{
  m_id = 0;
  m_waiting_workers_count = 0;
  m_name = NULL;
  m_modified = false;
}

Service::Service(int64_t _id, const char *_name)
{
  m_name = NULL;
  m_waiting_workers_count = 0;
  SetID(_id);
  SetNAME(_name);
}

Service::~Service()
{
  delete []m_name;
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

const int64_t Service::GetID()
{
  return m_id;
}

const char* Service::GetNAME()
{
  return m_name;
}

void Service::SetWaitingWorkersCount(int _count)
{
  m_waiting_workers_count = _count;
}

const int Service::GetWaitingWorkersCount()
{
  return m_waiting_workers_count;
}
