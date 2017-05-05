///////////////////////////////////////////////////////////////////////////////////////////
// Класс представления конифгурации модуля EGSA
// 2016/04/14
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
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
// Название ключа "имя файла SMAD EGSA"
const char* EgsaConfig::s_SECTION_COMMON_NAME_SMAD_FILE  = "SMAD_FILE";
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
ega_ega_odm_t_RequestEntry EgsaConfig::g_request_dictionary[] = {
// Идентификатор запроса
// |                    Название запроса
// |                    |                         Приоритет
// |                    |                         |   Тип объекта - информация, СС, оборудование
// |                    |                         |   |       Режим сбора - дифференциальный или нет
// |                    |                         |   |       |              Признак отношения запроса:
// |                    |                         |   |       |              к технологическим данным (1)
// |                    |                         |   |       |              к состоянию системы сбора (0)
// |                    |                         |   |       |              |      Вложенные запросы
// |                    |                         |   |       |              |      (Ненулевое значение говорит о количестве
// |                    |                         |   |       |              |      подзапросов подобного типа)
// |                    |                         |   |       |              |      |
  {ECH_D_GENCONTROL,    EGA_EGA_D_STRGENCONTROL,  80, ACQSYS, NONDIFF,       true,  {} },   // 0
  {ECH_D_INFOSACQ,      EGA_EGA_D_STRINFOSACQ,    80, ACQSYS, DIFF,          true,  {} },   // 1
  {ECH_D_URGINFOS,      EGA_EGA_D_STRURGINFOS,    80, ACQSYS, DIFF,          true,  {} },   // 2
  {ECH_D_GAZPROCOP,     EGA_EGA_D_STRGAZPROCOP,   80, ACQSYS, NONDIFF,       false, {} },   // 3 - не используется, игнорировать
  {ECH_D_EQUIPACQ,      EGA_EGA_D_STREQUIPACQ,    80, EQUIP,  NONDIFF,       true,  {} },   // 4
  {ECH_D_ACQSYSACQ,     EGA_EGA_D_STRACQSYSACQ,   80, ACQSYS, NONDIFF,       false, {} },   // 5
  {ECH_D_ALATHRES,      EGA_EGA_D_STRALATHRES,    80, ACQSYS, DIFF,          true,  {} },   // 6
  {ECH_D_TELECMD,       EGA_EGA_D_STRTELECMD,    101, INFO,   NOT_SPECIFIED, true,  {} },   // 7
  {ECH_D_TELEREGU,      EGA_EGA_D_STRTELEREGU,   101, INFO,   NOT_SPECIFIED, true,  {} },   // 8
  {ECH_D_SERVCMD,       EGA_EGA_D_STRSERVCMD,     80, ACQSYS, NOT_SPECIFIED, false, {} },   // 9
  {ECH_D_GLOBDWLOAD,    EGA_EGA_D_STRGLOBDWLOAD,  80, ACQSYS, NOT_SPECIFIED, false, {} },   // 10
  {ECH_D_PARTDWLOAD,    EGA_EGA_D_STRPARTDWLOAD,  80, ACQSYS, NOT_SPECIFIED, false, {} },   // 11
  {ECH_D_GLOBUPLOAD,    EGA_EGA_D_STRGLOBUPLOAD,  80, ACQSYS, NOT_SPECIFIED, false, {} },   // 12
  {ECH_D_INITCMD,       EGA_EGA_D_STRINITCMD,     82, ACQSYS, NOT_SPECIFIED, false, {} },   // 13
  {ECH_D_GCPRIMARY,     EGA_EGA_D_STRGCPRIMARY,   80, ACQSYS, NONDIFF,       true,  {} },   // 14
  {ECH_D_GCSECOND,      EGA_EGA_D_STRGCSECOND,    80, ACQSYS, NONDIFF,       true,  {} },   // 15
  {ECH_D_GCTERTIARY,    EGA_EGA_D_STRGCTERTIARY,  80, ACQSYS, NONDIFF,       true,  {} },   // 16
  {ECH_D_DIFFPRIMARY,   EGA_EGA_D_STRIAPRIMARY,   80, ACQSYS, DIFF,          true,  {} },   // 17
  {ECH_D_DIFFSECOND,    EGA_EGA_D_STRIASECOND,    80, ACQSYS, DIFF,          true,  {} },   // 18
  {ECH_D_DIFFTERTIARY,  EGA_EGA_D_STRIATERTIARY,  80, ACQSYS, DIFF,          true,  {} },   // 19
  {ECH_D_INFODIFFUSION, EGA_EGA_D_STRINFOSDIFF,   80, ACQSYS, NONDIFF,       true,  {} },   // 20
  {ECH_D_DELEGATION,    EGA_EGA_D_STRDELEGATION,  80, ACQSYS, NOT_SPECIFIED, true,  {} }    // 21
};

