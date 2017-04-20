#if !defined EXCHANGE_CONFIG_EGSA_HPP
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
  std::string smad_filename;
} egsa_common_t;

// Набор допустимых идентификаторов Цикла
typedef enum {
  ID_CYCLE_GENCONTROL       = 0,
  ID_CYCLE_URGINFOS         = 1,
  ID_CYCLE_INFOSACQ_URG     = 2,
  ID_CYCLE_INFOSACQ         = 3,
  ID_CYCLE_INFO_DACQ_DIPADJ = 4,
  ID_CYCLE_GEN_GACQ_DIPADJ  = 5,
  ID_CYCLE_GCP_PGACQ_DIPL   = 6,
  ID_CYCLE_GCS_SGACQ_DIPL   = 7,
  ID_CYCLE_INFO_DACQ_DIPL   = 8,
  ID_CYCLE_IAPRIMARY        = 9,
  ID_CYCLE_IASECOND         = 10,
  ID_CYCLE_SERVCMD          = 11,
  ID_CYCLE_INFODIFF         = 12,
  ID_CYCLE_ACQSYSACQ        = 13,
  ID_CYCLE_GCT_TGACQ_DIPL   = 14,
  ID_CYCLE_UNKNOWN          = 15   // Последняя запись, неизвестный цикл
} cycle_id_t;

const int ID_CYCLE_MAX = ID_CYCLE_UNKNOWN + 1;

#define REQUESTNAME_MAX     17
#define ECH_D_MAXNBSITES    28
#define EGA_EGA_D_LGCYCLENAME 23

// Target object class
typedef enum 
{
  INFO,     // information
  EQUIP,    // technological equipment,
  ACQSYS    // acquisition system
} ega_ega_t_ObjectClass;

// acquisition mode
typedef enum {
  DIFF,
  NONDIFF,
  NOT_SPECIFIED
} ech_t_AcqMode;

typedef enum {
  // Нормальный цикл инициализации
  TN_INITAUTO,
  // Цикл инициализации, требующий повышенной производительности
  // (более требователен к ресурсам)
  /* TH_INITAUTO, */
  // Цикл определения устаревших запросов
  TC_OLDREQUEST,
  // Цикл отправки ответов
  TC_SENDREPLY
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

// Типы запросов
// NB: Поддерживать синхронизацию с типами запросов в common.proto
// Нумерация начинается с нуля и вохрастает монотонно без разрывов
typedef enum {
  ECH_D_NONEXISTANT  = -1,  /* request not exist */
  ECH_D_GENCONTROL   = 0,   /* general control */
  ECH_D_INFOSACQ     = 1,   /* global acquisition */
  ECH_D_URGINFOS     = 2,   /* urgent data acquisition */
  ECH_D_GAZPROCOP    = 3,   /* gaz volume count acquisition */
  ECH_D_EQUIPACQ     = 4,   /* equipment acquisition */
  ECH_D_ACQSYSACQ    = 5,   /* Sac data acquisition */
  ECH_D_ALATHRES     = 6,   /* alarms and thresholds acquisition */
  ECH_D_TELECMD      = 7,   /* telecommand */
  ECH_D_TELEREGU     = 8,   /* teleregulation */
  ECH_D_SERVCMD      = 9,   /* service command */
  ECH_D_GLOBDWLOAD   = 10,  /* global download */
  ECH_D_PARTDWLOAD   = 11,  /* partial download */
  ECH_D_GLOBUPLOAD   = 12,  /* Sac configuration global upload */
  ECH_D_INITCMD      = 13,  /* initialisation of the exchanges */
  ECH_D_GCPRIMARY    = 14,  /* Primary general control */
  ECH_D_GCSECOND     = 15,  /* Secondary general control */
  ECH_D_GCTERTIARY   = 16,  /* Tertiary general control */
  ECH_D_DIFFPRIMARY  = 17,  /* Primary differential acquisition */
  ECH_D_DIFFSECOND   = 18,  /* Secondary differential acquisition */
  ECH_D_DIFFTERTIARY = 19,  /* Tertiary differential acquisition */
  ECH_D_INFODIFFUSION= 20,  /* Information diffusion              */
  ECH_D_DELEGATION   = 21   /* Process order delegation-end of delegation */
} ech_t_ReqId;

// NB: Должен быть последним значением ech_t_ReqId + 1
#define NBREQUESTS          (ECH_D_DELEGATION + 1)
// Максимальное количество вложенных подзапросов
#define MAX_INTERNAL_REQUESTS   10

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
#define EGA_EGA_D_STRINFOSDIFF    "D_INFODIFF"  // diffusion informations
#define EGA_EGA_D_STRDIFFPRIMARY  "D_DIFFPRIMARY"   // diffusion primary
#define EGA_EGA_D_STRDIFFSECOND   "D_DIFFSECONDARY" // diffusion secondary
#define EGA_EGA_D_STRDIFFTERTIARY "D_DIFFTERTIARY"  // diffusion tertiary
#define EGA_EGA_D_STRDELEGATION   "P_DELEGATION"    // delegation telecommand

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
  // Идентификатор запроса
  ech_t_ReqId   e_RequestId;
  // Название запроса
  char  s_RequestName[REQUESTNAME_MAX + 1];
  // Приоритет
  int   i_RequestPriority;
  // Тип объекта - информация, СС, оборудование
  ega_ega_t_ObjectClass e_RequestObject;
  // Режим сбора - дифференциальный или нет
  ech_t_AcqMode e_RequestMode;
  // Признак отношения запроса к технологическим данным (1)
  // или к состоянию самой системы сбора как опрашиваемого объекта (2)
  bool  b_Requestprocess;
  // Включенные в данный запрос подзапросы
  ech_t_ReqId   r_IncludingRequests[MAX_INTERNAL_REQUESTS];
  //ega_ega_t_Requests r_IncludingRequests;
} ega_ega_odm_t_RequestEntry;

