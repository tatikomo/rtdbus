#include <glog/logging.h>
#include <assert.h>
#include <string.h>
#include "xdb_database_service.hpp"
#include "xdb_database_worker.hpp"
#include "xdb_database_letter.hpp"

Letter::Letter()
{
  m_id = m_service_id = m_worker_id = 0;
  m_expiration.tv_sec = 0;
  m_expiration.tv_nsec = 0;
  m_modified = false;
}

Letter::Letter(const int64_t _self_id, const int64_t _srv_id, const int64_t _wrk_id)
: m_id(_self_id), m_service_id(_srv_id), m_worker_id(_wrk_id)
{
  m_modified = true;
}

Letter::~Letter()
{
}

void Letter::SetID(int64_t self_id)
{
  m_id = self_id;
  m_modified = true;
}

void Letter::SetSERVICE(Service* srv)
{
  m_service_id = (NULL == srv)? 0 : srv->GetID();
  m_modified = true;
}

void Letter::SetSERVICE_ID(int64_t srv_id)
{
  m_service_id = srv_id;
  m_modified = true;
}

void Letter::SetWORKER(Worker* wrk)
{
  if (wrk)
  {
    m_worker_id = wrk->GetID();
    if (!m_service_id)
    {
      // Ранее не было заполнено, сделаем сейчас
      m_service_id = wrk->GetSERVICE_ID();
    }
    else
    {
      // Было присвоено ранее
      // Проверим, не изменилась ли привязка к Сервису
      if (wrk->GetSERVICE_ID() && m_service_id != wrk->GetSERVICE_ID())
      {
        LOG(ERROR) <<"Change previously assigned service id("
                   <<m_service_id<<" to new id("<<wrk->GetSERVICE_ID()<<")";
      }
    }
  }
}

void Letter::SetWORKER_ID(int64_t wrk_id)
{
  m_worker_id = wrk_id;
  m_modified = true;
}

void Letter::SetSTATE(State new_state)
{
  m_state = new_state;
}

int64_t Letter::GetID()
{
  return m_id;
}

int64_t Letter::GetSERVICE_ID()
{
  return m_service_id;
}

int64_t Letter::GetWORKER_ID()
{
  return m_worker_id;
}

const std::string& Letter::GetHEAD()
{
  return m_head;
}

void Letter::SetHEAD(const std::string& head)
{
  m_head.assign(head);
}

void Letter::SetBODY(const std::string& body)
{
  m_body.assign(body);
}

const std::string& Letter::GetBODY()
{
  return m_body;
}

Letter::State Letter::GetSTATE()
{
  return m_state;
}

/* Подтвердить соответствие состояния объекта своему отображению в БД */
void Letter::SetVALID()
{
  m_modified = false;
}

/* Соответствует или нет экземпляр своему хранимому в БД состоянию */
bool Letter::GetVALID()
{
  return (m_modified == false);
}




