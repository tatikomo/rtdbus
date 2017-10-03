#ifndef EXCHANGE_CONFIG_EGSA_HPP
#define EXCHANGE_CONFIG_EGSA_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <string>
#include <map>

// Служебные заголовочные файлы сторонних утилит
#include "rapidjson/document.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"

// ==========================================================================================
// Приведенные структуры данных используются для первоначальной загрузки данных из конфигурации.
// Далее эта информация, включая дополнительные атрибуты СС, используется при работе Службы.
// Общая информация
typedef struct {
  // Название файла SMED
  std::string smed_filename;
  // Название конфигурационного файла ESG
  std::string dict_esg_filename;
} egsa_common_t;

// Набор допустимых идентификаторов Цикла
typedef enum {
  ID_CYCLE_GENCONTROL       = 0,
  ID_CYCLE_URGINFOS         = 1,
  ID_CYCLE_INFOSACQ_URG     = 2,
  ID_CYCLE_INFOSACQ         = 3,
  ID_CYCLE_INFO_DACQ_DIPADJ = 4,
  ID_CYCLE_INFO_DIFF_DIPADJ = 5,
  ID_CYCLE_INFO_DIFF_PRIDIR = 6,
  ID_CYCLE_INFO_DIFF_SECDIR = 7,
  ID_CYCLE_GEN_GACQ_DIPADJ  = 8,
  ID_CYCLE_GCP_PGACQ_DIPL   = 9,
  ID_CYCLE_GCS_SGACQ_DIPL   = 10,
  ID_CYCLE_INFO_DACQ_DIPL   = 11,
  ID_CYCLE_IAPRIMARY        = 12,
  ID_CYCLE_IASECOND         = 13,
  ID_CYCLE_SERVCMD          = 14,
  ID_CYCLE_INFODIFF         = 15,
  ID_CYCLE_ACQSYSACQ        = 16,
  ID_CYCLE_GCT_TGACQ_DIPL   = 17,
  ID_CYCLE_UNKNOWN          = 18   // Последняя запись, неизвестный цикл
} cycle_id_t;

const int ID_CYCLE_MAX = ID_CYCLE_UNKNOWN + 1;

#define REQUESTNAME_MAX     17
#define ECH_D_MAXNBSITES    28
#define EGA_EGA_D_LGCYCLENAME 23

// Target object class
typedef enum
{
  INFO  = 1,    // information - команды телеуправления
  EQUIP = 2,    // technological equipment - опрос наличия оборудования в НСИ
  ACQSYS= 4,    // acquisition system - запрос телеметрии
  SERV  = 8,    // system maintenace - служебные запросы на обслуживание системы сбора
  ALL   = 16    // all of it - все вышеперечисленное
} ega_ega_t_ObjectClass;

// acquisition mode
typedef enum {
  DIFF,
  NONDIFF,
  NOT_SPECIFIED
} ech_t_AcqMode;

typedef enum {
  // Нормальный цикл инициализации
  TN_INITAUTO   = 0,
  // Цикл инициализации, требующий повышенной производительности
  // (более требователен к ресурсам)
  TH_INITAUTO   = 1,
  // Цикл определения устаревших запросов
  TC_OLDREQUEST = 2,
  // Цикл отправки ответов
  TC_SENDREPLY  = 3
} cycle_family_2_t; //GEV непонятно пока к чему относится

typedef enum {
  // Нормальный цикл
  CYCLE_NORMAL      = 1,
  // Цикл, требующий повышенной производительности
  // (более требователен к ресурсам)
  CYCLE_HCPULOAD    = 2,
  // Цикл управления
  CYCLE_CONTROL     = 3
} cycle_family_t;

