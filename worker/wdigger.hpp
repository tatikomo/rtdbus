#pragma once
#ifndef WORKER_DIGGER_HPP
#define WORKER_DIGGER_HPP

#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <functional>

#include "config.h"
#include "mdp_worker_api.hpp"
#include "mdp_zmsg.hpp"
#include "mdp_letter.hpp"

namespace mdp {

// Вторичный обработчик запросов:
// Процедура:
// 1. Создать новое подключение к БДРВ
// 2. Повторять шаги 3-5 до сигнала завершения работы
//      3. Ожидать новое сообщение по транспорту inproc от DiggerProxy
//      4. Обработать запрос, с обращением к БД
//      5. Отправить ответ в адрес DiggerProxy
class DiggerWorker
{
  public:
    DiggerWorker(zmq::context_t&, int, xdb::RtEnvironment*);
    ~DiggerWorker();
    //
    void work();

  private:
    // Копия контекста основной нити Digger
    // требуется для работы транспорта inproc
    zmq::context_t &m_context;
    zmq::socket_t   m_worker;
    // Сигнал к завершению работы
    bool            m_interrupt;

    // Объекты доступа к БДРВ
    // TODO: поведение БДРВ при одномоментном исполнении нескольких конкурирующих экземпляров?
    xdb::RtEnvironment *m_environment;
    xdb::RtConnection  *m_db_connection;
};

//  ---------------------------------------------------------------------
//  Класс прокси многонитевой Службы
//  Инициализирует пул нитей, непосредственно занимающихся обработкой запросов
//  Перенаправляет запросы между Digger и DiggerWorker
//
//  Принимает входящие подключения с сокета ROUTER транспорт tcp://*:5570
//  Передает задачи в пул через сокет DEALER транспорт inproc://backend
//
//  Первичный получатель всех запросов Клиентов (при прямом обмене)
//
//  TODO: предусмотреть интерфейс изменения размера пула
//
//  ---------------------------------------------------------------------
class DiggerProxy
{
  public:
    static const int kMaxThread;

    DiggerProxy(zmq::context_t&, xdb::RtEnvironment*);
    ~DiggerProxy();

    // Запуск прокси-треда обмена между DiggerProxy и DiggerWorker
    void run();

  private:
    // Копия контекста основной нити Digger
    // требуется для работы транспорта inproc
    zmq::context_t  &m_context;
    // Управляющий сокет Прокси - для паузы, продолжения или завершения работы
    zmq::socket_t    m_control;
    // Входящее соединение от Клиентов
    zmq::socket_t    m_frontend;
    // Соединения с экземплярами DiggerWorker
    zmq::socket_t    m_backend;
    // Передача доступа в БДРВ
    xdb::RtEnvironment *m_environment;
};


// Первичный получатель всех запросов от Брокера (надежная доставка) 
class Digger : public mdp::mdwrk
{
  public:
    // Зарезервированный размер памяти для БДРВ
    static const int DatabaseSizeBytes;
    Digger(std::string, std::string, int);
    ~Digger();

    // Запуск DiggerProxy и цикла получения сообщений
    void run();
    // Останов DiggerProxy и освобождение занятых в run() ресурсов
    void cleanup();

    int handle_request(mdp::zmsg*, std::string *&);
    int handle_read(mdp::Letter*, std::string*);
    int handle_write(mdp::Letter*, std::string*);
    int handle_asklife(mdp::Letter*, std::string*);

    // Пауза прокси-треда
    void proxy_pause();
    // Продолжить исполнение прокси-треда
    void proxy_resume();
    // Завершить работу прокси-треда DiggerProxy
    void proxy_terminate();

  private:
    zmq::socket_t       m_helpers_control;    //  "tcp://" socket to control proxy helpers
    DiggerProxy        *m_digger_proxy; 
    std::thread        *m_proxy_thread;

    xdb::RtApplication *m_appli;
    xdb::RtEnvironment *m_environment;
    xdb::RtConnection  *m_db_connection;
};

} // namespace mdp

#endif
