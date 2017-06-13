///////////////////////////////////////////////////////////////////////////////////////////
// Класс представления конифгурации модуля EGSA
// 2016/04/14
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <iostream> // std::cout
#include <string>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_config_egsa.hpp"
#include "exchange_config_nature.hpp"

using namespace rapidjson;

// Секция "COMMON" конфигурационного файла ==============================
const char* EgsaConfig::s_SECTION_NAME_COMMON_NAME       = "COMMON";
// Название ключа "имя файла SMED EGSA"
const char* EgsaConfig::s_SECTION_COMMON_NAME_SMED_FILE  = "SMED_FILE";
const char* EgsaConfig::s_SECTION_COMMON_NAME_DICT_ESG_FILE  = "DICT_ESG_FILE";
// Секция "SITES" конфигурационного файла ===============================
// Название секции
const char* EgsaConfig::s_SECTION_NAME_SITES_NAME    = "SITES";
// Название ключа "имя файла SMAD EGSA"
const char* EgsaConfig::s_SECTION_SITES_NAME_NAME    = "NAME";
// Уровень в иерархии
const char* EgsaConfig::s_SECTION_SITES_NAME_LEVEL   = "LEVEL";
// Уровень в иерархии - локальная АСУ
const char* EgsaConfig::s_SECTION_SITES_NAME_LEVEL_LOCAL = "LOCAL";
// Уровень в иерархии - вышестоящая система
const char* EgsaConfig::s_SECTION_SITES_NAME_LEVEL_UPPER = "UPPER";
// Уровень в иерархии - соседняя/смежная система
const char* EgsaConfig::s_SECTION_SITES_NAME_LEVEL_ADJACENT = "ADJACENT";
// Уровень в иерархии - подчиненная система
const char* EgsaConfig::s_SECTION_SITES_NAME_LEVEL_LOWER = "LOWER";
const char* EgsaConfig::s_SECTION_SITES_NAME_NATURE  = "NATURE";
const char* EgsaConfig::s_SECTION_SITES_NAME_AUTO_INIT       = "AUTO_INIT";
const char* EgsaConfig::s_SECTION_SITES_NAME_AUTO_GENCONTROL = "AUTO_GENCONTROL";
// Секция "CYCLES" конфигурационного файла ==============================
// Название секции
const char* EgsaConfig::s_SECTION_NAME_CYCLES_NAME   = "CYCLES";
const char* EgsaConfig::s_SECTION_CYCLES_NAME_NAME   = "NAME";
const char* EgsaConfig::s_SECTION_CYCLES_NAME_PERIOD = "PERIOD";
const char* EgsaConfig::s_SECTION_CYCLES_NAME_REQUEST= "REQUEST";
const char* EgsaConfig::s_SECTION_CYCLES_NAME_SITE   = "SITE";
const char* EgsaConfig::s_SECTION_CYCLES_NAME_SITE_NAME = "NAME";
// Секция "REQUESTS" конфигурационного файла ==============================
// Название секции
const char* EgsaConfig::s_SECTION_NAME_REQUESTS_NAME            = "REQUESTS";
const char* EgsaConfig::s_SECTION_REQUESTS_NAME_NAME            = "NAME";
const char* EgsaConfig::s_SECTION_REQUESTS_NAME_PRIORITY        = "PRIORITY";
const char* EgsaConfig::s_SECTION_REQUESTS_NAME_OBJECT          = "OBJECT";
const char* EgsaConfig::s_SECTION_REQUESTS_NAME_MODE            = "MODE";
const char* EgsaConfig::s_SECTION_REQUESTS_NAME_INC_REQ         = "INCLUDED_REQUESTS";
const char* EgsaConfig::s_SECTION_REQUESTS_NAME_INC_REQ_NAME    = "NAME";
// Допустимые значения названий Циклов
const char* EgsaConfig::s_CYCLENAME_GENCONTROL       = "GENCONTROL";
const char* EgsaConfig::s_CYCLENAME_URGINFOS         = "URGINFOS";
const char* EgsaConfig::s_CYCLENAME_INFOSACQ_URG     = "INFOSACQ_URG";
const char* EgsaConfig::s_CYCLENAME_INFOSACQ         = "INFOSACQ";
const char* EgsaConfig::s_CYCLENAME_INFO_DACQ_DIPADJ = "INFO_DACQ_DIPADJ";
const char* EgsaConfig::s_CYCLENAME_INFO_DIFF_DIPADJ = "INFO_DIFF_DIPADJ";
const char* EgsaConfig::s_CYCLENAME_INFO_DIFF_PRIDIR = "INFO_DIFF_PRIDIR";
const char* EgsaConfig::s_CYCLENAME_INFO_DIFF_SECDIR = "INFO_DIFF_SECDIR";
const char* EgsaConfig::s_CYCLENAME_GEN_GACQ_DIPADJ  = "GEN_GACQ_DIPADJ";
const char* EgsaConfig::s_CYCLENAME_GCP_PGACQ_DIPL   = "GCS_SGACQ_DIPL";
const char* EgsaConfig::s_CYCLENAME_GCS_SGACQ_DIPL   = "GCS_SGACQ_DIPL";
const char* EgsaConfig::s_CYCLENAME_INFO_DACQ_DIPL   = "INFO_DACQ_DIPL";
const char* EgsaConfig::s_CYCLENAME_IAPRIMARY        = "IAPRIMARY";
const char* EgsaConfig::s_CYCLENAME_IASECOND         = "IASECOND";
const char* EgsaConfig::s_CYCLENAME_SERVCMD          = "SERVCMD";
const char* EgsaConfig::s_CYCLENAME_INFODIFF         = "INFODIFF";
const char* EgsaConfig::s_CYCLENAME_ACQSYSACQ        = "ACQSYSACQ";
const char* EgsaConfig::s_CYCLENAME_GCT_TGACQ_DIPL   = "GCT_TGACQ_DIPL";