// Типы запросов, передаваемых соответствующим процессам.
// NB: Поддерживать синхронизацию с типами запросов в common.proto
// Нумерация начинается с нуля и вохрастает монотонно без разрывов
typedef enum {
  // Запросы к локальным подчинённым системам сбора [0..21]
  // Требуют постоянного подключения, после обрыва связи - EGA_INITCMD
  EGA_GENCONTROL   = 0,   // general control
  EGA_INFOSACQ     = 1,   // global acquisition
  EGA_URGINFOS     = 2,   // urgent data acquisition
  EGA_GAZPROCOP    = 3,   // gaz volume count acquisition
  EGA_EQUIPACQ     = 4,   // equipment acquisition
  EGA_ACQSYSACQ    = 5,   // Sac data acquisition
  EGA_ALATHRES     = 6,   // alarms and thresholds acquisition
  EGA_TELECMD      = 7,   // telecommand
  EGA_TELEREGU     = 8,   // teleregulation
  EGA_SERVCMD      = 9,   // service command
  EGA_GLOBDWLOAD   = 10,  // global download
  EGA_PARTDWLOAD   = 11,  // partial download
  EGA_GLOBUPLOAD   = 12,  // Sac configuration global upload
  EGA_INITCMD      = 13,  // initialisation of the exchanges
  EGA_GCPRIMARY    = 14,  // Primary general control
  EGA_GCSECOND     = 15,  // Secondary general control
  EGA_GCTERTIARY   = 16,  // Tertiary general control
  EGA_DIFFPRIMARY  = 17,  // Primary differential acquisition
  EGA_DIFFSECOND   = 18,  // Secondary differential acquisition
  EGA_DIFFTERTIARY = 19,  // Tertiary differential acquisition
  EGA_INFODIFFUSION= 20,  // Information diffusion
  EGA_DELEGATION   = 21,  // Process order delegation-end of delegation

  // Запросы к глобальным/внешним иноформационным системам.
  // Не требуют постоянного подключения.
  ESG_BASID_STATECMD      = 22,
  ESG_BASID_STATEACQ      = 23,
  ESG_BASID_SELECTLIST    = 24,
  ESG_BASID_GENCONTROL    = 25,
  ESG_BASID_INFOSACQ      = 26,
  ESG_BASID_HISTINFOSACQ  = 27,
  ESG_BASID_ALARM         = 28,
  ESG_BASID_THRESHOLD     = 29,
  ESG_BASID_ORDER         = 30,
  ESG_BASID_HHISTINFSACQ  = 31,
  ESG_BASID_HISTALARM     = 32,
  ESG_BASID_CHGHOUR       = 33,
  ESG_BASID_INCIDENT      = 34,
  ESG_BASID_MULTITHRES    = 35,  // OutLine Thresholds
  ESG_BASID_TELECMD       = 36,
  ESG_BASID_TELEREGU      = 37,
  ESG_BASID_EMERGENCY     = 38,
  ESG_BASID_ACDLIST       = 39,
  ESG_BASID_ACDQUERY      = 40,

  // Local requests are exchanged with local EGSA of our site
  ESG_LOCID_GENCONTROL    = 41,
  ESG_LOCID_GCPRIMARY     = 42,
  ESG_LOCID_GCSECONDARY   = 43,
  ESG_LOCID_GCTERTIARY    = 44,
  ESG_LOCID_INFOSACQ      = 45,
  ESG_LOCID_INITCOMD      = 46,
  ESG_LOCID_CHGHOURCMD    = 47,
  ESG_LOCID_TELECMD       = 48,
  ESG_LOCID_TELEREGU      = 49,
  ESG_LOCID_EMERGENCY     = 50,
  NOT_EXISTENT = (ESG_LOCID_EMERGENCY + 1)   // 51, request not exist
} ech_t_ReqId;

// Коды запросов ГОФО отличаются от приведенных выше ech_t_ReqId, и для обмена с объектами ГОФО должны использоваться старые коды 
typedef enum {
  ESG_ESG_D_BASID_STATECMD      = 0,
  ESG_ESG_D_BASID_STATEACQ      = 1,
  ESG_ESG_D_BASID_SELECTLIST    = 2,
  ESG_ESG_D_BASID_GENCONTROL    = 3,
  ESG_ESG_D_BASID_INFOSACQ      = 4,
  ESG_ESG_D_BASID_HISTINFOSACQ  = 5,
  ESG_ESG_D_BASID_ALARM         = 6,
  ESG_ESG_D_BASID_THRESHOLD     = 7,
  ESG_ESG_D_BASID_ORDER         = 8,
  ESG_ESG_D_BASID_HHISTINFSACQ  = 9,
  ESG_ESG_D_BASID_HISTALARM     = 10,
  ESG_ESG_D_BASID_CHGHOUR       = 11,
  ESG_ESG_D_BASID_INCIDENT      = 12,
  ESG_ESG_D_BASID_MULTITHRES    = 13,  // OutLine Thresholds
  ESG_ESG_D_BASID_TELECMD       = 14,
  ESG_ESG_D_BASID_TELEREGU      = 15,
  ESG_ESG_D_BASID_EMERGENCY     = 16,
  ESG_ESG_D_BASID_ACDLIST       = 17,
  ESG_ESG_D_BASID_ACDQUERY      = 18
} gofo_ech_t_ReqId;

