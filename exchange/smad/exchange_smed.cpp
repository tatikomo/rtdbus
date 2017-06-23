///////////////////////////////////////////////////////////////////////////////////////////
// Класс представления модуля EGSA
// 2016/04/14
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "sqlite3.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include "xdb_common.hpp"       // Инфотипы GOF_D_BDR_OBJCLASS_*
#include "exchange_config.hpp"
#include "exchange_config_egsa.hpp"
#include "exchange_smed.hpp"

// ==========================================================================================================
SMED::SMED(EgsaConfig* _config, const char* snap_file)
 : m_db(NULL),
   m_state(STATE_DISCONNECTED),
   m_db_err(NULL),
   m_egsa_config(_config),
   m_snapshot_filename(NULL)
{
  m_snapshot_filename = strdup(snap_file);
}

// ==========================================================================================================
SMED::~SMED()
{
  int rc;

  if (SQLITE_OK != (rc = sqlite3_close(m_db)))
    LOG(ERROR) << "Closing SMED '" << m_snapshot_filename << "': " << rc;
  else
    LOG(INFO) << "SMED file '" << m_snapshot_filename << "' successfuly closed";

  free(m_snapshot_filename);
}

// ==========================================================================================================
smad_connection_state_t SMED::connect()
{
  const char *fname = "connect";
  //  char sql_operator[1000 + 1];
  //  int printed;
  int rc = 0;

  // ------------------------------------------------------------------
  // Открыть файл SMED с базой данных SQLite
  rc = sqlite3_open_v2(m_snapshot_filename,
                       &m_db,
                       // Если таблица только что создана, проверить наличие
                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                       "unix-dotfile");
  if (rc == SQLITE_OK)
  {
    LOG(INFO) << fname << ": SMED file '" << m_snapshot_filename << "' successfuly opened";
    m_state = STATE_OK;

    // Установить кодировку UTF8
    if (sqlite3_exec(m_db, "PRAGMA ENCODING = \"UTF-8\"", 0, 0, &m_db_err)) {
      LOG(ERROR) << fname << ": Unable to set UTF-8 support: " << m_db_err;
      sqlite3_free(m_db_err);
    }
    else LOG(INFO) << "Enable UTF-8 support for HDB";

    // Включить поддержку внешних ключей
    if (sqlite3_exec(m_db, "PRAGMA FOREIGN_KEYS = 1", 0, 0, &m_db_err)) {
      LOG(ERROR) << fname << ": Unable to set FOREIGN_KEYS support: " << m_db_err;
      sqlite3_free(m_db_err);
    }
    else LOG(INFO) << "Enable FOREIGN_KEYS support";

    // Создать структуру, прочитав DDL из файла
    rc = create_internal();
  }
  else
  {
    LOG(ERROR) << fname << ": opening SMED file '" << m_snapshot_filename << "' error=" << rc;
    m_state = STATE_ERROR;
  }

  return m_state;
}