cycle_dictionary_item_t EgsaConfig::g_cycle_dictionary[] = {
  { ID_CYCLE_GENCONTROL,        EgsaConfig::s_CYCLENAME_GENCONTROL      },
  { ID_CYCLE_URGINFOS,          EgsaConfig::s_CYCLENAME_URGINFOS        },
  { ID_CYCLE_INFOSACQ_URG,      EgsaConfig::s_CYCLENAME_INFOSACQ_URG    },
  { ID_CYCLE_INFOSACQ,          EgsaConfig::s_CYCLENAME_INFOSACQ        },
  { ID_CYCLE_INFO_DACQ_DIPADJ,  EgsaConfig::s_CYCLENAME_INFO_DACQ_DIPADJ},
  { ID_CYCLE_INFO_DIFF_DIPADJ,  EgsaConfig::s_CYCLENAME_INFO_DIFF_DIPADJ},
  { ID_CYCLE_INFO_DIFF_PRIDIR,  EgsaConfig::s_CYCLENAME_INFO_DIFF_PRIDIR},
  { ID_CYCLE_INFO_DIFF_SECDIR,  EgsaConfig::s_CYCLENAME_INFO_DIFF_SECDIR},
  { ID_CYCLE_GEN_GACQ_DIPADJ,   EgsaConfig::s_CYCLENAME_GEN_GACQ_DIPADJ },
  { ID_CYCLE_GCP_PGACQ_DIPL,    EgsaConfig::s_CYCLENAME_GCP_PGACQ_DIPL  },
  { ID_CYCLE_GCS_SGACQ_DIPL,    EgsaConfig::s_CYCLENAME_GCS_SGACQ_DIPL  },
  { ID_CYCLE_INFO_DACQ_DIPL,    EgsaConfig::s_CYCLENAME_INFO_DACQ_DIPL  },
  { ID_CYCLE_IAPRIMARY,         EgsaConfig::s_CYCLENAME_IAPRIMARY       },
  { ID_CYCLE_IASECOND,          EgsaConfig::s_CYCLENAME_IASECOND        },
  { ID_CYCLE_SERVCMD,           EgsaConfig::s_CYCLENAME_SERVCMD         },
  { ID_CYCLE_INFODIFF,          EgsaConfig::s_CYCLENAME_INFODIFF        },
  { ID_CYCLE_ACQSYSACQ,         EgsaConfig::s_CYCLENAME_ACQSYSACQ       },
  { ID_CYCLE_GCT_TGACQ_DIPL,    EgsaConfig::s_CYCLENAME_GCT_TGACQ_DIPL  },
  { ID_CYCLE_UNKNOWN,           NULL                                    }
};

