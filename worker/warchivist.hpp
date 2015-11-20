/*
 * ============================================================================
 * Процесс обработки и накопления архивных данных
 *
 * eugeni.gorovoi@gmail.com
 * 01/11/2015
 * ============================================================================
 */
#pragma once
#ifndef WORKER_ARCHIVIST_HPP
#define WORKER_ARCHIVIST_HPP

#include <string>
//#include <vector>
#include <thread>
//#include <memory>
//#include <functional>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "helper.hpp"
// класс измерения производительности
#include "tool_metrics.hpp"

#include "xdb_common.hpp"
#include "mdp_worker_api.hpp"
//#include "mdp_zmsg.hpp"
//#include "msg_message.hpp"

// ---------------------------------------------------------------------
//  Класс прокси многонитевой Службы
//  Инициализирует пул нитей, непосредственно занимающихся обработкой
//  запросов получения истории.
//  Перенаправляет запросы между Archivist и ArchivistWorker
//
//  Принимает входящие подключения с сокета ROUTER транспорт tcp://*:55??
//  Передает задачи в пул через сокет DEALER транспорт inproc://backend
//
//  TODO: предусмотреть интерфейс изменения размера пула
//
// ---------------------------------------------------------------------
class ArchivistProxy
{
  public:
    ArchivistProxy(zmq::context_t&/*, xdb::RtEnvironment* */);
   ~ArchivistProxy();

    // Запуск прокси-треда обмена между ArchivistProxy и экземплярами ArchivistWorker
    // По превышению минутной границы текущего времени посылает сообщение
    // о начале работы 1-минутному семплеру.
    void run();

  private:
    DISALLOW_COPY_AND_ASSIGN(ArchivistProxy);
    // Сигнал к завершению работы
    static bool m_interrupt;
    // Копия контекста основной нити Archivist
    // требуется для работы транспорта inproc
    zmq::context_t  &m_context;
    // Управляющий сокет Прокси - для паузы, продолжения или завершения работы
    zmq::socket_t    m_control;
    // Входящее соединение от Клиентов
    zmq::socket_t    m_frontend;
    // Соединения с экземплярами ArchivistWorker
    zmq::socket_t    m_backend;
    // Передача доступа в БДРВ
//    xdb::RtEnvironment *m_environment;
    // Объект проверки состояния (Пробник)
    //Probe     *m_probe;
    // Нить проверки состояния нитей ArchivistWorker
    std::thread     *m_probe_thread;
    // Останов Пробника
    void wait_probe_stop();
};

class ArchivistWorker
{
  public:
    ArchivistWorker(std::string&, std::string&);
    virtual ~ArchivistWorker();

  private:
    DISALLOW_COPY_AND_ASSIGN(ArchivistWorker);
    // Точка подключения к Брокеру
    std::string m_broker_endpoint;
    // Собственное название Службы
    std::string m_service_name;
};

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
class Archivist: public mdp::mdwrk
{
  public:
    Archivist(std::string&, std::string&);
    virtual ~Archivist();

  private:
    DISALLOW_COPY_AND_ASSIGN(Archivist);
    // Точка подключения к Брокеру
    std::string m_broker_endpoint;
    // Собственное название Службы
    std::string m_service_name;
};

#endif
