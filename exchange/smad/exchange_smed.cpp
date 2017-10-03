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
#include "exchange_config_site.hpp"
#include "exchange_config_cycle.hpp"
#include "exchange_config_request.hpp"
#include "exchange_smed.hpp"
//#include "exchange_egsa_cycle.hpp"

static const char *SQL_TEMPLATE_SELECT_DATA_BY_LED =
  "SELECT D.DATA_ID,D.TAG,D.OBJCLASS,D.CATEGORY,D.ALARM_INDIC,D.HISTO_INDIC"
  " FROM DATA D,SITES S,PROCESSING P"
  " WHERE S.NAME=\"%s\""
  " AND D.LED=%d"
  " AND P.TYPE=%d"
  " AND S.SITE_ID=P.SITE_REF"
  " AND P.DATA_REF=D.DATA_ID;";

// Созданы или нет в SMED служебные таблицы
bool SMED::m_content_initialized = false;

// ==========================================================================================================
// NB: Экземпляр EgsaConfig удаляется в EGSA после своего использования, и доступ к EgsaConfig возможен только на этапе
// инициализации
SMED::SMED(EgsaConfig* _config, const char* snap_file)
 : m_db(NULL),
   m_state(STATE_DISCONNECTED),
   m_db_err(NULL),
   m_egsa_config(_config),
   m_snapshot_filename(NULL)
{
  m_snapshot_filename = strdup(snap_file);

  // Создать экземпляры доступа к Сайтам/Запросам/Циклам в SMED
  m_site_list    = new SmedObjectList<AcqSiteEntry*>(this);
  m_request_list = new SmedObjectList<Request*>(this);
  m_cycle_list   = new SmedObjectList<Cycle*>(this);
  m_request_dict_list = new SmedObjectList<Request*>(this);
}

// ==========================================================================================================
SMED::~SMED()
{
  int rc;

  // Сменить состояние Сайтов в "ОБРЫВ СВЯЗИ"
  detach_sites();

  if (SQLITE_OK != (rc = sqlite3_close(m_db)))
    LOG(ERROR) << "Closing SMED '" << m_snapshot_filename << "': " << rc;
  else
    LOG(INFO) << "SMED file '" << m_snapshot_filename << "' successfuly closed";

  free(m_snapshot_filename);

  delete m_request_dict_list;
  delete m_request_list;
  delete m_site_list;
  delete m_cycle_list;
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
  // NB: создание БД в ОЗУ, а не на НЖМД, на порядок ускоряет с ней работу
  rc = sqlite3_open_v2(m_snapshot_filename /* ":memory:" */,
                       &m_db,
                       // Если таблица только что создана, проверить наличие
                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                       "unix-dotfile");
  if (rc == SQLITE_OK)
  {
    LOG(INFO) << fname << ": SMED file '" << m_snapshot_filename << "' successfuly opened";
    m_state = STATE_OK;

    // Проверить содержимое, чтобы повторно не вносить данные
    if (!m_content_initialized) {

      // Установить кодировку UTF8
      if (sqlite3_exec(m_db, "PRAGMA ENCODING=\"UTF-8\"", 0, 0, &m_db_err)) {
        LOG(ERROR) << fname << ": Unable to set UTF-8 support: " << m_db_err;
        sqlite3_free(m_db_err);
      }
      else LOG(INFO) << "Enable UTF-8 support for HDB";

      // Включить поддержку внешних ключей
      if (sqlite3_exec(m_db, "PRAGMA FOREIGN_KEYS=1", 0, 0, &m_db_err)) {
        LOG(ERROR) << fname << ": Unable to set FOREIGN_KEYS support: " << m_db_err;
        sqlite3_free(m_db_err);
      }
      else LOG(INFO) << "Enable FOREIGN_KEYS support";

      // Создать структуру, прочитав DDL из файла
      rc = create_internal();
      m_content_initialized = true;
    }
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
    "  DROP TABLE IF EXISTS FIELDS_LINK;"
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
    "  HISTO_INDIC integer DEFAULT 0);"
    "CREATE TABLE PROCESSING ("
    "  PROCESS_ID integer PRIMARY KEY AUTOINCREMENT,"
    "  SITE_REF integer NOT NULL,"
    "  DATA_REF integer NOT NULL,"
    "  TYPE integer,"
    "  LAST_PROC integer DEFAULT 0,"
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
    "  IDX integer DEFAULT 0,"
    "  ELEMSTRUCT_REF integer NOT NULL,"
    "  ELEMTYPE_REF integer NOT NULL,"
    "  FOREIGN KEY (ELEMSTRUCT_REF) REFERENCES ELEMSTRUCT(STRUCT_ID) ON DELETE CASCADE,"
    "  FOREIGN KEY (ELEMTYPE_REF) REFERENCES ELEMTYPE(TYPE_ID) ON DELETE CASCADE);"
    "CREATE TABLE FIELDS_LINK ("
    "  LINK_ID integer PRIMARY KEY AUTOINCREMENT,"
    "  FIELD_REF integer NOT NULL,"
    "  ELEMSTRUCT_REF integer NOT NULL,"
    "  DATA_REF integer NOT NULL,"
    "  LAST_UPDATE integer DEFAULT 0,"
    "  NEED_SYNC integer DEFAULT 0,"
    "  VAL_INT integer,"
    "  VAL_DOUBLE double,"
    "  VAL_STR varchar,"
    "  FOREIGN KEY (DATA_REF) REFERENCES DATA(DATA_ID) ON DELETE CASCADE,"
    "  FOREIGN KEY (FIELD_REF) REFERENCES FIELDS(FIELD_ID) ON DELETE CASCADE,"
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
    "  SYNTHSTATE integer DEFAULT 0,"
    "  EXPMODE integer DEFAULT 1,"
    "  INHIBITION integer DEFAULT 1,"
    "  NATURE integer,"
    "  LAST_UPDATE integer DEFAULT 0);"
    "COMMIT;";

#ifdef CREATE_INDEX_SMED
  const char* ddl_create_indexes =
    "BEGIN;"
    "  CREATE INDEX FIELDS_LINK_IDX_FIELDS ON FIELDS_LINK(FIELD_REF);"
    "  CREATE INDEX FIELDS_LINK_IDX_ELEMSTRUCT ON FIELDS_LINK(ELEMSTRUCT_REF);"
    "  CREATE INDEX PROCESS_IDX_SITES ON PROCESSING(SITE_REF);"
    "  CREATE INDEX PROCESS_IDX_DATA ON PROCESSING(DATA_REF);"
    "  CREATE INDEX FIELDS_IDX_ELEMTYPE ON FIELDS(ELEMTYPE_REF);"
    "  CREATE INDEX FIELDS_IDX_ELEMSTRUCT ON FIELDS(ELEMSTRUCT_REF);"
    "  CREATE INDEX ASSOCIATE_LINK_IDX_DATA ON ASSOCIATE_LINK(DATA_REF);"
    "  CREATE INDEX ASSOCIATE_LINK_IDX_ELEMSTRUCT ON ASSOCIATE_LINK(ELEMSTRUCT_REF);"
    "COMMIT;";
#endif

  // Выполнить DDL
  if (sqlite3_exec(m_db, ddl_commands, 0, 0, &m_db_err)) {
    LOG(ERROR) << fname << ": fail to creating SMED structure: " << m_db_err;
    sqlite3_free(m_db_err);
  }
  else {
    LOG(INFO) << fname << ": successfully creates SMED structure";
#ifdef SQL_TRACE
    std::cout << "SQL " << ddl_commands << std::endl;
#endif
    rc = load_internal(m_egsa_config->elemtypes(), m_egsa_config->elemstructs());

#ifndef CREATE_INDEX_SMED
#warning "Suppress creating INDEX for SMED"
    // TODO: БД без индексов занимает вдвое меньше места (5.8Мб против 2.5Мб), нужно ли использовать индексы в этом виде?
#else
    // Создать индексы После заполнения таблиц
    if (OK == rc) {
      if (sqlite3_exec(m_db, ddl_create_indexes, 0, 0, &m_db_err)) {
        // NB: Возникновение ошибки при создании индексов не критично, можно игнорировать
        LOG(WARNING) << fname << ": creating SMED indexes: " << m_db_err;
        sqlite3_free(m_db_err);
      }
      else {
#ifdef SQL_TRACE
        std::cout << "SQL " << ddl_create_indexes << std::endl;
#endif
      }
    }
#endif
  }

  return rc;
}

// ==========================================================================================================
// Начальная загрузка данных в SMED - справочник структур и полей ESG, перечень известных Сайтов,...
int SMED::load_internal(elemtype_item_t* _elemtype, elemstruct_item_t* _elemstruct)
{
  static const char* fname = "load_internal";
  int rc = NOK;
  int types_stored = 0;
  int fields_stored = 0;
  int structs_stored = 0;
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
    else {
#ifdef SQL_TRACE
        std::cout << "SQL " << sql << std::endl;
#endif
        rc = OK;
    }
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
    else {
#ifdef SQL_TRACE
      std::cout << "SQL " << sql << std::endl;
#endif
      rc = OK;
    }
  }

  if (OK == rc) {
    // Занести описания Атрибутов из ELEMSTRUCTS
    // 1) FIELDS:
    //      ATTR название атрибута БДРВ
    //      TYPE тип атрибута БДРВ
    //      LENGTH длина строкового атрибута
    //      IDX  порядковый номер в Структуре
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
      sql = "BEGIN;INSERT INTO FIELDS(ATTR,TYPE,LENGTH,IDX,ELEMSTRUCT_REF,ELEMTYPE_REF) VALUES";
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

          snprintf(dml_operator, 100, "%s(\"%s\",%d,%d,%d,%d,%d)",
                   ((fields_stored++)? "," : ""),
                   elemstruct->fields[idx].attribute,
                   elemstruct->fields[idx].type,
                   elemstruct->fields[idx].length,
                   idx,
                   elemstruct_struct_id,
                   elemtype_type_id);

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
#ifdef SQL_TRACE
          std::cout << "SQL " << sql << std::endl;
#endif

          // TODO: сформировать БД, содержащую все используемые Циклы, каждый из
          // которых содержит ссылки на Сайты, которые в свою очередь содержат
          // очередь актуальных Запросов.

          // 2) Наполнить таблицу SITES и объект-список Сайтов
          rc = store_sites();

          // 3) Заполнить таблицу REQUESTS
          // 3.1) Загрузить НСИ Запросов
          rc = store_requests_dict();
          // 3.2) Загрузить используемые Запросы
          rc = store_requests();
          
#if 0
          // 4) Заполнить таблицу CYCLES
          rc = store_cycles();
#else
#warning "Store Cycles here"
#endif
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
// Занесем сведения о Сайтах в таблицу SITES в SMED
int SMED::store_sites()
{
  static const char *fname = "store_sites";
  int rc = OK;
  char dml_operator[100 + 1];
  std::string sql;
  int sites_stored = 0;

  m_site_list->clear();
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

    m_site_list->insert(new AcqSiteEntry(this, (*it).second));
  }

  if (sites_stored) {
    sql += ";COMMIT;";
    // Выполнить запрос
    if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
      LOG(ERROR) << fname << ": insert sites: " << m_db_err;
      sqlite3_free(m_db_err);
    }
    else {
      LOG(INFO) << fname << ": store " << sites_stored << " sites";

#ifdef SQL_TRACE
      std::cout << "SQL " << sql << std::endl;
#endif

      if (OK != get_map_id("SELECT SITE_ID,NAME FROM SITES;", m_sites_dict)) {
        LOG(ERROR) << fname << ": unable to load SITES dictionary";
      }
    }
  }
  else {
    LOG(ERROR) << fname << ": nothing to store in SITES";
    // Ошибка, обнулим список Сайтов, поскольку он был уже заполнен ранее
    m_site_list->clear();
    rc = NOK;
  }

  return rc;
}

