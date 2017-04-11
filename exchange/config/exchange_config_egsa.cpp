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
using namespace std;

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
    LOG(INFO) << "Free cycle " << ++idx;
    delete (*it).second;
  }

  idx = 0;
  for (egsa_config_sites_t::iterator it = m_sites.begin();
       it != m_sites.end();
       ++it)
  {
    LOG(INFO) << "Free site " << ++idx;
    delete (*it).second;
  }

  idx = 0;
  for (egsa_config_requests_t::iterator it = m_requests.begin();
       it != m_requests.end();
       ++it)
  {
    LOG(INFO) << "Free request " << ++idx;
    delete (*it).second;
  }
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
int EgsaConfig::load_cycles()
{
  const char* fname = "load_cycles";
  egsa_config_cycle_info_t *item;
  int rc = OK;

  // Получение данных по конфигурации Циклов
  Value& section = m_document[s_SECTION_NAME_CYCLES_NAME];
  if (section.IsArray()) {
    for (Value::ValueIterator itr = section.Begin(); itr != section.End(); ++itr)
    {
      // Получили доступ к очередному элементу Циклов
      const Value::Object& cycle_item = itr->GetObject();

      item = new egsa_config_cycle_info_t;

      item->name = cycle_item[s_SECTION_CYCLES_NAME_NAME].GetString();
      item->period = cycle_item[s_SECTION_CYCLES_NAME_PERIOD].GetInt();
      item->request_name = cycle_item[s_SECTION_CYCLES_NAME_REQUEST].GetString();

      // Прочитать массив сайтов, поместить их в m_cycles.sites
      Value& site_list = cycle_item[s_SECTION_CYCLES_NAME_SITE];
      int idx = 0;
      for (Value::ValueIterator itr = site_list.Begin(); itr != site_list.End(); ++itr)
      {
        const Value::Object& site_item = itr->GetObject();
        const std::string site_name = site_item[s_SECTION_CYCLES_NAME_SITE_NAME].GetString();
        LOG(INFO) << fname << ": " << item->name
                  << " period=" << item->period
                  << " req_name=" << item->request_name
                  << " [" << (++idx) << "] " << site_name;
        item->sites.push_back(site_name);
      }

      m_cycles.insert(std::pair<std::string, egsa_config_cycle_info_t*>(item->name, item));
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
  egsa_config_request_info_t *item;
  int rc = NOK;

  LOG(INFO) << fname << ": CALL";


  // Получение данных по конфигурации Запросов
  Value& section = m_document[s_SECTION_NAME_REQUESTS_NAME];
  if (section.IsArray())
  {
    for (Value::ValueIterator itr = section.Begin(); itr != section.End(); ++itr)
    {
      // Получили доступ к очередному элементу Циклов
      const Value::Object& request_item = itr->GetObject();

      item = new egsa_config_request_info_t;
      // Значения по умолчанию
      item->priority = 127;
      item->object = 'I';
      item->mode = 'N';

      item->name = request_item[s_SECTION_REQUESTS_NAME_NAME].GetString();
      item->priority = request_item[s_SECTION_REQUESTS_NAME_PRIORITY].GetInt();
      const std::string& s_object = request_item[s_SECTION_REQUESTS_NAME_OBJECT].GetString();
      if (0 == s_object.compare("I")) {
        item->object = 'I';
      }
      else if (0 == s_object.compare("A")) {
        item->object = 'A';
      }
      else {
        LOG(WARNING) << "Unsupported value '" << s_object << "' as OBJECT";
      }

      // Далее следуют необязательные поля
      if (request_item.HasMember(s_SECTION_REQUESTS_NAME_MODE)) {
        const std::string& s_mode = request_item[s_SECTION_REQUESTS_NAME_MODE].GetString();
        if (0 == s_mode.compare("DIFF")) {
          item->mode = 'D';
        }
        else if (0 == s_mode.compare("NONDIFF")) {
          item->mode = 'N';
        }
        else {
          LOG(WARNING) << "Unsupported value '" << s_mode << "' as MODE";
        }
        LOG(INFO) << item->name << ":" << (unsigned int)item->priority << ":" << s_object << ":" << s_mode;
      }
      else {
        LOG(INFO) << item->name << ":" << (unsigned int)item->priority << ":" << s_object;
      }

      if (request_item.HasMember(s_SECTION_REQUESTS_NAME_INC_REQ)) {
        // Прочитать массив сайтов, поместить их в m_cycles.sites
        Value& req_list = request_item[s_SECTION_REQUESTS_NAME_INC_REQ];
        int idx = 0;
        for (Value::ValueIterator itr = req_list.Begin(); itr != req_list.End(); ++itr)
        {
          const Value::Object& incl_req_item = itr->GetObject();
          const std::string incl_req_name = incl_req_item[s_SECTION_REQUESTS_NAME_INC_REQ_NAME].GetString();
          LOG(INFO) << fname << ": " << item->name
                    << " priority=" << (unsigned int)item->priority
                    << " object=" << (unsigned int)item->object
                    << " mode=" << (unsigned int)item->mode
                    << " incl_req_names "
                    << " [" << (++idx) << "] " << incl_req_name;
          item->included_requests.push_back(incl_req_name);
        }
      }
      m_requests.insert(std::pair<std::string, egsa_config_request_info_t*>(item->name, item));
    }
  }

  return rc;
}

// ==========================================================================================
// загрузка всех параметров обмена: разделы COMMON, SITES, CYCLES
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

