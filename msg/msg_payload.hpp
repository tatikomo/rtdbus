#if !defined MSG_PAYLOAD_HPP
#define MSG_PAYLOAD_HPP
#pragma once

#include <stdint.h>
#include <string>

class RTDBUS_MessageHeader;
class RTDBUS_MessageBody;

/*
 * Хранимое в БД сообщение Клиента Сервису 
 * Используется как контейнер, сохраняя заголовок и тело сообщения
 */

class Payload
{
  public:
    ~Payload();
    // на входе - объект zmsg
    Payload(void*);
    // Заголовок сообщения
    const std::string&    header() { return m_frame_header; }
    // Тело сообщения
    const std::string&    data() { return m_frame_body; }

  private:
    std::string            m_frame_body;
    std::string            m_frame_header;
    RTDBUS_MessageHeader  *m_header;
    RTDBUS_MessageBody    *m_body;

    Payload();
    // Создать экземпляр на основе заголовка и тела сообщения
    // NB: может стоит удалить этот метод, поскольку он заставляет 
    // экземпляр разбираться в типах передаваемых сообщений.
    // Более предпочтительным вариантом является разбор заголовка, 
    // поскольку он неизменен, а тело сообщения хранить в "сыром" виде (string?)
    Payload(RTDBUS_MessageHeader *h, RTDBUS_MessageBody *b);
    // Создать экземпляр на основе заголовка и тела сообщения
    Payload(RTDBUS_MessageHeader *h, std::string& b);
    // Заголовок сообщения
    RTDBUS_MessageHeader* get_header() { return m_header; }
    // Тело сообщения
    RTDBUS_MessageBody*   get_data() { return m_body; }
};

#endif

