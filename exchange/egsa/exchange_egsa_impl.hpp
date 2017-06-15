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


enum {
  ESG_ESG_D_ERR_CANNOTOPENFILE, //	Cannot open file
  ESG_ESG_D_ERR_CANNOTREADFILE, //Cannot read from file
  ESG_ESG_D_ERR_CANNOTWRITFILE, //Cannot write into file

  ESG_ESG_D_WAR_STARTINGESDL,   //Starting ESDL
  ESG_ESG_D_WAR_ESDLTERMINATED, //End of ESDL treatments. Waiting to be killed

  ESG_ESG_D_ERR_CANNOTCLOSESMAD,    //Cannot close SMAD for site

  ESG_ESG_D_ERR_INEXISTINGENTRY,    //Entry does not exist in Oper Data Table
  ESG_ESG_D_ERR_ACQSITETABLEFULL,   //Oper Data Acquisition Sites Table full
  ESG_ESG_D_WAR_CANNOTUPDATECYCLE,  //Cannot update Oper Data Cycle Table
  ESG_ESG_D_ERR_CYCLETABLEFULL, //Oper Data Cycles Table full

  ESG_ESG_D_ERR_UNKNOWNMESS,    //Unknown received message by ESDL
  ESG_ESG_D_ERR_BADMESS,    //Bad received message by ESDL

  ESG_ESG_D_ERR_NOACQSITE,  //Oper Data Table for acquisition sites is empty

  ESG_ESG_D_ERR_MAXREQSREACHED, //Request(s) rejected : max. number reached
  ESG_ESG_D_ERR_MAXCYCSREACHED, //Cycle(s) rejected : max. number reached
  ESG_ESG_D_ERR_MAXSITESREACHED,    //Site(s) rejected : max. number reached

  ESG_ESG_D_ERR_BADCONFFILE,    //Configuration file not correct
  ESG_ESG_D_ERR_UNKNOWNREQTYPE, //Unknown request type
  ESG_ESG_D_ERR_BADSTATEMODEVAL,    //Bad value of the state or mode
  ESG_ESG_D_ERR_BADENTRYID, //Empty string for identifier of Oper Data Table entry
  ESG_ESG_D_WAR_BADREQTYPE, //Bad received request type
  ESG_ESG_D_ERR_SMADEMPTY,  //SMAD for site has zero record
  ESG_ESG_D_ERR_ALLOCACQDATAID, //Acquired data universal names allocation
  ESG_ESG_D_ERR_ALLOCACQDATAINTID,  //Acquired data internal idents allocation
  ESG_ESG_D_ERR_ALLOCACQDATAEXFL,   //Acquired data existing indicators allocation
  ESG_ESG_D_ERR_ACQDATAUNKNOWN, //Acquired data does not exist in DIPL data base
  ESG_ESG_D_WAR_CYCLEWOACQSITE, //Cycle without acquisition site declaration
  ESG_ESG_D_ERR_EQUIPWOACQSITE, //Equipment without acquisition site associated
  ESG_ESG_D_ERR_INFOWOACQSITE,  //Information without acquisition site associated
  ESG_ESG_D_WAR_BADREQCLASS,    //Bad request class

  ESG_ESG_D_ERR_BADALLOC,   //Cannot allocate memory

  ESG_ESG_D_ERR_UNKNOWNFLAG,    //Unknown flag for consulting or updating
  ESG_ESG_D_ERR_INEXISTINGREQ,  //Inexisting request in In Progress List
  ESG_ESG_D_WAR_BADCYCFAMILY,   //Bad cycle family

  ESG_ESG_D_ERR_UNKNOWNREPLY,   //Unknown reply identication
  ESG_ESG_D_ERR_CANNOTSENDMSG,  //Message cannot be sent
  ESG_ESG_D_ERR_NOACQSITE_HCPU, //No acquisition site found for the high CPU load

  ESG_ESG_D_ERR_BADMULRESPSPARAM,   //Parameter error for request multi-responses
  ESG_ESG_D_ERR_MULRESPSTABLEFULL,  //Request multi-responses Table full
  ESG_ESG_D_ERR_INEXISTINGENVVAR,   //Inexisting environment variable

  ESG_ESG_D_ERR_CANNOTLOCK, //Smad lock cannot be performed

  ESG_ESG_D_ERR_CANNOTDELFILE,  //Cannot delete file
  ESG_ESG_D_ERR_TOOMANYOPEN,    //Too many open file
  ESG_ESG_D_ERR_CLOSENOTOPEN,   //Cannot close file : maybe not correctly open
  ESG_ESG_D_ERR_NOTWRITABLE,    //File is not writable
  ESG_ESG_D_ERR_NOTREADABLE,    //File is not readable
  ESG_ESG_D_ERR_EOF,    //End of file entcountered

