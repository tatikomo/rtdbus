#if !defined EXCHANGE_EGSA_HOST_HPP
#define EXCHANGE_EGSA_HOST_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <string>
#include <vector>
#include <thread>

// Служебные заголовочные файлы сторонних утилит
// Служебные файлы RTDBUS
#include "mdp_zmsg.hpp"
#include "mdp_worker_api.hpp"
#include "msg_message.hpp"
// Конфигурация
#include "exchange_config.hpp"
#include "exchange_config_egsa.hpp"
// Внешняя память, под управлением EGSA
#include "exchange_smad_ext.hpp"
#include "exchange_egsa_init.hpp"
#include "exchange_egsa_sa.hpp"

class EGSA;

// Класс-обработчик функций верхнего уровня EGSA - взаимодействие с Брокером
class EGSA_Host : public mdp::mdwrk {
  public:
    EGSA_Host(std::string, std::string, int);
   ~EGSA_Host();
    
    // Обработчик входящих соединений
    int handle_request(mdp::zmsg*, std::string*, bool&);
    // Основной рабочий цикл
    int run();

  private:
    DISALLOW_COPY_AND_ASSIGN(EGSA_Host);

    // Инициализация, создание/подключение к внутренней SMAD
    int init();
    int handle_asklife(msg::Letter*, std::string*);
    int handle_stop(msg::Letter*, std::string*);

    std::string m_broker_endpoint;
    std::string m_server_name;

    // Нить ответчика на запросы к службе обменов "EGSA"
    EGSA        *m_egsa_instance;
    std::thread *m_egsa_thread;

    // Фабрика сообщений
    msg::MessageFactory *m_message_factory;

    // Сокет получения данных
    int m_socket;
};

#endif

