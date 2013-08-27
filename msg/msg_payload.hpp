#if !defined MSG_PAYLOAD_HPP
#define MSG_PAYLOAD_HPP
#pragma once

#include <stdint.h>
#include <string>

class RTDBUS_MessageHeader;
class RTDBUS_MessageBody;

/* Хранимое в БД сообщение Клиента Сервису */
class Payload
{
  public:
    Payload();
    ~Payload();
    Payload(void*);
    // Создать экземпляр на основе заголовка и тела сообщения
    Payload(RTDBUS_MessageHeader *h, RTDBUS_MessageBody *b);
    // Заголовок сообщения
    RTDBUS_MessageHeader* header() { return m_header; }
    // Тело сообщения
    RTDBUS_MessageBody*   data() { return m_body; }

  private:
    RTDBUS_MessageHeader  *m_header;
    RTDBUS_MessageBody    *m_body;
};

#endif

