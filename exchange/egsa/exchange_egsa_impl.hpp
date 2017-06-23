#ifndef EXCHANGE_EGSA_IMPL_HPP
#define EXCHANGE_EGSA_IMPL_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <thread>

// Служебные заголовочные файлы сторонних утилит
// Служебные файлы RTDBUS
#include "mdp_worker_api.hpp"
#include "msg_message.hpp"
// Конфигурация
#include "exchange_config.hpp"
#include "exchange_egsa_site.hpp"
#include "exchange_egsa_cycle.hpp"
#include "exchange_egsa_request.hpp"

class SMED;
class AcqSiteEntry;
class ExchangeTranslator;

// Команды от EGSA::implementation в адрес ES_ACQ и ES_SEND
#define INIT         "INIT"
#define ENDALLINIT   "ENDALLINIT"
#define SHUTTINGDOWN "SHUTTINGDOWN"
#define STOP         "STOP"
#define ASKLIFE      "ASKLIFE"
#define TK_MESSAGE   "TK_MESSAGE"   // Сообщение от таймкипера. Сообщение содержит вторым параметром название Цикла (OLDREQUEST|SENDAGAIN)
#define OLDREQUEST   "OLDREQUEST"   // Общий Цикл для ES_ACQ и ES_SEND
#define SENDAGAIN    "SENDAGAIN"    // Специфичный Цикл для ES_SEND

class EGSA : public mdp::mdwrk {
  public:
    friend class ExchangeTranslator;

    enum { BROKER_ITEM = 0, SUBSCRIBER_ITEM = 1, SERVICE_ITEM = 2, SOCKET_MAX_COUNT = SERVICE_ITEM + 1 };
    static const int PollingTimeout;

    EGSA(const std::string&, const std::string&);
   ~EGSA();

    // Основной рабочий цикл
    int run();
    // Доступ к Запросам
    RequestDictionary& dictionary_requests();

    //============================================================================
#ifdef _FUNCTIONAL_TEST
    // Если собирается тест, вынести эти функции в публичный доступ
  public:
#else
    // Если собирается рабочая версия, спрятать эти функции в приватный доступ
  private:
#endif
    ExchangeTranslator* translator() { return m_translator; }
    // Разбор файла с данными от ГОФО
    int load_esg_file(const char*);
    // Внутренний тест SMED - загрузка указанного файла словаря типа ESG_EXACQINFOS или ESG_EXSNDINFOS
    int test_smed(const char*);
    // Доступ ко внутренней буферной памяти с данными от/для смежных систем
    SMED* smed() { return m_smed; }
    // Запуск Интерфейса ES_ACQ
    int implementation_acq();
    // Запуск Интерфейса ES_SEND
    int implementation_send();
    int process_requests_for_all_sites();
    int process_request_for_site(AcqSiteEntry*);
    // Обработка в implementation команд от EGSA::run()
    int process_command(const std::string&);
    // Доступ к конфигурации
    EgsaConfig* config();
    // Доступ к Сайтам
    AcqSiteList& sites();
    // Доступ к Циклам
    CycleList& cycles();
    // Ввести в оборот новый Цикл сбора
    size_t push_cycle(Cycle*);
    // Активировать циклы
    int activate_cycles();
    // Деактивировать циклы
    int deactivate_cycles();
    // Тестовая функция ожидания срабатывания таймеров в течении заданного времени
    int wait(int);
    // Инициализация, создание/подключение к внутренней SMAD
    int init();
    // Инициализация внутренних массивов
    int load_config();
    // Первичная обработка нового запроса
    int processing(mdp::zmsg*, const std::string&, bool&);
    void tick_tack();
    int esg_acq_rsh_CheckBreq (AcqSiteEntry* /*esg_esg_t_ReqResp r_ReqResp*/);
    //============================================================================