// ==========================================================================================================
// Запросы в адрес подчиненных систем сбора - с префиксом EGA
// Запросы в адрес смежных информационных систем - с префиксом ESG (esg_esg_d.h, esg_esg_odm_p.h, esg_esg_odm_OperDataManage.c)
RequestEntry EgsaConfig::g_request_dictionary[] = {
//         Идентификатор запроса
//         |                      Название запроса
//         |                      |                         Приоритет (меньше значение - меньше приоритет)
//         |                      |                         |   Тип объекта - информация, СС, оборудование
//         |                      |                         |   |       Режим сбора - дифференциальный (без запроса сервера)
//         |                      |                         |   |       или нет (запрос-ответ)
//         |                      |                         |   |       |              Признак отношения запроса:
//         |                      |                         |   |       |              к технологическим данным (true)
//         |                      |                         |   |       |              к состоянию системы сбора (false)
//         |                      |                         |   |       |              |      Вложенные запросы
//         |                      |                         |   |       |              |      (Ненулевое значение говорит о количестве
//         |                      |                         |   |       |              |      подзапросов подобного типа)
//         |                      |                         |   |       |              |      |
/* 00 */  {EGA_GENCONTROL,        EGA_EGA_D_STRGENCONTROL,  80, ACQSYS, NONDIFF,       true,  {} },
/* 01 */  {EGA_INFOSACQ,          EGA_EGA_D_STRINFOSACQ,    80, ACQSYS, DIFF,          true,  {} },
/* 02 */  {EGA_URGINFOS,          EGA_EGA_D_STRURGINFOS,    80, ACQSYS, DIFF,          true,  {} },
/* 03 */  {EGA_GAZPROCOP,         EGA_EGA_D_STRGAZPROCOP,   80, ACQSYS, NONDIFF,       false, {} },
/* 04 */  {EGA_EQUIPACQ,          EGA_EGA_D_STREQUIPACQ,    80, EQUIP,  NONDIFF,       true,  {} },
/* 05 */  {EGA_ACQSYSACQ,         EGA_EGA_D_STRACQSYSACQ,   80, ACQSYS, NONDIFF,       false, {} },
/* 06 */  {EGA_ALATHRES,          EGA_EGA_D_STRALATHRES,    80, ACQSYS, DIFF,          true,  {} },
/* 07 */  {EGA_TELECMD,           EGA_EGA_D_STRTELECMD,    101, INFO,   NOT_SPECIFIED, true,  {} },
/* 08 */  {EGA_TELEREGU,          EGA_EGA_D_STRTELEREGU,   101, INFO,   NOT_SPECIFIED, true,  {} },
/* 09 */  {EGA_SERVCMD,           EGA_EGA_D_STRSERVCMD,     80, SERV,   NOT_SPECIFIED, false, {} },
/* 10 */  {EGA_GLOBDWLOAD,        EGA_EGA_D_STRGLOBDWLOAD,  80, SERV,   NOT_SPECIFIED, false, {} },
/* 11 */  {EGA_PARTDWLOAD,        EGA_EGA_D_STRPARTDWLOAD,  80, SERV,   NOT_SPECIFIED, false, {} },
/* 12 */  {EGA_GLOBUPLOAD,        EGA_EGA_D_STRGLOBUPLOAD,  80, SERV,   NOT_SPECIFIED, false, {} },
/* 13 */  {EGA_INITCMD,           EGA_EGA_D_STRINITCMD,     82, ACQSYS, NOT_SPECIFIED, false, {} },
/* 14 */  {EGA_GCPRIMARY,         EGA_EGA_D_STRGCPRIMARY,   80, ACQSYS, NONDIFF,       true,  {} },
/* 15 */  {EGA_GCSECOND,          EGA_EGA_D_STRGCSECOND,    80, ACQSYS, NONDIFF,       true,  {} },
/* 16 */  {EGA_GCTERTIARY,        EGA_EGA_D_STRGCTERTIARY,  80, ACQSYS, NONDIFF,       true,  {} },
/* 17 */  {EGA_DIFFPRIMARY,       EGA_EGA_D_STRIAPRIMARY,   80, ACQSYS, DIFF,          true,  {} },
/* 18 */  {EGA_DIFFSECOND,        EGA_EGA_D_STRIASECOND,    80, ACQSYS, DIFF,          true,  {} },
/* 19 */  {EGA_DIFFTERTIARY,      EGA_EGA_D_STRIATERTIARY,  80, ACQSYS, DIFF,          true,  {} },
/* 20 */  {EGA_INFODIFFUSION,     EGA_EGA_D_STRINFOSDIFF,   80, ACQSYS, NONDIFF,       true,  {} },
/* 21 */  {EGA_DELEGATION,        EGA_EGA_D_STRDELEGATION,  80, ACQSYS, NOT_SPECIFIED, true,  {} },
//
//         Идентификатор запроса
//         |                      Название запроса
//         |                      |                               Приоритет
//         |                      |                               |   Тип объекта - информация, СС, оборудование
//         |                      |                               |   |       Режим сбора - дифференциальный (без запроса сервера)
//         |                      |                               |   |       или нет (запрос-ответ)
//         |                      |                               |   |       |        Признак отношения запроса:
//         |                      |                               |   |       |        к технологическим данным (true)
//         |                      |                               |   |       |        к состоянию системы сбора (false)
//         |                      |                               |   |       |        |      Вложенные запросы
//         |                      |                               |   |       |        |      (Ненулевое значение говорит о количестве
//         |                      |                               |   |       |        |      подзапросов подобного типа)
//         |                      |                               |   |       |        |      |
/* 22 */  {ESG_BASID_STATECMD,    ESG_ESG_D_BASSTR_STATECMD,      80, ACQSYS, DIFF,    true,  {} }, // Site state ask
/* 23 */  {ESG_BASID_STATEACQ,    ESG_ESG_D_BASSTR_STATEACQ,      80, ACQSYS, DIFF,    true,  {} }, // Site state
/* 24 */  {ESG_BASID_SELECTLIST,  ESG_ESG_D_BASSTR_SELECTLIST,    80, ACQSYS, NONDIFF, true,  {} }, // Selective list
/* 25 */  {ESG_BASID_GENCONTROL,  ESG_ESG_D_BASSTR_GENCONTROL,    80, ACQSYS, NONDIFF, true,  {} }, // TI general control
/* 26 */  {ESG_BASID_INFOSACQ,    ESG_ESG_D_BASSTR_INFOSACQ,      80, ACQSYS, NONDIFF, true,  {} }, // TI
/* 27 */  {ESG_BASID_HISTINFOSACQ,ESG_ESG_D_BASSTR_HISTINFOSACQ,  80, ACQSYS, NONDIFF, true,  {} }, // TI Historicals
/* 28 */  {ESG_BASID_ALARM,       ESG_ESG_D_BASSTR_ALARM,        101, ACQSYS, NONDIFF, true,  {} }, // Alarms
/* 29 */  {ESG_BASID_THRESHOLD,   ESG_ESG_D_BASSTR_THRESHOLD,     80, ACQSYS, NONDIFF, true,  {} }, // Thresholds
/* 30 */  {ESG_BASID_ORDER,       ESG_ESG_D_BASSTR_ORDER,         80, ACQSYS, NONDIFF, true,  {} }, // Orders
/* 31 */  {ESG_BASID_HHISTINFSACQ,ESG_ESG_D_BASSTR_HHISTINFSACQ,  80, ACQSYS, NONDIFF, true,  {} }, // TI Historics
/* 32 */  {ESG_BASID_HISTALARM,   ESG_ESG_D_BASSTR_HISTALARM,    101, ACQSYS, NONDIFF, true,  {} }, // TI Historics
/* 33 */  {ESG_BASID_CHGHOUR,     ESG_ESG_D_BASSTR_CHGHOUR,       80, ACQSYS, NONDIFF, true,  {} }, // Hour change
/* 34 */  {ESG_BASID_INCIDENT,    ESG_ESG_D_BASSTR_INCIDENT,      80, ACQSYS, NONDIFF, true,  {} }, // Incident
/* 35 */  {ESG_BASID_MULTITHRES,  ESG_ESG_D_BASSTR_MULTITHRES,    80, ACQSYS, NONDIFF, true,  {} }, // Multi Thresholds (outline)
/* 36 */  {ESG_BASID_TELECMD,     ESG_ESG_D_BASSTR_TELECMD,      101, INFO,   NONDIFF, true,  {} }, // Telecommand
/* 37 */  {ESG_BASID_TELEREGU,    ESG_ESG_D_BASSTR_TELEREGU,     101, INFO,   NONDIFF, true,  {} }, // Teleregulation
/* 38 */  {ESG_BASID_EMERGENCY,   ESG_ESG_D_BASSTR_EMERGENCY,     80, ACQSYS, NONDIFF, true,  {} }, // Emergency cycle request
/* 39 */  {ESG_BASID_ACDLIST,     ESG_ESG_D_BASSTR_ACDLIST,       80, ACQSYS, NONDIFF, true,  {} }, // ACD list element
/* 40 */  {ESG_BASID_ACDQUERY,    ESG_ESG_D_BASSTR_ACDQUERY,      80, ACQSYS, NONDIFF, true,  {} }, // ACD query element

// Локальные композитные запросы, при попытке их добавления в очередь запросов происходит замещение
// на подчиненные запросы из INCLUDED_REQUESTS
/* 41 */  {ESG_LOCID_GENCONTROL,  ESG_ESG_D_LOCSTR_GENCONTROL,     0, ACQSYS, NONDIFF, true, {} },
/* 42 */  {ESG_LOCID_GCPRIMARY,   ESG_ESG_D_LOCSTR_GCPRIMARY,      0, ACQSYS, NONDIFF, true, {} },
/* 43 */  {ESG_LOCID_GCSECONDARY, ESG_ESG_D_LOCSTR_GCSECONDARY,    0, ACQSYS, NONDIFF, true, {} },
/* 44 */  {ESG_LOCID_GCTERTIARY,  ESG_ESG_D_LOCSTR_GCTERTIARY,     0, ACQSYS, NONDIFF, true, {} },
/* 45 */  {ESG_LOCID_INFOSACQ,    ESG_ESG_D_LOCSTR_INFOSACQ,       0, ACQSYS, NONDIFF, true, {} },
/* 46 */  {ESG_LOCID_INITCOMD,    ESG_ESG_D_LOCSTR_INITCOMD,       0, ACQSYS, NONDIFF, true, {} },
/* 47 */  {ESG_LOCID_CHGHOURCMD,  ESG_ESG_D_LOCSTR_CHGHOURCMD,     0, ACQSYS, NONDIFF, true, {} },
/* 48 */  {ESG_LOCID_TELECMD,     ESG_ESG_D_LOCSTR_TELECMD,        0, ACQSYS, NONDIFF, true, {} },
/* 49 */  {ESG_LOCID_TELEREGU,    ESG_ESG_D_LOCSTR_TELEREGU,       0, ACQSYS, NONDIFF, true, {} },
/* 50 */  {ESG_LOCID_EMERGENCY,   ESG_ESG_D_LOCSTR_EMERGENCY,      0, ACQSYS, NONDIFF, true, {} },
};

//
// ==========================================================================================
EgsaConfig::EgsaConfig(const char* config_filename)
 : m_config_filename(NULL),
   m_config_has_good_format(false),
   m_elemtype_items(NULL),
   m_elemstruct_items(NULL)
{
  const char* fname = "EgsaConfig";
  FILE* f_params = NULL;
  struct stat configfile_info;

#if VERBOSE>8
  LOG(INFO) << "CTOR EgsaConfig";
#endif
  m_config_filename = strdup(config_filename);

  // Выделить буфер readBuffer размером с читаемый файл
  if (-1 == stat(m_config_filename, &configfile_info)) {
    LOG(ERROR) << fname << ": Unable to stat() '" << m_config_filename << "': " << strerror(errno);
  }
  else {
    if (NULL != (f_params = fopen(m_config_filename, "r"))) {
      // Файл открылся успешно, прочитаем содержимое
      LOG(INFO) << fname << ": size of '" << m_config_filename << "' is " << configfile_info.st_size;
      char *readBuffer = new char[configfile_info.st_size + 1];

      FileReadStream is(f_params, readBuffer, configfile_info.st_size + 1);
      m_egsa_config_document.ParseStream(is);
      delete []readBuffer;

      assert(m_egsa_config_document.IsObject());

      if (!m_egsa_config_document.HasParseError())
      {
        // Конфиг-файл имеет корректную структуру
        m_config_has_good_format = true;
      }
      else {
        LOG(ERROR) << fname << ": Parsing " << m_config_filename;
      }

      fclose (f_params);
    } // Конец успешного чтения содержимого файла
    else {
      LOG(FATAL) << fname << ": Locating config file " << m_config_filename
                 << " (" << strerror(errno) << ")";
    }
  } // Конец успешной проверки размера файла

}

