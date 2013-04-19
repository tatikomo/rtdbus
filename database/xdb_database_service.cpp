#include <assert.h>
#include <string.h>
#include "xdb_database_service.hpp"

XDBService::XDBService()
{
  m_id = -1;
  m_name = NULL;
  m_modified = false;
}

XDBService::XDBService(int64_t _id, const char *_name)
{
  m_name = NULL;
  SetID(_id);
  SetNAME(_name);
}

XDBService::~XDBService()
{
  if (m_name)
    delete m_name;
}

void XDBService::SetID(int64_t _id)
{
  m_id = _id;
  m_modified = true;
}

void XDBService::SetNAME(const char *_name)
{
  assert(_name);

  if (!_name) return;

  /* удалить старое значение названия сервиса */
  if (m_name)
    delete m_name;

  m_name = new char[strlen(_name)+1];
  strcpy(m_name, _name);

  m_modified = true;
}

const int64_t XDBService::GetID()
{
  return m_id;
}

const char* XDBService::GetNAME()
{
  return m_name;
}