// ==========================================================================================================
// Первоначальное создание таблиц и индексов SMED
// Загрузить нужный файл с DDL и выполнить его. Файл предварительно полностью очищает содержимое БД
// Руководство по оптимизации вставок в SQLite: https://stackoverflow.com/questions/1711631/improve-insert-per-second-performance-of-sqlite
int SMED::create_internal()
{
  static const char* fname = "create_internal";
  int rc = NOK;
  const char* ddl_commands =
    "BEGIN EXCLUSIVE;"
    "  PRAGMA foreign_keys = 1;"
    "  DROP TABLE IF EXISTS ASSOCIATE_LINK;"
    "  DROP TABLE IF EXISTS PROCESSING;"
    "  DROP TABLE IF EXISTS DATA;"
    "  DROP TABLE IF EXISTS FIELDS;"
    "  DROP TABLE IF EXISTS ELEMSTRUCT;"
    "  DROP TABLE IF EXISTS ELEMTYPE;"
    "  DROP TABLE IF EXISTS SITES;"
    "COMMIT;"
    "BEGIN EXCLUSIVE;"
    "CREATE TABLE DATA ("
    "  DATA_ID integer PRIMARY KEY AUTOINCREMENT,"
    "  TAG varchar,"
    "  LED integer,"
    "  OBJCLASS integer DEFAULT 127,"
    "  CATEGORY integer DEFAULT 0,"
    "  ALARM_INDIC integer DEFAULT 0,"
    "  HISTO_INDIC integer DEFAULT 0,"
    "  CONV_COEFF integer DEFAULT 1.0,"
    "  LAST_UPDATE datetime,"
    "  VAL_INT integer DEFAULT 0,"
    "  VAL_DOUBLE double DEFAULT 0.0,"
    "  VAL_TIME datetime);"
    "CREATE TABLE PROCESSING ("
    "  PROCESS_ID integer PRIMARY KEY AUTOINCREMENT,"
    "  SITE_REF integer NOT NULL,"
    "  DATA_REF integer NOT NULL,"
    "  PROCESSING_TYPE integer,"
    "  LAST_PROC datetime,"
    "  FOREIGN KEY (SITE_REF) REFERENCES SITES(SITE_ID) ON DELETE CASCADE,"
    "  FOREIGN KEY (DATA_REF) REFERENCES DATA(DATA_ID) ON DELETE CASCADE);"
    "CREATE TABLE ELEMTYPE ("
    "  TYPE_ID integer PRIMARY KEY AUTOINCREMENT,"
    "  NAME varchar,"
    "  TYPE integer DEFAULT 0,"
    "  SIZE varchar);"
    "CREATE TABLE FIELDS ("
    "  FIELD_ID integer PRIMARY KEY AUTOINCREMENT,"
    "  ATTR varchar,"
    "  TYPE integer DEFAULT 0,"
    "  LENGTH integer DEFAULT 0,"
    "  ELEMTYPE_REF integer NOT NULL,"
    "  ELEMSTRUCT_REF integer NOT NULL,"
    "  FOREIGN KEY (ELEMTYPE_REF) REFERENCES ELEMTYPE(TYPE_ID) ON DELETE CASCADE,"
    "  FOREIGN KEY (ELEMSTRUCT_REF) REFERENCES ELEMSTRUCT(STRUCT_ID) ON DELETE CASCADE);"
    "CREATE TABLE ELEMSTRUCT ("
    "  STRUCT_ID integer PRIMARY KEY AUTOINCREMENT,"
    "  NAME varchar,"
    "  ASSOCIATE varchar,"
    "  CLASS integer DEFAULT 0);"
    "CREATE TABLE ASSOCIATE_LINK ("
    "  DATA_REF integer NOT NULL,"
    "  ELEMSTRUCT_REF integer NOT NULL,"
    "  FOREIGN KEY (DATA_REF)  REFERENCES DATA(DATA_ID) ON DELETE CASCADE,"
    "  FOREIGN KEY (ELEMSTRUCT_REF) REFERENCES ELEMSTRUCT(STRUCT_ID) ON DELETE CASCADE);"
    "CREATE TABLE SITES ("
    "  SITE_ID integer PRIMARY KEY AUTOINCREMENT,"
    "  NAME varchar,"
    "  LEVEL integer,"
    "  NATURE integer,"
    "  AUTO_INIT integer,"
    "  AUTO_GENCONTROL integer,"
    "  LAST_UPDATE datetime);"
    "COMMIT;";

  const char* ddl_create_indexes =
    "BEGIN;"
    "  CREATE INDEX PROCESS_IDX_SITES ON PROCESSING(SITE_REF);"
    "  CREATE INDEX PROCESS_IDX_DATA ON PROCESSING(DATA_REF);"
    "  CREATE INDEX FIELDS_IDX_ELEMTYPE ON FIELDS(ELEMTYPE_REF);"
    "  CREATE INDEX FIELDS_IDX_ELEMSTRUCT ON FIELDS(ELEMSTRUCT_REF);"
    "  CREATE INDEX ASSOCIATE_LINK_IDX_DATA ON ASSOCIATE_LINK(DATA_REF);"
    "  CREATE INDEX ASSOCIATE_LINK_IDX_ELEMSTRUCT ON ASSOCIATE_LINK(ELEMSTRUCT_REF);"
    "COMMIT;";

  // Выполнить DDL
  if (sqlite3_exec(m_db, ddl_commands, 0, 0, &m_db_err)) {
    LOG(ERROR) << fname << ": fail to creating SMED structure: " << m_db_err;
    sqlite3_free(m_db_err);
  }
  else {
    LOG(INFO) << fname << ": successfully creates SMED structure";
    rc = load_internal(m_egsa_config->elemtypes(), m_egsa_config->elemstructs());

    // Создать индексы После заполнения таблиц, это быстрее
    if (OK == rc) {
      if (sqlite3_exec(m_db, ddl_create_indexes, 0, 0, &m_db_err)) {
        // NB: Возникновение ошибки при создании индексов не критично, можно игнорировать
        LOG(WARNING) << fname << ": creating SMED indexes: " << m_db_err;
        sqlite3_free(m_db_err);
      }
    }
  }

  return rc;
}

