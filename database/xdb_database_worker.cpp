#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include "xdb_database_worker.hpp"
#include "helper.hpp"

using namespace xdb;

Worker::Worker()
{
  memset((void*)&m_expiration, '\0', sizeof(m_expiration));
  m_identity[0] = '\0';
  m_id = m_service_id = 0;
  m_state = DISARMED;
  m_modified = false;
}

// NB: Если self_id равен нулю, значит объект пока не содержится в базе данных
Worker::Worker(int64_t     _service_id,
               const char *_self_identity,
               int64_t     _self_id)
{
  timer_mark_t mark = {0, 0};

  SetID(_self_id);
  SetSERVICE_ID(_service_id);
  SetIDENTITY(_self_identity);
  SetEXPIRATION(mark);
  SetSTATE(INIT);
}

Worker::~Worker()
{
}

void Worker::SetID(int64_t _id)
{
  m_id = _id;
  m_modified = true;
}

void Worker::SetSERVICE_ID(int64_t _service_id)
{
  m_service_id = _service_id;
  m_modified = true;
}

void Worker::SetIDENTITY(const char *_identity)
{
  assert(_identity);

  if (!_identity) return;

  /* удалить старое значение идентификатора Обработчика */
  strncpy(m_identity, _identity, WORKER_IDENTITY_MAXLEN);
  m_identity[WORKER_IDENTITY_MAXLEN] = '\0';

  m_modified = true;
}

void Worker::SetVALID()
{
  m_modified = false;
}

bool Worker::GetVALID() const
{
  return (m_modified == false);
}

void Worker::SetSTATE(const State _state)
{
  m_state = _state;
}

Worker::State Worker::GetSTATE() const
{
  return m_state;
}

const char* Worker::GetIDENTITY() const
{
  return m_identity;
}

void Worker::SetEXPIRATION(const timer_mark_t& _expiration)
{
  m_expiration.tv_sec = _expiration.tv_sec;
  m_expiration.tv_nsec = _expiration.tv_nsec;
  m_modified = true;
}

timer_mark_t Worker::GetEXPIRATION() const
{
  return m_expiration;
}

// Проверка превышения текущего времени отметки expiration
// NB: Может учитывать и милисекунды, но пока проверка разницы 'мс' отключена
bool Worker::Expired() const
{
  timer_mark_t now_time;
  bool expired = false;

  if (GetTimerValue(now_time))
  {
    if ((m_expiration.tv_sec < now_time.tv_sec)
#if defined USE_MSEC_IN_EXPIRATION
     || (m_expiration.tv_nsec < now_time.tv_nsec)
#endif
     )
    expired = true;
  }
  else throw;

  return expired;
}