// NB: Должен быть последним значением ech_t_ReqId + 1
#define NBREQUESTS          (NOT_EXISTENT)
#define REQUEST_ID_FIRST    EGA_GENCONTROL
#define REQUEST_ID_LAST     ESG_LOCID_EMERGENCY

// Names of External System Requests
// ---------------------------------
#define EGA_EGA_D_STRGENCONTROL "A_GENCONTROL"  // general control     (none differential)
#define EGA_EGA_D_STRINFOSACQ   "A_INFOSACQ"    // all informations         (differential)
#define EGA_EGA_D_STRURGINFOS   "A_URGINFOS"    // urgent informations      (differential)
#define EGA_EGA_D_STRALATHRES   "A_ALATHRES"    // alarms and thresholds    (differential)
#define EGA_EGA_D_STRGAZPROCOP  "A_GAZPROCOP"   // gaz volume count    (none differential)
#define EGA_EGA_D_STREQUIPACQ   "A_EQUIPACQ"    // equipment           (none differential)
#define EGA_EGA_D_STRACQSYSACQ  "A_ACQSYSACQ"   // SATO informations   (none differential)
#define EGA_EGA_D_STRTELECMD    "P_TELECMD"	    // telecommand
#define EGA_EGA_D_STRTELEREGU   "P_TELEREGU"    // teleregulation
#define EGA_EGA_D_STRSERVCMD    "S_SERVCMD"	    // SATO service command
#define EGA_EGA_D_STRGLOBDWLOAD "C_GLOBDWLOAD"  // global downloading
#define EGA_EGA_D_STRPARTDWLOAD "C_PARTDWLOAD"  // partial downloading
#define EGA_EGA_D_STRGLOBUPLOAD "C_GLOBUPLOAD"  // global uploading
#define EGA_EGA_D_STRINITCMD    "I_INITCMD"	    // initialization
#define EGA_EGA_D_STRGCPRIMARY  "A_GCPRIMARY"   // primary general control  (none differential)
#define EGA_EGA_D_STRGCSECOND   "A_GCSECOND"    // secondary general control(none differential)
#define EGA_EGA_D_STRGCTERTIARY "A_GCTERTIARY"  // tertiary general control (none differential)
#define EGA_EGA_D_STRIAPRIMARY  "A_IAPRIMARY"   // acq.prim.informations    (differential)
#define EGA_EGA_D_STRIASECOND   "A_IASECOND"    // acq.sec. informations    (differential)
#define EGA_EGA_D_STRIATERTIARY "A_IATERTIARY"  // acq.tert. informations   (differential)
#define EGA_EGA_D_STRINFOSDIFF  "D_INFODIFF"    // global diffusion of informations
#define EGA_EGA_D_STRDELEGATION "P_DELEGATION"  // delegation telecommand
#define EGA_EGA_D_STRNOEXISTENT "NOT_EXISTENT"  // internal error sign