// ==========================================================================================
EgsaConfig::~EgsaConfig()
{
#if VERBOSE>8
  int idx = 0;
  LOG(INFO) << "Destructor EgsaConfig";
#endif

  free(m_config_filename); // NB: free() after strdup()
  delete[] m_elemtype_items;
  delete[] m_elemstruct_items;

  for (egsa_config_cycles_t::iterator it = m_cycles.begin();
       it != m_cycles.end();
       ++it)
  {
#if VERBOSE>8
    LOG(INFO) << "Free cycle " << ++idx << " " << (*it).first;
#endif
    delete (*it).second;
  }
  m_cycles.clear();

#if VERBOSE>8
  idx = 0;
#endif
  for (egsa_config_sites_t::iterator it = m_sites.begin();
       it != m_sites.end();
       ++it)
  {
#if VERBOSE>8
    LOG(INFO) << "Free site " << ++idx << " " << (*it).first;
#endif
    delete (*it).second;
  }
  m_sites.clear();

#if VERBOSE>8
  idx = 0;
#endif
  for (egsa_config_requests_t::iterator it = m_requests.begin();
       it != m_requests.end();
       ++it)
  {
#if VERBOSE>8
    LOG(INFO) << "Free request " << ++idx << " " << (*it).first;
#endif
    delete (*it).second;
  }
  m_requests.clear();
}

// ==========================================================================================
const std::string& EgsaConfig::smed_name()
{
  return m_common.smed_filename;
}

// ==========================================================================================
int EgsaConfig::load_common()
{
  int rc = NOK;
  const char* fname = "load_common";

  if (true == (m_egsa_config_document.HasMember(s_SECTION_NAME_COMMON_NAME))) {
    // Получение общих данных по конфигурации интерфейсного модуля RTDBUS Системы Сбора
    Value& section = m_egsa_config_document[s_SECTION_NAME_COMMON_NAME];
    if (section.IsObject()) {
      // TODO: Предварительно проверить наличие таких записуй перед их чтением
      m_common.smed_filename = section[s_SECTION_COMMON_NAME_SMED_FILE].GetString();
      m_common.dict_esg_filename = section[s_SECTION_COMMON_NAME_DICT_ESG_FILE].GetString();
      rc = OK;
      LOG(INFO) << fname << ": Use egsa SMED file: " << m_common.smed_filename;
      LOG(INFO) << fname << ": Use ESG DICTIONARY file: " << m_common.dict_esg_filename;
    }
    else LOG(ERROR) << fname << ": Section " << s_SECTION_NAME_COMMON_NAME << ": not valid format";
  }

  return rc;
}

// ==========================================================================================
cycle_id_t EgsaConfig::get_cycle_id_by_name(const std::string& look_name)
{
  cycle_id_t look_id = ID_CYCLE_UNKNOWN;

  for(int idx = ID_CYCLE_GENCONTROL; idx < ID_CYCLE_UNKNOWN; idx++) {
    if (0 == look_name.compare(0, TAG_NAME_MAXLEN, g_cycle_dictionary[idx].cycle_name))  {
      look_id = static_cast<cycle_id_t>(idx);
      break;
    }
  }

  return look_id;
}

// ==========================================================================================
int EgsaConfig::load_cycles()
{
  static const char* fname = "load_cycles";
  egsa_config_cycle_info_t *cycle_info = NULL;
  cycle_id_t cycle_id = ID_CYCLE_UNKNOWN;
  int rc = OK;

  // Получение данных по конфигурации Циклов
  Value& section = m_egsa_config_document[s_SECTION_NAME_CYCLES_NAME];
  if (section.IsArray()) {
    for (Value::ValueIterator itr = section.Begin(); itr != section.End(); ++itr)
    {
      // Получили доступ к очередному элементу Циклов
      const Value::Object& cycle_item = itr->GetObject();

      const std::string possible_cycle_name = cycle_item[s_SECTION_CYCLES_NAME_NAME].GetString();
      if (ID_CYCLE_UNKNOWN != (cycle_id = get_cycle_id_by_name(possible_cycle_name))) {
        cycle_info = new egsa_config_cycle_info_t;

        cycle_info->name = possible_cycle_name;
        cycle_info->id = cycle_id;
        cycle_info->period = cycle_item[s_SECTION_CYCLES_NAME_PERIOD].GetInt();
        cycle_info->request_name = cycle_item[s_SECTION_CYCLES_NAME_REQUEST].GetString();

        // Прочитать массив сайтов, поместить их в m_cycles.sites
        Value& site_list = cycle_item[s_SECTION_CYCLES_NAME_SITE];
#if VERBOSE > 7
        int idx = 0;
#endif
        for (Value::ValueIterator itr = site_list.Begin(); itr != site_list.End(); ++itr)
        {
          const Value::Object& site_item = itr->GetObject();
          const std::string site_name = site_item[s_SECTION_CYCLES_NAME_SITE_NAME].GetString();
#if VERBOSE > 7
          LOG(INFO) << fname << ": " << cycle_info->name
                    << " period=" << cycle_info->period
                    << " req_name=" << cycle_info->request_name
                    << " [" << (++idx) << "] " << site_name;
#endif
          cycle_info->sites.push_back(site_name);
        }

        m_cycles.insert(std::pair<std::string, egsa_config_cycle_info_t*>(cycle_info->name, cycle_info));
      }
      else {
        LOG(ERROR) << "Skip unsupported cycle name: " << possible_cycle_name;
      }
    }
  }
  else rc = NOK;

  return rc;
}

