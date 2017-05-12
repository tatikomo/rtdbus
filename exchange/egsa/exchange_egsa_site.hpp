#ifndef EXCHANGE_EGSA_SITE_HPP
#define EXCHANGE_EGSA_SITE_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <memory>
#include <list>
#include <map>

// Служебные заголовочные файлы сторонних утилит

// Служебные файлы RTDBUS
#include "exchange_config_egsa.hpp"
#include "exchange_config.hpp"

class EGSA;
class SystemAcquisition;
class Cycle;
class Request;
class InternalSMAD;
class AcqSiteEntry;

// Transition definition
// ---------------------
#define  EGA_EGA_AUT_D_TRANS_O  0   // Operational
#define  EGA_EGA_AUT_D_TRANS_NO 1   // No Operational
#define  EGA_EGA_AUT_D_TRANS_E  2   // Exploitation
#define  EGA_EGA_AUT_D_TRANS_NE 3   // Maintenance
#define  EGA_EGA_AUT_D_TRANS_I  4   // Inhibition
#define  EGA_EGA_AUT_D_TRANS_NI 5   // No inhibition

// States definition
// -----------------
#define  EGA_EGA_D_STATEINIT    0
#if 0
/*
EGA_EGA_AUT_D_STATE_A;   // Не в запрете, не в техобслуживании, не оперативна    NI_NM_NO
EGA_EGA_AUT_D_STATE_N;   // Не в запрете, не в техобслуживании,    оперативна    NI_NM_O
EGA_EGA_AUT_D_STATE_B;   // Не в запрете,    в техобслуживании, не оперативна    NI_M_NO
EGA_EGA_AUT_D_STATE_C;   // Не в запрете,    в техобслуживании,    оперативна    NI_M_O
EGA_EGA_AUT_D_STATE_WA;  //    в запрете, не в техобслуживании, не оперативна    I_NM_NO
EGA_EGA_AUT_D_STATE_WN;  //    в запрете, не в техобслуживании,    оперативна    I_NM_O
EGA_EGA_AUT_D_STATE_WB;  //    в запрете,    в техобслуживании, не оперативна    I_M_NO
EGA_EGA_AUT_D_STATE_WC;  //    в запрете,    в техобслуживании,    оперативна    I_M_O
*/
#define  EGA_EGA_AUT_D_STATE_A  EGA_EGA_D_STATEINIT     // Acquisition systems requests only
#define  EGA_EGA_AUT_D_STATE_B  EGA_EGA_D_STATEINIT+1   // only process commands in maintain mode
#define  EGA_EGA_AUT_D_STATE_N  EGA_EGA_D_STATEINIT+2   // Normal operation state
#define  EGA_EGA_AUT_D_STATE_C  EGA_EGA_D_STATEINIT+3   // Dispatcher request only
#define  EGA_EGA_AUT_D_STATE_WA EGA_EGA_D_STATEINIT+4   // Inhibition
#define  EGA_EGA_AUT_D_STATE_WB EGA_EGA_D_STATEINIT+5   // Inhibition
#define  EGA_EGA_AUT_D_STATE_WN EGA_EGA_D_STATEINIT+6   // Inhibition
#define  EGA_EGA_AUT_D_STATE_WC EGA_EGA_D_STATEINIT+7   // Inhibition

#define  EGA_EGA_AUT_D_NB_TRANS (EGA_EGA_AUT_D_TRANS_NI+1)   // transition number (state/mode values)
#define  EGA_EGA_AUT_D_NB_STATE (EGA_EGA_AUT_D_STATE_WC+1)   // automate state number

#else