  private:
    DISALLOW_COPY_AND_ASSIGN(EGSA);
    // Состояния нити EGSA
    typedef enum {
      STATE_INITIAL   = 0,    // начальное состояние до инициализации
      STATE_CNF_OK    = 1,    // Conf Ok - Wait for STOP/SBS msg
      STATE_INI_OK    = 2,    // SBS received - wait for EndAllInit/Stop
      STATE_INI_KO    = 3,    // Init Ko - Wait only for Stop message
      STATE_WAITING_INIT = 4, // состояние ожидания получения разрешения на работу
      STATE_END_INIT  = 5,    // разрешение получено, подготовка к работе
      STATE_RUN       = 6,    // работа
      STATE_SHUTTINGDOWN = 7, // состояние завершения работы
      STATE_STOP      = 8,    // получен приказ на останов
      STATE_NB        = STATE_STOP+1  // количество состояний
    } internal_state_t;

    // Индексы фраз для ответа EGSA::implement
    enum { GOOD=0, ALREADY=1, FAIL=2, UNWILLING=3 };
    static const char* internal_report_string[];

    // Тип источника данных (сокета), с которого прочиталось сообщение
    typedef enum {
      SOURCE_CONTROL  = 1,
      SOURCE_DATA     = 2
    } message_source_t;

    // Запуск Интерфейса второго уровня
    int implementation();
    // Сменить состояние EGSA на указанное
    bool change_egsa_state_to(internal_state_t);
    internal_state_t egsa_state() { return m_state_egsa; }
    // Сменить состояние ES_ACQ на указанное
    bool change_acq_state_to(internal_state_t);
    internal_state_t acq_state() { return m_state_acq; }
    // Сменить состояние ES_SEND на указанное
    bool change_send_state_to(internal_state_t);
    internal_state_t send_state() { return m_state_send; }
    // Обработка события смены Фазы для данного Сайта
    int esg_acq_inm_PhaseChgMan(AcqSiteEntry*, AcqSiteEntry::init_phase_state_t);
    // Получить информацию по локальному Сайту из БДРВ
    int get_local_info();

    // Инициализация интерфейса ES_ACQ
    int init_acq();
    // Завершение работы интерфейса ES_ACQ
    int fini_acq();
    // Инициализация интерфейса ES_SEND
    int init_send();
    // Завершение работы интерфейса ES_SEND
    int fini_send();
    // Обработка сообщения о чтении значений БДРВ (включая ответ группы подписки)
    int process_read_response(msg::Letter*);
    // Обработка сообщения внутри нити ES_ACQ
    int process_acq(const std::string&);
    // Обработка сообщения внутри нити ES_SEND
    int process_send(const std::string&);
    // Останов экземпляра
    int stop();
    // Ответ на простой запрос верхнего уровня
    int esg_ine_ini_Acknowledge(const std::string&, int, int);
    // Получить новое сообщение
    // Второй параметр - количество милисекунд ожидания получения сообщения.
    // =  0 - разовое чтение сообщений, немедленный выход в случае отсутствия таковых
    // >  0 - время ожидания нового сообщения в милисекундах, от 1 до HEARTBEAT-интервала
    int recv(msg::Letter*&, int = 1000);

    // --------------------------------------------------------------------------
    // Обслуживание запросов:
    // SIG_D_MSG_READ_MULTI - ответ на множественное чтение данных (первый пакет от группы подписки)
    int handle_read_multiple(msg::Letter*, const std::string&);
    // SIG_D_MSG_GRPSBS - порция обновления данных от группы подписки
    int handle_sbs_update(msg::Letter*, const std::string&);
    // Телерегулирование
    // SIG_D_MSG_ECHCTLPRESS
    // SIG_D_MSG_ECHDIRCTLPRESS
    int handle_teleregulation(msg::Letter*, const std::string&);
    int handle_end_all_init(msg::Letter*, const std::string&);
    int handle_asklife(msg::Letter*, const std::string&);
    int handle_stop(msg::Letter*, const std::string&);

    // Отправить всем подчиненным системам запрос готовности
    //void fire_ENDALLINIT();

    // --------------------------------------------------------------------------
    // Активация группы подписки точек систем сбора
    int activateSBS();
    // Дождаться ответа на запрос активации группы подписки
    int waitSBS();
    // Подключиться к SMAD систем сбора
    int attach_to_sites_smad();
    // Изменение состояния подключенных систем сбора и отключение от их внутренней SMAD
    int detach();
    // Функция срабатывания при наступлении времени очередного таймера
    void cycle_trigger(size_t, size_t);

