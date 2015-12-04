/*
 * Класс ArchivistSampler
 * Назначение: Сбор и хранение данных, подлежащих длительному хранению
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
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"
#include "mdp_worker_api.hpp"
#include "mdp_zmsg.hpp"
#include "msg_message.hpp"

// Тип генератора предыстории
typedef enum {
  PER_1_MINUTE  = 0,
  PER_5_MINUTES = 1,
  PER_HOUR      = 2,
  PER_DAY       = 3,
  PER_MONTH     = 4,
  STAGE_LAST    = PER_MONTH + 1
} sampler_type_t;

// Связка между собирателями текущего и следующего типов
typedef struct {
  sampler_type_t current;
  const char* pair_name_current;
  sampler_type_t next;
  const char* pair_name_next;
  // Количество анализируемых семплов предыдущей стадии для
  // получения одного семпла текущей стадии
  int num_prev_samples;
  // Суффикс файла, содержащего предысторию данного цикла
  char suffix[3];
  // Длительность интервала данного типа в секундах
  // минутная = 60 секунд
  // 5 минут = 300 секунд
  // 1 час = 3600 секунд
  // 1 сутки = 86400 секунд
  int duration;
} state_pair_t;

// Единое хранилище значений семплов - как для аналогов, так и для дискретов
typedef union {
  uint64_t  val_int;
  double    val_double;
} common_value_t;

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
class SamplerWorker
{
  public:
    SamplerWorker(zmq::context_t&,      /* Ссылка на общий ZMQ контекст         */
                  sampler_type_t,       /* Тип семпла - минутный, часовой,...   */
                  xdb::RtEnvironment*,  /* Ссылка на экземпляр БД               */
                  const char*,          /* адрес сервера хранения предыстории   */
                  int                   /* Номер порта сервера хранения предыстории */
                  );
   ~SamplerWorker();

    // Бесконечный цикл обработки запросов
    void work();
    void stop(); // { m_interrupt = true; };

  private:
    // Сигнал к завершению работы
    static bool m_interrupt;
    // Массив переходов от текущего типа собирателя к следующему
    //static const state_pair_t m_stages[STAGE_LAST];
    // Указатель на объект связи с БДРВ
    xdb::RtEnvironment  *m_env;
    xdb::RtConnection   *m_db_connection;
    zmq::context_t &m_context;
    // Признак успешности подключения к внешней БД хранения предыстории
    bool m_history_db_connected;
    sampler_type_t m_sampler_type;
#if 0
    // рабочий каталог файловой системы, используется как
    // вершина файловой иерархии хранения исторических значений
    char* m_cwd;
#endif
    // ip-адрес сервера хранения предыстории
    const char* m_history_db_address;
    // Номер порта БД хранения предыстории
    int m_history_db_port;

    // собрать предысторию текущего типа
    void collect_sample();
    // передать эстафету сбора предыстории по цепочке
    /*void push_next_stage(sampler_type_t);
    //void process_analog_samples(struct tm&, xdb::map_id_name_t&);
    void process_discrete_samples(xdb::map_id_name_t&);
    bool store_samples(void*,
                       struct tm& timer,
                       std::string&,
                       xdb::AttributeInfo_t&,
                       xdb::AttributeInfo_t&); 
    bool store_samples_in_files(std::string&, xdb::AttributeInfo_t&, xdb::AttributeInfo_t&);*/
    void make_samples(const time_t);
};

// ---------------------------------------------------------------------
// Управляющий процессов по генерации предыстории.
// Запускает несколько экземпляров SamplerWorker соответcтвующего типа.
class ArchivistSamplerProxy
{
  public:
    ArchivistSamplerProxy(zmq::context_t&, xdb::RtEnvironment*);
   ~ArchivistSamplerProxy();
    // запуск всех нитей SamplerWorker
    void run();
    // установка признака завершения цикла работы
    void stop();

  private:
    zmq::context_t &m_context;
    // Локальный список экземпляров класса DiggerWorker
    std::vector<SamplerWorker*> m_worker_list;
    // Локальный список экземпляров нитей DiggerWorker::work()
    std::vector<std::thread*>   m_worker_thread;
    xdb::RtEnvironment  *m_env;
    xdb::RtConnection   *m_db_connection;
    // Сигнал к завершению работы
    static bool m_interrupt;
    timer_mark_t m_time_before;
    timer_mark_t m_time_after;
    // ip-адрес сервера хранения предыстории
    char m_history_db_address[20];
    // Номер порта БД хранения предыстории
    int m_history_db_port;

    // первоначальная инициализация списков аналоговых и дискретных точек
    void fill_points_list();
};

// ---------------------------------------------------------------------
class ArchivistSampler : public mdp::mdwrk
{
  public:
    ArchivistSampler(const std::string&, const std::string&);
   ~ArchivistSampler();
    // Инициализация подключения к БДРВ
    bool init();
    // Запуск ArchivistSamplerProxy и цикла получения сообщений
    void run();

    int handle_request(mdp::zmsg*, std::string*);

  private:
    std::string m_broker_endpoint;
    std::string m_server_name;
    ArchivistSamplerProxy   *m_proxy;
    std::thread             *m_proxy_thread;

    // Фабрика сообщений
    msg::MessageFactory *m_message_factory;
    xdb::RtApplication  *m_appli;
    xdb::RtEnvironment  *m_environment;
    xdb::RtConnection   *m_db_connection;

    int handle_asklife(msg::Letter*, std::string*);
    int handle_stop(msg::Letter*, std::string*);
};

#endif

