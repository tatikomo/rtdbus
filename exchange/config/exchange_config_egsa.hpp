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
  std::string request_name;         // TODO: непонятно назначение связанного с Циклом Запроса
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
typedef struct {
  std::string name;
  char priority;// 1..127
  char object;  // {A|I}
  char mode;    // {DIFF|NONDIFF}
  std::vector <std::string> included_requests;
} egsa_config_request_info_t;

// Перечень циклов
typedef std::map<const std::string, egsa_config_cycle_info_t*> egsa_config_cycles_t;
// Перечень типов Запросов
typedef std::map<const std::string, egsa_config_request_info_t*> egsa_config_requests_t;

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

  private:
    DISALLOW_COPY_AND_ASSIGN(EgsaConfig);
    cycle_id_t get_cycle_id_by_name(const std::string&);

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

    static cycle_dictionary_item_t g_cycle_dictionary[ID_CYCLE_MAX];
};

#endif

