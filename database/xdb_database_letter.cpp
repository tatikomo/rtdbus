#include <glog/logging.h>
#include <assert.h>
#include <iostream>
#include <string.h>

#include "config.h"
#include "zmsg.hpp"
#include "msg_common.h"
#include "msg_message.hpp"
#include "xdb_database_service.hpp"
#include "xdb_database_worker.hpp"
#include "xdb_database_letter.hpp"

using namespace xdb;

Letter::Letter() :
  m_id(0),
  m_service_id(0),
  m_worker_id(0),
  m_state(EMPTY),
  m_modified(false)
{
  timer_mark_t tictac = {0, 0};
  SetEXPIRATION(tictac);
}

Letter::Letter(const int64_t _self_id, const int64_t _srv_id, const int64_t _wrk_id) :
  m_id(_self_id),
  m_service_id(_srv_id),
  m_worker_id(_wrk_id),
  m_state(UNASSIGNED),
  m_modified(true)
{
  timer_mark_t tictac = {0, 0};
  SetEXPIRATION(tictac);
}

Letter::~Letter()
{
}

// на входе - объект zmsg
Letter::Letter(void* data) :
  m_id(0),
  m_service_id(0),
  m_worker_id(0),
  m_state(UNASSIGNED),
  m_modified(true)
{
  mdp::zmsg *msg = static_cast<mdp::zmsg*> (data);
  timer_mark_t tictac = {0, 0};
  SetEXPIRATION(tictac);

  assert(msg);
  // Прочитать служебные поля транспортного сообщения zmsg
  // и восстановить на его основе прикладное сообщение.
  // Первый фрейм - адрес возврата,
  // Два последних фрейма - заголовок и тело сообщения.
  strncpy(m_reply_to, msg->get_part(0)->c_str(), WORKER_IDENTITY_MAXLEN);
  m_reply_to[WORKER_IDENTITY_MAXLEN] = '\0';

  int msg_frames = msg->parts();
  assert(msg_frames >= 2);

  const std::string* head = msg->get_part(msg_frames-2);
  const std::string* body = msg->get_part(msg_frames-1);

  assert (head);
  assert (body);

  m_frame_header.assign(head->data(), head->size());
  m_frame_data.assign(body->data(), body->size());
}

// Создать экземпляр на основе заголовка и тела сообщения
Letter::Letter(const char* _reply_to, const std::string& _head, const std::string& _data) :
  m_frame_header(_head),
  m_frame_data(_data),
  m_id(0),
  m_service_id(0),
  m_worker_id(0),
  m_state(UNASSIGNED),
  m_modified(true)
{
  timer_mark_t tictac = {0, 0};
  strncpy(m_reply_to, _reply_to, WORKER_IDENTITY_MAXLEN);
  m_reply_to[WORKER_IDENTITY_MAXLEN] = '\0';
  SetEXPIRATION(tictac);
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

void Letter::SetEXPIRATION(const timer_mark_t& _exp_mark)
{
  m_expiration.tv_sec  = _exp_mark.tv_sec;
  m_expiration.tv_nsec = _exp_mark.tv_nsec;
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

int64_t Letter::GetID() const
{
  return m_id;
}

int64_t Letter::GetSERVICE_ID() const
{
  return m_service_id;
}

int64_t Letter::GetWORKER_ID() const
{
  return m_worker_id;
}

const std::string& Letter::GetHEADER()
{
  return m_frame_header;
}

void Letter::SetHEADER(const std::string& head)
{
  m_frame_header.assign(head);
}

void Letter::SetDATA(const std::string& body)
{
  m_frame_data.assign(body);
}

void Letter::SetREPLY_TO(const char* reply_to)
{
  strncpy(m_reply_to, reply_to, WORKER_IDENTITY_MAXLEN);
  m_reply_to[WORKER_IDENTITY_MAXLEN] = '\0';
}

const std::string& Letter::GetDATA()
{
  return m_frame_data;
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

void Letter::Dump()
{
  LOG(INFO)
    <<"Letter:" << m_id
    <<" srv:"   << m_service_id
    <<" wrk:"   << m_worker_id;
}