// ==========================================================================================================
// Загрузить НСИ таблицу Запросов
// Используется на этапе подключения к SMED
// NB: эта информация статична, можно не заносить в БД
int SMED::store_requests_dict()
{
  int rc = OK;
  RequestEntry* entry = NULL;
  int rid;

  for(rid = EGA_GENCONTROL; rid < NOT_EXISTENT; rid++)
  {
    if (OK == m_egsa_config->get_request_dict_by_id(static_cast<ech_t_ReqId>(rid), entry)) {

      LOG(INFO) << "ReqDict "
                << entry->s_RequestName << " "
                << entry->i_RequestPriority << " "
                << (unsigned int)entry->e_RequestObject << " "
                << (unsigned int)entry->e_RequestMode;

      m_request_dict_list->insert(new Request(entry));
    }
  }

  return rc;
}

// ==========================================================================================================
// Загрузить таблицу используемых Запросов
int SMED::store_requests()
{
  int rc = OK;
  //char dml_operator[100 + 1];
  std::string sql;

  for(egsa_config_requests_t::const_iterator rit = m_egsa_config->requests().begin();
      rit != m_egsa_config->requests().end();
      ++rit)
  {
#if VERBOSE>6
    LOG(INFO) << "Req "
              << (*rit).second->s_RequestName << " "
              << (*rit).second->i_RequestPriority << " "
              << (unsigned int)(*rit).second->e_RequestObject << " "
              << (unsigned int)(*rit).second->e_RequestMode;
#endif

    m_request_list->insert(new Request((*rit).second));
    /* Request* rq = new Request((*rit).second);
    m_ega_ega_odm_ar_Requests.add(rq);*/
  }

  return rc;
}

#if 0
// ==========================================================================================================
// Загрузить таблицу Циклов
int SMED::store_cycles()
{
  int rc = OK;
  ech_t_ReqId request_id;

  for(egsa_config_cycles_t::const_iterator cit = m_egsa_config->cycles().begin();
      cit != m_egsa_config->cycles().end();
      ++cit)
  {
    // request_id получить по имени запроса
    if (NOT_EXISTENT != (request_id = m_egsa_config->get_request_id((*cit).second->request_name))) {

      // Создадим экземпляр Цикла, удалится он в деструкторе EGSA
      Cycle *cycle = new Cycle((*cit).first.c_str(),
                               (*cit).second->period,
                               (*cit).second->id,
                               request_id,
                               CYCLE_NORMAL);

      // Для данного цикла получить все использующие его сайты
      for(std::vector <std::string>::const_iterator sit = (*cit).second->sites.begin();
          sit != (*cit).second->sites.end();
          ++sit)
      {
        // Найти AcqSiteEntry для текущей SA в m_ega_ega_odm_ar_AcqSites
        AcqSiteEntry* site = m_ega_ega_odm_ar_AcqSites[(*sit)];
        if (site) {
          cycle->link(site);
        }
        else {
          LOG(ERROR) << fname << ": skip undefined site " << (*sit);
        }
      }
      //
      cycle->dump();

      // Ввести в оборот новый Цикл сбора, вернуть новый размер очереди циклов
      m_ega_ega_odm_ar_Cycles.insert(cycle);
    }
    else {
      LOG(ERROR) << fname << ": cycle " << (*cit).first << ": unknown included request " << (*cit).second->request_name;
    }
  }

  return rc;
}
#else
#warning "TODO store Cycles"
#endif

#if 0
// ==========================================================================================================
// Получить текущий список Сайтов из БД SMED
AcqSiteList* SMED::get_site_list()
{
  AcqSiteList* current = NULL;

  return current;
}