    // Состояние
    internal_state_t m_state;
    // Текущее состояние процесса EGSA::implementation
    internal_state_t m_state_egsa;
    // Текущее состояние процесса EGSA
    internal_state_t m_state_acq;
    // Текущее состояние процесса EGSA
    internal_state_t m_state_send;

    // Набор для zmq::poll
    //zmq::pollitem_t m_socket_items[2];
    msg::MessageFactory *m_message_factory;
    // Сокет обмена сообщениями с Интерфейсом второго уровня
    zmq::socket_t   m_backend_socket;
    // Сокет обмена сообщениями внутри нити ES_ACQ
    zmq::socket_t  *m_acq_control;
    // Сокет обмена сообщениями внутри нити ES_SEND
    zmq::socket_t  *m_send_control;
    // Нить подчиненного интерфейса
    std::thread*    m_servant_thread;
    // аналог ES_ACQ
    std::thread*    m_acquisition_thread;
    // аналог ES_SEND
    std::thread*    m_sending_thread;

    // Экземпляр SMED для хранения конфигурации и данных EGSA
    SMED        *m_smed;
    EgsaConfig  *m_egsa_config;
    // Транслятор данных между БДРВ и форматом обмена со смежными объектами
    ExchangeTranslator *m_translator;
    // Хранилище изменивших своё значение параметров, используется для всех СС
    sa_parameters_t m_list;
    // После чтения из конфигурации имя файла потребуется не сразу
    std::string m_smed_filename;
    // Название локального Сайта
    char    m_local_sa_name[TAG_NAME_MAXLEN + 1];

    // General data
    //static ega_ega_odm_t_GeneralData ega_ega_odm_r_GeneralData;

    // Acquisition Sites Table
    AcqSiteList m_ega_ega_odm_ar_AcqSites;
	
    // Cyclic Operations Table
    //static ega_ega_odm_t_CycleEntity ega_ega_odm_ar_Cycles[NBCYCLES];
    CycleList   m_ega_ega_odm_ar_Cycles;

    //static ega_ega_odm_t_RequestEntry m_requests_table[/*NBREQUESTS*/]; // ega_ega_odm_ar_Requests
    RequestDictionary m_ega_ega_odm_ar_Requests;
    //std::vector<Request*> m_ega_ega_odm_ar_Requests;

    RequestRuntimeList m_waiting_requests;
    RequestRuntimeList m_deferred_requests;


    // Функции нити ES_ACQ <============================================================================
    // Обработка команды от верхнего уровня
    int process_acq_command(mdp::zmsg*);
    // обработка информации от смежной системы
    int process_acq_request(mdp::zmsg*, const std::string&);
    // Удалить просроченные Запросы
    int esg_acq_rsh_ReqsDelMan();
    // EGSA request
    int esg_acq_inm_ReqMan(msg::Letter*);
    // Broadcast receipt coming from COM
    int esg_acq_inm_ReceiptIndic(msg::Letter*);
    // Broadcast report coming from COM
    int esg_acq_inm_TransmReport(msg::Letter*);
    // new state value for a SATO
    int esg_acq_inm_StatesMan(msg::Letter*);
    // send basic request HHISTINFOSAQ
    int esg_acq_inm_SendHhist(msg::Letter*);
    // request for emergency cycle
    int esg_acq_inm_EmergenCycReq(msg::Letter*);
    // distant init. phase is finished - treat the synthetic state of this site
    int esg_acq_inm_PhaseChgMan(msg::Letter*);

    // Функции нити ES_SEND <============================================================================
    int esg_ine_ini_OpenDifDataAccess();
    int esg_ine_ini_AllocExchInfo(const char*);
    int esg_ine_tim_WakeupsAsk();
    int process_send_command(mdp::zmsg*);
    int process_send_request(mdp::zmsg*, const std::string&);
    int esg_esg_odm_TraceAllCycleEntry();
    int esg_ine_ini_UpdateDataDiffCycle();
    int esg_ine_ini_GetStatesMode();
    int esg_ine_ini_AlaHistSub();
};

#endif

