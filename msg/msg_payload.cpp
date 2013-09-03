#include <glog/logging.h>
#include <assert.h>

#include "zmsg.hpp"
#include "msg_message.hpp"
#include "msg_payload.hpp"

Payload::Payload() : m_header(NULL), m_body(NULL)
{
}

// Прочитать ДВА ПОСЛЕДНИХ фрейма, содержаших
// RTDBM::Header
// RTDBM::Message
Payload::Payload(void* data)
{
  zmsg *msg = static_cast<zmsg*> (data);

  assert(msg);
  msg->dump();

  // Прочитать служебные поля транспортного сообщения zmsg
  // и восстановить на его основе прикладное сообщение.

  // два последних фрейма - заголовок и тело сообщения
  assert(msg->parts() >= 2);
  int msg_frames = msg->parts();

  m_frame_header = msg->get_part(msg_frames-2);
  m_header = new RTDBUS_MessageHeader(m_frame_header);

  m_frame_body = msg->get_part(msg_frames-1);
//  m_body = new RTDBUS_MessageBody(m_header, b);
  m_body = NULL;

  LOG(INFO) << msg_frames;
}

// Создать экземпляр на основе заголовка и тела сообщения
Payload::Payload(RTDBUS_MessageHeader *h, std::string& b)
{
  LOG(WARNING) << "DEPRECATED";
}

// Создать экземпляр на основе заголовка и тела сообщения
Payload::Payload(RTDBUS_MessageHeader *h, RTDBUS_MessageBody *b)
{
#warning "Using deprecated method Payload(RTDBUS_MessageHeader*, RTDBUS_MessageBody*)"
  LOG(WARNING) << "DEPRECATED";

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