// ==========================================================================================================
// Начальная загрузка данных в SMED - словари ESG, перечень известных Сайтов,...
int SMED::load_internal(elemtype_item_t* _elemtype, elemstruct_item_t* _elemstruct)
{
  static const char* fname = "load_internal";
  int rc = NOK;
  int types_stored = 0;
  int fields_stored = 0;
  int structs_stored = 0;
  int sites_stored = 0;
  char dml_operator[100 + 1];
  elemtype_item_t* elemtype = _elemtype;
  elemstruct_item_t* elemstruct = _elemstruct;
  std::string sql;
  map_id_by_name_t elemtype_dict;  // Словарь для хранения связи между строкой и идентификатором
  map_id_by_name_t elemstruct_dict;  // Словарь для хранения связи между строкой и идентификатором
  size_t elemtype_type_id;
  size_t elemstruct_struct_id;
  std::string elemtype_name;

  // Занести DED_ELEMTYPES
  types_stored = 0;
  sql = "BEGIN;INSERT INTO ELEMTYPE(NAME,TYPE,SIZE) VALUES"; // Первая команда - "Открыть транзакцию"
  while (elemtype && elemtype->name[0])
  {
    snprintf(dml_operator, 100, "%s(\"%s\",%d,\"%s\")",
             ((types_stored++)? "," : ""),
             elemtype->name, elemtype->tm_type, elemtype->format_size);
    sql += dml_operator;

    elemtype++;
  }
  sql += ";COMMIT;";

  if (types_stored) {    // Если список был не пуст, выполнить DML
    if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
      LOG(ERROR) << fname << ": insert elemtypes: " << m_db_err;
      sqlite3_free(m_db_err);
    }
    else rc = OK;
  }

  // Занести DCD_ELEMSTRUCTS
  sql = "BEGIN;INSERT INTO ELEMSTRUCT(NAME,ASSOCIATE,CLASS) VALUES";
  structs_stored = 0;
  while (elemstruct && elemstruct->name[0])
  {
    snprintf(dml_operator, 100, "%s(\"%s\",\"%s\",%d)",
             ((structs_stored++)? "," : ""),
             elemstruct->name, elemstruct->associate, elemstruct->tm_class);
    sql += dml_operator;

    elemstruct++;
  }
  sql += ";COMMIT;";  // Последняя команда - "Закрыть транзакцию"

  if (structs_stored) {
    if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
      LOG(ERROR) << fname << ": insert elemstructs: " << m_db_err;
      sqlite3_free(m_db_err);
    }
    else rc = OK;
  }

  if (OK == rc) {
    // Занести описания Атрибутов из ELEMSTRUCTS
    // 1) FIELDS: ATTR, TYPE, LENGTH
    // Чтобы вставить новую запись в FIELDS, нужно знать ссылку на FIELDS.ELEMTYPE_REF для elemstruct->fields[].field в таблице ELEMTYPES
    // Для этого нужно после занесения данных в ELEMTYPES получить соответствия значений ELEMTYPE.TYPE_ID и ELEMTYPE.NAME
    // Далее использовать соответствующие ELEMTYPE.TYPE_ID для elemstruct->fields[].field в составе:
    // "INSERT INTO FIELDS(ATTR,TYPE,LENGTH,ELEMTYPE_REF) VALUES(field.attribute,field.type,field.length,map[field.field])"
    if ((OK == get_map_id("SELECT TYPE_ID,NAME FROM ELEMTYPE;", elemtype_dict))
      &&(OK == get_map_id("SELECT STRUCT_ID,NAME FROM ELEMSTRUCT;", m_elemstruct_dict))
      &&(OK == get_map_id("SELECT STRUCT_ID,ASSOCIATE FROM ELEMSTRUCT;", m_elemstruct_associate_dict))) {

      // Словарь соответствия значений ELEMTYPE.TYPE_ID и ELEMTYPE.NAME заполнен
      elemstruct = _elemstruct;
      fields_stored = 0;
      elemtype_type_id = 0; elemstruct_struct_id = 0;
      sql = "BEGIN;INSERT INTO FIELDS(ATTR,TYPE,LENGTH,ELEMTYPE_REF,ELEMSTRUCT_REF) VALUES";
      while (elemstruct && elemstruct->name[0])
      {
        for (size_t idx = 0; idx < elemstruct->num_fields; idx++) {

          map_id_by_name_t::const_iterator it_et = elemtype_dict.find(elemstruct->fields[idx].field);
          if (it_et != elemtype_dict.end()) {
            elemtype_type_id = (*it_et).second;
          }
          else LOG(ERROR) << fname << ": ID not found for elementary type " << elemstruct->fields[idx].field;

          map_id_by_name_t::const_iterator it_es = m_elemstruct_dict.find(elemstruct->name);
          if (it_es != m_elemstruct_dict.end()) {
            elemstruct_struct_id = (*it_es).second;
          }
          else LOG(ERROR) << fname << ": ID not found for struct " << elemstruct->name;

          snprintf(dml_operator, 100, "%s(\"%s\",%d,%d,%d,%d)",
                   ((fields_stored++)? "," : ""),
                   elemstruct->fields[idx].attribute,
                   elemstruct->fields[idx].type,
                   elemstruct->fields[idx].length,
                   elemtype_type_id,
                   elemstruct_struct_id);

          sql += dml_operator;
        }

        elemstruct++;
      }

      if (elemtype_type_id && elemstruct_struct_id) {  // Ненулевое значение - значит был вставлен как минимум один FIELDS
        sql += ";COMMIT;";
        // Выполнить запрос
        if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
          LOG(ERROR) << fname << ": insert fields: " << m_db_err;
          sqlite3_free(m_db_err);
        }
        else {
          LOG(INFO) << fname << ": store " << fields_stored << " fields";

          // 2) Наполнить таблицу SITES
          sql = "BEGIN;INSERT INTO SITES(NAME,NATURE) VALUES";
          for (egsa_config_sites_t::const_iterator it = m_egsa_config->sites().begin();
               it != m_egsa_config->sites().end();
               ++it)
          {
            snprintf(dml_operator, 100, "%s(\"%s\",%d)",
                    ((sites_stored++)? "," : ""),
                    (*it).second->name.c_str(),
                    (*it).second->nature);
            sql += dml_operator;
          }
          if (sites_stored) {
            sql += ";COMMIT;";
            // Выполнить запрос
            if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
              LOG(ERROR) << fname << ": insert sites: " << m_db_err;
              sqlite3_free(m_db_err);
            }
            else {
              LOG(INFO) << fname << ": store " << fields_stored << " sites";
              rc = OK;

              if (OK != get_map_id("SELECT SITE_ID,NAME FROM SITES;", m_sites_dict)) {
                LOG(ERROR) << fname << ": unable to load SITES dictionary";
              }

            }
          }
          else {
            LOG(ERROR) << fname << ": nothing to store in SITES";
          }

        }

      }
      else {
        LOG(ERROR) << fname << ": no one FIELDS inserted";
      }
    }
    else {
      LOG(ERROR) << fname << ": unable to load ELEMTYPE or ELEMSTRUCT dictionaries";
    }
  }

  if ((OK == rc) && (fields_stored)) {  // поля Структур были успешно занесены в SMED
  }

  LOG(INFO) << fname << ": store " << types_stored << " types and " << structs_stored << " structs";

  return rc;
}

