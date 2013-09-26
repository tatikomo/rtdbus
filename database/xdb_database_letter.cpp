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

// на входе - объект zmsg
Letter::Letter(void* data) : m_modified(true)
{
  mdp::zmsg *msg = static_cast<mdp::zmsg*> (data);

  assert(msg);
  SetID(0);
  SetWORKER_ID(0);
  SetSERVICE_ID(0);

  // Прочитать служебные поля транспортного сообщения zmsg
  // и восстановить на его основе прикладное сообщение.
  // Первый фрейм - адрес возврата,
  // Два последних фрейма - заголовок и тело сообщения.
  strncpy(m_reply_to, msg->get_part(0).c_str(), WORKER_IDENTITY_MAXLEN);
  m_reply_to[WORKER_IDENTITY_MAXLEN] = '\0';
//  LOG(INFO) << "Reply to " << m_reply_to << std::endl;
//  std::cout << "Reply to " << m_reply_to << std::endl;

  assert(msg->parts() >= 2);
  int msg_frames = msg->parts();

  m_frame_header.assign(msg->get_part(msg_frames-2));

  m_frame_data.assign(msg->get_part(msg_frames-1));
}

// Создать экземпляр на основе заголовка и тела сообщения
Letter::Letter(std::string& reply, msg::Header *h, std::string& b) : m_modified(true)
{
  // TODO сделать копию заголовка, т.к. он создан в вызывающем контексте и нами не управляется
  assert(!reply.empty());
  assert(h);
  assert(!b.empty());
  assert(1 == 0);
}

// Создать экземпляр на основе заголовка и тела сообщения
Letter::Letter(const char* _reply_to, const std::string& _head, const std::string& _data)
{
  m_frame_header = _head;
  m_frame_data = _data;

  strncpy(m_reply_to, _reply_to, WORKER_IDENTITY_MAXLEN);
  m_reply_to[WORKER_IDENTITY_MAXLEN] = '\0';

  if (false == m_header.ParseFrom(m_frame_header))
    LOG(ERROR) << "Unable to deserialize header for reply to "<<m_reply_to;

  SetID(0);
  SetWORKER_ID(0);
  SetSERVICE_ID(0);
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

int8_t Letter::GetPROTOCOL_VERSION() const
{
//  assert(m_header);
  return m_header.get_protocol_version();
}

rtdbExchangeId Letter::GetEXCHANGE_ID() const
{
//  assert(m_header);
  return m_header.get_exchange_id();
}

rtdbPid Letter::GetSOURCE_PID() const
{
//  assert(m_header);
  return m_header.get_source_pid();
}

const std::string& Letter::GetPROC_DEST() const
{
  return m_header.get_proc_dest();
}

const std::string& Letter::GetPROC_ORIGIN() const
{
  return m_header.get_proc_origin();
}

rtdbMsgType Letter::GetSYS_MSG_TYPE() const
{
  return m_header.get_sys_msg_type();
}

rtdbMsgType Letter::GetUSR_MSG_TYPE() const
{
  return m_header.get_usr_msg_type();
}

std::ostream& operator<<(std::ostream& os, const Letter& letter)
{
  os << "Letter:"<<letter.GetID()
    <<" srv:"   << letter.GetSERVICE_ID()
    <<" wrk:"   << letter.GetWORKER_ID()
    <<" prot:"  << letter.GetPROTOCOL_VERSION()
    <<" exchg:" << letter.GetEXCHANGE_ID()
    <<" spid:"  << letter.GetSOURCE_PID()
    <<" reply:'"<< letter.GetREPLY_TO()
    <<"' dest:'"<< letter.GetPROC_DEST()
    <<"' orig:'"<< letter.GetPROC_ORIGIN()
    <<"' sys_t:"<< letter.GetSYS_MSG_TYPE()
    <<" usr_t:" << letter.GetUSR_MSG_TYPE() << std::endl;
    return os;
}

void Letter::Dump()
{
  LOG(INFO)
    <<"Letter:" << m_id
    <<" srv:"   << m_service_id
    <<" wrk:"   << m_worker_id
    <<" prot:"  << (int)GetPROTOCOL_VERSION()
    <<" exchg:" << GetEXCHANGE_ID()
    <<" spid:"  << GetSOURCE_PID()
    <<" reply:'"<< GetREPLY_TO()
    <<"' dest:'"<< GetPROC_DEST()
    <<"' orig:'"<< GetPROC_ORIGIN()
    <<"' sys_t:"<< GetSYS_MSG_TYPE()
    <<" usr_t:" << GetUSR_MSG_TYPE();
}
