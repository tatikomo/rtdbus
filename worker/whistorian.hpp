/*
 * Класс Historian
 * Назначение: Сбор, хранение и выдача данных, подлежащих длительному хранению
 * Описание: doc/history_collector.mkd
 */
#pragma once
#ifndef WRK_HIST_SAMPLER_HPP
#define WRK_HIST_SAMPLER_HPP

#include <string>
#include <vector>
#include <thread>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "tool_metrics.hpp"
#include "xdb_common.hpp"
#include "mdp_worker_api.hpp"
#include "mdp_zmsg.hpp"
#include "msg_message.hpp"

#include "xdb_rtap_application.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"

#include "hdb_impl_processor.hpp"

// ---------------------------------------------------------------------
// Управляющий процессов по генерации предыстории.
class HistorianProducer
{
  public:
    HistorianProducer(zmq::context_t&, xdb::RtEnvironment*);
   ~HistorianProducer();
    // запуск всех нитей SamplerWorker
    void run();
    // установка признака завершения цикла работы
    void stop();

  private:
    DISALLOW_COPY_AND_ASSIGN(HistorianProducer);
    zmq::context_t &m_context;
    xdb::RtEnvironment *m_env;
    // Сигнал к завершению работы
    volatile static bool m_interrupt;
    timer_mark_t m_time_before;
    timer_mark_t m_time_after;
};

// ---------------------------------------------------------------------
// Управляющий процессов по обработке запросов предыстории.
// Не требует подключения к БДРВ, читая данные из HDB
class HistorianResponder
{
  public:
    static const int PollingTimeout;

    HistorianResponder(zmq::context_t&);
   ~HistorianResponder();
    // запуск всех нитей SamplerWorker
    void run();
    // установка признака завершения цикла работы
    void stop();
    // Первичная обработка нового запроса
    int processing(mdp::zmsg*, std::string&);

  private:
    DISALLOW_COPY_AND_ASSIGN(HistorianResponder);
    zmq::context_t &m_context;
    // Входящее соединение от Клиентов
    zmq::socket_t   m_frontend;
    // Сигнал к завершению работы
    volatile static bool m_interrupt;
    timer_mark_t m_time_before;
    timer_mark_t m_time_after;
    msg::MessageFactory *m_message_factory;
    HistoricImpl* m_historic;

    // Обработчик запросов истории от Клиентов
    int handle_query_history(msg::Letter*, std::string*);
};

// ---------------------------------------------------------------------
//  Класс прокси многонитевой Службы
//  Инициализирует пул нитей, непосредственно занимающихся обработкой
//  запросов получения истории.
//  Перенаправляет запросы между Historian и HistorianResponder
//
//  Принимает входящие подключения с сокета ROUTER транспорт tcp://*:55??
//  Передает задачи в пул через сокет DEALER транспорт inproc://backend
//
//  TODO: предусмотреть интерфейс изменения размера пула
// ---------------------------------------------------------------------
class HistorianProxy
{
  public:
    HistorianProxy(zmq::context_t&/*, xdb::RtEnvironment* */);
   ~HistorianProxy();

    // Запуск прокси-треда обмена между HistorianProxy и экземплярами HistorianResponder
    // По превышению минутной границы текущего времени посылает сообщение
    // о начале работы 1-минутному семплеру.
    void run();

  private:
    DISALLOW_COPY_AND_ASSIGN(HistorianProxy);
    // Сигнал к завершению работы
    volatile static bool m_interrupt;
    // Копия контекста основной нити Historian
    // требуется для работы транспорта inproc
    zmq::context_t  &m_context;
    // Управляющий сокет Прокси - для паузы, продолжения или завершения работы
    zmq::socket_t    m_control;
    // Входящее соединение от Клиентов
    zmq::socket_t    m_frontend;
    // Соединения с экземплярами HistorianResponder
    zmq::socket_t    m_backend;
    // Передача доступа в БДРВ
//    xdb::RtEnvironment *m_environment;
    // Объект проверки состояния (Пробник)
    //Probe     *m_probe;
    // Нить проверки состояния нитей HistorianResponder
    std::thread     *m_probe_thread;
    // Останов Пробника
    void wait_probe_stop();
};

// ---------------------------------------------------------------------
class Historian : public mdp::mdwrk
{
  public:
    Historian(const std::string&, const std::string&);
   ~Historian();
    // Инициализация подключения к БДРВ
    bool init();
    // Запуск Historian и цикла получения сообщений
    void run();

    int handle_request(mdp::zmsg*, std::string*, bool&);

  private:
    DISALLOW_COPY_AND_ASSIGN(Historian);
    std::string m_broker_endpoint;
    std::string m_server_name;
    // Нить генерации предыстории
    HistorianProducer   *m_producer;
    std::thread         *m_producer_thread;
    // Нить ответчика на запросы истории
    HistorianResponder  *m_responder;
    std::thread         *m_responder_thread;

    // Фабрика сообщений
    msg::MessageFactory *m_message_factory;
    xdb::RtApplication  *m_appli;
    xdb::RtEnvironment  *m_environment;
    xdb::RtConnection   *m_db_connection;

    int handle_asklife(msg::Letter*, std::string*);
    int handle_stop(msg::Letter*, std::string*);
};

#endif

