#include <assert.h>

#include "zmsg.hpp"
#include "msg_message.hpp"
#include "msg_payload.hpp"

Payload::Payload() : m_header(NULL), m_body(NULL)
{
}

Payload::Payload(void* data)
{
  zmsg *msg = static_cast<zmsg*> (data);

  assert(msg);
  msg->dump();

  // Прочитать служебные поля транспортного сообщения zmsg
  // и восстановить на его основе прикладное сообщение.

  assert(msg->parts() == 2); // два фрейма - заголовок и тело сообщения
  std::string h = msg->pop_front();
  m_header = new RTDBUS_MessageHeader(h);

  std::string b = msg->pop_front();
  m_body = new RTDBUS_MessageBody(m_header, b);
}

// Создать экземпляр на основе заголовка и тела сообщения
Payload::Payload(RTDBUS_MessageHeader *h, RTDBUS_MessageBody *b)
{
  assert(h);
  assert(b);

  m_header = h;
  m_body = b;
}

Payload::~Payload()
{
  delete m_header;
  delete m_body;
}