// Статическая связка между идентификатором Цикла (его порядковым номером) и его названием
typedef struct {
  cycle_id_t  cycle_id;     // Идентификаторы Циклов
  const char *cycle_name;   // Ссылка на константные названия Циклов
} cycle_dictionary_item_t;

// Характеристика одного сайта, содержащиеся в конфигурационном файле
typedef struct {
  std::string name;         // название
  // size_t id;                // идентификатор
  sa_object_level_t level;  // место в иерархии - выше/ниже/локальный/смежный
  gof_t_SacNature nature;   // тип СС
  bool auto_init;           // флаг необходимости проводить инициализацию связи
  bool auto_gencontrol;     // флаг допустимости проведения процедуры сбора данных
} egsa_config_site_item_t;

// Перечень сайтов со своими характеристиками
typedef std::map<const std::string, egsa_config_site_item_t*> egsa_config_sites_t;

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

// Перечень циклов
typedef std::map<const std::string, egsa_config_cycle_info_t*> egsa_config_cycles_t;
// Перечень типов Запросов
typedef std::map<const std::string, ega_ega_odm_t_RequestEntry*> egsa_config_requests_t;

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
    // Загрузка обещей части конфигурационного файла
    int load_common();
    // Загрузка информации о циклах
    int load_cycles();
    // Загрузка информации о сайтах
    int load_sites();
    // Загрузка информации о запросах
    int load_requests();
    // Вернуть название внутренней SMAD для EGSA
    const std::string& smad_name();
    // Загруженные Циклы
    egsa_config_cycles_t& cycles() { return m_cycles; };
    // Загруженные Сайты
    egsa_config_sites_t& sites() { return m_sites; };
    // Загруженные Запросы
    egsa_config_requests_t& requests() { return m_requests; };
    ech_t_ReqId get_request_id(const std::string&);
    const char* get_request_name(ech_t_ReqId);

  private:
    DISALLOW_COPY_AND_ASSIGN(EgsaConfig);
    cycle_id_t get_cycle_id_by_name(const std::string&);
    // Найти по таблице НСИ запрос с заданным идентификатором
    int get_request_by_id(ech_t_ReqId, ega_ega_odm_t_RequestEntry*&);
    // Найти по таблице НСИ запрос с заданным названием
    int get_request_by_name(const std::string&, ega_ega_odm_t_RequestEntry*&);

    rapidjson::Document m_document;
    char   *m_config_filename;
    bool    m_config_has_good_format;

    // Секция "COMMON" конфигурационного файла ==============================
    // Название секции
    static const char*  s_SECTION_NAME_COMMON_NAME;
    // Название ключа "имя файла SMAD EGSA"
    static const char*  s_SECTION_COMMON_NAME_SMAD_FILE;
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
    static ega_ega_odm_t_RequestEntry g_request_dictionary[NBREQUESTS];
};

#endif

