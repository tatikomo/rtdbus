#ifndef SERVICE_SYSACQ_MODULE_H
#define SERVICE_SYSACQ_MODULE_H
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы

// Служебные заголовочные файлы сторонних утилит
#include "zmq.hpp"
// Служебные файлы RTDBUS
#include "mdp_worker_api.hpp"

// Ссылка на общий интерфейс систем сбора
class SysAcqInterface;

namespace mdp {

enum {
  STATE_INIT_NONE               = 0,
  STATE_INIT_OK                 = 1,
  STATE_INIT_FAIL               = 2,
  STATE_CONNECTING              = 3,
  STATE_CONNECTED               = 4,
  STATE_CONNECTED_ACQUIRE       = 5,
  STATE_CONNECTED_MAINTENANCE   = 5,
  STATE_CONNECTED_DELEGATE      = 7
    };

class SystemAcquisitionModule : public mdwrk {
  public:
    SystemAcquisitionModule(const std::string&, const std::string&, const std::string&);
   ~SystemAcquisitionModule();
    int run();
    int handle_request(mdp::zmsg*, std::string*&);

  private:
    // Подчиненный интерфейс системы сбора
    SysAcqInterface *m_impl;
    // Сокет связи с подчиненным интерфейсом системы сбора
    zmq::socket_t    m_to_slave;
    // Внутреннее состояние
    int m_status;
    // Фабрика сообщений
    msg::MessageFactory *m_message_factory;
    // Нить подчиненного интерфейса с Системой Сбора
    std::thread* m_servant_thread;

    // Внутренние обработчики полученных внешних запросов
    int handle_asklife(msg::Letter*, std::string*);
    int handle_stop(msg::Letter*, std::string*);
    int handle_init(msg::Letter*, std::string*);
    int handle_end_all_init(msg::Letter*, std::string*);
    int handle_internal_request(msg::Letter*, std::string*);
    // Ответ сообщением на простейшие запросы, в ответе только числовой код
    int response_as_simple_reply(msg::Letter*, std::string*, int);
    // Ответ сообщением EXEC_RESULT на запросы, в ответе числовой код (обязательно) и строка (возможно)
    int response_as_exec_result(msg::Letter*, std::string*, int, const std::string&);
};

}; // end namespace

#endif