// ==========================================================================================================
// Получить текущий список Запросов из БД SMED
RequestList* SMED::get_request_list()
{
  RequestList* current = NULL;

  return current;
}

// ==========================================================================================================
// Получить текущий список Циклов из БД SMED
CycleList* SMED::get_cycle_list()
{
  CycleList* current = NULL;

  return current;
}
#else
#warning "TODO get_cycle_list"
#endif

// ==========================================================================================================
int SMED::get_info(const gof_t_UniversalName site, sa_flow_direction_t direction, const int i_Led, esg_esg_odm_t_ExchInfoElem* elem)
{
  static const char* fname = "get_info";
  sqlite3_stmt* stmt = 0;
#if 0
  const size_t operator_size = strlen(SQL_TEMPLATE_SELECT_DATA_BY_LED)
                               + sizeof(gof_t_UniversalName)    // Параметр #1
                               + 12 // #2: длина символьного представления LED
                               + 4; // #3: длина представления TYPE
#endif
  char dml_operator[1000 /*operator_size + 1*/];
  int num_lines = 0;
  int rc = NOK;

  const int n=snprintf(dml_operator, 1000 /* operator_size */, SQL_TEMPLATE_SELECT_DATA_BY_LED, site, i_Led, direction);
  assert(n < 1000); // 1000 = sizeof(dml_operator)

  // Начальные значения
  elem->s_Name[0] = '\0';
  elem->i_LED = i_Led;
  elem->infotype = GOF_D_BDR_OBJCLASS_UNUSED;
  elem->o_Categ = TM_CATEGORY_OTHER;
  elem->b_AlarmIndic = false;
  elem->i_HistoIndic = 0;
  elem->f_ConvertCoeff = 1.0;
  elem->d_LastUpdDate.tv_sec = 0;
  elem->d_LastUpdDate.tv_usec = 0;

  if (SQLITE_OK == sqlite3_prepare(m_db, dml_operator, -1, &stmt, 0)) {

    // Запрос успешно выполнился
    while(sqlite3_step(stmt) == SQLITE_ROW) {

      elem->h_RecId = sqlite3_column_int(stmt, 0);
      strncpy(elem->s_Name, (const char*)sqlite3_column_text(stmt, 1), sizeof(gof_t_UniversalName));
      elem->infotype = sqlite3_column_int(stmt, 2);
      const int input_category = sqlite3_column_int(stmt, 3);
      if ((TM_CATEGORY_OTHER <= input_category) && (input_category <= TM_CATEGORY_ALL))
        elem->o_Categ = static_cast<telemetry_category_t>(input_category);
      elem->b_AlarmIndic = (sqlite3_column_int(stmt, 4) > 0);
      elem->i_HistoIndic = sqlite3_column_int(stmt, 5);
      num_lines++;

    }
    sqlite3_finalize(stmt);

    if (num_lines == 1) {
      LOG(INFO) << fname << ": SITE " << site << ", LED " << i_Led << ", TAG=" << elem->s_Name;
      rc = OK;
    }
    else if (num_lines > 1) {
      LOG(ERROR) << fname << ": SITE " << site << " has more than one (" << num_lines << ") LED " << i_Led << " TAG=" << elem->s_Name;
    }
    else {
      LOG(WARNING) << fname << ": nothing to read from " << site << " by led #" << i_Led; // << " SQL=" << dml_operator;
    }
  }
  else {
    LOG(ERROR) << fname << ": select " << site << " LED=" << i_Led << " info, SQL=" << dml_operator;
  }

  return rc;
}

// ==========================================================================================================
// Получить информацию по параметру, получаемому от указанной смежной системы
int SMED::get_acq_info(const gof_t_UniversalName site, const int i_Led, esg_esg_odm_t_ExchInfoElem* elem)
{
  int rc = NOK;

  assert(m_db);

  if (STATE_OK == m_state)
    rc = get_info(site, SA_FLOW_ACQUISITION, i_Led, elem);
  else LOG(ERROR) << "incorrect SMED state: " << m_state;

  return rc;
}

// ==========================================================================================================
// Получить информацию по параметру, передаваемому в указанную смежную систему
int SMED::get_diff_info(const gof_t_UniversalName site, const int i_Led, esg_esg_odm_t_ExchInfoElem* elem)
{
  int rc = NOK;

  assert(m_db);

  if (STATE_OK == m_state)
    rc = get_info(site, SA_FLOW_DIFFUSION, i_Led, elem);
  else LOG(ERROR) << "incorrect SMED state: " << m_state;

  return rc;
}

// ==========================================================================================================
// Загрузка словаря обменов из данного файла.
// Заполняются таблицы:
// ASSOCIATE_LINK
// FIELDS_LINK
// DATA
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
        else if (0 == flow_direction.compare((SECTION_FLOW_VALUE_DIFF)))
          flow = SA_FLOW_DIFFUSION;
        else {
          LOG(ERROR) << fname << ": unsupported information flow: " << flow_direction;
          flow = SA_FLOW_UNKNOWN;
        }
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

#if VERBOSE>6
          LOG(INFO) << fname << ": " << parameter->info.s_Name << ":" << parameter->info.infotype << ":" << parameter->info.i_LED << ":" << parameter->info.o_Categ;
