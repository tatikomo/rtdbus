#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>

#include "glog/logging.h"
#include "msg_message.hpp"
#include "msg_common.h"
#include "helper.hpp"

// TODO: Создать набор классов, создающих готовое к передаче сообщение типа zmsg
//
// А) Класс "Фабрика сообщений"
//  При создании указываются:
//  1. Версия системы сообщений
//  2. Версия rtdbus(?)
//  3. PID процесса-отправителя
//
// Б) Метод "Создание сообщения по заданным условиям"
//  На вход подаются:
//  1. Код сообщения (ASKLIFE и т.п.)
//  2. Идентификатор последовательности (монотонно возрастающее, уникальное в пределах сессии)
//  3. Идентификатор ответа (уникальный номер в пределах последовательности)
//  Метод самостоятельно заполняет служебные поля (А.1, А.2)
//  На выходе - уникальный класс, соответствующий типу Б.1
//
// В) Метод "Отправка сообщения на заданный сокет"
//
using namespace msg;

Header::Header()
{
}

Header::Header(const std::string& frame)
{
  if (false == m_header_instance.ParseFromString(frame))
  {
    LOG(ERROR) << "Unable to unserialize header";
    hex_dump(frame);
  }
}

Header::~Header()
{
//  LOG(INFO) << "Destructor";
}

bool Header::ParseFrom(const std::string& frame)
{
  return m_header_instance.ParseFromString(frame);
}

uint32_t Header::get_protocol_version() const
{
  return m_header_instance.protocol_version();
}

rtdbExchangeId Header::get_exchange_id() const
{
  return m_header_instance.exchange_id();
}

rtdbExchangeId Header::get_interest_id() const
{
  return m_header_instance.interest_id();
}

rtdbPid Header::get_source_pid() const
{
  return m_header_instance.source_pid();
}

const std::string& Header::get_proc_dest() const
{
  return m_header_instance.proc_dest();
}

const std::string& Header::get_proc_origin() const
{
  return m_header_instance.proc_origin();
}

rtdbMsgType Header::get_sys_msg_type() const
{
  return static_cast<rtdbMsgType>(m_header_instance.sys_msg_type());
}

rtdbMsgType Header::get_usr_msg_type() const
{
  return static_cast<rtdbMsgType>(m_header_instance.usr_msg_type());
}

const std::string& Header::get_serialized()
{
  m_header_instance.SerializeToString(&pb_serialized);
  return pb_serialized;
}

void Header::set_usr_msg_type(int16_t type)
{
  m_header_instance.set_usr_msg_type(type);
}

// Тело сообщения
Payload::Payload()
{
  pb_serialized.clear();
}

// входной параметр - фрейм заголовка из zmsg
Payload::Payload(const std::string& body)
{
  pb_serialized.assign(body);
}

Payload::~Payload()
{
}

const std::string& Payload::get_serialized()
{
  return pb_serialized;
}

// Сообщение целиком
Message::Message(rtdbMsgType _type, rtdbExchangeId _id)
 : m_header(NULL),
   m_payload(NULL),
   m_system_type(USER_MESSAGE_TYPE),
   m_user_type(_type),
   m_exchange_id(_id),
   m_interest_id(0)
{
}

// входные параметры - фреймы заголовка и нагрузки из zmsg
Message::Message(const std::string& head_str, const std::string& body_str)
{
  m_header = new Header(head_str);
  assert(m_header);

  // TODO: отбрасывать сообщения с более новым протоколом
  //assert(m_header->get_protocol_version() == m_version_message_system);

  m_user_type = m_header->get_usr_msg_type();
  m_exchange_id = m_header->get_exchange_id();
  m_interest_id = m_header->get_interest_id();

  m_payload = new Payload(body_str);
  assert(m_payload);
}

Message::~Message()
{
  delete m_header;
  delete m_payload;
}

const Header* Message::get_head()
{
  return m_header;
}

const Payload* Message::get_body()
{
  return m_payload;
}

void Message::dump()
{
  LOG(INFO) << "dump message type:" << type()
            << " exchange_id:" << m_exchange_id
            << " interest_id:" << m_interest_id;
}

AskLife::AskLife() : Message(ADG_D_MSG_ASKLIFE, 0)
{
}

AskLife::AskLife(rtdbExchangeId _id) : Message(ADG_D_MSG_ASKLIFE, _id)
{
}

AskLife::AskLife(const std::string& head, const std::string& body) : Message(head, body)
{
  assert(type() == ADG_D_MSG_ASKLIFE);

  m_pb_header.ParseFromString(head);
  m_pb_impl.ParseFromString(body);
}

AskLife::~AskLife()
{
}

int AskLife::status()
{
  assert(m_payload);
  return -1;
//  return m_payload->get_status();
}

ExecResult::ExecResult() : Message(ADG_D_MSG_EXECRESULT, 0)
{
}

ExecResult::ExecResult(rtdbExchangeId _id) : Message(ADG_D_MSG_EXECRESULT, _id)
{
}

ExecResult::ExecResult(const std::string& head, const std::string& body) : Message(head, body)
{
  assert(type() == ADG_D_MSG_EXECRESULT);
}

ExecResult::~ExecResult()
{
}

MessageFactory::MessageFactory()
{
  m_version_message_system = 1;
  m_version_rtdbus = 1;

  m_pid = getpid();

  m_exchange_id = 0;
}

MessageFactory::~MessageFactory()
{
}

Message* MessageFactory::create(rtdbMsgType type)
{
  Message *created = NULL;

  ++m_exchange_id;

  switch (type)
  {
    case ADG_D_MSG_EXECRESULT:
        created = new ExecResult();
    break;
    case ADG_D_MSG_ENDINITACK:
    break;
    case ADG_D_MSG_INIT:
    break;
    case ADG_D_MSG_STOP:
    break;
    case ADG_D_MSG_ENDALLINIT:
    break;
    case ADG_D_MSG_ASKLIFE:
        created = new AskLife();
    break;
    case ADG_D_MSG_LIVING:
    break;
    case ADG_D_MSG_STARTAPPLI:
    break;
    case ADG_D_MSG_STOPAPPLI:
    break;
    case ADG_D_MSG_ADJTIME:
    break;

    default:
      LOG(ERROR) << "Unsupported message type " << type; 
  }
  return created;
}

