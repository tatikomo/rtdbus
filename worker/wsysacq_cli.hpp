#ifndef SERVICE_SYSACQ_MODULE_H
#define SERVICE_SYSACQ_MODULE_H
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <thread>

// Служебные заголовочные файлы сторонних утилит
#include "zmq.hpp"
// Служебные файлы RTDBUS
#include "mdp_worker_api.hpp"

// Ссылка на общий интерфейс систем сбора
class SysAcqInterface;

namespace mdp {

// -----------------------------------------------------------------------------
// Общий интерфейс Службы Систем Сбора 
// Реализация:
// 2016: поддержка системы сбора типа EELE
// -----------------------------------------------------------------------------
class SA_Module_Instance : public mdwrk {
  public:
    SA_Module_Instance(const std::string&, const std::string&);
   ~SA_Module_Instance();
    int run();
    int stop();
    int handle_request(mdp::zmsg*, std::string*);

    // Обработчики запросов
    int handle_asklife(msg::Letter*, std::string*);
    int handle_stop(msg::Letter*, std::string*);
    virtual int handle_init(msg::Letter*, std::string*);
    virtual int handle_end_all_init(msg::Letter*, std::string*);
    virtual int handle_internal_request(msg::Letter*, std::string*);


  private:
    const std::string& service_name() { return m_service; };
    // Подчиненный интерфейс системы сбора - EELE, OPC,...
    SysAcqInterface *m_impl;
    // Сокет связи с подчиненным интерфейсом системы сбора
    zmq::socket_t    m_slave;
    // Внутреннее состояние
    int m_status;
    // Нить подчиненного интерфейса с Системой Сбора
    std::thread* m_servant_thread;
    // Конфигурационный файл СС, для получения типа СС на этапе старта
    AcquisitionSystemConfig* m_config;
    msg::MessageFactory *m_message_factory;
    // -----------------------------------------------------------------------------
    // Ответ сообщением на простейшие запросы, в ответе только числовой код
    int response_as_simple_reply(msg::Letter*, std::string*, int);
    // Ответ сообщением EXEC_RESULT на запросы,
    // в ответе числовой код (обязательно) и строка (возможно)
    int response_as_exec_result(msg::Letter*, std::string*, int, const std::string&);
};

}; // end namespace

#endif

