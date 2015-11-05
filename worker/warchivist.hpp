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

#ifdef __HAVE_CONFIG__
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

// ---------------------------------------------------------------------
// Собиратель архивов
// Выполняется несколько экземпляров, каждый из которых занимается обработкой
// соответствующей таблицы семплов:
// 1-минутный семплер
// 5-минутный семплер
// 1-часовой семплер
// 1-суточный семплер
// Ведущим экземпляром является 1-минутый семплер. По завершении своей работы он
// передает сигнал начала работы следующему экземпляру (5-минутному). Тот, в свою
// очередь, передает сигнал дальше по завершению своей работы.
//
// Принимает на вход параметры:
//  а) Тип процесса (разновидность семплера)
//  б) указатель на контекст zmq (для обмена inproc-сообщениями)
// ---------------------------------------------------------------------
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
