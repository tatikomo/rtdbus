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
#include "msg_message.hpp"

namespace mdp {

const char* const DIGGER_NAME = "digger";

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
    // Бесконечный цикл обработки запросов
    void work();
    // Первичная обработка нового запроса
    int processing(mdp::zmsg*, std::string&);
    // Обработка запроса на чтение данных
    int handle_read(msg::Letter*, std::string&);
    // Обработка запроса на изменение данных
    int handle_write(msg::Letter*, std::string&);
    // Отправка квитанции о выполнении запроса
    // TODO: Устранить дублирование с такой же функцией в классе Digger
    void send_exec_result(int, std::string&);

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
    // Фабрика сообщений
    msg::MessageFactory *m_message_factory;

};

class DiggerProbe
{
  public:
    DiggerProbe(zmq::context_t&, xdb::RtEnvironment*);
   ~DiggerProbe();
    // Нить проверки состояния нитей DiggerWorker
    void work();
    void stop();

  private:
    // Копия контекста основной нити Digger
    // требуется для работы транспорта inproc
    zmq::context_t  &m_context;
    // Признак продолжения работы нити Probe
    static bool      m_probe_continue;
    // Передача доступа в БДРВ
    xdb::RtEnvironment *m_environment;
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
    // Также инициирует запуск нити Probe
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
    // Объект проверки состояния (Пробник)
    DiggerProbe     *m_probe;
    // Нить проверки состояния нитей DiggerWorker
    std::thread     *m_probe_thread;
    // Останов Пробника
    void wait_probe_stop();
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
//    int handle_read(msg::Letter*, std::string*);
//    int handle_write(msg::Letter*, std::string*);
    int handle_asklife(msg::Letter*, std::string*);

    // Пауза прокси-треда
    void proxy_pause();
    // Продолжить исполнение прокси-треда
    void proxy_resume();
    // Проверить работу прокси-треда
    void proxy_probe();
    // Завершить работу прокси-треда DiggerProxy
    void proxy_terminate();

  private:
    // отправить адресату сообщение о статусе исполнения команды
    void send_exec_result(int, std::string*);

    zmq::socket_t       m_helpers_control;    //  "tcp://" socket to control proxy helpers
    DiggerProxy        *m_digger_proxy; 
    std::thread        *m_proxy_thread;

    // Фабрика сообщений
    msg::MessageFactory *m_message_factory;

    xdb::RtApplication *m_appli;
    xdb::RtEnvironment *m_environment;
    xdb::RtConnection  *m_db_connection;
    bool                m_verbose;
};

} // namespace mdp

#endif