// Names of Distant Sites Requests
// ---------------------------------
#define ESG_ESG_D_BASSTR_STATECMD       "AB_STATESCMD"    // Site state ask
#define ESG_ESG_D_BASSTR_STATEACQ       "DB_STATESACQ"    // Site state
#define ESG_ESG_D_BASSTR_SELECTLIST     "AB_SELECTLIST"   // Selective list
#define ESG_ESG_D_BASSTR_GENCONTROL     "AB_GENCONTROL"   // TI general control
#define ESG_ESG_D_BASSTR_INFOSACQ       "DB_INFOSACQ"     // TI
#define ESG_ESG_D_BASSTR_HISTINFOSACQ   "DB_HISTINFOSACQ" // TI Historicals
#define ESG_ESG_D_BASSTR_ALARM          "DB_ALARM"        // Alarms
#define ESG_ESG_D_BASSTR_THRESHOLD      "DB_THRESHOLD"    // Thresholds
#define ESG_ESG_D_BASSTR_ORDER          "DB_ORDER"        // Orders
#define ESG_ESG_D_BASSTR_HHISTINFSACQ   "AB_HHISTINFSACQ" // TI Historics
#define ESG_ESG_D_BASSTR_HISTALARM      "AB_HISTALARM"    // TI Historics
#define ESG_ESG_D_BASSTR_CHGHOUR        "DB_CHGHOUR"      // Hour change
#define ESG_ESG_D_BASSTR_INCIDENT       "DB_INCIDENT"     // INCIDENT
#define ESG_ESG_D_BASSTR_MULTITHRES     "DB_MULTITHRES"   // Multi Thresholds (outline)
#define ESG_ESG_D_BASSTR_TELECMD        "DB_TELECMD"      // Telecommand
#define ESG_ESG_D_BASSTR_TELEREGU       "DB_TELEREGU"     // Teleregulation
#define ESG_ESG_D_BASSTR_EMERGENCY      "DB_EMERGENCY"    // Emergency cycle request
#define ESG_ESG_D_BASSTR_ACDLIST        "DB_ACDLIST"      // ACD list element
#define ESG_ESG_D_BASSTR_ACDQUERY       "DB_ACDQUERY"     // ACD query element

#define ESG_ESG_D_NBRBASREQ             (ESG_ESG_D_BASID_ACDQUERY - EGA_DELEGATION)
// Индекс последнего базового запроса, далее идут локальные
#define LAST_BASIC_REQUEST              ESG_BASID_ACDQUERY

// Names of LOCAL Requests
// Local requests are exchanged with local EGSA of our site
// --------------------------------------------------------
#define ESG_ESG_D_LOCSTR_GENCONTROL  "AL_GENCONTROL" // Ti general control
#define ESG_ESG_D_LOCSTR_GCPRIMARY   "AL_GCPRIMARY"  // Ti primary general control
#define ESG_ESG_D_LOCSTR_GCSECONDARY "AL_GCSECONDARY"// Ti second general control
#define ESG_ESG_D_LOCSTR_GCTERTIARY  "AL_GCTERTIARY" // Ti tertiary general control
#define ESG_ESG_D_LOCSTR_INFOSACQ    "AL_INFOSACQ"   // TeleInformations
#define ESG_ESG_D_LOCSTR_INITCOMD    "IL_INITCOMD"   // Initialisation command
#define ESG_ESG_D_LOCSTR_CHGHOURCMD  "DL_CHGHOURCMD" // Summer/Winter hour change
#define ESG_ESG_D_LOCSTR_TELECMD     "DL_TELECMD"    // TC order request name
#define ESG_ESG_D_LOCSTR_TELEREGU    "DL_TELEREGU"   // TR order request name
#define ESG_ESG_D_LOCSTR_EMERGENCY   "AL_EMERGENCY"  // Emergency state

// number of local requests of local site
#define ESG_ESG_D_NBRLOCREQ         10

// Наибольшая допустимая длина названия поля (к примеру, C0101)
#define MAX_ICD_NAME_LENGTH         6
// Максимально допустимое количество вложенных полей в одном LOCSTRUCT
#define MAX_LOCSTRUCT_FIELDS_NUM    21

// ==============================================================================
// Acquisition Site Entry Structure
typedef struct {
  // identifier of the acquisition site (name)
  char s_IdAcqSite[SERVICE_NAME_MAXLEN + 1];

  // nature of the acquisition site (acquisition system, adjacent DIPL)
  gof_t_SacNature i_NatureAcqSite;

  // Не требуется хранить номер интерфейсного процесса o_IdInterfaceComponent
  // identifier of the interface component (procnum)
  //gof_t_Uint8 o_IdInterfaceComponent;
  // indicator of automatical initialisation (TRUE -> has to be performed,
  //                                          FALSE-> has not to be performed)
  bool b_AutomaticalInit;

  // indicator of automatical general control (TRUE -> has to be performed,
  //                                           FALSE-> has not to be performed)
  bool b_AutomaticalGenCtrl;

  // Не требуется
  // data base information subscription for synthetic state, inhibition state, exploitation mode
  //ega_ega_odm_t_SubscriptedInfo r_SubscrInhibState;
  //ega_ega_odm_t_SubscriptedInfo r_SubscrSynthState;
  //ega_ega_odm_t_SubscriptedInfo r_SubscrExploitMode;

  // functional EGSA state of the acquisition site
  // (acquisition site command only, all operations, all dispatcher requests only, waiting for no inhibition)
  int i_FunctionalState;

  // interface component state : active / stopped (TRUE / FALSE)
  bool b_InterfaceComponentActive;

  // requests in progress list access
  void* p_ProgList;

  // acquired data access
  void* p_AcquiredData;
} ega_ega_odm_t_AcqSiteEntry;

