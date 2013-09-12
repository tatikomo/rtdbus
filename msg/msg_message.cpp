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

