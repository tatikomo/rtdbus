#if !defined RTDB_MOSQUITO_HPP
#define RTDB_MOSQUITO_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mdp_client_async_api.hpp"
#include "msg_message.hpp"

#define OPTION_MODE_READ  "read"
#define OPTION_MODE_WRITE "write"
#define OPTION_MODE_PROBE "probe"
#define OPTION_MODE_SUBSCRIBE "sub"
// Максимально допустимая длина пути к файлу с входными параметрами
#define MAX_INPUT_FILENAME_LEN 1000

class Mosquito: public mdp::mdcli
{
  public:
    typedef enum {
      MODE_READ  = 1, // режим чтения из БДРВ
      MODE_WRITE = 2, // режим записи в БДРВ
      MODE_PROBE = 3,  // режим тестирования доступа к БДРВ
      MODE_SUBSCRIBE = 4  // режим подписки на группу
    } WorkMode_t;

    Mosquito(std::string broker, int verbose, WorkMode_t);
   ~Mosquito();

    // Вернуть прочитанное из сокета сообщение
    msg::Letter* recv();
    // Подписаться на рассылку Группы для заданного Сервиса
    int subscript(const std::string&, const std::string&);
    // Создать новое сообщение заданного типа
    msg::Letter* create_message(rtdbMsgType);

    // Обработка ответа на запрос чтения
    bool process_read_response(msg::Letter*);
    // Обработка обновления значений группы подписки
    bool process_sbs_update_response(msg::Letter*);

    const msg::MessageFactory* message_factory() { return m_factory; };
  private:
    msg::MessageFactory *m_factory;
    WorkMode_t           m_mode;
};

#endif
