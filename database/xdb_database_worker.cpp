#include <assert.h>
#include <string.h>
#include "xdb_database_worker.hpp"

Worker::Worker()
{
  m_id = 0;
  m_expiration = 0;
  m_identity = NULL;
  m_modified = false;
}

Worker::Worker(int64_t _self_id, const char *_identity, int64_t _service_id)
{
  m_identity = NULL;
  SetID(_self_id);
  SetServiceID(_service_id);
  SetIDENTITY(_identity);
}

#if 0
Worker::Worker(const char *_identity)
{
  m_id = 0;
  m_identity = NULL;
  SetIDENTITY(_identity);
}
#endif

Worker::~Worker()
{
  delete []m_identity;
}

void Worker::SetID(int64_t _id)
{
  m_id = _id;
  m_modified = true;
}

void Worker::SetServiceID(int64_t _service_id)
{
  m_service_id = _service_id;
  m_modified = true;
}

void Worker::SetIDENTITY(const char *_identity)
{
  assert(_identity);

  if (!_identity) return;

  /* удалить старое значение идентификатора Обработчика */
  delete []m_identity;

  m_identity = new char[strlen(_identity)+1];
  strcpy(m_identity, _identity);

  m_modified = true;
}

const int64_t Worker::GetID()
{
  return m_id;
}

const char* Worker::GetIDENTITY()
{
  return m_identity;
}

void Worker::SetEXPIRATION(int64_t _expiration)
{
  m_expiration = _expiration;
  m_modified = true;
}

const int64_t Worker::GetEXPIRATION()
{
  return m_expiration;
}

bool Worker::Expired()
{
  return false;
}