// ==========================================================================================
int EgsaConfig::load_sites()
{
  const char* fname = "load_sites";
  egsa_config_site_item_t *item;
  int rc = OK;

  LOG(INFO) << fname << ": CALL";

  // Получение данных по конфигурации Сайтов
  Value& section = m_egsa_config_document[s_SECTION_NAME_SITES_NAME];
  if (section.IsArray()) {
    for (Value::ValueIterator itr = section.Begin(); itr != section.End(); ++itr)
    {
      // Получили доступ к очередному элементу Циклов
      const Value::Object& cycle_item = itr->GetObject();

      item = new egsa_config_site_item_t;

      item->name = cycle_item[s_SECTION_SITES_NAME_NAME].GetString();
      const std::string nature = cycle_item[s_SECTION_SITES_NAME_NATURE].GetString();
      if  (GOF_D_SAC_NATURE_EUNK == (item->nature = SacNature::get_natureid_by_name(nature))) {
        LOG(WARNING) << fname << ": unknown nature label: " << nature;
      }

      std::string level = cycle_item[s_SECTION_SITES_NAME_LEVEL].GetString();
      if (0 == level.compare(s_SECTION_SITES_NAME_LEVEL_LOCAL))
        item->level = LEVEL_LOCAL;
      else if (0 == level.compare(s_SECTION_SITES_NAME_LEVEL_LOWER))
        item->level = LEVEL_LOWER;
      else if (0 == level.compare(s_SECTION_SITES_NAME_LEVEL_ADJACENT))
        item->level = LEVEL_ADJACENT;
      else if (0 == level.compare(s_SECTION_SITES_NAME_LEVEL_UPPER))
        item->level = LEVEL_UPPER;
      else {
        LOG(ERROR) << fname << ": unsupported " << s_SECTION_SITES_NAME_LEVEL << " value: " << level;
        item->level = LEVEL_UNKNOWN;
      }

      item->auto_init = cycle_item[s_SECTION_SITES_NAME_AUTO_INIT].GetInt();
      item->auto_gencontrol = cycle_item[s_SECTION_SITES_NAME_AUTO_GENCONTROL].GetInt();

      m_sites[item->name] = item;

#if VERBOSE > 7
      LOG(INFO) << fname << ": " << item->name
                << " level=" << item->level
                << " nature=" << item->nature
                << " auto_init=" << item->auto_init
                << " auto_gencontrol=" << item->auto_gencontrol;
#endif
    }
  }
  else rc = NOK;

  LOG(INFO) << fname << ": FINISH";

  return rc;
}

// ==========================================================================================
// Найти по таблице запрос с заданным идентификатором.
// Есть такой - присвоить параметру его адрес и вернуть OK
// Нет такого - присвоить второму параметру NULL и вернуть NOK
int EgsaConfig::get_request_by_id(ech_t_ReqId _id, RequestEntry*& _entry)
{
  int rc = NOK;

  if ((EGA_GENCONTROL <= _id) && (_id < NOT_EXISTENT)) {
    rc = OK;
    _entry = &g_request_dictionary[_id];
  }
  else {
    _entry = NULL;
  }

  return rc;
}

// ==========================================================================================
// Найти по таблице запрос с заданным названием.
// Есть такой - присвоить параметру его адрес и вернуть OK
// Нет такого - присвоить второму параметру NULL и вернуть NOK
int EgsaConfig::get_request_by_name(const std::string& _name, RequestEntry*& _entry)
{
  for (int id = EGA_GENCONTROL; id < NOT_EXISTENT; id++) {
    if (id == g_request_dictionary[id].e_RequestId) {
      if (0 == _name.compare(g_request_dictionary[id].s_RequestName)) {
        _entry = &g_request_dictionary[id];
        return OK;
      }
    }
    else {
      // Нарушение структуры таблицы - идентификатор запроса не монотонно возрастающая последовательность
      assert(0 == 1);
    }
  }

  _entry = NULL;
  return NOK;
}

// ==========================================================================================
ech_t_ReqId EgsaConfig::get_request_id(const std::string& name)
{
  ech_t_ReqId result = NOT_EXISTENT;

  for (int id = EGA_GENCONTROL; id < NOT_EXISTENT; id++) {
    if (0 == name.compare(g_request_dictionary[id].s_RequestName)) {
      result = static_cast<ech_t_ReqId>(id);
      break;
    }
  }
  return result;
}

// ==========================================================================================
const char* EgsaConfig::get_request_name(ech_t_ReqId id)
{
  return g_request_dictionary[id].s_RequestName;
}

// ==========================================================================================
// Загрузка информации о запросах
// Обязательные поля в файле:
// NAME : string
// PRIORITY : integer
// OBJECT : string
// необязательные поля:
// MODE : string
// INCLUDED_REQUEST : array
int EgsaConfig::load_requests()
{
  const char* fname = "load_requests";
  RequestEntry *dict_entry = NULL, *request;
  int rc = OK;

  LOG(INFO) << fname << ": CALL";


  // Получение данных по конфигурации Запросов
  Value& section = m_egsa_config_document[s_SECTION_NAME_REQUESTS_NAME];
  if (section.IsArray())
  {
    for (Value::ValueIterator itr = section.Begin(); itr != section.End(); ++itr)
    {
      // Получили доступ к очередному элементу Циклов
      const Value::Object& request_item_json = itr->GetObject();

      // Проверить в НСИ, известен ли этот Запрос
      if (OK == get_request_by_name(request_item_json[s_SECTION_REQUESTS_NAME_NAME].GetString(), dict_entry))
      { // Да - собираем по нему информацию

        // в dict_entry заполнились поля из НСИ
        assert(dict_entry);

        request = new RequestEntry;
        // Скопируем значения по-умолчанию
        memcpy(request, dict_entry, sizeof(RequestEntry));

        strncpy(request->s_RequestName,
                request_item_json[s_SECTION_REQUESTS_NAME_NAME].GetString(),
                REQUESTNAME_MAX);

        request->i_RequestPriority = request_item_json[s_SECTION_REQUESTS_NAME_PRIORITY].GetInt();

        const std::string& s_object = request_item_json[s_SECTION_REQUESTS_NAME_OBJECT].GetString();
        if (0 == s_object.compare("I")) {
          request->e_RequestObject = INFO;
        }
        else if (0 == s_object.compare("A")) {
          request->e_RequestObject = ACQSYS;
        }
        else if (0 == s_object.compare("E")) {
          request->e_RequestObject = EQUIP;
        }
        else {
          LOG(WARNING) << "Unsupported value '" << s_object << "' as OBJECT";
          rc = NOK;
        }

        // Далее следуют необязательные поля
        if (request_item_json.HasMember(s_SECTION_REQUESTS_NAME_MODE)) {
          const std::string& s_mode = request_item_json[s_SECTION_REQUESTS_NAME_MODE].GetString();
          if (0 == s_mode.compare("DIFF")) {
            request->e_RequestMode = DIFF;
          }
          else if (0 == s_mode.compare("NONDIFF")) {
            request->e_RequestMode = NONDIFF;
          }
          else {
            request->e_RequestMode = NOT_SPECIFIED;
          }
          // LOG(INFO) << request->s_RequestName << ":" << (unsigned int)request->i_RequestPriority << ":" << s_object << ":" << s_mode;
        }
        /*else {
          LOG(INFO) << request->s_RequestName << ":" << (unsigned int)request->i_RequestPriority << ":" << s_object;
        }*/

        if (request_item_json.HasMember(s_SECTION_REQUESTS_NAME_INC_REQ)) {
          // Прочитать массив сайтов, поместить их в m_cycles.sites
          Value& req_list = request_item_json[s_SECTION_REQUESTS_NAME_INC_REQ];
          int sequence = 0;
          for (Value::ValueIterator itr = req_list.Begin(); itr != req_list.End(); ++itr)
          {
            const Value::Object& incl_req_item = itr->GetObject();
            const std::string incl_req_name = incl_req_item[s_SECTION_REQUESTS_NAME_INC_REQ_NAME].GetString();
            // Проверить имя вложенного Запроса
            if (OK == get_request_by_name(incl_req_name, dict_entry))
            {
#if VERBOSE > 7
              LOG(INFO) << fname << ": " << request->s_RequestName
                        << " priority=" << (unsigned int)request->i_RequestPriority
                        << " object=" << (unsigned int)request->e_RequestObject
                        << " mode=" << (unsigned int)request->e_RequestMode << " "
                        << incl_req_name; // << " sequence=" << (sequence+1);
#endif

              // Ненулевое значение говорит о порядке исполнения подзапросов
              request->r_IncludingRequests[dict_entry->e_RequestId] = ++sequence;
            } // end нашли запрос по его имени 
          } // конец цикла чтения вложенных запросов
        } // конец блока "этот запрос составной"
        m_requests.insert(std::pair<std::string, RequestEntry*>(request->s_RequestName, request));
      } // если название Запроса известно
      else
      {
        LOG(WARNING) << "Skip unknown request " << request_item_json[s_SECTION_REQUESTS_NAME_NAME].GetString();
      }
    } // новая запись
  } // если записи были

  return rc;
}

