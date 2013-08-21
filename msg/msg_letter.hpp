#if !defined MSG_LETTER_HPP
#define MSG_LETTER_HPP
#pragma once

#include <stdint.h>
#include <string>

/* Хранимое в БД сообщение Клиента Сервису */
class Letter
{
  public:
    // Создать экземпляр на основе zmsg
    Letter(void*);
    // Отправитель
    const std::string& sender();
    // Получатель
    const std::string& receiver();
    // Тело сообщения
    const std::string& payload();
    // Название Сервиса, к которому относится сообщение
    // NB: внутренний сервис Брокера = "mmi.*"
    const std::string& service_name();

  private:
    std::string m_sender;
    std::string m_receiver;
    std::string m_payload;
    std::string m_service_name;
};

#endif

