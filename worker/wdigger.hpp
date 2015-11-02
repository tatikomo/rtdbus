#pragma once
#ifndef WORKER_DIGGER_HPP
#define WORKER_DIGGER_HPP

#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <functional>

#include "config.h"
#include "helper.hpp"
// класс измерения производительности
#include "tool_metrics.hpp"

#include "xdb_common.hpp"
#include "mdp_worker_api.hpp"
#include "mdp_zmsg.hpp"
#include "msg_message.hpp"

namespace mdp {

// Количество семплов времени в DiggerWorker
#define NUM_TIME_SAMPLES        25
// Интервал времени в секундах между периодами выдачи
// DiggerProbe команды POLL в адрес DiggerWorker
#define POLLING_PROBE_PERIOD    7

const char* const DIGGER_NAME = "digger";

// Класс для измерения и хранения метрик производительности
class Metrics;

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
    static const int PollingTimeout;

    DiggerWorker(zmq::context_t&, int, xdb::RtEnvironment*);
   ~DiggerWorker();
    // Бесконечный цикл обработки запросов
    void work();
    // Обработка запроса статистики
    int probe(mdp::zmsg*);
    // Первичная обработка нового запроса
    int processing(mdp::zmsg*, std::string&);
    // Обработка запроса на чтение данных
    int handle_read(msg::Letter*, std::string&);
    // Обработка запроса на изменение данных
    int handle_write(msg::Letter*, std::string&);
    // Управление Группами подписок
    int handle_sbs_ctrl(msg::Letter*, std::string&);
    // Отправка квитанции о выполнении запроса
    // TODO: Устранить дублирование с такой же функцией в классе Digger
    void send_exec_result(int, std::string&);

  private:
    // Копия контекста основной нити Digger
    // требуется для работы транспорта inproc
    zmq::context_t &m_context;
    zmq::socket_t   m_worker;
    zmq::socket_t   m_commands;
    pid_t           m_thread_id;
    // Локальный флаг завершения работы, чтобы разные экземпляры не зависели друг от друга
    bool m_interrupt;
    // Объекты доступа к БДРВ
    // TODO: поведение БДРВ при одномоментном исполнении нескольких конкурирующих экземпляров?
    xdb::RtEnvironment *m_environment;
    xdb::RtConnection  *m_db_connection;
    // Фабрика сообщений
    msg::MessageFactory *m_message_factory;

    // Сбор статистики
    tool::Metrics *m_metric_center;
};

// Класс Управляющего группами подписки
class DiggerPoller
{
  public:
    static const int PollingPeriod;
    DiggerPoller(zmq::context_t&, xdb::RtEnvironment*);
   ~DiggerPoller(); 
    void work();
    void stop();

  private:
    bool clear_modification_flag(std::string&, xdb::SubscriptionPoints_t&);
    // Публикация (название группы, перечень тегов со значениями)
    //bool publish_sbs(std::string&, xdb::SubscriptionPoints_t&);
    // Освободить динамические части из кучи
    void release_attribute_info(xdb::AttributeInfo_t*);
    void release_point_info(xdb::PointDescription_t*);

    // Сигнал к завершению работы
    static bool m_interrupt;

    // Копия контекста основной нити Digger
    // требуется для работы транспорта inproc
    zmq::context_t &m_context;
    // Публикация данных по подписке
    zmq::socket_t   m_publisher;

    // Передача доступа в БДРВ
    xdb::RtEnvironment *m_environment;
    // Фабрика сообщений на обновления групп подписки
    msg::MessageFactory *m_message_factory;
};

// Класс управляющего над экземплярами DiggerPoller и DiggerWorker
// Следит за их работоспособностью и при необходимости корректирует их работу (как?)
class DiggerProbe
{
  public:
    // Максимальное количество нитей DiggerWorker-ов.
    // Общее количество нитей +1 (добавится DiggerPoller)
    static const int kMaxWorkerThreads;

    DiggerProbe(zmq::context_t&, xdb::RtEnvironment*);
   ~DiggerProbe();
    // Нить проверки состояния нитей DiggerWorker
    void work();
    // Запуск нитей DiggerWorker и DiggerPoller
    bool start();
    void stop();

  private:
    // Признак продолжения работы нити Probe
    static bool m_interrupt;

    int  start_workers();
    bool start_sbs_poller();
    // Останов Обработчиков
    void shutdown_workers();
    // Останов Управляющего Группами Подписки
    void shutdown_sbs_poller();

    // Копия контекста основной нити Digger
    // требуется для работы транспорта inproc
    zmq::context_t  &m_context;
    // Сокет для связи с экземплярами DiggerWorker
    zmq::socket_t    m_worker_command_socket;
    // Передача доступа в БДРВ
    xdb::RtEnvironment *m_environment;
    // Локальный список экземпляров класса DiggerWorker
    std::vector<DiggerWorker*> m_worker_list;
    // Локальный список экземпляров нитей DiggerWorker::work()
    std::vector<std::thread*>  m_worker_thread;
    // Экземпляр управляющего группами подписки
    DiggerPoller *m_sbs_checker;
    // Нить управляющего группами подписки
    std::thread  *m_sbs_thread;
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
    DiggerProxy(zmq::context_t&, xdb::RtEnvironment*);
   ~DiggerProxy();

    // Запуск прокси-треда обмена между DiggerProxy и DiggerWorker
    // Также инициирует запуск нити Probe
    void run();

  private:
    // Сигнал к завершению работы
    static bool m_interrupt;
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
    Digger(std::string, std::string/*, int*/);
    virtual ~Digger();

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
//    int                 m_verbose_digg;
};

} // namespace mdp

#endif