// ==========================================================================================
// Разбор конфигурационного файла со словарями ESG.
// Название конфигурационного файла содержится в egsa.json секции COMMON, запись ESG_FILE
int EgsaConfig::load_esg()
{
  int rc = OK;
  static const char* fname = "load_esg";
  FILE* f_params = NULL;
  struct stat configfile_info;

  LOG(INFO) << fname << ": CALL";

  // Выделить буфер readBuffer размером с читаемый файл
  if (-1 == stat(m_common.dict_esg_filename.c_str(), &configfile_info)) {
    LOG(ERROR) << fname << ": Unable to stat() '" << m_common.dict_esg_filename << "': " << strerror(errno);
  }
  else {
    if (NULL != (f_params = fopen(m_common.dict_esg_filename.c_str(), "r"))) {
      // Файл открылся успешно, прочитаем содержимое
      LOG(INFO) << fname << ": size of '" << m_common.dict_esg_filename << "' is " << configfile_info.st_size;
      char *readBuffer = new char[configfile_info.st_size + 1];

      FileReadStream is(f_params, readBuffer, configfile_info.st_size + 1);
      m_esg_config_document.ParseStream(is);
      delete []readBuffer;

      // На этапе разработки - аварийное завершение есть предпочтительный способ обозначения ошибок
      assert(m_esg_config_document.IsObject());

      if (!m_esg_config_document.HasParseError()) // Конфиг-файл имеет корректную структуру?
      { 
        rc = NOK;

        do {
          // Получение данных по конфигурации
          Value& elemtypes = m_esg_config_document["DED_ELEMTYPES"];
          rc = load_DED_ELEMTYPES(elemtypes);
          if (OK != rc) {
            LOG(ERROR) << fname << ": unable to load DED_ELEMTYPES";
            break;
          }

          Value& elemstructs = m_esg_config_document["DCD_ELEMSTRUCTS"];
          rc = load_DCD_ELEMSTRUCTS(elemstructs);
          if (OK != rc) {
            LOG(ERROR) << fname << ": unable to load DCD_ELEMSTRUCTS";
            break;
          }

          rc = OK;
        } while (false);
      }
      else {
        LOG(ERROR) << fname << ": Parsing " << m_common.dict_esg_filename;
      }

      fclose (f_params);
    } // Конец успешного чтения содержимого файла
    else {
      LOG(FATAL) << fname << ": Locating config file " << m_common.dict_esg_filename
                 << " (" << strerror(errno) << ")";
    }
  }


  LOG(INFO) << fname << ": get ESG configurations";
  return rc;
}

// ==========================================================================================
// Вернуть идентификатор типа записи ESG_LOCSTRUCT
field_type_t EgsaConfig::get_db_attr_type_id(const std::string& type_name)
{
  field_type_t type;

  if (0 == type_name.compare("LOGIC"))
    type = FIELD_TYPE_LOGIC;    // 1
  else if (0 == type_name.compare("INT8"))
    type = FIELD_TYPE_INT8;     // 2
  else if (0 == type_name.compare("UINT8"))
    type = FIELD_TYPE_UINT8;    // 3
  else if (0 == type_name.compare("INT16"))
    type = FIELD_TYPE_INT16;    // 4
  else if (0 == type_name.compare("UINT16"))
    type = FIELD_TYPE_UINT16;   // 5
  else if (0 == type_name.compare("INT32"))
    type = FIELD_TYPE_INT32;    // 6
  else if (0 == type_name.compare("UINT32"))
    type = FIELD_TYPE_UINT32;   // 7
  else if (0 == type_name.compare("FLOAT"))
    type = FIELD_TYPE_FLOAT;    // 8
  else if (0 == type_name.compare("DOUBLE"))
    type = FIELD_TYPE_DOUBLE;   // 9
  else if (0 == type_name.compare("DATE"))
    type = FIELD_TYPE_DATE;     // 10
  else if (0 == type_name.compare("STRING"))
    type = FIELD_TYPE_STRING;   // 11
  else type = FIELD_TYPE_UNKNOWN;

//  LOG(INFO) << "IN:" << type_name << " OUT:" << type;
  return type;
}

