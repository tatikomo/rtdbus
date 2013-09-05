#include <assert.h>

#include "glog/logging.h"
#include "msg_message.hpp"

RTDBUS_MessageHeader::RTDBUS_MessageHeader()
{
}

RTDBUS_MessageHeader::RTDBUS_MessageHeader(const std::string& frame)
{
  m_header_instance.ParseFromString(frame);
}

RTDBUS_MessageHeader::~RTDBUS_MessageHeader()
{
}

int8_t RTDBUS_MessageHeader::get_protocol_version()
{
  return m_header_instance.protocol_version();
}


rtdbExchangeId RTDBUS_MessageHeader::get_exchange_id()
{
  return m_header_instance.exchange_id();
}

rtdbPid RTDBUS_MessageHeader::get_source_pid()
{
  return m_header_instance.source_pid();
}

const std::string& RTDBUS_MessageHeader::get_proc_dest()
{
  return m_header_instance.proc_dest();
}

const std::string& RTDBUS_MessageHeader::get_proc_origin()
{
  return m_header_instance.proc_origin();
}

rtdbMsgType RTDBUS_MessageHeader::get_sys_msg_type()
{
  return static_cast<rtdbMsgType>(m_header_instance.sys_msg_type());
}

rtdbMsgType RTDBUS_MessageHeader::get_usr_msg_type()
{
  return static_cast<rtdbMsgType>(m_header_instance.usr_msg_type());
}

const std::string& RTDBUS_MessageHeader::get_serialized()
{
  m_header_instance.SerializeToString(&pb_serialized);
  return pb_serialized;
}

void RTDBUS_MessageHeader::set_usr_msg_type(int16_t type)
{
  m_header_instance.set_usr_msg_type(type);
}


RTDBUS_MessageBody::RTDBUS_MessageBody()
{
  m_header_instance = NULL;
  m_body_instance = NULL;
}

/*
 * Создать экземпляр сообщения для транспортировки и хранения в XDB
 * -----------------------------------------------------------------
 * 1. Создается объект типа protobuf на основе типа сообщения, 
 * хранящегося в RTDBUS_MessageHeader
 * 2. Созранить ссылку на заголовок
 * 3. Десериализовать экземпляр protobuf данными из фрейма
 */
RTDBUS_MessageBody::RTDBUS_MessageBody(const RTDBUS_MessageHeader* _header, const std::string& _frame)
{
  assert(_header);

  m_header_instance = const_cast<RTDBUS_MessageHeader*>(_header);
  // NB: фабрика может вернуть NULL, если тип сообщения ей неизвестен
  m_body_instance = RTDBUS_MessageFactory::create(m_header_instance->get_usr_msg_type());
  // TODO: создать класс для исключений
  if (!m_body_instance) throw ;
  assert(m_body_instance);
  m_body_instance->ParseFromString(_frame);
}

RTDBUS_MessageBody::~RTDBUS_MessageBody()
{
  delete m_body_instance;
}

const std::string& RTDBUS_MessageBody::get_serialized()
{
  m_body_instance->SerializeToString(&pb_serialized);
  return pb_serialized;
}

rtdbMsgType RTDBUS_MessageBody::get_type()
{
  assert(m_header_instance);
  return m_header_instance->get_usr_msg_type();
}

RTDBUS_MessageFactory::RTDBUS_MessageFactory()
{
}

RTDBUS_MessageFactory::~RTDBUS_MessageFactory()
{
}

// создать экземпляр заданного клиентского типа
::google::protobuf::Message *RTDBUS_MessageFactory::create(rtdbMsgType _type)
{
  ::google::protobuf::Message* message = NULL;

  switch (_type)
  {
    case ADG_D_MSG_EXECRESULT:
      message = new RTDBM::ExecResult;
      break;

    default:
      LOG(ERROR) << "Unknown message type " << _type;
  }

  return message;
}