//
// ==========================================================================================
EgsaConfig::EgsaConfig(const char* config_filename)
 : m_config_filename(NULL),
   m_config_has_good_format(false)
{
  const char* fname = "EgsaConfig";
  FILE* f_params = NULL;
  struct stat configfile_info;

  LOG(INFO) << "Constructor EgsaConfig";
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
      m_document.ParseStream(is);
      delete []readBuffer;

      assert(m_document.IsObject());

      if (!m_document.HasParseError())
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
  int idx = 0;

  LOG(INFO) << "Destructor EgsaConfig";
  free(m_config_filename); // NB: free() after strdup()

  for (egsa_config_cycles_t::iterator it = m_cycles.begin();
       it != m_cycles.end();
       ++it)
  {
    LOG(INFO) << "Free cycle " << ++idx << " " << (*it).first;
    delete (*it).second;
  }
  m_cycles.clear();

  idx = 0;
  for (egsa_config_sites_t::iterator it = m_sites.begin();
       it != m_sites.end();
       ++it)
  {
    LOG(INFO) << "Free site " << ++idx << " " << (*it).first;
    delete (*it).second;
  }
  m_sites.clear();

  idx = 0;
  for (egsa_config_requests_t::iterator it = m_requests.begin();
       it != m_requests.end();
       ++it)
  {
    LOG(INFO) << "Free request " << ++idx << " " << (*it).first;
    delete (*it).second;
  }
  m_requests.clear();
}

// ==========================================================================================
const std::string& EgsaConfig::smad_name()
{
  return m_common.smad_filename;
}

// ==========================================================================================
int EgsaConfig::load_common()
{
  int rc = NOK;
  const char* fname = "load_common";

  if (true == (m_document.HasMember(s_SECTION_NAME_COMMON_NAME))) {
    // Получение общих данных по конфигурации интерфейсного модуля RTDBUS Системы Сбора
    Value& section = m_document[s_SECTION_NAME_COMMON_NAME];
    if (section.IsObject()) {
      m_common.smad_filename = section[s_SECTION_COMMON_NAME_SMAD_FILE].GetString();
      rc = OK;
      LOG(INFO) << fname << ": Use egsa SMAD file: " << m_common.smad_filename;
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
  const char* fname = "load_cycles";
  egsa_config_cycle_info_t *cycle_info = NULL;
  cycle_id_t cycle_id = ID_CYCLE_UNKNOWN;
  int rc = OK;

  // Получение данных по конфигурации Циклов
  Value& section = m_document[s_SECTION_NAME_CYCLES_NAME];
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
        int idx = 0;
        for (Value::ValueIterator itr = site_list.Begin(); itr != site_list.End(); ++itr)
        {
          const Value::Object& site_item = itr->GetObject();
          const std::string site_name = site_item[s_SECTION_CYCLES_NAME_SITE_NAME].GetString();
          LOG(INFO) << fname << ": " << cycle_info->name
                    << " period=" << cycle_info->period
                    << " req_name=" << cycle_info->request_name
                    << " [" << (++idx) << "] " << site_name;
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
  Value& section = m_document[s_SECTION_NAME_SITES_NAME];
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

      LOG(INFO) << fname << ": " << item->name
                << " level=" << item->level
                << " nature=" << item->nature
                << " auto_init=" << item->auto_init
                << " auto_gencontrol=" << item->auto_gencontrol;
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
int EgsaConfig::get_request_by_id(ech_t_ReqId _id, ega_ega_odm_t_RequestEntry*& _entry)
{
  int rc = NOK;

  if ((ECH_D_GENCONTROL <= _id) && (_id <= ECH_D_DELEGATION)) {
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
int EgsaConfig::get_request_by_name(const std::string& _name, ega_ega_odm_t_RequestEntry*& _entry)
{
  for (int id = ECH_D_GENCONTROL; id <= ECH_D_DELEGATION; id++) {
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
  ech_t_ReqId result = ECH_D_NOT_EXISTENT;

  for (int id = ECH_D_GENCONTROL; id <= ECH_D_DELEGATION; id++) {
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
  ega_ega_odm_t_RequestEntry *dict_entry = NULL, *request;
  int rc = OK;

  LOG(INFO) << fname << ": CALL";


  // Получение данных по конфигурации Запросов
  Value& section = m_document[s_SECTION_NAME_REQUESTS_NAME];
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

        request = new ega_ega_odm_t_RequestEntry;
        // Скопируем значения по-умолчанию
        memcpy(request, dict_entry, sizeof(ega_ega_odm_t_RequestEntry));

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
          int NumIncludedRequests = 0;
          for (Value::ValueIterator itr = req_list.Begin(); itr != req_list.End(); ++itr)
          {
            const Value::Object& incl_req_item = itr->GetObject();
            const std::string incl_req_name = incl_req_item[s_SECTION_REQUESTS_NAME_INC_REQ_NAME].GetString();
            // Проверить имя вложенного Запроса
            if (OK == get_request_by_name(incl_req_name, dict_entry))
            {
              LOG(INFO) << fname << ": " << request->s_RequestName
                        << " priority=" << (unsigned int)request->i_RequestPriority
                        << " object=" << (unsigned int)request->e_RequestObject
                        << " mode=" << (unsigned int)request->e_RequestMode
                        << " incl_req_names "
                        << " [" << NumIncludedRequests + 1 << "] " << incl_req_name;

              // Ненулевое значение говорит о количестве подзапросов подобного типа
              request->r_IncludingRequests[dict_entry->e_RequestId]++;
            }
          }
        }
        m_requests.insert(std::pair<std::string, ega_ega_odm_t_RequestEntry*>(request->s_RequestName, request));
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
// загрузка всех параметров обмена: разделы COMMON, SITES, CYCLES, REQUESTS
int EgsaConfig::load()
{
  const char* fname = "load";
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
    status = OK;

  } while(false);

  LOG(INFO) << fname << ": FINISH";

  return status;
}

// ==========================================================================================