#endif

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
// Загрузка словаря обменов из данного файла
// Заполняются таблицы:
// ASSOCIATE_LINK   соответствие между Тегом и используемыми им Структурами. Заполняется при загрузке словарей обмена.
// FIELDS_LINK      связь между Структурой данных, набором входящих в неё полей, и их оперативными значениями.
// DATA             описание всех Параметров, заполняется путем загрузки словарей обмена.
//
// Параметры с одним и тем же тегом могут встречаться в разных словарях на распространение данных, поскольку
// они передаются более чем одному Сайту. В этом случае рассчитываем, что их описания идентичны друг другу в
// разных словарях, различаясь только адресатами (SITE_ID).
// Для этого при занесении данных на передачу нужно учитывать возможность того, что параметр с определенным
// Тегом может уже существовать в таблице DATA.
// ВАЖНО! Совершенно обратная ситуация для параметров, подлежащих приему от смежных систем. Такие параметры
// не должны существовать, это ошибка! Для каждого Параметра может быть только один источник!
//
int SMED::load_data_dict(const char* site_name, std::vector<exchange_parameter_t*>& parameters, sa_flow_direction_t flow)
{
  static const char* fname = "load_data_dict";
  char dml_operator[100 + 1];
  size_t data_stored = 0;
  size_t associates_stored = 0;
  size_t processing_stored = 0;
  size_t fields_stored = 0;
  int rc = NOK;
  map_id_by_name_t data_dict;
  size_t elem_id, site_id, field_id;
  std::string sql;
  std::string sql2;
  sqlite3_stmt* stmt = 0;

  // Проверить наличие указанного Сайта
  map_id_by_name_t::const_iterator iter_sa = m_sites_dict.find(site_name);
  if (iter_sa == m_sites_dict.end()) {
    LOG(ERROR) << fname << ": site unknown: " << site_name;
  }
  else {
    accelerate(true);

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
        LOG(ERROR) << fname << ": unsupported transmition mode: " << flow << " for site " << site_name;
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
#ifdef SQL_TRACE
        std::cout << "SQL " << sql << std::endl;
#endif

        // Выгрузить DATA_ID и связать их с TAG
        // В data_dict содержатся все параметры из DATA, включая и те, которые подлежат обмену с другими Сайтами
        if (OK == get_map_id("SELECT DATA_ID,TAG FROM DATA;", data_dict)) {

          // Заполнить таблицу PROCESSING новыми точками, их теги содержатся в parameters
          sql = "BEGIN;INSERT INTO PROCESSING(SITE_REF,DATA_REF,TYPE) VALUES";
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
#ifdef SQL_TRACE
              std::cout << "SQL " << sql << std::endl;
#endif
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
            map_id_by_name_t::const_iterator iter_da = data_dict.find((*it)->info.s_Name);
            if (data_dict.end() != iter_da) {
              (*it)->info.h_RecId = (*iter_da).second; // Запомним DATA_ID параметра

              for (std::vector<std::string>::const_iterator is = (*it)->struct_list.begin();
                   is != (*it)->struct_list.end();
                   ++is)
              {
                // в (*is) находится название ASSOCIATE структуры текущего элемента DATA
                // в data->info.s_Name находится Тег текущего элемента DATA
                map_id_by_name_t::const_iterator iter_el = m_elemstruct_associate_dict.find((*is));

                if (m_elemstruct_associate_dict.end() != iter_el) {

                  elem_id = (*iter_el).second;

                  snprintf(dml_operator, 100, "%s(%d,%d)",
                           ((associates_stored++)? "," : ""),
                           (*it)->info.h_RecId,
                           elem_id);
                  sql += dml_operator;
                }
                else {
                  LOG(ERROR) << fname << ": unable to find associate " << (*is) << " for tag " << (*it)->info.s_Name;
                }
              }
            }
            else {
              (*it)->info.h_RecId = 0;
              LOG(ERROR) << fname << ": unable to find tag " << (*it)->info.s_Name;
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
#ifdef SQL_TRACE
              std::cout << "SQL " << sql << std::endl;
#endif

              // Вставить связи в таблицу FIELDS_LINK
              // 1) Цикл по всем Ассоциациям <ИМЯ> Структур данного Параметра:
              //    2) Найти STRUCT_ID из ELEMSTRUCT, ассоциация которой равна текущей ассоциации цикла для Параметра
              //    select STRUCT_ID from ELEMSTRUCT where ASSOCIATE = <ИМЯ>
              //
              //    3) Найти все FIELD_ID для полей, входящих в состав ранее найденной на этапе (2) Структуры
              //    select FIELD_ID from FIELDS where ELEMSTRUCT_REF = <STRUCT_ID>
              //
              //    Цикл по всем FIELD_ID, найденным на этапе (3)
              //        4) Вставить новую запись в FIELDS_LINK с <STRUCT_ID> и <FIELD_ID>, содержащую поля для хранения данных из файлов обмена

              // (1) Цикл по всем Ассоциациям
              for (std::vector<exchange_parameter_t*>::const_iterator it = parameters.begin();
                  it != parameters.end();
                  ++it)
              {
                for (std::vector<std::string>::const_iterator is = (*it)->struct_list.begin();
                     is != (*it)->struct_list.end();
                     ++is)
                {
                  elem_id = 0;    // если значение останется нулевым - это признак ошибки
                  // в (*is) находится название ASSOCIATE структуры текущего элемента DATA
                  sql = "SELECT STRUCT_ID FROM ELEMSTRUCT WHERE ASSOCIATE=\"";
                  sql+= (*is);
                  sql+= "\";";
                  if (SQLITE_OK == sqlite3_prepare(m_db, sql.c_str(), -1, &stmt, 0)) {

                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                      elem_id = sqlite3_column_int(stmt, 0);
                    }
                    else {
                      LOG(ERROR) << fname << ": nothing to select from ELEMSTRUCT: " << sql;
                    }
                    sqlite3_finalize(stmt);

                    if (elem_id) { // (2) Найден идентификатор Ассоциативной Структуры
                      sprintf(dml_operator, "SELECT FIELD_ID FROM FIELDS WHERE ELEMSTRUCT_REF=%d;", elem_id);
                      if (SQLITE_OK == sqlite3_prepare(m_db, dml_operator, -1, &stmt, 0)) {

                        fields_stored = 0;
                        sql2 = "INSERT INTO FIELDS_LINK(ELEMSTRUCT_REF,FIELD_REF,DATA_REF) VALUES";
                        // (3) Найти идентификаторы полей
                        while(sqlite3_step(stmt) == SQLITE_ROW) {
                          field_id = 0;   // если значение останется нулевым - это признак ошибки

                          field_id = sqlite3_column_int(stmt, 0);
                          if (field_id) {
                            // (4) Вставить новую запись в FIELDS_LINK
                            snprintf(dml_operator, 100, "%s(%d,%d,%d)",
                                     ((fields_stored++)? "," : ""),
                                     elem_id,               // ELEMSTRUCT_ID
                                     field_id,              // FIELD_ID
                                     (*it)->info.h_RecId);  // DATA_ID
                            sql2 += dml_operator;
                          }
                        }
                        if (fields_stored) {  // Были записи для занесения в БД
                          sql2 += ";";
                          if (sqlite3_exec(m_db, sql2.c_str(), 0, 0, &m_db_err)) {
                            LOG(ERROR) << fname << ": insert into FIELDS_LINK: " << sql2 << ": " << m_db_err;
                            sqlite3_free(m_db_err);
                          }
                          else {
#if VERBOSE>7
                            LOG(INFO) << fname << ": store " << fields_stored << " FIELDS_LINK items";
#endif
#ifdef SQL_TRACE
                            std::cout << "SQL " << sql2 << std::endl;
#endif
                            // Успешно
                            rc = OK;
                          }
                        }
                        else {
                          LOG(ERROR) << fname << ": nothing to store info FIELDS_LINK";
                        }

                      }
                      else {
                        LOG(ERROR) << fname << ": nothing to select from FIELDS: " << dml_operator; 
                      }
                      sqlite3_finalize(stmt);

                    }

                  }
                  else {
                    LOG(ERROR) << fname << ": SQL : " << sql; 
                  }

                }

              }

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
      snprintf(dml_operator, 100, "BEGIN;UPDATE OR ROLLBACK SITES SET LAST_UPDATE=%ld WHERE SITE_ID=%d;COMMIT;", time(0), site_id);
      if (sqlite3_exec(m_db, dml_operator, 0, 0, &m_db_err)) {
        LOG(ERROR) << fname << ": update date of last communication with site " << site_name << " : " << m_db_err;
        sqlite3_free(m_db_err);
      }
      else {
        LOG(INFO) << fname << ": update date of last communication with site " << site_name;
#ifdef SQL_TRACE
        std::cout << "SQL " << dml_operator << std::endl;
#endif
      }

    }

    accelerate(false);
  }

  return rc;
}

