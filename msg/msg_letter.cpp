#include "zmsg.hpp"
#include "msg_letter.hpp"

Letter::Letter(void* data)
{
  zmsg *msg = static_cast<zmsg*> (data);
  msg->dump();
  // TODO Прочитать служебные поля транспортного сообщения zmsg
  // и восстановить на его основе прикладное сообщение.

}

const std::string& Letter::sender()
{
  return m_sender;
}

const std::string& Letter::receiver()
{
  return m_receiver;
}

const std::string& Letter::service_name()
{
  return m_service_name;
}

const std::string& Letter::payload()
{
  return m_payload;
}