// ==============================================================================
typedef struct {
  // Идентификатор базового запроса
  ech_t_ReqId   e_RequestId;
  // Название запроса
  char  s_RequestName[REQUESTNAME_MAX + 1];
  // Приоритет
  int   i_RequestPriority;
  // Тип объекта - информация, СС, оборудование
  ega_ega_t_ObjectClass e_RequestObject;
  // Режим сбора - дифференциальный или нет
  ech_t_AcqMode e_RequestMode;
  // Признак отношения запроса к технологическим данным (true)
  // или к состоянию самой системы сбора как опрашиваемого объекта (false)
  bool  b_Requestprocess;
  // Включенные в данный запрос подзапросы.
  // Индекс ненулевого элемента есть его ech_t_ReqId, значение индекса - очередность исполнения запроса.
  // Очередность должна сохраниться той же, что и в конфигурации.
  int r_IncludingRequests[NBREQUESTS];
} RequestEntry;

// ==============================================================================
// Статическая связка между идентификатором Цикла (его порядковым номером) и его названием
typedef struct {
  cycle_id_t  cycle_id;     // Идентификаторы Циклов
  const char *cycle_name;   // Ссылка на константные названия Циклов
} cycle_dictionary_item_t;

// ==============================================================================
// Характеристика одного сайта, содержащиеся в конфигурационном файле
typedef struct {
  std::string name;         // название
  // size_t id;                // идентификатор
  sa_object_level_t level;  // место в иерархии - выше/ниже/локальный/смежный
  gof_t_SacNature nature;   // тип СС
  bool auto_init;           // флаг необходимости проводить инициализацию связи
  bool auto_gencontrol;     // флаг допустимости проведения процедуры сбора данных
} egsa_config_site_item_t;

// ==============================================================================
// Перечень сайтов со своими характеристиками
typedef std::map<const std::string, egsa_config_site_item_t*> egsa_config_sites_t;

// ==============================================================================
// Характеристика одного цикла из конфигурации
typedef struct {
  std::string name; // Название Цикла, берется из файла конфигурации
  cycle_id_t id;    // Числовой уникальный идентификатор Цикла, берется из таблицы связи "Имя"<->"ID"
  int period;       // Период активации в секундах
  std::string request_name;         // Связанный Запрос
  std::vector <std::string> sites;  // Список Сайтов, задействованных в Цикле, берется из конфигурации
} egsa_config_cycle_info_t;

// Характеристика одного Запроса из конфигурации
//     {
//     "OBJECT": "A",
//     "MODE": "DIFF",
//     "INCLUDED_REQUESTS": [
//      { "NAME": "URGINFOS" },
//      { "NAME": "ALATHRES" }
//      ]
//

// ==============================================================================
// Перечень циклов
typedef std::map<const std::string, egsa_config_cycle_info_t*> egsa_config_cycles_t;
// ==============================================================================
// Перечень типов Запросов
typedef std::map<const std::string, RequestEntry*> egsa_config_requests_t;

// ==============================================================================
// Описание элементарного типа данных из DED_ELEMTYPES
typedef struct {
  // Название элементарного типа, 6 символов
  char name[MAX_ICD_NAME_LENGTH + 1];
  // Тип телеинформации
  elemtype_t tm_type;
  // Строка размерности, 6 символов
  char format_size[MAX_ICD_NAME_LENGTH + 1];
} elemtype_item_t;

// ==============================================================================
typedef struct {
  char field[MAX_ICD_NAME_LENGTH + 1];// Название поля
  char attribute[TAG_NAME_MAXLEN + 1];// Название соответствующего атрибута в БДРВ (если есть)
  field_type_t type;                  // Для атрибута БДРВ - тип данных. Для обычного поля - FIELD_TYPE_UNKNOWN
  int length;                         // Для атрибута символьного типа - длина строки
} field_item_t;