#define  EGA_EGA_D_STATEINIT    0
typedef enum {
/* 0 */  EGA_EGA_AUT_D_STATE_NI_NM_NO  = EGA_EGA_D_STATEINIT,    // Acquisition systems requests only
/* 1 */  EGA_EGA_AUT_D_STATE_NI_M_NO   = EGA_EGA_D_STATEINIT+1,  // Only commands processing in maintaince mode
/* 2 */  EGA_EGA_AUT_D_STATE_NI_NM_O   = EGA_EGA_D_STATEINIT+2,  // Normal operation state
/* 3 */  EGA_EGA_AUT_D_STATE_NI_M_O    = EGA_EGA_D_STATEINIT+3,  // Dispatcher request only
/* 4 */  EGA_EGA_AUT_D_STATE_I_NM_NO   = EGA_EGA_D_STATEINIT+4,  // Inhibition
/* 5 */  EGA_EGA_AUT_D_STATE_I_M_NO    = EGA_EGA_D_STATEINIT+5,  // Inhibition
/* 6 */  EGA_EGA_AUT_D_STATE_I_NM_O    = EGA_EGA_D_STATEINIT+6,  // Inhibition
/* 7 */  EGA_EGA_AUT_D_STATE_I_M_O     = EGA_EGA_D_STATEINIT+7   // Inhibition
} sa_state_t;

#define EGA_EGA_AUT_D_NB_TRANS (EGA_EGA_AUT_D_TRANS_NI + 1)     // transition number (state/mode values)
#define EGA_EGA_AUT_D_NB_STATE (EGA_EGA_AUT_D_STATE_I_M_O + 1)  // automate state number
#endif

// ==============================================================================
// Acquisition Site Entry Structure
//
// Состояние Систем Сбора:
// Конфигурационные файлы (cnf)
// Способ подключения (comm)
// Перечень параметров обмена (params)
// Состояние связи (link)
//
// UNKNOWN (Неизвестное - первоначальное значение)
//  |       cnf-,comm-params-,link-
//  v
// DISCONNECT (используется EGSA до завершения инициализации СС)
//  |       cnf+,comm-params-,link-
//  v
// UNREACH  <---------------------------+---+   (СС недоступна)
//  |       cnf+,comm+params+,link-     |   |
//  v                                   |   |
// PRE_OPERATIONAL ---------------------+   |   (СС в процессе инициализации связи)
//  |       cnf+,comm+params+,link+-        |
//  v                                       |
//  OPERATIONAL ----------------------------+   (СС в рабочем режиме)
//          cnf+,comm+params+,link+

class AcqSiteEntry {
  // Automation structure definition
  // -------------------------------
  typedef enum {
    ACTION_NONE       = 0,
    ACTION_AUTOINIT   = 1,
    ACTION_GENCONTROL = 2
  } action_type_t;

  // Запись об элементарном состоянии СС
  typedef struct {
    sa_state_t  next_state;
    action_type_t action_type;
  } ega_ega_aut_t_automate;

  // Состояние ТИ определенного типа
  typedef enum {
    UNKNOWN  = 0,
    RECEIVE  = 1,
    ACQUIRE  = 2,
    SENT     = 3,
    TRANSMIT = 4
  } teleinformation_processing_t;

  typedef enum {
    NONE    = 0,
    BEGIN   = 1,
    END_NOHHIST = 2,
    END     = 3
  } init_phase_t;

  public:
    AcqSiteEntry(EGSA*, const egsa_config_site_item_t*);
   ~AcqSiteEntry();

    const char* name()      const { return m_IdAcqSite; }
    gof_t_SacNature nature()const { return m_NatureAcqSite; }
    bool        auto_i()    const { return m_AutomaticalInit; }
    bool        auto_gc()   const { return m_AutomaticalGenCtrl; }
    sa_state_t  state()     const { return m_FunctionalState; }
    sa_object_level_t level() const { return m_Level; }

    // Регистрация Запросов в указанном Цикле
    int push_request_for_cycle(Cycle*, const int*);