// ===========================================================================
void SMED::accelerate(bool speedup)
{
  const char* fname = "accelerate";
  const char* sync_MODE = SQLITE_SYNC_MODE_ON_BULK;
  const char* journal_MODE = SQLITE_JOURNAL_MODE_ON_BULK;
  // Возможные значения JOURNAL_MODE: [DELETE] | TRUNCATE | PERSIST | MEMORY | WAL | OFF
  const char* pragma_set_journal_mode_template = "PRAGMA JOURNAL_MODE = %s";
  // Возможные значения SYNCHRONOUS: 0 | OFF | 1 | NORMAL | 2 | [FULL]
  const char* pragma_set_synchronous_template = "PRAGMA SYNCHRONOUS = %s";
  char sql_operator_journal[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  char sql_operator_synchronous[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  int printed;

  // Для начальной вставки больших объёмов данных в пустую БД (когда не жалко потерять данные)
  // PRAGMA synchronous = 0;
  // PRAGMA journal_mode = OFF;
  // BEGIN;
  // INSERT INTO XXX ...;
  // INSERT INTO XXX ...;
  // ... 10000 вставок...synchronous
  // COMMIT;

  // BEGIN;
  // INSERT INTO XXX ...;
  // INSERT INTO XXX ...;
  // ...10000 вставок...
  // COMMIT;

  // ... пока все не вставится ...

  // PRAGMA synchronous = FULL;
  // PRAGMA journal_mode = DELETE;

  if (speedup) {
        printed = snprintf(sql_operator_journal,
                           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                           pragma_set_journal_mode_template,
                           journal_MODE);
        assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
#if 1
        printed = snprintf(sql_operator_synchronous,
                           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                           pragma_set_synchronous_template,
                           sync_MODE);
        assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
#endif
  }
  else {
        printed = snprintf(sql_operator_journal,
                           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                           pragma_set_journal_mode_template,
                           "DELETE");
        assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
#if 1
        printed = snprintf(sql_operator_synchronous,
                           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                           pragma_set_synchronous_template,
                           "FULL");
        assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
#endif
  }

  if (sqlite3_exec(m_db, sql_operator_journal, 0, 0, &m_db_err)) {
    LOG(ERROR) << fname << ": Unable to change JOURNAL_MODE: " << m_db_err;
    sqlite3_free(m_db_err);
  }
  else LOG(INFO) << "JOURNAL_MODE is changed";

  if (sqlite3_exec(m_db, sql_operator_synchronous, 0, 0, &m_db_err)) {
    LOG(ERROR) << fname << ": Unable to change SYNCHRONOUS mode: " << m_db_err;
    sqlite3_free(m_db_err);
  }
  else LOG(INFO) << "SYNCHRONOUS mode is changed";
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

// ==========================================================================================================
/*
Более быстрый вариант (в FIELDS_LINK добавлен поле DATA_REF)
SELECT D.DATA_ID,FL.LINK_ID,ES.STRUCT_ID,ES.ASSOCIATE,F.FIELD_ID,F.ATTR,ET.TYPE_ID,ET.NAME,ET.TYPE,ET.SIZE
FROM DATA D,ELEMSTRUCT ES,ASSOCIATE_LINK AL,FIELDS F,FIELDS_LINK FL,ELEMTYPE ET,SITES S,PROCESSING P
WHERE D.LED=1
AND AL.DATA_REF=D.DATA_ID
AND D.DATA_ID=FL.DATA_REF
AND AL.ELEMSTRUCT_REF=ES.STRUCT_ID
AND FL.FIELD_REF=F.FIELD_ID
AND F.ELEMTYPE_REF=ET.TYPE_ID
AND F.ELEMSTRUCT_REF=ES.STRUCT_ID
AND S.NAME='K42001'
AND S.SITE_ID=P.SITE_REF
AND P.DATA_REF=D.DATA_ID;

Медленный вариант
SELECT D.DATA_ID,FL.LINK_ID,ES.STRUCT_ID,ES.ASSOCIATE,F.FIELD_ID,F.ATTR,ET.TYPE_ID,ET.NAME,ET.TYPE,ET.SIZE
FROM DATA D,ELEMSTRUCT ES,ASSOCIATE_LINK AL,FIELDS F,FIELDS_LINK FL,ELEMTYPE ET,SITES S,PROCESSING P
WHERE D.LED=1
AND AL.DATA_REF=D.DATA_ID
AND AL.ELEMSTRUCT_REF=ES.STRUCT_ID
AND FL.ELEMSTRUCT_REF=ES.STRUCT_ID
AND FL.FIELD_REF=F.FIELD_ID
AND F.ELEMTYPE_REF=ET.TYPE_ID
AND F.ELEMSTRUCT_REF=ES.STRUCT_ID
AND S.NAME='K42001'
AND S.SITE_ID=P.SITE_REF
AND P.DATA_REF=D.DATA_ID
GROUP BY ES.STRUCT_ID,F.FIELD_ID,F.ATTR,ET.TYPE_ID;
*/

// DATA_ID LINK_ID STRUCT_ID   ASSOCIATE   FIELD_ID  ATTR      TYPE_ID NAME    TYPE    SIZE
// ------- ------- ----------- ----------- --------- --------- ------- ------- ------- ----
// 107     2428    64          I0114       379                 3       L3800   1       8
// 107     2429    64          I0114       380       STATUS    66      E5809   5       1
// 107     2430    64          I0114       381       VAL       60      E5803   1       5
// 107     2431    64          I0114       382       VALID     61      E5804   1       5
// 107     2432    64          I0114       383       VALIDR    62      E5805   1       5
// 107     2433    64          I0114       384       DATEHOURM 11      E2800   2       U17
// 107     2434    64          I0114       385                 1       L1800   1       4
// 107     2435    64          I0114       386       VAL_LABEL 64      E5807   3       0
// 107     2436    27          H0028       87                  3       L3800   1       8
// 107     2437    27          H0028       88                  25      E2828   2       U20
// 107     2438    27          H0028       89                  68      E5812   1       3
// 107     2439    27          H0028       90                  69      E5814   1       3
// 107     2440    27          H0028       91                  59      E5802   1       5
// 107     2441    27          H0028       92                  87      E6800   4       11e4
// 107     2442    14          A0014       54                  3       L3800   1       8
// 107     2443    14          A0014       55                  25      E2828   2       U20
// 107     2444    14          A0014       56                  68      E5812   1       3
// 107     2445    14          A0014       57                  69      E5814   1       3
// 107     2446    14          A0014       58                  59      E5802   1       5
// 107     2447    14          A0014       59                  87      E6800   4       11e4

int SMED::processing(const gof_t_UniversalName s_IAcqSiteId,
                   const esg_esg_edi_t_StrComposedData* pr_IInternalCData,
                   elemstruct_item_t* pr_ISubTypeElem,
                   const esg_esg_edi_t_StrQualifyComposedData* pr_IQuaCData,
                   const struct timeval d_IReceivedDate)
{
  static const char *fname = "processing";
  const char* SELECT_INFO = "SELECT FL.LAST_UPDATE,D.DATA_ID,FL.LINK_ID,F.FIELD_ID,F.ATTR,F.TYPE,ET.TYPE_ID,ET.NAME,ET.TYPE,ET.SIZE"
                            " FROM DATA D,ELEMSTRUCT ES,ASSOCIATE_LINK AL,FIELDS F,FIELDS_LINK FL,ELEMTYPE ET,SITES S,PROCESSING P"
                            " WHERE D.LED=%d"
                            " AND AL.DATA_REF=D.DATA_ID"
                            " AND D.DATA_ID=FL.DATA_REF"
                            " AND AL.ELEMSTRUCT_REF=ES.STRUCT_ID"
                            " AND FL.FIELD_REF=F.FIELD_ID"
                            " AND ES.STRUCT_ID=%d"
                            " AND F.ELEMTYPE_REF=ET.TYPE_ID"
                            " AND F.ELEMSTRUCT_REF=ES.STRUCT_ID"
                            " AND S.NAME='%s'"
                            " AND S.SITE_ID=P.SITE_REF"
                            " AND P.DATA_REF=D.DATA_ID;";
//  const int dml_sql_size = strlen(SELECT_INFO) + 9 /*LED*/ + 9 /*STRUCT_ID*/ + 7 /*site tag*/ + 300 /*значение*/;
/*  const char* SELECT_DATEHOURM = "SELECT D.DATA_ID"
                            " FROM SITES S,PROCESSING P,DATA D,FIELDS_LINK FL,FIELDS F"
                            " WHERE S.NAME='%s'"
                            " AND S.SITE_ID=P.SITE_REF"
                            " AND P.DATA_REF=D.DATA_ID"
                            " AND P.TYPE=%d"
                            " AND D.TAG='%s'"
                            " AND D.DATA_ID=FL.DATA_REF"
                            " AND FL.FIELD_REF=F.FIELD_ID"
                            " AND F.ATTR='%s';";*/
  char dml_operator[1000 /*dml_sql_size + 1*/];  // буфер SQL для обновления данных в SMED
  char dml_value[300 + 1];              // буфер для сохранения значения Параметра в зависимости от его типа
  sqlite3_stmt* stmt = 0;
  time_t last_update;
  size_t data_id, link_id, struct_id, field_id, type_id;
  int et_type, attr_type;
//  esg_esg_odm_t_ExchInfoElem r_ExchInfoElem;
  int rc = NOK;
  size_t field_idx = 0;
  size_t attr_idx = 0; // Индекс Абтрибута БДРВ в массиве Полей (народная мудрость: не каждое Поле - Атрибут БДРВ)
//  unsigned char attr_name[TAG_NAME_MAXLEN + 1];
//  unsigned char *et_name, *et_size;
//  elemstruct_item_t* r_ExchCompElem = NULL;         // subtype structure
  std::string sql;

//  ech_t_InternalVal value;

  do {

    if (!d_IReceivedDate.tv_sec) {
      LOG(WARNING) << fname << ": timestamp wrong of received data: " << d_IReceivedDate.tv_sec;
      break;
    }

    map_id_by_name_t::const_iterator it = m_elemstruct_dict.find(pr_ISubTypeElem->name);
    if (it == m_elemstruct_dict.end()) {
      LOG(ERROR) << fname << ": unable to find ID for ELEMSTRUCT " << pr_ISubTypeElem->name;
      break;
    }
    // Получили идентификатор ELEMSTRUCT по её названию
    struct_id = (*it).second;

    // Если заданное время получения данных не нулевое (т.е. корректно прочиталось), проверить "время последнего обновления"
    // данных для всех подтипов данного LED. Если обновление необходимо, взвести флаг b_DataOK и обновить хранимый маркер времени".
    const int n=sprintf(dml_operator, SELECT_INFO,
            /* LED */       pr_IQuaCData->i_QualifyValue,
            /* STRUCT_ID */ struct_id,
            /* SITE NAME */ s_IAcqSiteId);
    assert(n < 1000); // 1000 = sizeof(dml_sql_size)
    if (SQLITE_OK == sqlite3_prepare(m_db, dml_operator, -1, &stmt, 0)) {

      // Запрос успешно выполнился
      attr_idx = 0;
      sql = "BEGIN;";
      while(sqlite3_step(stmt) == SQLITE_ROW) {

        last_update = sqlite3_column_int(stmt, 0);  // FL.LAST_UPDATE
        data_id     = sqlite3_column_int(stmt, 1);  // D.DATA_ID
        link_id     = sqlite3_column_int(stmt, 2);  // FL.LINK_ID
        field_id    = sqlite3_column_int(stmt, 3);  // F.FIELD_ID
        const unsigned char* attr_name = sqlite3_column_text(stmt, 4);           // F.ATTR
        attr_type   = sqlite3_column_int(stmt, 5);  // F.TYPE
        type_id     = sqlite3_column_int(stmt, 6);  // ET.TYPE_ID
        const unsigned char* et_name = sqlite3_column_text(stmt,7);  // ET.NAME
        et_type     = sqlite3_column_int(stmt, 8);  // ET.TYPE
        const unsigned char* et_size = sqlite3_column_text(stmt,9); // ET.SIZE

        // Если это служебное поле, не имеющее соответствия в БДРВ, пропустим
        if (FIELD_TYPE_UNKNOWN != attr_type) { // Нет, это один из атрибутов БДРВ

          // TODO: пропускать параметры, имеющие фактический тип, несовпадающий со своим типом в SMED
          //1 if (pr_IInternalCData->ar_EDataTable[attr_idx].type != attr_type)
          if (pr_ISubTypeElem->fields[field_idx].type != attr_type)
            LOG(ERROR) << "LED=" << pr_IQuaCData->i_QualifyValue
                      << " attr_idx=" << attr_idx
                      << " assoc=" << pr_ISubTypeElem->name
                      << " data_id=" << data_id
                      << " attr=" << attr_name
                      << " field_id=" << field_id
                      << " struct_id=" << struct_id
                      << " et(name=" << et_name
                      << " type=" << et_type
                      << " size=" << et_size << ")"
                      << " type_id=" << type_id
                      << " link_id=" << link_id
                      << " type1=" << pr_IInternalCData->ar_EDataTable[attr_idx].type
                      << " type2=" << attr_type;
          //1 assert(pr_IInternalCData->ar_EDataTable[attr_idx].type == attr_type);
          assert(pr_ISubTypeElem->fields[field_idx].type == attr_type);

          // TODO: Для телеметрии, имеющей атрибут DATEHOURM, обновлять значение только тогда, когда этот атрибут содержит более ранний
          // маркер времени, чем данный. Обновляется вся телеметрия, не имеющая данного атрибута, или имеющая его более раннее значение.
          // sprintf(SELECT_DATEHOURM, s_IAcqSiteId, SA_FLOW_DIFFUSION, 
#warning "ГОФО дополнительно для ТИ проверяет значение атрибута DATEHOURM при обновлении данных - проверить необходимость"

          // Данные обновляются, только если полученные данные имеют больший маркер времени, чем содержащийся в SMED
          if (last_update < d_IReceivedDate.tv_sec) {  // Да, пришли более свежие данные
            LOG(INFO) << fname << ": update smed record id " << link_id << ", type=" << attr_type << " attr=" << attr_name << " size=" << et_size;

            // Шапка команды обновления
            sprintf(dml_operator, "UPDATE FIELDS_LINK SET LAST_UPDATE=%ld", d_IReceivedDate.tv_sec);
            dml_value[0] = '\0';

            switch (attr_type) {
              case FIELD_TYPE_LOGIC:  sprintf(dml_value, ",NEED_SYNC=1,VAL_INT=%d", pr_IInternalCData->ar_EDataTable[attr_idx++].u_val.b_Logical);  break;
              case FIELD_TYPE_INT8:   sprintf(dml_value, ",NEED_SYNC=1,VAL_INT=%d", pr_IInternalCData->ar_EDataTable[attr_idx++].u_val.o_Int8);     break;
              case FIELD_TYPE_UINT8:  sprintf(dml_value, ",NEED_SYNC=1,VAL_INT=%d", pr_IInternalCData->ar_EDataTable[attr_idx++].u_val.o_Uint8);    break;
              case FIELD_TYPE_INT16:  sprintf(dml_value, ",NEED_SYNC=1,VAL_INT=%d", pr_IInternalCData->ar_EDataTable[attr_idx++].u_val.h_Int16);    break;
              case FIELD_TYPE_UINT16: sprintf(dml_value, ",NEED_SYNC=1,VAL_INT=%d", pr_IInternalCData->ar_EDataTable[attr_idx++].u_val.h_Uint16);   break;
              case FIELD_TYPE_INT32:  sprintf(dml_value, ",NEED_SYNC=1,VAL_INT=%d", pr_IInternalCData->ar_EDataTable[attr_idx++].u_val.i_Int32);    break;
              case FIELD_TYPE_UINT32: sprintf(dml_value, ",NEED_SYNC=1,VAL_INT=%d", pr_IInternalCData->ar_EDataTable[attr_idx++].u_val.i_Uint32);   break;
              case FIELD_TYPE_FLOAT:  sprintf(dml_value, ",NEED_SYNC=1,VAL_DOUBLE=%f", pr_IInternalCData->ar_EDataTable[attr_idx++].u_val.f_Float); break;
              case FIELD_TYPE_DOUBLE: sprintf(dml_value, ",NEED_SYNC=1,VAL_DOUBLE=%g", pr_IInternalCData->ar_EDataTable[attr_idx++].u_val.g_Double);break;
              case FIELD_TYPE_DATE:   sprintf(dml_value, ",NEED_SYNC=1,VAL_INT=%ld",   pr_IInternalCData->ar_EDataTable[attr_idx++].u_val.d_Timeval.tv_sec);break;
              case FIELD_TYPE_STRING:
                // Размер сохраняемой строки ограничен
                if (pr_IInternalCData->ar_EDataTable[attr_idx].u_val.r_Str.i_LgString < 256) {
                  if (pr_IInternalCData->ar_EDataTable[attr_idx].u_val.r_Str.i_LgString) {
                    sprintf(dml_value, ",NEED_SYNC=1,VAL_STR='%s'",  pr_IInternalCData->ar_EDataTable[attr_idx].u_val.r_Str.ps_String);
                  }
                }
                else {
                  LOG(ERROR) << fname << ": LED " << pr_IQuaCData->i_QualifyValue << " of " << s_IAcqSiteId
                             << " exceeds size limit: " << pr_IInternalCData->ar_EDataTable[attr_idx].u_val.r_Str.i_LgString;
                }
                assert(pr_IInternalCData->ar_EDataTable[attr_idx].u_val.r_Str.i_LgString < 256); // режим отладки
                attr_idx++;
                break;

              default:
                LOG(ERROR) << fname << ": unwilling LED " << pr_IQuaCData->i_QualifyValue << " type: " << attr_type;
            }

            if (dml_value[0]) strcat(dml_operator, dml_value);
            sprintf(dml_value, " WHERE LINK_ID=%d;", link_id);
            strcat(dml_operator, dml_value);
            sql += dml_operator;
          }
        }   // Конец блока обработки атрибута БДРВ

        field_idx++;
      } // Конец блока обработки строк, возвращенных при выборке из SMED полей Ассоциированной структуры
      sqlite3_finalize(stmt);

      if (attr_idx) { // Если атрибуты БДРВ встречались в выборке

        // обновить маркер времени последнего обмена информацией с Сайтом в таблице SITES
        sprintf(dml_operator, "UPDATE SITES SET LAST_UPDATE=%ld WHERE NAME='%s';", d_IReceivedDate.tv_sec, s_IAcqSiteId);
        sql += dml_operator;





        sql += "COMMIT;";
        if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
          LOG(ERROR) << fname << ": SMED " << s_IAcqSiteId << " led=" << pr_IQuaCData->i_QualifyValue << " updating: " << m_db_err;
          sqlite3_free(m_db_err);
        }
        else {
          LOG(INFO) << fname << ": SMED " << s_IAcqSiteId << " led=" << pr_IQuaCData->i_QualifyValue << " update " << attr_idx << " atributes";
#ifdef SQL_TRACE
          std::cout << "SQL " << sql << std::endl;
#endif
        }
      }  // Конец блока занесения данных в SMED

      rc = OK;

    } // Конец блока успешного выполнения запроса на выбор
    else {
      LOG(ERROR) << fname << ": SQL : " << dml_operator;
    }

  } while (false);

  return rc;
}

// ==============================================================================
// Чтение из SMED для Сайта значения указанного Атрибута для данного Тега
int SMED::ech_smd_p_ReadRecAttr (
	// Input parameters
	const char* site_name,       // IN : site object
	const gof_t_UniversalName   s_ObjName,  // IN : object universal name of the record
	const char*                 s_AttrName, // IN : attribut universal name
	// Output parameters
	xdb::AttributeInfo_t       *pr_RecAttr, // OUT : value of the concerned attribut
	bool                       &b_RecAttrChanged) // OUT : changed indicator of the concerned attribut
{
  static const char* fname = "ech_smd_p_ReadRecAttr";
  static const char* SELECT_TEMPLATE = "SELECT D.OBJCLASS,F.TYPE,FL.NEED_SYNC,FL.LAST_UPDATE,FL.VAL_INT,FL.VAL_DOUBLE,FL.VAL_STR"
                                       " FROM DATA D,PROCESSING P, SITES S,FIELDS F, FIELDS_LINK FL"
                                       " WHERE P.DATA_REF=D.DATA_ID"
                                       " AND FL.DATA_REF=D.DATA_ID"
                                       " AND P.DATA_REF=FL.DATA_REF"
                                       " AND FL.FIELD_REF=F.FIELD_ID"
                                       " AND D.TAG='%s'"
                                       " AND F.ATTR='%s'"
                                       " AND S.NAME='%s';";
#if 0
  const int operator_size = strlen(SELECT_TEMPLATE)
          + /* DATA.TAG   */ TAG_NAME_MAXLEN
          + /* FIELD.ATTR */ TAG_NAME_MAXLEN
          + /* SITE.NAME  */ TAG_NAME_MAXLEN
          + /* RESERVE    */ 1;
#endif
  char dml_operator[1000 /*operator_size + 1*/];
  sqlite3_stmt* stmt = 0;
  int rc = OK;

  // Первоначальная инициализация
  pr_RecAttr->type = xdb::DB_TYPE_UNDEF;
  pr_RecAttr->quality = xdb::ATTR_NOT_FOUND;
  pr_RecAttr->value.fixed.val_uint64 = 0;
  pr_RecAttr->value.dynamic.size = 0;
  pr_RecAttr->value.dynamic.varchar = NULL;
  pr_RecAttr->value.dynamic.val_string = NULL;

  pr_RecAttr->name = s_ObjName;
  pr_RecAttr->name+= ".";
  pr_RecAttr->name+= s_AttrName;

  LOG(INFO) << fname << ": site=" << site_name << " tag=" << pr_RecAttr->name;

  // univname_t name;    /* имя атрибута */
  // DbType_t   type;    /* его тип - целое, дробь, строка */
  // Quality_t  quality; /* качество атрибута (из БДРВ) */
  // AttrVal_t  value;   /* значение атрибута */

  const int n=snprintf(dml_operator, 1000 /*operator_size*/, SELECT_TEMPLATE, s_ObjName, s_AttrName, site_name);
  assert(n < 1000);

  if (SQLITE_OK == sqlite3_prepare(m_db, dml_operator, -1, &stmt, 0)) {

    int num_lines = 1;
    // Запрос успешно выполнился
    while(sqlite3_step(stmt) == SQLITE_ROW) {

      const unsigned int objclass    = sqlite3_column_int(stmt, 0);
      assert(objclass <= GOF_D_BDR_OBJCLASS_LASTUSED);
      const unsigned int field_type  = sqlite3_column_int(stmt, 1);
      assert(field_type <= FIELD_TYPE_STRING);
      const int need_sync   = sqlite3_column_int(stmt, 2);
      const int last_update = sqlite3_column_int(stmt, 3);
      const unsigned long val_int = sqlite3_column_int(stmt, 4);
      const double val_double   = sqlite3_column_double(stmt, 5);
      const std::string val_str = (const char*)sqlite3_column_text(stmt, 6);

      b_RecAttrChanged = (need_sync > 0);
      // В выборке должна быть только одна строка
      assert(--num_lines == 0);

      switch(field_type) {
        case 1:  pr_RecAttr->type = xdb::DB_TYPE_LOGICAL; pr_RecAttr->value.fixed.val_bool   = (val_int > 0);     break;
        case 2:  pr_RecAttr->type = xdb::DB_TYPE_INT8;    pr_RecAttr->value.fixed.val_int8   = (int8_t)val_int;   break;
        case 3:  pr_RecAttr->type = xdb::DB_TYPE_UINT8;   pr_RecAttr->value.fixed.val_uint8  = (uint8_t)val_int;  break;
        case 4:  pr_RecAttr->type = xdb::DB_TYPE_INT16;   pr_RecAttr->value.fixed.val_int16  = (int16_t)val_int;  break;
        case 5:  pr_RecAttr->type = xdb::DB_TYPE_UINT16;  pr_RecAttr->value.fixed.val_uint16 = (uint16_t)val_int; break;
        case 6:  pr_RecAttr->type = xdb::DB_TYPE_INT32;   pr_RecAttr->value.fixed.val_int32  = (int32_t)val_int;  break;
        case 7:  pr_RecAttr->type = xdb::DB_TYPE_UINT32;  pr_RecAttr->value.fixed.val_uint32 = (uint32_t)val_int; break;
        case 8:  pr_RecAttr->type = xdb::DB_TYPE_FLOAT;   pr_RecAttr->value.fixed.val_float  = (float)val_double; break;
        case 9:  pr_RecAttr->type = xdb::DB_TYPE_DOUBLE;  pr_RecAttr->value.fixed.val_double = val_double;        break;
        case 10: pr_RecAttr->type = xdb::DB_TYPE_ABSTIME; pr_RecAttr->value.fixed.val_time.tv_sec = val_int;      break;
        case 11:
          pr_RecAttr->type = xdb::DB_TYPE_BYTES;
          pr_RecAttr->value.dynamic.size = val_str.size();
          pr_RecAttr->value.dynamic.varchar = new char[val_str.size() + 1];
          strcpy(pr_RecAttr->value.dynamic.varchar, val_str.c_str());
          break;

        default:
          LOG(ERROR) << fname << ": unsupported SMED attribute " << pr_RecAttr->name << " type: " << pr_RecAttr->type;
          rc = NOK;
      }

/*
      if ((TM_CATEGORY_OTHER <= input_category) && (input_category <= TM_CATEGORY_ALL))
        elem->o_Categ = static_cast<telemetry_category_t>(input_category);
      elem->b_AlarmIndic = (sqlite3_column_int(stmt, 4) > 0);
      elem->i_HistoIndic = sqlite3_column_int(stmt, 5);
      num_lines++; */
    }
    sqlite3_finalize(stmt);
  }
  else {
    LOG(ERROR) << fname << ": SQL : " << dml_operator;
    rc = NOK;
  }

  return rc;
}

// ==============================================================================
// Обновить указанной Системе синтетическое состояние
int SMED::esg_acq_dac_SmdSynthState(const char* site_name, synthstate_t synthstate)
{
  const char* fname = "esg_acq_dac_SmdSynthState";
  static const char* SQL_TEMPLATE_1 = "BEGIN;UPDATE FIELDS_LINK SET VAL_INT=%d WHERE DATA_REF IN("
            "SELECT D.DATA_ID"
            " FROM DATA D, ELEMSTRUCT ES, ASSOCIATE_LINK AL,FIELDS_LINK FL,FIELDS F,SITES S,PROCESSING P"
            " WHERE D.DATA_ID=AL.DATA_REF"
            " AND FL.DATA_REF=D.DATA_ID"
            " AND FL.FIELD_REF=F.FIELD_ID"
            " AND P.DATA_REF=D.DATA_ID"
            " AND S.SITE_ID=P.SITE_REF"
            " AND S.NAME='%s'"
            " AND P.TYPE=%d"
            " AND D.OBJCLASS=%d"
            " AND ES.ASSOCIATE='%s');";
#if 0
  const int operator_size = strlen(SQL_TEMPLATE_1)
            + /* VAL_INT */  3
            + /* S.NAME */   TAG_NAME_MAXLEN
            + /* P.TYPE */   3
            + /* OBJCLASS */ 4
            + /* ASSOCIATE*/ 5 /* ECH_D_COMPIDLG */
            + /* резерв */   1;
#endif
  static const char* SQL_TEMPLATE_2 = "UPDATE SITES SET SYNTHSTATE=%d WHERE NAME='%s';COMMIT;";
  char dml_operator[500 /*operator_size + 1*/]; // NB: размер SQL_TEMPLATE_1 много больше, чем SQL_TEMPLATE_2, выбираем размер буфера по нему
  int rc = OK;
  std::string sql;

  const int n1=sprintf(dml_operator, SQL_TEMPLATE_1, synthstate, site_name, SA_FLOW_ACQUISITION, GOF_D_BDR_OBJCLASS_SA, "I0017" /*ECH_D_SYNTHSTATE_VALUE*/);
  assert(n1 < 500);
  sql = dml_operator;

  const int n2=sprintf(dml_operator, SQL_TEMPLATE_2, synthstate, site_name);
  assert(n2 < 500);
  sql += dml_operator;

  // Установить кодировку UTF8
  if (sqlite3_exec(m_db, dml_operator, 0, 0, &m_db_err)) {
    LOG(ERROR) << fname << ": updating " << site_name << "." << RTDB_ATT_SYNTHSTATE << " to " << synthstate
#if VERBOSE > 7
               << ", SQL: " << sql
#endif
               << " : " << m_db_err;
    sqlite3_free(m_db_err);
    rc = NOK;
  }
  else {
    LOG(INFO) << fname << ": " << site_name << "." << RTDB_ATT_SYNTHSTATE << " := " << synthstate;
  }

  return rc;
}
// ==============================================================================

// ==============================================================================
// TODO: Для всех подчиненных систем сбора:
// 1. Изменить их состояние SYNTHSTATE на "ОТКЛЮЧЕНО"
// 2. Отключиться от их внутренней SMAD
int SMED::detach_sites()
{
  const char* fname = "detach_sites";
  const char* SQL_UPDATE_SYNTHSTATES = "BEGIN;UPDATE SITES SET SYNTHSTATE=0,LAST_UPDATE=%ld WHERE SYNTHSTATE>0;COMMIT;";
  int rc = OK;
#if 0
  const int operator_size = strlen(SQL_UPDATE_SYNTHSTATES)
            + /* LAST_UPDATE */  10
            + /* резерв */   1;
#endif
  char dml_operator[100 /*operator_size + 1*/];

  const int n=sprintf(dml_operator, SQL_UPDATE_SYNTHSTATES, time(0));
  assert(n < 100);
  if (sqlite3_exec(m_db, dml_operator, 0, 0, &m_db_err)) {
    LOG(ERROR) << fname << ": updating attributes " << RTDB_ATT_SYNTHSTATE
#if VERBOSE > 7
               << ", SQL: " << dml_operator
#endif
               << " : " << m_db_err;
    sqlite3_free(m_db_err);
    rc = NOK;
  }
  else {
    LOG(INFO) << fname << ": update atributes " << RTDB_ATT_SYNTHSTATE;
  }

  /*{
    if (*it) {
      AcqSiteEntry* entry = *(*it);
      LOG(INFO) << "TODO: set " << entry->name() << ".SYNTHSTATE = 0";
      LOG(INFO) << "TODO: detach " << entry->name() << " SMAD";
    }
  }*/

  return rc;
}

// ==========================================================================================================
// Вернуть шаблон запроса по его идентификатору
Request* SMED::get_request_dict_by_id(ech_t_ReqId id)
{
  Request* req = NULL;

  // TODO: проверить все Запросы в m_request_dict_list и найти тот, что имеет заданный идентификатор
  for(size_t idx = 0; idx < m_request_dict_list->count(); idx++) {
    req = (*m_request_dict_list)[idx];
    if (id == req->id()) {
      LOG(INFO) << "gotcha: get_request_dict_by_id id=" << id;
      break;
    }
  }
  return req;
}

// ==========================================================================================================
// Вернуть Запрос по его идентификатору
Request* SMED::get_request_by_id(ech_t_ReqId id)
{
  Request* req = NULL;

  // TODO: проверить все Запросы в m_request_list и найти тот, что имеет заданный идентификатор
  for(size_t idx = 0; idx < m_request_list->count(); idx++) {
    req = (*m_request_list)[idx];
    if (id == req->id()) {
      LOG(INFO) << "gotcha: get_request_by_id id=" << id;
      break;
    }
  }
  return req;
}

