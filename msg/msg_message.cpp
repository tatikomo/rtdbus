#include <assert.h>
#include <stdio.h>

#include "glog/logging.h"
#include "msg_message.hpp"
#include "helper.hpp"

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
  LOG(INFO) << "Destructor";
}

bool Header::ParseFrom(const std::string& frame)
{
  return m_header_instance.ParseFromString(frame);
}

const int8_t Header::get_protocol_version()
{
  return m_header_instance.protocol_version();
}


const rtdbExchangeId Header::get_exchange_id()
{
  return m_header_instance.exchange_id();
}

const rtdbPid Header::get_source_pid()
{
  return m_header_instance.source_pid();
}

const std::string& Header::get_proc_dest()
{
  return m_header_instance.proc_dest();
}

const std::string& Header::get_proc_origin()
{
  return m_header_instance.proc_origin();
}

const rtdbMsgType Header::get_sys_msg_type()
{
  return static_cast<rtdbMsgType>(m_header_instance.sys_msg_type());
}

const rtdbMsgType Header::get_usr_msg_type()
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