    // TODO: СС и EGSA могут работать на разных хостах, в этом случае подключение EGSA к smad СС
    // не получит доступа к реальным данным от СС. Их придется EGSA туда заносить самостоятельно.
    int attach_smad();
    int detach_smad();
    // Доступ к списку текущих Запросов
    std::list<Request*>& requests() { return m_requests_in_progress; }
    // Послать сообщение указанного типа
    int send(int);
    // Послать сообщение инициализации
    void init();
    void process_end_all_init(); // Сообщение о завершении инициализации
    void process_end_init_acq(); // Запрос состояния завершения инициализации
    void process_init();         // Конец инициализации
    void process_dif_init();     // Запрос завершения инициализации после аварийного завершения
    // Отключение
    void shutdown();
    // Выполнить указанное действие с системой сбора
    int control(int /* тип сообщения */);
    // Прочитать изменившиеся данные
    int pop(sa_parameters_t&);
    // Передать в СС указанные значения
    int push(sa_parameters_t&);
    // Запросить готовность
    int ask_ENDALLINIT();
    // Проверить поступление ответа готовности
    void check_ENDALLINIT();
    // Сменить состояние в зависимости от поданного на вход атрибута
    // Управление состояниями СС в зависимости от асинхронных изменений состояния атрибутов
    // SYNTHSTATE, INHIBITION, EXPMODE в БДРВ, приходящих по подписке
    int change_state(int, int);

    // Проверить допустимость принятия Запроса к исполнению
    bool state_filter(const Request*);
    bool ega_state_filter(const Request*); // для запросов к локальным подчиненным СС
    bool esg_state_filter(const Request*, int);  // для запросов к удаленным/вышестоящим системам
    // Попытаться добавить в очередь указанный Запрос
    int add_request(const Request*);
    // Функция вызывается при необходимости создания Запросов на инициализацию связи
    int cbAutoInit();
    // Функция вызывается при необходимости создания Запросов на общий сбор информации (есть, Генерал Контрол!)
    int cbGeneralControl();

  private:
    // Exchanged Info table header */
    // --------------------------- */
    //  . Global number of exchanged data 
    //  . Primary exchanged data number
    //  . Secondary exchanged data number 
    //  . Tertiary exchanged data number
    //  . Exploitation exchanged data number
    //  . TI number, concerned by quarter historic 
    //  . TI number, concerned by hour historic 
    //  . TI number, concerned by day historic 
    //  . TI number, concerned by month historic 
    //  . TI, which alarm has to be transmit number
    typedef struct {
        size_t                 h_EffNb; 
        size_t                 h_PrimNb;
        size_t                 h_SecondNb;
        size_t                 h_TertiaryNb;
        size_t                 h_ExploitNb;
        size_t                 h_QuarterNb;
        size_t                 h_HourNb;
        size_t                 h_DayNb;
        size_t                 h_MonthNb;
        size_t                 h_AlarmNb;
    } esg_esg_odm_t_ExchInfoNumb;

    DISALLOW_COPY_AND_ASSIGN(AcqSiteEntry);
    // Инициализация внутреннего состояния в зависимости от атрибутов SYNTHSTATE, INHIB, EXPMODE
    void init_functional_state();
    // Получить ссылку на экземпляр Запроса указанного типа
    const Request* get_dict_request(ech_t_ReqId);
    // Состояние авторизации (?)
    bool OPStateAuthorised() { return m_OPStateAuthorised; }
    init_phase_t InitPhase() { return m_init_phase; };

    EGSA* m_egsa;
    synthstate_t m_synthstate;
    bool m_expmode;
    bool m_inhibition;

    // identifier of the acquisition site (name)
    char m_IdAcqSite[TAG_NAME_MAXLEN + 1];
    // Имя диспетчера
    char m_DispatcherName[TAG_NAME_MAXLEN + 1];

    // nature of the acquisition site (acquisition system, adjacent DIPL)
    gof_t_SacNature m_NatureAcqSite;

    // indicator of automatical initialisation
    // TRUE -> has to be performed
    // FALSE-> has not to be performed
    bool m_AutomaticalInit;

    // indicator of automatical general control
    // TRUE -> has to be performed
    // FALSE-> has not to be performed
    bool m_AutomaticalGenCtrl;