// ==========================================================================================
// Данные для чтения конфигурации разделщв конфигурации DCD_ELEMSTRUCTS и ESG_LOCSTRUCTS
// Описание композитного(составного) типа данных
typedef struct {
  char name[MAX_ICD_NAME_LENGTH + 1];           // json-атрибут "NAME"
  char associate[MAX_ICD_NAME_LENGTH + 1];      // json-атрибут "ASSOCIATE"
  elemtype_class_t tm_class;
  // Количество структур в массиве fields
  size_t    num_fields;
  // Количество атрибутов БДРВ из всего набора fields (имеющие поле TYPE)
  size_t    num_attributes;
  // Вложенные структуры
  field_item_t fields[MAX_LOCSTRUCT_FIELDS_NUM];
} elemstruct_item_t;

// ==========================================================================================
// Разбор конфигурационных файлов:
//   EGA_SITES - перечень зарегистрированных объектов с указанием
//     NAME - Кода объекта (тег БДРВ)
//     NATURE - Типа объекта (ГТП, ЛПУ, СС определённого типа,...)
//     FLAG_AI - Флаг необходимости автоматической инициализации связи (auto init)
//     FLAG_GC - Флаг необходимости выполнения действий по сбору данных (general control)
//
//   EGA_CYCLES - перечень циклов, применяемых к зарегистрированным объектам
//     NAME - наименование цикла
//     PERIOD - периодичность инициации данного цикла
//     REQUEST - название запроса, связанного с этим циклом
//     Последовательность SITE от 0..N - название объекта, обрабатываемого в данном цикле
//
//   EGA_REQUESTS - перечень характеристик запросов, используемых в Циклах
//     NAME - название запроса
//     PRIORITY - уровень приоритета (255 - минимальный, 1 - максимальный)
//     OBJECT - sA, Equipment, Info
//     MODE
//     INCLUDED_REQUESTS
//
//   TODO: перенести данные из ESG_SAHOST в EGA_SITES
//   ESG_SAHOST - связь между тегами БДРВ объектов и их физическими хостами
//     NAME - Кода объекта (тег БДРВ)
//     HOST - наименование хоста объекта (из /etc/hosts)
//
class EgsaConfig {
  public:
    EgsaConfig(const char*);
   ~EgsaConfig();

    // Загрузка данных
    int load();
    // Загрузка общей части конфигурационного файла
    int load_common();
    // Загрузка информации о циклах
    int load_cycles();
    // Загрузка информации о сайтах
    int load_sites();
    // Загрузка информации о запросах
    int load_requests();
    // Загрузка НСИ ESG
    int load_esg();
    elemtype_item_t*    elemtypes()   { return m_elemtype_items; }
    size_t elemtypes_count() const    { return m_elemtypes_count; }
    elemstruct_item_t*  elemstructs() { return m_elemstruct_items; }
    size_t elemstructs_count() const  { return m_elemstructs_count; }
    // Название SMED
    const std::string& smed_name();
    // Загруженные Циклы
    egsa_config_cycles_t& cycles() { return m_cycles; }
    // Загруженные Сайты
    egsa_config_sites_t& sites() { return m_sites; }
    // Загруженные Запросы
    egsa_config_requests_t& requests() { return m_requests; }
    ech_t_ReqId get_request_dict_id(const std::string&);
    const char* get_request_dict_name(ech_t_ReqId);
    // Найти по таблице НСИ запрос с заданным идентификатором
    int get_request_dict_by_id(ech_t_ReqId, RequestEntry*&);

  private:
    DISALLOW_COPY_AND_ASSIGN(EgsaConfig);
    cycle_id_t get_cycle_id_by_name(const std::string&);
    // Найти по таблице НСИ запрос с заданным названием
    int get_request_dict_by_name(const std::string&, RequestEntry*&);
    // Вернуть идентификатор типа атрибута
    field_type_t get_db_attr_type_id(const std::string&);

    int load_DCD_ELEMSTRUCTS(rapidjson::Value&);
    int load_DED_ELEMTYPES(rapidjson::Value&);

    rapidjson::Document m_egsa_config_document;
    rapidjson::Document m_esg_config_document;
    char   *m_config_filename;
    bool    m_config_has_good_format;
    // Данные DED_ELEMTYPES из конфигурационного файла
    elemtype_item_t  *m_elemtype_items;
    size_t  m_elemtypes_count;
    // Данные DCD_ELEMSTRUCTS + ESG_LOCSTRUCTS из конфигурационного файла
    elemstruct_item_t  *m_elemstruct_items;
    size_t  m_elemstructs_count;

