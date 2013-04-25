#include <assert.h>
#include <string.h>
#include "xdb_database_worker.hpp"

XDBWorker::XDBWorker()
{
  m_id = 0;
  m_identity = NULL;
  m_modified = false;
}

XDBWorker::XDBWorker(int64_t _id, const char *_identity)
{
  m_identity = NULL;
  SetID(_id);
  SetIDENTITY(_identity);
}

XDBWorker::XDBWorker(const char *_identity)
{
  m_id = 0;
  m_identity = NULL;
  SetIDENTITY(_identity);
}

XDBWorker::~XDBWorker()
{
  if (m_identity)
    delete m_identity;
}

void XDBWorker::SetID(int64_t _id)
{
  m_id = _id;
  m_modified = true;
}

void XDBWorker::SetIDENTITY(const char *_identity)
{
  assert(_identity);

  if (!_identity) return;

  /* удалить старое значение идентификатора Обработчика */
  if (m_identity)
    delete m_identity;

  m_identity = new char[strlen(_identity)+1];
  strcpy(m_identity, _identity);

  m_modified = true;
}

const int64_t XDBWorker::GetID()
{
  return m_id;
}

const char* XDBWorker::GetIDENTITY()
{
  return m_identity;
}

