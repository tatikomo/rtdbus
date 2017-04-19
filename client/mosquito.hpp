#if !defined RTDB_MOSQUITO_HPP
#define RTDB_MOSQUITO_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mdp_client_async_api.hpp"

#define OPTION_MODE_READ  "read"
#define OPTION_MODE_WRITE "write"
#define OPTION_MODE_PROBE "probe"
#define OPTION_MODE_STOP  "stop"
#define OPTION_MODE_SUBSCRIBE "sub"
#define OPTION_MODE_HISTORY   "hist"

class Mosquito: public mdp::mdcli
{
  public:
    typedef enum {
      MODE_READ  = 1,   // режим чтения из БДРВ
      MODE_WRITE = 2,   // режим записи в БДРВ
      MODE_PROBE = 3,   // режим тестирования доступности Службы
      MODE_STOP  = 4,   // режим останова Службы
      MODE_SUBSCRIBE   = 5, // режим подписки на группу
      MODE_HISTORY_REQ = 6  // режим запроса истории
    } WorkMode_t;

    Mosquito(const std::string& broker, int verbose, WorkMode_t);
   ~Mosquito();

    // Вернуть прочитанное из сокета сообщение
    int recv(msg::Letter*&);
    // Подписаться на рассылку Группы для заданного Сервиса
    int subscript(const std::string&, const std::string&);
    // Создать новое сообщение заданного типа
    msg::Letter* create_message(rtdbMsgType);

    // Обработка ответа на запрос чтения
    bool process_read_response(msg::Letter*);
    // Обработка обновления значений группы подписки
    bool process_sbs_update_response(msg::Letter*);
    // Обработка ответа на запрос истории
    bool process_history(msg::Letter*);

    const msg::MessageFactory* message_factory() { return m_factory; };
  private:
    DISALLOW_COPY_AND_ASSIGN(Mosquito);
    msg::MessageFactory *m_factory;
    WorkMode_t           m_mode;
};

#endif
