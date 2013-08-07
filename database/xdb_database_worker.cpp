#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include "xdb_database_worker.hpp"
#include "helper.hpp"

Worker::Worker()
{
  memset((void*)&m_expiration, '\0', sizeof(m_expiration));
  m_identity = NULL;
  m_modified = false;
}

/*
 * NB если self_id == -1, значит идентификатор будет 
 * автоматически сгенерирован базой данных 
 */
Worker::Worker(const char *_identity, int64_t _service_id)
{
  timer_mark_t mark = {0, 0};

  m_identity = NULL;
  SetSERVICE_ID(_service_id);
  SetIDENTITY(_identity);
  SetEXPIRATION(mark);
  SetINDEX(-1);
  SetSTATE(INIT);
}


Worker::~Worker()
{
  delete []m_identity;
}


void Worker::SetSERVICE_ID(int64_t _service_id)
{
  m_service_id = _service_id;
  m_modified = true;
}

const int64_t Worker::GetSERVICE_ID()
{
  return m_service_id;
}

void Worker::SetIDENTITY(const char *_identity)
{
  assert(_identity);

  if (!_identity) return;

  /* удалить старое значение идентификатора Обработчика */
  delete []m_identity;

//  printf("Worker::SetIDENTITY('%s' %d)\n", _identity, strlen(_identity));
  m_identity = new char[strlen(_identity)+1];
  strcpy(m_identity, _identity);

  m_modified = true;
}

void Worker::SetINDEX(const uint16_t &new_index)
{
  m_index_in_spool = new_index;
  m_modified = true;
}

void Worker::SetVALID()
{
  m_modified = false;
}

bool Worker::GetVALID()
{
  return (m_modified == false);
}

void Worker::SetSTATE(const State _state)
{
  m_state = _state;
}

const Worker::State Worker::GetSTATE()
{
  return m_state;
}

const uint16_t Worker::GetINDEX()
{
  return m_index_in_spool;
}

const char* Worker::GetIDENTITY()
{
  return m_identity;
}

void Worker::SetEXPIRATION(const timer_mark_t& _expiration)
{
  memcpy((void*)&m_expiration, 
         (void*) &_expiration, 
         sizeof (m_expiration));
/*  printf("SetEXPIRATION(sec=%ld, nsec=%ld)\n", 
         m_expiration.tv_sec,
         m_expiration.tv_nsec);*/
  m_modified = true;
}

const timer_mark_t Worker::GetEXPIRATION()
{
  return m_expiration;
}

// Проверка превышения текущего времени отметки expiration
bool Worker::Expired()
{
  timer_mark_t now_time;
  bool expired = false;

  if (GetTimerValue(now_time))
  {
    if ((m_expiration.tv_sec < now_time.tv_sec)
     && (m_expiration.tv_nsec < now_time.tv_nsec))
    expired = true;
  }
  else throw;

  return expired;
}

