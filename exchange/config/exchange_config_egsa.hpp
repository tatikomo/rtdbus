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
#include "glog/logging.h"
#include "rapidjson/document.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"

typedef struct {
  int nature_code;
  std::string designation;
} map_nature_dict_t;

// ==========================================================================================
// Общая информация
typedef struct {
  std::string smad_filename;
} egsa_common_t;

// Характеристика одного сайта
typedef struct {
  std::string name; // название
  int  level;       // место в иерархии - выше/ниже/локальный/смежный
  int  nature;      // тип СС
  bool auto_init;   // флаг необходимости проводить инициализацию связи
  bool auto_gencontrol; // флаг допустимости проведения процедуры сбора данных
} egsa_config_site_item_t;

// Перечень сайтов со своими характеристиками
typedef std::map<std::string, egsa_config_site_item_t*> egsa_config_sites_t;

// Характеристика одного цикла из конфигурации
typedef struct {
  std::string name;
  int period;
  std::string request_name;
  std::vector <std::string> sites;
} egsa_config_cycle_info_t;

// Перечень циклов
typedef std::map<std::string, egsa_config_cycle_info_t*> egsa_config_cycles_t;

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
    // Загрузка информации о запросах
    int load_sites();
    // Вернуть название внутренней SMAD для EGSA
    const std::string& smad_name();
    // Загруженные циклы
    egsa_config_cycles_t& cycles() { return m_cycles; };
    // Загруженные сайты
    egsa_config_sites_t& sites() { return m_sites; };

  private:
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

    static const map_nature_dict_t m_nature_dict[];
    // По символьному наименованию типа СС вернуть его числовой код (из массива m_nature_dict)
    int get_nature_code_by_name(const std::string&);
};

#endif