// ==========================================================================================================
// Получить информацию по параметру, передаваемому в указанную смежную систему
int SMED::get_acq_info(const gof_t_UniversalName site, const int i_Led, esg_esg_odm_t_ExchInfoElem* elem)
{
  static const char* fname = "get_acq_info";
  int rc = NOK;

  LOG(ERROR) << fname << ": SITE " << site << ", LED " << i_Led;
  return rc;
}

// ==========================================================================================================
// Получить информацию по параметру, принимаемую из указанной смежной системы
int SMED::get_diff_info(const gof_t_UniversalName site, const int i_Led, esg_esg_odm_t_ExchInfoElem* elem)
{
  static const char* fname = "get_diff_info";
  int rc = NOK;

  LOG(ERROR) << fname << ": SITE " << site << ", LED " << i_Led;
  return rc;
}

// ==========================================================================================================
int SMED::load_dict(const char* config_filename)
{
  static const char* fname = "load_dict";
  FILE* f_params = NULL;
  struct stat configfile_info;
  int rc = NOK;
  rapidjson::Document esg_dict_document;
  bool dict_has_good_format = false;
  std::string site_from_dict;   // Название Сайта, к которому относится данный словарь
  std::vector<exchange_parameter_t*> fresh;
  sa_flow_direction_t flow = SA_FLOW_UNKNOWN;

#if VERBOSE>8
  LOG(ERROR) << fname << ": try to load SMED file " << config_name;
#endif

  // Выделить буфер readBuffer размером с читаемый файл
  if (-1 == stat(config_filename, &configfile_info)) {
    LOG(ERROR) << fname << ": Unable to stat() '" << config_filename << "': " << strerror(errno);
  }
  else {
    if (NULL != (f_params = fopen(config_filename, "r"))) {
      // Файл открылся успешно, прочитаем содержимое
      LOG(INFO) << fname << ": size of '" << config_filename << "' is " << configfile_info.st_size;
      char *readBuffer = new char[configfile_info.st_size + 1];

      rapidjson::FileReadStream is(f_params, readBuffer, configfile_info.st_size + 1);
      esg_dict_document.ParseStream(is);
      delete []readBuffer;

      assert(esg_dict_document.IsObject());

      if (!esg_dict_document.HasParseError())
      {
        // Конфиг-файл имеет корректную структуру
        dict_has_good_format = true;
      }
      else {
        LOG(ERROR) << fname << ": Parsing " << config_filename;
      }

      fclose (f_params);
    } // Конец успешного чтения содержимого файла
    else {
      LOG(FATAL) << fname << ": Locating config file " << config_filename << " (" << strerror(errno) << ")";
    }
  } // Конец успешной проверки размера файла

  if (dict_has_good_format) {

    if (true == (esg_dict_document.HasMember(SECTION_NAME_DESCRIPTION))) {

      // Получить код Сайта, с которым осуществляется информационный обмен при помощи данного словаря
      rapidjson::Value& section = esg_dict_document[SECTION_NAME_DESCRIPTION];
      if (section.HasMember(SECTION_NAME_SITE)) {
        site_from_dict = section[SECTION_NAME_SITE].GetString();
        if (site_from_dict.empty()) {
          LOG(WARNING) << fname << ": dictionary internal site name is unknown";
        }
        else {
          LOG(INFO) << fname << ": dictionary internal site name: " << site_from_dict;
        }
      }

      // Получить направление переноса этих данных - от Сайта или на Сайт
      if (section.HasMember(SECTION_NAME_FLOW)) {
        const std::string flow_direction = section[SECTION_NAME_FLOW].GetString();
        if (0 == flow_direction.compare(SECTION_FLOW_VALUE_ACQ))
          flow = SA_FLOW_ACQUISITION;
        else if (flow_direction.compare((SECTION_FLOW_VALUE_DIFF)))
          flow = SA_FLOW_DIFFUSION;
        else
          flow = SA_FLOW_UNKNOWN;
      }
    }   // Конец блока "Есть секция с описанием словаря"

    if (true == (esg_dict_document.HasMember(SECTION_NAME_ACQINFOS))) {

      // Получение общих данных по конфигурации интерфейсного модуля RTDBUS Системы Сбора
      rapidjson::Value& section = esg_dict_document[SECTION_NAME_ACQINFOS];
      if (section.IsArray()) {

        for (rapidjson::Value::ValueIterator itr = section.Begin(); itr != section.End(); ++itr) {

          // Получили доступ к очередному элементу Циклов
          const rapidjson::Value::Object& dict_item = itr->GetObject();

          exchange_parameter_t* parameter = new exchange_parameter_t;
          strncpy(parameter->info.s_Name, dict_item[SECTION_NAME_TAG].GetString(), sizeof(gof_t_UniversalName));
          parameter->info.i_LED = dict_item[SECTION_NAME_LED].GetInt();
          parameter->info.o_Categ = TM_CATEGORY_OTHER;

          if (dict_item.HasMember(SECTION_NAME_CATEGORY)) {
            switch(dict_item[SECTION_NAME_CATEGORY].GetString()[0]) {
              case 'P': parameter->info.o_Categ = TM_CATEGORY_PRIMARY;   break;
              case 'S': parameter->info.o_Categ = TM_CATEGORY_SECONDARY; break;
              case 'T': parameter->info.o_Categ = TM_CATEGORY_TERTIARY;  break;
              case 'E': parameter->info.o_Categ = TM_CATEGORY_EXPLOITATION; break;
              case '\0':
              default:
                parameter->info.o_Categ = TM_CATEGORY_OTHER;
            }
          }

          // Инфотип - это OBJCLASS телеинформации, преобразовать в код
          const std::string infotype = dict_item[SECTION_NAME_TYPE].GetString();
          if (0 == infotype.compare("AL")) {
            parameter->info.infotype = GOF_D_BDR_OBJCLASS_AL;
          } else if (0 == infotype.compare("DIPL")) {
            parameter->info.infotype = GOF_D_BDR_OBJCLASS_DIPL;
          } else if (0 == infotype.compare("DIR")) {
            parameter->info.infotype = GOF_D_BDR_OBJCLASS_DIR;
          } else if (0 == infotype.compare("ICS")) {
            parameter->info.infotype = GOF_D_BDR_OBJCLASS_ICS;
          } else if (0 == infotype.compare("ICM")) {
            parameter->info.infotype = GOF_D_BDR_OBJCLASS_ICM;
          } else if (0 == infotype.compare("SA")) {
            parameter->info.infotype = GOF_D_BDR_OBJCLASS_SA;
          } else if (0 == infotype.compare("TM")) {
            parameter->info.infotype = GOF_D_BDR_OBJCLASS_TM;
          } else if (0 == infotype.compare("TSC")) {
            parameter->info.infotype = GOF_D_BDR_OBJCLASS_TSC;
          } else {
            LOG(ERROR) << fname << ": unsupported infotype " << infotype << " for " << parameter->info.s_Name;
            parameter->info.infotype = GOF_D_BDR_OBJCLASS_UNUSED;
          }

          LOG(INFO) << fname << ": " << parameter->info.s_Name << ":" << parameter->info.infotype << ":" << parameter->info.i_LED << ":" << parameter->info.o_Categ;

          if (dict_item.HasMember(SECTION_NAME_ASSOCIATES)) {

            rapidjson::Value& structs = dict_item[SECTION_NAME_ASSOCIATES];

            if (structs.IsArray()) {

              for (rapidjson::Value::ValueIterator itr = structs.Begin(); itr != structs.End(); ++itr)
              {
                const rapidjson::Value::Object& struct_item = itr->GetObject();
                const std::string struct_name = struct_item[SECTION_NAME_STRUCT].GetString();
                parameter->struct_list.push_back(struct_name);
              }

            }
            else {
              LOG(ERROR) << fname << ": Section " << SECTION_NAME_ASSOCIATES << ": not valid format";
            }
          }

          fresh.push_back(parameter);
          rc = OK;
        }

        rc = load_data_dict(site_from_dict.c_str(), fresh, flow);

        // Освободим память
        for (std::vector<exchange_parameter_t*>::const_iterator it = fresh.begin();
             it != fresh.end();
             ++it)
        {
          delete (*it);
        }

      }
      else {
        LOG(ERROR) << fname << ": Section " << SECTION_NAME_ACQINFOS << ": not valid format";
      }

    }

  }

  return rc;
}