    // Секция "COMMON" конфигурационного файла ==============================
    // Название секции
    static const char*  s_SECTION_NAME_COMMON_NAME;
    // Название ключа "имя файла SMAD EGSA"
    static const char*  s_SECTION_COMMON_NAME_SMED_FILE;
    static const char*  s_SECTION_COMMON_NAME_DICT_ESG_FILE;
    egsa_common_t       m_common;

    // Секция "SITES" конфигурационного файла ===============================
    // Название секции
    static const char*  s_SECTION_NAME_SITES_NAME;
    // Название ключа "имя файла SMAD EGSA"
    static const char*  s_SECTION_SITES_NAME_NAME;
    static const char*  s_SECTION_SITES_NAME_LEVEL;
    static const char*  s_SECTION_SITES_NAME_LEVEL_LOCAL;
    static const char*  s_SECTION_SITES_NAME_LEVEL_UPPER;
    static const char*  s_SECTION_SITES_NAME_LEVEL_ADJACENT;
    static const char*  s_SECTION_SITES_NAME_LEVEL_LOWER;
	static const char*  s_SECTION_SITES_NAME_NATURE;
	static const char*  s_SECTION_SITES_NAME_AUTO_INIT;
	static const char*  s_SECTION_SITES_NAME_AUTO_GENCONTROL;
    egsa_config_sites_t m_sites;

    // Секция "CYCLES" конфигурационного файла ==============================
    // Название секции
    static const char*  s_SECTION_NAME_CYCLES_NAME;
    static const char*  s_SECTION_CYCLES_NAME_NAME;
	static const char*  s_SECTION_CYCLES_NAME_PERIOD;
	static const char*  s_SECTION_CYCLES_NAME_REQUEST;
	static const char*  s_SECTION_CYCLES_NAME_SITE;
	static const char*  s_SECTION_CYCLES_NAME_SITE_NAME;
    egsa_config_cycles_t m_cycles;

    // Секция "REQUESTS" конфигурационного файла ==============================
    // Название секции
    static const char*  s_SECTION_NAME_REQUESTS_NAME;
    static const char*  s_SECTION_REQUESTS_NAME_NAME;
    static const char*  s_SECTION_REQUESTS_NAME_PRIORITY;
    static const char*  s_SECTION_REQUESTS_NAME_OBJECT;
    static const char*  s_SECTION_REQUESTS_NAME_MODE;
    static const char*  s_SECTION_REQUESTS_NAME_INC_REQ;
    static const char*  s_SECTION_REQUESTS_NAME_INC_REQ_NAME;
    egsa_config_requests_t m_requests;

    static const char*  s_CYCLENAME_GENCONTROL;
    static const char*  s_CYCLENAME_URGINFOS;
    static const char*  s_CYCLENAME_INFOSACQ_URG;
    static const char*  s_CYCLENAME_INFOSACQ;
    static const char*  s_CYCLENAME_INFO_DACQ_DIPADJ;
    static const char*  s_CYCLENAME_INFO_DIFF_DIPADJ;
    static const char*  s_CYCLENAME_INFO_DIFF_PRIDIR;
    static const char*  s_CYCLENAME_INFO_DIFF_SECDIR;
    static const char*  s_CYCLENAME_GEN_GACQ_DIPADJ;
    static const char*  s_CYCLENAME_GCP_PGACQ_DIPL;
    static const char*  s_CYCLENAME_GCS_SGACQ_DIPL;
    static const char*  s_CYCLENAME_INFO_DACQ_DIPL;
    static const char*  s_CYCLENAME_IAPRIMARY;
    static const char*  s_CYCLENAME_IASECOND;
    static const char*  s_CYCLENAME_SERVCMD;
    static const char*  s_CYCLENAME_INFODIFF;
    static const char*  s_CYCLENAME_ACQSYSACQ;
    static const char*  s_CYCLENAME_GCT_TGACQ_DIPL;

    static cycle_dictionary_item_t    g_cycle_dictionary[ID_CYCLE_MAX];
    static RequestEntry g_request_dictionary[NBREQUESTS + 1]; // +1 для NOT_EXISTENT
};

#endif