// ==========================================================================================
int EgsaConfig::load_DCD_ELEMSTRUCTS(rapidjson::Value& dcd_elemstructs)
{
  static const char* fname = "load_DCD_ELEMSTRUCTS";
  int rc = OK;
  size_t idx = 0, attr_idx;

  if (dcd_elemstructs.IsArray())
  {
    LOG(INFO) << fname << ": Initial size=" << dcd_elemstructs.Size();

    m_elemstruct_items = new elemstruct_item_t[dcd_elemstructs.Size() + 1];
    memset(m_elemstruct_items, '\0', sizeof(elemstruct_item_t) * (dcd_elemstructs.Size() + 1));

    for (Value::ValueIterator itr = dcd_elemstructs.Begin(); itr != dcd_elemstructs.End(); ++itr) {
      // Получили доступ к очередному элементу элементарных типов
      const Value::Object& dcd_elemstructs_json = itr->GetObject();

      const std::string name = dcd_elemstructs_json["NAME"].GetString();
      strncpy(m_elemstruct_items[idx].name, name.c_str(), MAX_ICD_NAME_LENGTH);

      // Не для всех типов есть ASSOCIATE
      if (dcd_elemstructs_json.HasMember("ASSOCIATE")) {
        const std::string associate = dcd_elemstructs_json["ASSOCIATE"].GetString();
        strncpy(m_elemstruct_items[idx].associate, associate.c_str(), MAX_ICD_NAME_LENGTH);
      }

      // Первый символ ассоциации - тип данных
      if (m_elemstruct_items[idx].associate[0]) {
        switch (m_elemstruct_items[idx].associate[0]) {
          case 'I': m_elemstruct_items[idx].tm_class = TM_CLASS_INFORMATION;   break;
          case 'A': m_elemstruct_items[idx].tm_class = TM_CLASS_ALARM;         break;
          case 'S': m_elemstruct_items[idx].tm_class = TM_CLASS_STATE;         break;
          case 'O': m_elemstruct_items[idx].tm_class = TM_CLASS_ORDER;         break;
          case 'P': m_elemstruct_items[idx].tm_class = TM_CLASS_HISTORIZATION; break;
          case 'H': m_elemstruct_items[idx].tm_class = TM_CLASS_HISTORIC;      break;
          case 'T': m_elemstruct_items[idx].tm_class = TM_CLASS_THRESHOLD;     break;
          case 'R': m_elemstruct_items[idx].tm_class = TM_CLASS_REQUEST;       break;
          case 'G': m_elemstruct_items[idx].tm_class = TM_CLASS_CHANGEHOUR;    break;
          case 'D': m_elemstruct_items[idx].tm_class = TM_CLASS_INCIDENT;      break;
          default:  m_elemstruct_items[idx].tm_class = TM_CLASS_UNKNOWN;       break;
        }
      }

      if (dcd_elemstructs_json.HasMember("FIELDS")) { // Есть массив полей?
        attr_idx = 0;

        Value& field_list = dcd_elemstructs_json["FIELDS"];

        if (field_list.IsArray()) {

          // Обнулили счётчик полей, имеющих заданный тип в БДРВ - то есть он являющихся атрибутами БДРВ
          m_elemstruct_items[idx].num_attributes = 0;

          for (Value::ValueIterator itr = field_list.Begin(); itr != field_list.End(); ++itr)
          {
            const Value::Object& field_item = itr->GetObject();
            // поле FIELD обязательно
            const std::string field_str = field_item["FIELD"].GetString();
            strncpy(m_elemstruct_items[idx].fields[attr_idx].field, field_str.c_str(), MAX_ICD_NAME_LENGTH);

            // поле TYPE необязательно, но если оно есть, то могут быть еще поля ATTR, TYPE, LENGTH
            if (field_item.HasMember("TYPE")) {
              const std::string attr_type = field_item["TYPE"].GetString();

              // Если это поле - атрибут БДРВ, может быть задан тег ATTR
              if (field_item.HasMember("ATTR")) {
                const std::string attr_str = field_item["ATTR"].GetString();
                strncpy(m_elemstruct_items[idx].fields[attr_idx].attribute, attr_str.c_str(), TAG_NAME_MAXLEN);
              }

              if (FIELD_TYPE_UNKNOWN != (m_elemstruct_items[idx].fields[attr_idx].type = get_db_attr_type_id(attr_type))) {

                if (FIELD_TYPE_STRING == m_elemstruct_items[idx].fields[attr_idx].type) {

                  assert(field_item.HasMember("LENGTH"));

                  // Для строковых атрибутов должна быть задана их длина
                  if (field_item.HasMember("LENGTH")) {
                    m_elemstruct_items[idx].fields[attr_idx].length = field_item["LENGTH"].GetInt();
                  }
                  else {
                    LOG(ERROR) << fname << ": string field " << field_str << " missing his length";
                    m_elemstruct_items[idx].fields[attr_idx].length = 0;
                    rc = NOK;
                  }
                }

                m_elemstruct_items[idx].num_attributes++;
              }
              
            }

            attr_idx++;
            if (MAX_LOCSTRUCT_FIELDS_NUM < attr_idx) {
              LOG(ERROR) << fname << ": fields num for " << m_elemstruct_items[idx].name << " exceed limits";
              break;
            }
          } // конец блока "по всем атрибутам"

          m_elemstruct_items[idx].num_fileds = attr_idx;
        } // конец блока if "есть массив атрибутов"

      } // конец блока "по всем атрибутам"

#if VERBOSE>7
      LOG(INFO) << fname << ": NAME=" << m_elemstruct_items[idx].name << ", ASSOCIATE=" << m_elemstruct_items[idx].associate
                << ", type=" << m_elemstruct_items[idx].tm_class << ", num_fields=" << m_elemstruct_items[idx].num_fileds
                << ", num_attrs=" << m_elemstruct_items[idx].num_attributes;
#endif

      idx++;
    } // конец блока "есть сведения об атрибутаx"

  }  // конец блока "if DCD_ELEMSTRUCT содержит данные"
  else {
    LOG(ERROR) << fname << ": not an array";
    rc = NOK;
  }

  LOG(INFO) << fname << ": Load " << idx << " records";
  return rc;
}