  ESG_ESG_D_ERR_UNEXPARAMVAL,   //Unexpected value for a parameter
  ESG_ESG_D_ERR_RESERVEMPTY,    //Reserved table is empty
  ESG_ESG_D_ERR_BADDEDFORMAT,   //Bad EDI ACSCII format for elementary data
  ESG_ESG_D_ERR_DEDLGTOOSHORT,  //Buffer for coded elementary data too short
  ESG_ESG_D_ERR_BADEDINTERTYPE, //Bad internal type for elementary data
  ESG_ESG_D_ERR_BADDEDIDENT,    //Bad elementary data identifier
  ESG_ESG_D_ERR_MAXINFOSREACHED,    //Teleinfo(s) rejected : max. number reached
  ESG_ESG_D_ERR_TRANSASCUTCTIME,    //Cannot transforme ascii UTC time to internal
  ESG_ESG_D_ERR_BADDCDIDENT,    //Bad composed data identifier
  ESG_ESG_D_ERR_NBEDINTERDCD,   //Internal and DCD composed data different ED number
  ESG_ESG_D_ERR_DCDLGTOOSHORT,  //Buffer for coded elementary data too short

  ESG_ESG_D_ERR_BADLISTEN,  //Problem listening named pipes (select)
  ESG_ESG_D_ERR_CHILDFAILURE,   //Failure activating child process

  ESG_ESG_D_ERR_NOTOPEN,    //File is not open via file services
  ESG_ESG_D_ERR_BADFILEOP,  //Incorrect file operation
  ESG_ESG_D_ERR_BUFFSIZE,   //Buffer size
  ESG_ESG_D_ERR_BADCATEG,   //Bad category
  ESG_ESG_D_ERR_BADDCDDEDLIST,  //Bad elementary data list in DCD
  ESG_ESG_D_ERR_ODMTABLE,   //Incoherence in ODM table

  ESG_ESG_D_ERR_TABLEFULL,  //Table full
  ESG_ESG_D_ERR_BADARGS,    //Non correct process parameters
  ESG_ESG_D_ERR_ANYEXCHINF, //Any info has not been found
  ESG_ESG_D_ERR_BADINFOCATEGORY,    //Bad info category
  ESG_ESG_D_ERR_BADSEGLABEL,    //Bad segment label
  ESG_ESG_D_ERR_BADSNDRCVID,    //Bad sender-receiver identifier
  ESG_ESG_D_ERR_CRELISTREQENTRY,    //Bad request entry in progress list creating
  ESG_ESG_D_ERR_CONSLISTREQENTRY,   //Bad request entry in progress list consulting
  ESG_ESG_D_ERR_DELELISTREQENTRY,   //Bad request entry in progress list deleting
  ESG_ESG_D_ERR_REQAUTORISATION,    //Request autorisation err. processing
  ESG_ESG_D_WAR_ACQTERMINATED,  //End of ACQ treatments. Waiting to be killed
  ESG_ESG_D_WAR_DIFFTERMINATED, //End of DIFF treatments. Waiting to be killed
  ESG_ESG_D_ERR_NOSTATESEG, //State exchange contains no state segment

  ESG_ESG_D_ERR_BADSUBTYPEATTNB,    //Incorrect number of attributes in sub-type
  ESG_ESG_D_ERR_NOHISTTISUBSCRIPT,  //No subscription for the historic period
  ESG_ESG_D_ERR_NOHISTTISUBTYPE,    //No sub-type for the historic period
  ESG_ESG_D_ERR_TILEDFORHISTACQ,    //No Ti identifier for Ti historic acquisition
  ESG_ESG_D_ERR_HHISTTIFREEREQCONT, //No free content in local request HHistTi
  ESG_ESG_D_ERR_NOMOREHHISTTIREQ,   //No more HHistTi request in period table
  ESG_ESG_D_ERR_NOTIFORHHISTTIREQ,  //No Ti for HHistTi request

  ESG_ESG_D_ERR_INEXPCOMPDATA,  //Inexpected composed data
  ESG_ESG_D_ERR_INCOMPLIST, //The Dir and DIPL list are incompatible

  ESG_ESG_D_ERR_PERIODNFOUND,   //The period to update has not been found
  ESG_ESG_D_WAR_BADREQPRIORITY, //Request priority different than normal/urgent
  ESG_ESG_D_ERR_NONCORRECTDATA, //Incorrect data in BDTR
  ESG_ESG_D_ERR_NOPERIODIHHISTFILE, //No period in HHISTINFSACQ file
  ESG_ESG_D_ERR_QUERYDB,    //Error in query DataBase
  ESG_ESG_D_ERR_BADCOMMAND, //Bad command
  ESG_ESG_D_ERR_DBREAD, //Error in reading data base
};















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
    // Внутренний тест SMED
    int test_smed();
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