    // functional EGSA state of the acquisition site
    // (acquisition site command only, all operations, all dispatcher requests only, waiting for no inhibition)
    sa_state_t m_FunctionalState;

    // Уровень СС - вышестоящая, подчиненная, соседняя,...
    sa_object_level_t m_Level;

    // interface component state : active / stopped (TRUE / FALSE)
    bool m_InterfaceComponentActive;

    // Ссылка на внутреннюю SMAD - acquired data access
    InternalSMAD    *m_smad;

    // composed requests table - a dynamic table containing the number of requests and description of each request
    //std::list<const Request*> m_requests_composed;
    // requests in progress list access
    std::list<Request*> m_requests_in_progress;

    // Поля, специфичные для удаленных Сайтов того же, или верхнего уровня (соседние объекты или управление)
    // Поскольку процедура установления связи с ними растянута по времени и состоит из нескольких
    // последовательных этапов, необходимо иметь информацию о текущем состоянии процедуры инициализации
    // удаленного Сайта (см. описание esg_esg_odm_t_AcqSiteEntry в esg_esg_odm_p.h).
    //
    // b_OPStateAuthorised
    //   FALSE -> no Init to the distant in progress / Init to the distant in progress
    //   TRUE -> Init to the distant terminated with success
    // b_DistantInitTerminated
    //   FALSE -> no Init received from the distant / Init from the distant in progress
    //   TRUE -> Init from the distant terminated
    //
    static const ega_ega_aut_t_automate m_ega_ega_aut_a_auto [EGA_EGA_AUT_D_NB_TRANS][EGA_EGA_AUT_D_NB_STATE];


    // ========================= ESG ================================================
    // матрица допустимости Запросов по типам от текущего функционального состояния СС
    static const bool enabler_matrix[REQUEST_ID_LAST + 1][EGA_EGA_AUT_D_NB_STATE + 1];
    // Состояние предыстории определенной дискретизации - секундная (2017/05: пока нет),
    // минутная, 5-минутная, часовая, суточная, месячная.
    // NB: количество типов синхронизировать первоисточником в rtap_db.mco, тип HistoryType
    teleinformation_processing_t m_history_info[6];
    // Состояние авторизации
    bool m_OPStateAuthorised;
    // Состояние процедуры инициализации связи, используется для ESG
    init_phase_t m_init_phase;
    // Состояние типа данных "Тревоги"
    teleinformation_processing_t m_Alarms;
    // Состояние типа данных "Телеизмерения"
    teleinformation_processing_t m_Infos;
    // ========================= ESG ================================================
};

// ==============================================================================
// Acquisition Site Entry Structure
class AcqSiteList {
  public:
    AcqSiteList();
   ~AcqSiteList();

    int attach_to_smad(const char*);
    int detach_from_smad(const char*);
    // Отключиться от SMAD
    int detach();
    // Очистить ресурсы
    int release();
    void insert(AcqSiteEntry*);
    size_t size() const { return m_items.size(); };
    void set_egsa(EGSA*);

    // Вернуть элемент по имени Сайта
    AcqSiteEntry* operator[](const char*);
    // Вернуть элемент по имени Сайта
    AcqSiteEntry* operator[](const std::string&);
    // Вернуть элемент по индексу
    AcqSiteEntry* operator[](const std::size_t);

  private:
    DISALLOW_COPY_AND_ASSIGN(AcqSiteList);

    EGSA* m_egsa;
    struct map_cmp_str {
      // Оператор сравнения строк в m_site_map.
      // Иначе происходит арифметическое сравнение значений указателей, но не содержимого строк
      bool operator()(const std::string& a, const std::string& b) { return a.compare(b) < 0; }
    };
    // Связь между названием СС и её данными
    typedef std::map<const std::string, size_t, map_cmp_str> system_acquisition_list_t;

    std::vector<std::shared_ptr<AcqSiteEntry*>> m_items;
    // Связь между названием СС и её индексом в m_items
    system_acquisition_list_t  m_site_map;
};



#endif