// ==========================================================================================================
// Заполнить таблицы DATA и ASSOCIATES_LINK
// Параметры с одним и тем же тегом могут встречаться в разных словарях на распространение данных, поскольку
// они передаются более чем одному Сайту. В этом случае рассчитываем, что их описания идентичны друг другу в
// разных словарях, различаясь только адресатами (SITE_ID).
// Для этого при занесении данных на передачу нужно учитывать возможность того, что параметр с определенным
// Тегом может уже существовать в таблице DATA.
// ВАЖНО! Совершенно обратная ситуация для параметров, подлежащих приему от смежных систем. Такие параметры
// не должны существовать, это ошибка! Для каждого Параметра может быть только один источник!
//
// INSERT INTO ASSOCIATES_LINK(DATA_REF,ELEMSTRUCT_REF) VALUES
// (<DATA_ID только что внесенной записи>, <ELEMSTRUCT_ID той записи, у которой struct_name == ELEMSTRUCT.ASSOCIATE>
int SMED::load_data_dict(const char* site_name, std::vector<exchange_parameter_t*>& parameters, sa_flow_direction_t flow)
{
  static const char* fname = "load_data_dict";
  char dml_operator[100 + 1];
  size_t data_stored = 0;
  size_t associates_stored = 0;
  size_t processing_stored = 0;
  int rc = NOK;
  map_id_by_name_t data_dict;
  size_t elem_id, data_id, site_id;
  std::string sql;

  // Проверить наличие указанного Сайта
  map_id_by_name_t::const_iterator iter_sa = m_sites_dict.find(site_name);
  if (iter_sa == m_sites_dict.end()) {
    LOG(ERROR) << fname << ": site unknown: " << site_name;
  }
  else {
    // Сайт был известен на момент создания SMED
    // NB: Добавление Сайтов после создания SMED пока (2017/06) не предусмотрено
    site_id = (*iter_sa).second;

    switch(flow) {
      case SA_FLOW_ACQUISITION: // опрос внешних источников
        sql = "BEGIN;INSERT OR ROLLBACK INTO DATA(TAG,LED,OBJCLASS,CATEGORY) VALUES";
        break;

      case SA_FLOW_DIFFUSION:   // передача внешним потребителям
        sql = "BEGIN;INSERT OR IGNORE INTO DATA(TAG,LED,OBJCLASS,CATEGORY) VALUES";
        break;

      default:
        LOG(ERROR) << fname << ": unsupported transmittion mode: " << flow << " for site " << site_name;
    }

    if (!sql.empty()) { // Указан допустимый режим передачи

      for (std::vector<exchange_parameter_t*>::const_iterator it = parameters.begin();
           it != parameters.end();
           ++it)
      {
        const exchange_parameter_t* data = (*it);

        snprintf(dml_operator, 100, "%s(\"%s\",%d,%d,%d)",
                 ((data_stored++)? "," : ""),
                 data->info.s_Name,
                 data->info.i_LED,
                 data->info.infotype,
                 data->info.o_Categ);
        sql += dml_operator;
      }
    }

    if (data_stored) { // Если были активные параметры, и режим передачи допустимый
      sql += ";COMMIT;";

      // Выполнить запрос
      if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
        LOG(ERROR) << fname << ": insert into DATA: " << m_db_err;
        sqlite3_free(m_db_err);
      }
      else {
        LOG(INFO) << fname << ": store " << data_stored << " DATA items";

        // Выгрузить DATA_ID и связать их с TAG
        // В data_dict содержатся все параметры из DATA, включая и те, которые подлежат обмену с другими Сайтами
        if (OK == get_map_id("SELECT DATA_ID,TAG FROM DATA;", data_dict)) {

          // Заполнить таблицу PROCESSING новыми точками, их теги содержатся в parameters
          sql = "BEGIN;INSERT INTO PROCESSING(SITE_REF,DATA_REF,PROCESSING_TYPE) VALUES";
          for (std::vector<exchange_parameter_t*>::const_iterator it = parameters.begin();
               it != parameters.end();
               ++it)
          {
            // берем только те параметры, теги которых есть в parameters
            const map_id_by_name_t::const_iterator iter_da = data_dict.find((*it)->info.s_Name);
            if (iter_da != data_dict.end()) {
              snprintf(dml_operator, 100, "%s(%d,%d,%d)",
                 ((processing_stored++)? "," : ""),
                 site_id,
                 (*iter_da).second,
                 flow);
              sql += dml_operator;
            }
            else {
              LOG(ERROR) << fname << ": unable to get ID for " << (*it);
            }
          }
          if (processing_stored) {
            sql += ";COMMIT;";
            if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
              LOG(ERROR) << fname << ": insert into PROCESSING: " << m_db_err;
              sqlite3_free(m_db_err);
            }
            else {
              LOG(INFO) << fname << ": store " << processing_stored << " PROCESSING items";
            }
          }
          else {
            LOG(ERROR) << fname << "nothing to store into PROCESSING";
          }

          // Выгрузить STRUCT_ID и связать их с ASSOCIATE, если это не было сделано ранее
          sql = "BEGIN;INSERT INTO ASSOCIATE_LINK(DATA_REF,ELEMSTRUCT_REF) VALUES";
          for (std::vector<exchange_parameter_t*>::const_iterator it = parameters.begin();
               it != parameters.end();
               ++it)
          {
            for (std::vector<std::string>::const_iterator is = (*it)->struct_list.begin();
                 is != (*it)->struct_list.end();
                 ++is)
            {
              // в (*is) находится название ASSOCIATE структуры текущего элемента DATA
              // в data->info.s_Name находится Тег текущего элемента DATA
              map_id_by_name_t::const_iterator iter_el = m_elemstruct_associate_dict.find((*is));
              map_id_by_name_t::const_iterator iter_da = data_dict.find((*it)->info.s_Name);

              if ((m_elemstruct_associate_dict.end() != iter_el) && (data_dict.end() != iter_da)) {

                elem_id = (*iter_el).second;
                data_id = (*iter_da).second;

                snprintf(dml_operator, 100, "%s(%d,%d)",
                         ((associates_stored++)? "," : ""),
                         data_id,
                         elem_id);
                sql += dml_operator;
              }
              else {
                LOG(ERROR) << fname << ": unable to find associate " << (*is) << " or tag " << (*it)->info.s_Name;
              }
            }

          } // Конец перебора Параметров

          sql += ";COMMIT;";

          if (associates_stored) {
            // Выполнить запрос
            if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
              LOG(ERROR) << fname << ": insert into ASSOCIATE_LINK: " << m_db_err;
              sqlite3_free(m_db_err);
            }
            else {
              LOG(INFO) << fname << ": store " << associates_stored << " ASSOCIATE_LINK links";
              rc = OK;
            }
          }
          else {
            LOG(ERROR) << fname << ": nothing to store into ASSOCIATE_LINK";
          }

        } // Конец блока успешного чтения словаря DICT
        else {
          LOG(ERROR) << fname << ": unable to get dictionary data for table DATA";
        }
      }   // Конец блока успешного занесения записей в таблицу DATA

      if (OK == rc)
        LOG(INFO) << fname << ": Store " << data_stored << " DATA items";

    }
    else {
      LOG(ERROR) << "Nothing to store";
    }

    // Обновить время последнего информационного обмена с Сайтом
    if (OK == rc) {
      snprintf(dml_operator, 100, "BEGIN;UPDATE OR ROLLBACK SITES SET LAST_UPDATE=CURRENT_TIMESTAMP WHERE SITE_ID=%d;COMMIT;", site_id);
      if (sqlite3_exec(m_db, dml_operator, 0, 0, &m_db_err)) {
        LOG(ERROR) << fname << ": update date of last communication with site " << site_name << " : " << m_db_err;
        sqlite3_free(m_db_err);
      }
      else {
        LOG(INFO) << fname << ": update date of last communication with site " << site_name;
      }

    }
  }

  return rc;
}

// ==========================================================================================================
// Выбрать идентификаторы и названия по указанному входному запросу, заполнив ими ассоциативный массив на выходе
// Первый параметр выборки - числовой идентификатор
// Второй параметр выборки - символьная строка
int SMED::get_map_id(const char* dml_operator, map_id_by_name_t& map_id)
{
  static const char* fname = "get_map_id";
  sqlite3_stmt* stmt = 0;
  size_t id;
  int retcode;
  std::string name;
  int rc = NOK;

  if (SQLITE_OK != (retcode = sqlite3_prepare(m_db, dml_operator, -1, &stmt, 0))) {
    LOG(ERROR) << fname << ": select id : " << dml_operator;
  }
  else {
    // Запрос успешно выполнился
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
      id = sqlite3_column_int(stmt, 0);
      name.assign((const char*)sqlite3_column_text(stmt, 1));

      // Пропускаем пустые идентификаторы, если их несколько, то по ним будет невозможно однозначно находить идентификатор
      if (!name.empty()) {
        map_id[name] = id;
      }

    }
    sqlite3_finalize(stmt);
    rc = OK;
  }

  return rc;
}
