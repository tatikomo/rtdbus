#include <glog/logging.h>
#include <assert.h>
#include <string.h>
#include "zmsg.hpp"
#include "msg_common.h"
#include "msg_message.hpp"
#include "xdb_database_service.hpp"
#include "xdb_database_worker.hpp"
#include "xdb_database_letter.hpp"

Letter::Letter()
{
  m_id = m_service_id = m_worker_id = 0;
  m_expiration.tv_sec = 0;
  m_expiration.tv_nsec = 0;
  m_header = NULL;
  m_modified = false;
}

Letter::Letter(const int64_t _self_id, const int64_t _srv_id, const int64_t _wrk_id)
: m_id(_self_id), m_service_id(_srv_id), m_worker_id(_wrk_id)
{
  m_header = NULL;
  m_modified = true;
}

Letter::~Letter()
{
  delete m_header;
}

// на входе - объект zmsg
Letter::Letter(void* data) : m_modified(true)
{
  zmsg *msg = static_cast<zmsg*> (data);

  assert(msg);
  SetID(0);
  SetWORKER_ID(0);
  SetSERVICE_ID(0);

  // Прочитать служебные поля транспортного сообщения zmsg
  // и восстановить на его основе прикладное сообщение.
  // два последних фрейма - заголовок и тело сообщения
  assert(msg->parts() >= 2);
  int msg_frames = msg->parts();

  m_frame_header = msg->get_part(msg_frames-2);
  m_header = new RTDBUS_MessageHeader(m_frame_header);

  m_frame_data = msg->get_part(msg_frames-1);
}

// Создать экземпляр на основе заголовка и тела сообщения
Letter::Letter(RTDBUS_MessageHeader *h, std::string& b) : m_modified(true)
{
  // TODO сделать копию заголвка, т.к. он создан в вызывающем контексте и нами не управляется
  throw;
}

// Создать экземпляр на основе заголовка и тела сообщения
Letter::Letter(const std::string& _head, const std::string& _data) : m_modified(true)
{
  m_frame_header.assign(_head);
  m_frame_data.assign(_data);
  m_header = new RTDBUS_MessageHeader(m_frame_header);
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

int8_t Letter::GetPROTOCOL_VERSION()
{
  assert(m_header);
  return m_header->get_protocol_version();
}

rtdbExchangeId Letter::GetEXCHANGE_ID()
{
  assert(m_header);
  return m_header->get_exchange_id();
}

rtdbPid Letter::GetSOURCE_PID()
{
  assert(m_header);
  return m_header->get_source_pid();
}

const std::string& Letter::GetPROC_DEST()
{
  assert(m_header);
  return m_header->get_proc_dest();
}

const std::string& Letter::GetPROC_ORIGIN()
{
  assert(m_header);
  return m_header->get_proc_origin();
}

rtdbMsgType Letter::GetSYS_MSG_TYPE()
{
  assert(m_header);
  return m_header->get_sys_msg_type();
}

rtdbMsgType Letter::GetUSR_MSG_TYPE()
{
  assert(m_header);
  return m_header->get_usr_msg_type();
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
    <<" dest:'"  << GetPROC_DEST()
    <<"' orig:'"  << GetPROC_ORIGIN()
    <<"' sys_t:" << GetSYS_MSG_TYPE()
    <<" usr_t:" << GetUSR_MSG_TYPE();
}