#if 0
// ==========================================================================================
int EgsaConfig::load_ESG_LOCSTRUCTS(rapidjson::Value& esg_locstructs)
{
  static const char* fname = "load_ESG_LOCSTRUCTS";
  int rc = OK;
  size_t idx = 0, attr_idx = 0;

  if (esg_locstructs.IsArray())
  {
    LOG(INFO) << fname << ": Initial size=" << esg_locstructs.Size();

    // Последняя запись будет пустой, как индикатор конца данных
    m_locstruct_items = new locstruct_item_t[esg_locstructs.Size()+1];
    memset(m_locstruct_items, '\0', sizeof(locstruct_item_t) * (esg_locstructs.Size()+1));

    for (Value::ValueIterator itr = esg_locstructs.Begin(); itr != esg_locstructs.End(); ++itr)
    {
      // Получили доступ к очередному элементу элементарных типов
      const Value::Object& locstruct_item_json = itr->GetObject();
      const std::string name = locstruct_item_json["NAME"].GetString();
      const std::string icd = locstruct_item_json["ICD"].GetString();
      // const std::string comment = locstruct_item_json["COMMENT"].GetString();

      strncpy(m_locstruct_items[idx].name, name.c_str(), MAX_ICD_NAME_LENGTH);
      strncpy(m_locstruct_items[idx].icd, icd.c_str(), MAX_ICD_NAME_LENGTH);
      m_locstruct_items[idx].tm_class = TM_CLASS_UNKNOWN;

      if (m_locstruct_items[idx].name[0]) {
        switch (m_locstruct_items[idx].name[0]) {
          case 'I': m_locstruct_items[idx].tm_class = TM_CLASS_INFORMATION;   break;
          case 'A': m_locstruct_items[idx].tm_class = TM_CLASS_ALARM;         break;
          case 'S': m_locstruct_items[idx].tm_class = TM_CLASS_STATE;         break;
          case 'O': m_locstruct_items[idx].tm_class = TM_CLASS_ORDER;         break;
          case 'P': m_locstruct_items[idx].tm_class = TM_CLASS_HISTORIZATION; break;
          case 'H': m_locstruct_items[idx].tm_class = TM_CLASS_HISTORIC;      break;
          case 'T': m_locstruct_items[idx].tm_class = TM_CLASS_THRESHOLD;     break;
          case 'R': m_locstruct_items[idx].tm_class = TM_CLASS_REQUEST;       break;
          case 'G': m_locstruct_items[idx].tm_class = TM_CLASS_CHANGEHOUR;    break;
          case 'D': m_locstruct_items[idx].tm_class = TM_CLASS_INCIDENT;      break;
          default:  m_locstruct_items[idx].tm_class = TM_CLASS_UNKNOWN;       break;
        }

        if (locstruct_item_json.HasMember("FIELDS")) { // Есть массив полей?
          attr_idx = 0;

          Value& field_list = locstruct_item_json["FIELDS"];

          if (field_list.IsArray()) {

            for (Value::ValueIterator itr = field_list.Begin(); itr != field_list.End(); ++itr)
            {
              const Value::Object& field_item = itr->GetObject();

              if (field_item.HasMember("ATTR")) {
                const std::string attr_str = field_item["ATTR"].GetString();
                strncpy(m_locstruct_items[idx].fields[attr_idx].name, attr_str.c_str(), MAX_ICD_NAME_LENGTH);
              }

              if (field_item.HasMember("TYPE")) {
                const std::string type_str = field_item["TYPE"].GetString();
                m_locstruct_items[idx].fields[attr_idx].type = get_db_attr_type_id(type_str);

                if (m_locstruct_items[idx].fields[attr_idx].type == FIELD_TYPE_STRING) {
                  if (field_item.HasMember("LENGTH"))  {
                    m_locstruct_items[idx].fields[attr_idx].length = field_item["LENGTH"].GetInt();
                  }
                  else {
                    LOG(ERROR) << fname << ": string field " << name << " missing his length";
                    m_locstruct_items[idx].fields[attr_idx].length = 0;
                    rc = NOK;
                  }
                }

              }

              attr_idx++;
              if (MAX_LOCSTRUCT_FIELDS_NUM < attr_idx) {
                LOG(ERROR) << fname << ": fields num for " << m_locstruct_items[idx].name << " exceed limits";
                break;
              }

            } // конец цикла чтения вложенных полей

            m_locstruct_items[idx].num_fileds = attr_idx;

          } // конец проверки того, что FIELDS содержит массив значений

          idx++;

#if VERBOSE >= 5
          LOG(INFO) << fname << ": NAME=" << m_locstruct_items[idx].name << ", ICD=" << m_locstruct_items[idx].icd
                    << ", type=" << m_locstruct_items[idx].tm_class << ", num=" << m_locstruct_items[idx].num_fileds;
#endif
#if VERBOSE >= 7
          for (size_t i=0; i<m_locstruct_items[idx].num_fileds; i++) {
            std::cout << locstruct_idx << " " << m_locstruct_items[idx].name << " " << i+1 << "/" << m_locstruct_items[idx].num_fileds
                      << " " << m_locstruct_items[idx].fields[i].name << " " << m_locstruct_items[idx].fields[i].type
                      << " " << m_locstruct_items[idx].fields[i].length << std::endl; 
          }
#endif

        }
      } // end if name not empty
    } // end for every ESG_LOCSTRUCT entry
  } // end if ESG_LOCSTRUCT is an array
  else {
    LOG(ERROR) << fname << ": not an array";
    rc = NOK;
  }

  LOG(INFO) << fname << ": Load " << idx << " records";

  return rc;
}
#endif

// ==========================================================================================
int EgsaConfig::load_DED_ELEMTYPES(rapidjson::Value& ded_elemtypes)
{
  static const char* fname = "load_DED_ELEMTYPES";
  size_t idx = 0;
  int rc = OK;

  if (ded_elemtypes.IsArray())
  {
    LOG(INFO) << fname << ": Initial size=" << ded_elemtypes.Size();
    m_elemtype_items = new elemtype_item_t[ded_elemtypes.Size() + 1];
    memset(m_elemtype_items, '\0', sizeof(elemtype_item_t) * (ded_elemtypes.Size()+1));

    for (Value::ValueIterator itr = ded_elemtypes.Begin(); itr != ded_elemtypes.End(); ++itr)
    {
      // Получили доступ к очередному элементу элементарных типов
      const Value::Object& elemtype_item_json = itr->GetObject();
      const std::string elemtype_name = elemtype_item_json["NAME"].GetString();
      const std::string elemtype_type = elemtype_item_json["TYPE"].GetString();
      const std::string elemtype_size = elemtype_item_json["SIZE"].GetString();
      // const std::string elemtype_name = elemtype_item_json["COMMENT"].GetString();

      strncpy(m_elemtype_items[idx].name, elemtype_name.c_str(), MAX_ICD_NAME_LENGTH);
      strncpy(m_elemtype_items[idx].format_size, elemtype_size.c_str(), MAX_ICD_NAME_LENGTH);

      switch (elemtype_type.c_str()[0]) {
        case 'I': m_elemtype_items[idx].tm_type = TM_TYPE_INTEGER; break;
        case 'T': m_elemtype_items[idx].tm_type = TM_TYPE_TIME;    break;
        case 'S': m_elemtype_items[idx].tm_type = TM_TYPE_STRING;  break;
        case 'R': m_elemtype_items[idx].tm_type = TM_TYPE_REAL;    break;
        case 'L': m_elemtype_items[idx].tm_type = TM_TYPE_LOGIC;   break;
        default:  m_elemtype_items[idx].tm_type = TM_TYPE_UNKNOWN; break;
      }
  
#if VERBOSE > 7
      LOG(INFO) << fname << ": name=" << m_elemtype_items[idx].name << " type=" << m_elemtype_items[idx].tm_type
                << " format_size=" << m_elemtype_items[idx].format_size;
#endif
      idx++;
    }
  }
  else {
    LOG(ERROR) << fname << ": not an array";
    rc = NOK;
  }

  LOG(INFO) << fname << ": Load " << idx << " records";

  return rc;
}

// ==========================================================================================
// загрузка всех параметров обмена: разделы COMMON, SITES, CYCLES, REQUESTS
int EgsaConfig::load()
{
  static const char* fname = "load";
  int status = NOK;

  LOG(INFO) << fname << ": CALL";

  do {

   // загрузка раздела COMMON
    status = load_common();
    if (NOK == status) {
      LOG(ERROR) << fname << ": Get common configuration";
      break;
    }

    // загрузка Сайтов
    status = load_sites();
    if (NOK == status) {
      LOG(ERROR) << fname << ": Get cycles";
      break;
    }

    // загрузка параметров Циклов
    status = load_cycles();
    if (NOK == status) {
      LOG(ERROR) << fname << ": Get cycles";
      break;
    }

    // загрузка параметров Запросов
    status = load_requests();
    if (NOK == status) {
       LOG(ERROR) << fname << ": Get requests";
       break;
    }

    status = load_esg();
    if (NOK == status) {
       LOG(ERROR) << fname << ": Get ESG config";
       break;
    }

    status = OK;

  } while(false);

  LOG(INFO) << fname << ": FINISH";

  return status;
}

// ==========================================================================================

