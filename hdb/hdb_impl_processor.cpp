/*
 * Программа моделирования накопления предыстории для одного аналогового значения
 * Наименование моделируемой Точки: /KD4005/GOV20/PT02
 * Дата начала накопления истории: 00:00:00 1 января 2000 года
 * Дата завершения накопления истории:
 * Интервал семплирования: 1 минута
 *
 * Описание работы
 * В пустой БДРВ, созданной на основе структуры rtap_db.mco, создается запись о Точке
 * аналогового типа, с указанием:
 *   1. Номер слота = 100
 *   2. Тип хранимой истории = 5 минут
 * Счётчик текущего времени устанавливается в начало интервала накопления истории.
 * В цикле по времени от начала счетчика до конца интервала с шагом в 60 секунд:
 *   По синусоидальному закону для каждого семпла формируется новое достоверное значение.
 *   Создается новая запись в таблице HISTORY
 *   Значение и Достоверность заносятся в соответствующий слот новой записи HISTORY
 * Конец цикла.
 *
 */
#include <iostream>
#include <algorithm>    // std::sort
#include <vector>       // std::vector

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "glog/logging.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "helper.hpp"
#include "xdb_common.hpp"
#include "xdb_impl_common.h"
#include "xdb_rtap_database.hpp"
#include "xdb_rtap_connection.hpp"

#include "hdb_impl_processor.hpp"

// ===========================================================================
#include "sqlite3.h"

// Результат выполнения запроса на список точек с предысторией в формате RTDB.
// Требует конвертации, чтобы хранить еще и идентификатор записи в HDB.
xdb::map_name_id_t m_raw_actual_rtdb_points;
// Текущие теги из БДРВ
map_history_info_t m_actual_rtdb_points;
// Теги из существующего снимка Исторической БД
map_history_info_t m_actual_hist_points;
// Разница между старым и новым наборами тегов Исторической БД
map_history_info_t m_need_to_create_hist_points;
map_history_info_t m_need_to_delete_hist_points;
historized_attributes_t m_historized[MAX_PORTION_SIZE_LOADED_HISTORY];

// Массив точек связи между собирателями и управляющей нитью
const char* pairs[] = {
  "",                       // STAGE_NONE    = 0
  "inproc://pair_1_min",    // PER_1_MINUTE  = 1
  "inproc://pair_5_min",    // PER_5_MINUTES = 2
  "inproc://pair_hour",     // PER_HOUR      = 3
  "inproc://pair_day",      // PER_DAY       = 4
  "inproc://pair_month",    // PER_MONTH     = 5
  "inproc://pair_main",     // STAGE_LAST    = 6
};

// Массив переходов от текущего типа собирателя к следующему
const state_pair_t m_stages[STAGE_LAST] = {
  // текущая стадия (current)
  // |              сокет текующей стадии (pair_name_current)
  // |              |
  // |              |                    следующая стадия (next)
  // |              |                    |
  // |              |                    |              сокет следующей стадии (pair_name_next)
  // |              |                    |              |
  // |              |                    |              |                     анализируемых отсчетов
  // |              |                    |              |                     предыдущей стадии (num_prev_samples)
  // |              |                    |              |                     |
  // |              |                    |              |                     |  суффикс файла с семплом
  // |              |                    |              |                     |  текущей стадии (suffix)
  // |              |                    |              |                     |  |
  // |              |                    |              |                     |  |     длительность стадии
  // |              |                    |              |                     |  |     в секундах (duration)
  // |              |                    |              |                     |  |     |
  { STAGE_NONE,    pairs[STAGE_NONE],    STAGE_NONE,    pairs[STAGE_NONE],    0, "",   0},
  { PER_1_MINUTE,  pairs[PER_1_MINUTE],  PER_5_MINUTES, pairs[PER_5_MINUTES], 1, ".0", 60},
  { PER_5_MINUTES, pairs[PER_5_MINUTES], PER_HOUR,      pairs[PER_HOUR],      5, ".1", 300},
  { PER_HOUR,      pairs[PER_HOUR],      PER_DAY,       pairs[PER_DAY],      12, ".2", 3600},
  { PER_DAY,       pairs[PER_DAY],       PER_MONTH,     pairs[PER_MONTH],    24, ".3", 86400},
  { PER_MONTH,     pairs[PER_MONTH],     STAGE_LAST,    pairs[STAGE_LAST],   30, ".4", 2592000} // для 30 дней
};

#if 0
// ===========================================================================
static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
  int i;

  for(i=0; i<argc; i++)
  {
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");

  return 0;
}
#endif


// HIST_DICT table
// ---+-------------+-----------+--------------+
// #  |  Имя поля   | Тип поля  | По умолчанию |
// ---+-------------+-----------+--------------+
// 1  | TagId       | INTEGER   | generated    |<-------+   уник. идентификатор записи
// 2  | Tag         | TEXT      |              |    //  |   название тега точки
// 3  | Depth       | INTEGER   | 1            |    //  |   макс. глубина хранения предыстории
// ---+-------------+-----------+--------------+    //  |   (= макс. значение HistoryType из БДРВ)
const char *Historian::s_SQL_CREATE_TABLE_DICT_TEMPLATE=//|
        "CREATE TABLE IF NOT EXISTS %s"             //  |
        "("                                         //  |
        "TagId INTEGER PRIMARY KEY AUTOINCREMENT,"  //  |
        "Tag TEXT NOT NULL,"                        //  |
        "Depth INTEGER NOT NULL DEFAULT 1"          //  |
        ");";                                       //  |
// HIST table                                           |
// ---+-------------+-----------+--------------+        |
// #  | Имя поля    | Тип поля  | По умолчанию |        |
// ---+-------------+-----------+--------------+        |
// 1  | Timestamp   | INTEGER   |              |        |   отметка времени семпла
// 2  | HistoryType | INTEGER   | 1            |        |   тип истории текущего семпла
// 3  | TagId       | FK        | foreign key  |--------+   ссылка на тег в НСИ
// 4  | VAL         | DOUBLE    |              |            значение атрибута VAL
// 5  | VALID       | INTEGER   | 0            |            значение атрибута VALID
// ---+-------------+-----------+--------------+
const char *Historian::s_SQL_CREATE_TABLE_DATA_TEMPLATE =
        "CREATE TABLE IF NOT EXISTS %s"
        "("
        "Timestamp INTEGER,"
        "HistoryType integer default 1,"
        "TagId INTEGER NOT NULL,"
        "VAL REAL,"
        "VALID INTEGER DEFAULT 0,"
        "PRIMARY KEY (Timestamp,HistoryType,TagId) ON CONFLICT ABORT,"
//        "PRIMARY KEY (Timestamp,HistoryType,TagId) ON CONFLICT REPLACE,"
        "FOREIGN KEY (TagID) REFERENCES HIST_DICT(TagId) ON DELETE CASCADE"
        ");";

const char *Historian::s_SQL_CREATE_INDEX_DATA_TEMPLATE = 
        "CREATE INDEX IF NOT EXISTS TimestampIndex "
        "ON %s (\"Timestamp\" ASC);";

const char *Historian::s_SQL_CHECK_TABLE_TEMPLATE =
        "SELECT count(*) FROM sqlite_master WHERE type='TABLE' AND NAME='%s';";
const char *Historian::s_SQL_INSERT_DICT_HEADER_TEMPLATE = "INSERT INTO %s (Tag, Depth) VALUES\n";
const char *Historian::s_SQL_INSERT_DICT_VALUES_TEMPLATE = "('%s',%d),\n";
const char *Historian::s_SQL_SELECT_DICT_TEMPLATE = "SELECT TagId,Tag,Depth FROM %s;";
const char *Historian::s_SQL_DELETE_DICT_TEMPLATE = "DELETE FROM %s WHERE TagId=%d;";
const char *Historian::s_SQL_INSERT_DATA_ONE_ITEM_HEADER_TEMPLATE =
        "INSERT INTO %s (Timestamp,HistoryType,TagId,VAL,VALID) VALUES\n";
const char *Historian::s_SQL_INSERT_DATA_ONE_ITEM_VALUES_TEMPLATE = "(%ld,%d,%d,%g,%d),";
// Выбрать набор семплов определенного типа для указанного Тега в указанный период времени
const char *Historian::s_SQL_READ_DATA_ITEM_TEMPLATE =
        "SELECT Timestamp,VAL,VALID FROM %s "
        "WHERE TagId=%d "
        "AND HistoryType=%d "
        "AND Timestamp>=%d AND Timestamp<=%d;";
const char *Historian::s_SQL_READ_DATA_AVERAGE_ITEM_TEMPLATE =
        "SELECT AVG(VAL),MAX(VALID) FROM %s "
        "WHERE TagId=%d "
        "AND HistoryType=%d "
        // > && <= потому, что интересующий временной интервал начинается после своего начала,
        // иначе в выборку попадает на один семпл больше
        "AND Timestamp>%d AND Timestamp<=%d;";

// ===========================================================================
Historian::Historian(const char* rtdb_name, const char* hdb_name) :
  m_rtdb(NULL),
  m_rtdb_conn(NULL),
  m_hist_db(NULL),
  m_hist_err(NULL),
  m_history_db_connected(false),
  m_rtdb_connected(false)
{
  assert(strlen(rtdb_name) < SERVICE_NAME_MAXLEN);
  assert(strlen(hdb_name) < SERVICE_NAME_MAXLEN);

  if (rtdb_name && rtdb_name[0]) {
    strncpy(m_rtdb_name, rtdb_name, SERVICE_NAME_MAXLEN);
    m_rtdb_name[SERVICE_NAME_MAXLEN] = '\0';
  }
  else strcpy(m_rtdb_name, RTDB_NAME);

  if (hdb_name && hdb_name[0]) {
    strncpy(m_history_db_filename, hdb_name, SERVICE_NAME_MAXLEN);
    m_history_db_filename[SERVICE_NAME_MAXLEN] = '\0';
  }
  else strcpy(m_history_db_filename, HISTORY_DB_FILENAME);
}

// ===========================================================================
Historian::~Historian()
{
  Disconnect();
}

// ===========================================================================
bool Historian::Connect()
{
  bool status = false;

  // Подключиться к БДРВ
  if (true == (m_rtdb_connected = rtdb_connect()))
  {
    // Получить из БДРВ перечень точек, имеющих предысторию
    if (true == (status = rtdb_get_info_historized_points()))
    {
        // Подключиться к БД Предыстории
        if (true == (m_history_db_connected = history_db_connect()))
        {
          LOG(INFO) << "HDB connection OK";

          // Настроить НСИ, загрузить свежесозданные данные из HDB в m_actual_hist_points
          status = tune_dictionaries();
        }
        else
        {
          LOG(FATAL) << "Unable to connect to HDB";
        }
    }
    else
    {
      LOG(FATAL) << "Unable to load historized points info from RTDB";
    }
  }
  else
  {
    LOG(FATAL) << "Unable to connect to RTDB";
  }

  return status;
}

// ===========================================================================
bool Historian::Disconnect()
{
  bool status = false;

  if (m_rtdb_connected) {
    if (true == (status = rtdb_disconnect())) {
      LOG(INFO) << "Disconnection from RTDB";
    }
    else {
      LOG(INFO) << "Unable to disconnect from RTDB";
    }
  }

  if (m_history_db_connected) {
    if (true == (status = history_db_disconnect())) {
      LOG(INFO) << "Disconnection from HDB";
    }
    else {
      LOG(INFO) << "Unable to disconnect from HDB";
    }
  }

  return status;
}

// ===========================================================================
bool Historian::is_table_exist(sqlite3 *db, const char *table_name)
{
  int retcode;
  bool exists = false;
  sqlite3_stmt* stmt = 0;
  size_t printed;
  char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];

  printed = snprintf(sql_operator,
           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
           s_SQL_CHECK_TABLE_TEMPLATE,
           table_name);
  assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

  retcode = sqlite3_prepare(db, sql_operator, -1, &stmt, 0);
  if(retcode != SQLITE_OK)
  {
    LOG(ERROR) << "Could not execute SELECT";
    return false;
  }

  while(sqlite3_step(stmt) == SQLITE_ROW)
  {
    exists = (1 == sqlite3_column_int(stmt, 0));
  }
  sqlite3_finalize(stmt);

  if (!exists) {
    LOG(WARNING) << "Table " << table_name << " doesn't exist";
  }

  return exists;
}

// ===========================================================================
// Создать индексы, если они ранее не существовали
bool Historian::createIndexes()
{
  const char* fname = "createIndexes";
  bool created = false;
  char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  int printed;
  
  // Таблицы созданы успешно, создадим индексы
  printed = snprintf(sql_operator,
                     MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                     s_SQL_CREATE_INDEX_DATA_TEMPLATE,
                     HDB_DATA_TABLENAME);
  assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

  if (sqlite3_exec(m_hist_db, sql_operator, 0, 0, &m_hist_err)) {
    LOG(ERROR) << fname << ": Creating HDB indexes: " << m_hist_err;
    sqlite3_free(m_hist_err);
  }
  else {
    LOG(INFO) << "HDB indexes successfully created";
    created = true;
  }

  return created;
}

// ===========================================================================
// Создать соответствующую таблицу, если она ранее не существовала
bool Historian::createTable(const char* table_name)
{
  const char* fname = "createTable";
  bool created = true;
  bool known_table = false;
  int printed;
  std::string sql;
  char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];

  if (0 == strcmp(table_name, HDB_DICT_TABLENAME)) {
    // Создать таблицу НСИ
    printed = snprintf(sql_operator,
                       MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                       s_SQL_CREATE_TABLE_DICT_TEMPLATE,
                       HDB_DICT_TABLENAME);
    assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
    known_table = true;
  }
  else if (0 == strcmp(table_name, HDB_DATA_TABLENAME)) {
    // Создать таблицу данных
    printed = snprintf(sql_operator,
                       MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                       s_SQL_CREATE_TABLE_DATA_TEMPLATE,
                       HDB_DATA_TABLENAME);
    assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
    known_table = true;
  }
  else LOG(ERROR) << "Not supported to creating unknown table: " << table_name;

  if (known_table) {
    sql = "BEGIN;\n";
    sql += sql_operator;
    sql += "\nCOMMIT;";

    if (sqlite3_exec(m_hist_db, sql.c_str(), 0, 0, &m_hist_err)) {
      LOG(ERROR) << fname << ": creating HDB table '" << table_name
                 << "': " << m_hist_err;
      sqlite3_free(m_hist_err);
      created = false;
    }
    else LOG(INFO) << "HDB table '" << table_name << "' successfully created";
  }

  return created;
}

// ===========================================================================
bool Historian::history_db_connect()
{
  const char* fname = "history_db_connect";
  int retcode;
  bool status = true;

  /*
   * If the SQLITE_OPEN_NOMUTEX flag is set, then the database connection opens
   * in the multi-thread threading mode as long as the single-thread mode has
   * not been set at compile-time or start-time. If the SQLITE_OPEN_FULLMUTEX
   * flag is set then the database connection opens in the serialized threading
   * mode unless single-thread was previously selected at compile-time or start-time.
   */

  // Подключиться к БД Истории
  retcode = sqlite3_open_v2(m_history_db_filename,
                            &m_hist_db,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                            "unix-dotfile");
  if (retcode == SQLITE_OK)
  {
    LOG(INFO) << fname << ": HDB '" << m_history_db_filename << "' successfuly opened";
    m_history_db_connected = true;

    // Установить кодировку UTF8
    if (sqlite3_exec(m_hist_db, "PRAGMA ENCODING = \"UTF-8\"", 0, 0, &m_hist_err)) {
      LOG(ERROR) << fname << ": " << m_hist_err;
      sqlite3_free(m_hist_err);
      LOG(ERROR) << "Unable to set UTF-8 support for HDB";
    }
    else LOG(INFO) << "Enable UTF-8 support for HDB";

    // Включить поддержку внешних ключей
    if (sqlite3_exec(m_hist_db, "PRAGMA FOREIGN_KEYS = 1", 0, 0, &m_hist_err)) {
      LOG(ERROR) << fname << ": " << m_hist_err;
      sqlite3_free(m_hist_err);
      LOG(INFO) << "Unable to set FOREIGN_KEYS support for HDB";
    }
    else LOG(INFO) << "Enable FOREIGN_KEYS support for HDB";

/*    if (is_table_exist(m_hist_db, HDB_DATA_TABLENAME)) {
    }
    else .c_str()
    }*/

    // TODO: Проверить структуру только что открытой БД.
    // Для этого получить из БДРВ список всех параметров ТИ, для которых указано сохранение истории.
    // Если Историческая база пуста - создать ей новую структуру на основе данных из БДРВ
    // Если Историческая база не пуста, требуется сравнение новой и старой версий списка параметров.
    status = createTable(HDB_DICT_TABLENAME);

    if (true == (status = createTable(HDB_DATA_TABLENAME)))
      status = createIndexes();
  }
  else
  {
    LOG(ERROR) << fname << ": Opening HDB '" << m_history_db_filename << "':  " << sqlite3_errmsg(m_hist_db);
    status = false;
  }

  return status;
}

// ===========================================================================
bool Historian::rtdb_connect()
{
  Options opts;
  xdb::Error err;

  m_rtdb = new xdb::RtDatabase(m_rtdb_name, &opts);

  err = m_rtdb->LoadSnapshot();
  if (err.Ok()) {
    err = m_rtdb->Connect();

    LOG(INFO) << "Connection to " << m_rtdb_name << ": " << err.what();
    if (err.Ok()) {
      m_rtdb_conn = new xdb::RtConnection(m_rtdb);
    }
  }
  return (m_rtdb_conn != NULL);
}

// ===========================================================================
bool Historian::rtdb_disconnect()
{
  delete m_rtdb_conn;
  delete m_rtdb;
  m_rtdb_connected = false;

  return true;
}

// ===========================================================================
bool Historian::history_db_disconnect()
{
  bool status = false;

  if (m_history_db_connected) {
    status = sqlite3_close(m_hist_db);
    m_history_db_connected = (status == true)? false : true;
  }
  else LOG(ERROR) << "Try to close not connected history database";

  return status;
}

// ===========================================================================
void Historian::accelerate(bool speedup)
{
  const char* fname = "accelerate";
  const char* sync_FULL = "FULL";
  const char* sync_NORM = "NORMAL";
  const char* sync_OFF  = "OFF";
  const char* journal_DELETE = "DELETE"; // сброс лога транзакции на диск после каждой операции
  const char* journal_MEMORY = "MEMORY"; // лог транзакции не сбрасывается на диск, а ведется в ОЗУ
  const char* journal_OFF = "OFF";       // лог транзакции не ведется, быстрый и ненадежный способ
  // Возможные значения JOURNAL_MODE: DELETE | TRUNCATE | PERSIST | MEMORY | WAL | OFF
  // По умолчанию: DELETE
  const char* pragma_set_journal_mode_template = "PRAGMA JOURNAL_MODE = %s";
  // Возможные значения SYNCHRONOUS: 0 | OFF | 1 | NORMAL | 2 | FULL
  // По умолчанию: FULL
  const char* pragma_set_synchronous_template = "PRAGMA SYNCHRONOUS = %s";
  char sql_operator_journal[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  char sql_operator_synchronous[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  int printed;

  // Для начальной вsql_operator_journalставки больших объёмов данных в пустую БД (когда не жалко потерять данные)
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

  switch (speedup) {
      case true:
        printed = snprintf(sql_operator_journal,
                           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                           pragma_set_journal_mode_template,
        // Ослабить нагрузку на диск, журнал не будет скидываться в файл после каждой операции
        //                   journal_MEMORY);
                           journal_OFF);
        assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

        printed = snprintf(sql_operator_synchronous,
                           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                           pragma_set_synchronous_template,
        // Поднять производительность за счёт отказа от полной гарантии консистентности данных
        //                   sync_NORM);
                           sync_OFF);
        assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
        break;

      case false:
        printed = snprintf(sql_operator_journal,
                           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                           pragma_set_journal_mode_template,
                           journal_DELETE);
        assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

        printed = snprintf(sql_operator_synchronous,
                           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                           pragma_set_synchronous_template,
                           sync_FULL);
        assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
        break;
  }

  if (sqlite3_exec(m_hist_db, sql_operator_journal, 0, 0, &m_hist_err)) {
    LOG(ERROR) << fname << ": " << m_hist_err;
    sqlite3_free(m_hist_err);
    LOG(ERROR) << "Unable to change JOURNAL_MODE";
  }
  else LOG(INFO) << "JOURNAL_MODE is changed";

  if (sqlite3_exec(m_hist_db, sql_operator_synchronous, 0, 0, &m_hist_err)) {
    LOG(ERROR) << fname << ": " << m_hist_err;
    sqlite3_free(m_hist_err);
    LOG(ERROR) << "Unable to change SYNCHRONOUS mode";
  }
  else LOG(INFO) << "SYNCHRONOUS mode is changed";
}

// ===========================================================================
bool Historian::tune_dictionaries()
{
  bool result = true;
  const char* fname = "tune_dictionaries";
  int dict_size = 0;

  // Установить щадящий режим занесения данных, отключив сброс журнала на диск
  accelerate(true);

  // Загрузить в m_actual_hist_points содержимое таблицы данных HDB
  assert(m_actual_hist_points.empty());
  dict_size = getPointsFromDictHDB(m_actual_hist_points);

  // Синхронизация при старте баз данных HDB и RTDB
  // 1) найти и удалить точки, присутствующие в HDB, но отсутствующие в БДРВ
  // 2) найти и создать точки, отсутствующие в HDB, но присутствующие в БДРВ

  LOG(INFO) << "Select " << dict_size << " actual history points";
  if (!dict_size) {
    // HDB пуста, значит можно без проверки скопировать все из RTDB
    m_need_to_create_hist_points = m_actual_rtdb_points;
    m_need_to_delete_hist_points.clear();
  }
  else {
    // В снимке HDB уже присутствуют какие-то Точки.
    // Найти разницу между списками m_actual_rtdb_points и m_actual_hist_points,
    // включив в результат все то, что есть в первом списке, но нет во втором.
    calcDifferences(m_actual_rtdb_points, m_actual_hist_points, m_need_to_create_hist_points);
    calcDifferences(m_actual_hist_points, m_actual_rtdb_points, m_need_to_delete_hist_points);
  }

  LOG(INFO) << "size of need_to_create_rtdb_points=" << m_need_to_create_hist_points.size();
  LOG(INFO) << "size of need_to_delete_rtdb_points=" << m_need_to_delete_hist_points.size();

  // Сразу после этого заполнить массив m_actual_hist_points актуальной информацией
  createPointsDictHDB(m_need_to_create_hist_points);
  getPointsFromDictHDB(m_actual_hist_points);

  deletePointsDictHDB(m_need_to_delete_hist_points);

  accelerate(false);

  return result;
}

// ===========================================================================
// В результат (третий параметр) идет только тот тег, который присутствует
// в первом наборе, но отсутствует во втором.
int Historian::calcDifferences(map_history_info_t& whole,  /* входящий общий набор тегов */
                    map_history_info_t& part,   /* входящий пересекающий набор тегов */
                    map_history_info_t& result) /* исходящий набор пересечения */
{
  map_history_info_t::iterator it;
  map_history_info_t::iterator lookup;

  for (it = whole.begin(); it != whole.end(); it++) {

    // Найти Тег из первого множества во втором множестве
    lookup = part.find((*it).first);
    if (lookup == part.end())
    {
      // Не нашли
      result.insert(*it);
      LOG(INFO) << "differ : " << (*it).first;
    }

  }

  return result.size();
}

// ===========================================================================
// Выбрать из существующей таблицы SQLITE старые значения тегов
// Необходимо для того, чтобы корректно обновить НСИ в таблице истории,
// не удаляя теги, существующие в старой и новой версиях БД.
int Historian::getPointsFromDictHDB(map_history_info_t &actual_hist_points)
{
  const char *fname = "getPointsFromDictHDB";
  char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  int printed;
  bool complete = false;
  sqlite3_stmt* stmt = 0;
  // Ячейки таблицы HIST_DICT:
  history_info_t info;
  std::string tag;  // название тега

  actual_hist_points.clear();

  do {
    printed = snprintf(sql_operator,
                       MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                       s_SQL_SELECT_DICT_TEMPLATE,
                       HDB_DICT_TABLENAME);
    assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

    // Запросить старое содержимое НСИ HDB
    if (sqlite3_prepare(m_hist_db, sql_operator, -1, &stmt, 0) != SQLITE_OK) {
      LOG(ERROR) << fname << ": Could not select from " << HDB_DICT_TABLENAME;
      break;
    }

    // Поместить словарь тегов в actual_hist_points
    while(sqlite3_step(stmt) == SQLITE_ROW) {
      info.tag_id = sqlite3_column_int(stmt, 0);
      tag = (const char*)(sqlite3_column_text(stmt, 1));
      info.history_type = sqlite3_column_int(stmt, 2);
#ifdef VERBOSE
      LOG(INFO) << "load actual hist id=" << info.tag_id << " '" << tag << "' htype=" << info.history_type;
#endif
      actual_hist_points.insert(std::pair<std::string, history_info_t>(tag, info));
    }
    sqlite3_finalize(stmt);

    LOG(INFO) << "Succesfully load " << actual_hist_points.size() << " actual HDB tag(s)";
    complete = true;

  } while (false);

  if (!complete) {
    LOG(ERROR) << "Failure loading from previous version of HDB";
  }

  return actual_hist_points.size();
}

// ===========================================================================
// Создать НСИ HIST_DICT на основе связки "Тег" <-> { "идентификатор тега","глубина истории" }
int Historian::createPointsDictHDB(map_history_info_t& points_to_create)
{
  static const char *fname = "createPointsDictHDB";
  int idx = 0;
  int idx_last_item = points_to_create.size();
  std::string sql;
  char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  int printed;

  LOG(INFO) << "Prepare to create " << points_to_create.size() << " point(s)";
  if (points_to_create.size())
  {
    // Начать транзакцию
    sql = "BEGIN;\n";

    printed = snprintf(sql_operator,
                       MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                       s_SQL_INSERT_DICT_HEADER_TEMPLATE,
                       HDB_DICT_TABLENAME);
    assert (printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
    sql += sql_operator;

    for (map_history_info_t::iterator it = points_to_create.begin();
         it != points_to_create.end();
         it++)
    {
      // если не последняя запись, добавить символ ','
      printed = snprintf(sql_operator,
                         MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                         s_SQL_INSERT_DICT_VALUES_TEMPLATE,
                         (*it).first.c_str(),
                         (*it).second.history_type);
      assert (printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

      // Если это последняя запись, заменить конец строки из ",\n\0" на ";\0"
      if (idx == (idx_last_item - 1)) {
        sql_operator[strlen(sql_operator)-2] = ';';
        sql_operator[strlen(sql_operator)-1] = '\0';
      }

      sql += sql_operator;
      LOG(INFO) << "create: " << ++idx<< "\t " << (*it).first
		<< "\ttag_id=" << (*it).second.tag_id
		<< "\thist_type=" << (*it).second.history_type;
    }

    // Закрыть транзакцию
    sql += "\nCOMMIT;";

    if (sqlite3_exec(m_hist_db, sql.c_str(), 0, 0, &m_hist_err)) {
       LOG(ERROR) << fname << ": " << m_hist_err;
       sqlite3_free(m_hist_err);
    } else LOG(INFO) << fname << ": Create " << idx_last_item << " point(s)";

#ifdef VERBOSE
    LOG(INFO) << fname << ": SQL: " << sql;
#endif
  } // Конец блока, если были данные для вставки

  return idx;
}

// ===========================================================================
// Удалить данные из таблицы HIST с заданными идентификаторами
int Historian::deletePointsDataHDB(map_history_info_t& points_to_delete)
{
  const char* fname = "deletePointsDataHDB";
  int idx = 0;
  std::string sql;
  char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  int printed;

  LOG(INFO) << "Prepare to delete " << points_to_delete.size() << " point(s) from data table";
  if (points_to_delete.size()) {

    // Начать транзакцию
    sql = "BEGIN;\n";

    for (map_history_info_t::iterator it = points_to_delete.begin();
         it != points_to_delete.end();
         it++)
    {
      LOG(INFO) << "delete all: " << idx+1 << "\t " << (*it).first
                << "\ttag_id=" << (*it).second.tag_id;

      if (0 == idx) {
        // если это первая запись
        printed = snprintf(sql_operator,
                           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                           s_SQL_DELETE_DICT_TEMPLATE,
                           HDB_DICT_TABLENAME,
                           (*it).second.tag_id);
        assert (printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
      }
      else {
        // если это вторая или последующие записи
        printed = snprintf(sql_operator,
                           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                           " or TagId=%d",
                           (*it).second.tag_id);
        assert (printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
      }

      idx++;
      sql += sql_operator;
    }

    // Закончить транзакцию
    sql += ";\nCOMMIT;";

    if (sqlite3_exec(m_hist_db, sql.c_str(), 0, 0, &m_hist_err)) {
       LOG(ERROR) << fname << ": executing '" << sql << "': " << m_hist_err;
       sqlite3_free(m_hist_err);
    } else LOG(INFO) << fname << ": Delete " << idx << " unnecessary point(s)";

#if VERBOSE
    LOG(INFO) << fname << ": SQL=" << sql;
#endif
  }

  return idx;
}

// ===========================================================================
// Удалить данные из таблицы HIST_DICT с заданными идентификаторами
int Historian::deletePointsDictHDB(map_history_info_t& points_to_delete)
{
  const char* fname = "deletePointsDictHDB";
  int idx = 0;
  std::string sql;
  char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  int printed;

  LOG(INFO) << "Prepare to delete " << points_to_delete.size() << " point(s) from dictionary";
  if (points_to_delete.size()) {

    // На случай, если внешние ключи не работают, сперва необходимо удалить
    // записи с нужными TagId из таблицы HIST
    deletePointsDataHDB(points_to_delete);

    // Начать транзакцию
    sql = "BEGIN;\n";

    for (map_history_info_t::iterator it = points_to_delete.begin();
         it != points_to_delete.end();
         it++)
    {
      LOG(INFO) << "delete: " << idx+1 << "\t " << (*it).first
                << "\ttag_id=" << (*it).second.tag_id
                << "\thisttype=" << (*it).second.history_type;

      if (0 == idx) {
        // если это первая запись
        printed = snprintf(sql_operator,
                           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                           s_SQL_DELETE_DICT_TEMPLATE,
                           HDB_DICT_TABLENAME,
                           (*it).second.tag_id);
        assert (printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
      }
      else {
        // если это вторая или последующие записи
        printed = snprintf(sql_operator,
                           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                           " or TagId=%d",
                           (*it).second.tag_id);
        assert (printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);
      }

      idx++;
      sql += sql_operator;
    }

    // Закончить транзакцию
    sql += ";\nCOMMIT;";

    if (sqlite3_exec(m_hist_db, sql.c_str(), 0, 0, &m_hist_err)) {
       LOG(ERROR) << fname << ": executing '" << sql << "': " << m_hist_err;
       sqlite3_free(m_hist_err);
    } else LOG(INFO) << fname << ": Delete " << idx << " unnecessary point(s)";

#if VERBOSE
    LOG(INFO) << fname << ": SQL=" << sql;
#endif
  }

  return idx;
}

// ===========================================================================
// Получить из БДРВ перечень точек, имеющих предысторию
// NB: Есть два варианта получения данных из БДРВ:
// 1) Службы RTDB и HIST работают локально, на одном сервере;
// В этом случаее можно получать данные из БДРВ, напрямую подключаясь к ней.
// Классы RtApplication и т.п. при этом использовать не обязательно.
// 2) Службы RTDB и HIST работают на разных серверах;
// В этом случае для получения конфигурации и минутных срезов данных необходимо
// использовать запрос к Службе БДРВ (SINF). Средняя нагрузка на сеть возрастет
// незначительно, однако будут пики нагрузки в момент получения мгновенного
// среза данных.
bool Historian::rtdb_get_info_historized_points()
{
  bool status = false;
  history_info_t info;

  // К этому моменту список точек из БДРВ еще д.б. пуст
  assert(!m_actual_rtdb_points.size());

#if (HIST_DATABASE_LOCALIZATION==LOCAL)
  status = local_rtdb_get_info_historized_points(m_raw_actual_rtdb_points);
#elif (HIST_DATABASE_LOCALIZATION==DISTANT)
  status = remote_rtdb_get_info_historized_points(m_raw_actual_rtdb_points);
#else
#error "Unknown HIST localization (neither LOCAL nor DISTANT)"
#endif

  // Сконвертировать данные из формата словаря БДРВ "Тег" <-> "Глубина истории"
  // в формат "Тег" <-> { "Глубина истории", "Идентификатор тега в HDB" }
  for (xdb::map_name_id_t::iterator it = m_raw_actual_rtdb_points.begin();
       it != m_raw_actual_rtdb_points.end();
       it++)
  {
     info.tag_id = 0; // пустое значение, т.к. из БДРВ приходит только глубина истории и тег.
     info.history_type = (*it).second;
#ifdef VERBOSE
     LOG(INFO) << "load actual RTDB id=" << info.tag_id << " '" << (*it).first << "' htype=" << info.history_type;
#endif
     m_actual_rtdb_points.insert(std::pair<std::string, history_info_t>((*it).first, info));
  }

  if (status)
  {
    LOG(INFO) << "Historized " << m_actual_rtdb_points.size() << " point(s)";
  }

  return status;
}

// ===========================================================================
#if (HIST_DATABASE_LOCALIZATION==LOCAL)
bool Historian::local_rtdb_get_info_historized_points(xdb::map_name_id_t& points)
{
  const char *fname = "local_rtdb_get_info_historized_points";
  xdb::rtDbCq operation;
  xdb::Error result;
  xdb::category_type_t ctype = CATEGORY_HAS_HISTORY;

  // Получить список идентификаторов Точек с заданным в addrCnt значением категории
  operation.action.query = xdb::rtQUERY_PTS_IN_CATEG;
  operation.buffer = &points;
  // Плохо - передача значения Категории через атрибут, предназначенный для другой цели
  operation.addrCnt = ctype;
  result = m_rtdb_conn->QueryDatabase(operation);

  if (!result.Ok()) {
    LOG(ERROR) << fname << ": Query XDB failed: " << result.what();
  }

  return (true == result.Ok());
}
#endif

// ===========================================================================
#if (HIST_DATABASE_LOCALIZATION==DISTANT)
bool Historian::remote_rtdb_get_info_historized_points(xdb::map_name_id_t& points)
{
  bool status = false;
  LOG(FATAL) << "Not implemented";
  assert (0 == 1);
  return status;
}
#endif

// ===========================================================================
// Занести все историзированные значения в HDB
int Historian::store_history_samples(HistorizedInfoMap_t& historized_map, sampler_type_t store_only_htype)
{
  static const char* fname = "store_history_samples";
  static char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  int num_samples = 0; // Индекс элемента в списке на сохранение
  int sql_err_code;
  std::string sql;
  int printed;
  // "(%ld,%d,%d,%g,%d),"
  // strlen("(9999999999,9,1000000,1234567.89,1),") = 36
  // strlen(заголовок SQL-запроса) = 100

  if (historized_map.size() > 0) { // Есть данные к сохранению

    int estimated_sql_operator_size = ((historized_map.size() * 36) + 100);
    LOG(INFO) << "estimated_sql_operator_size=" << estimated_sql_operator_size
             << " for " << historized_map.size() << " point(s)";
    sql.reserve(estimated_sql_operator_size);

    // Команда на открытие транзакции
    // TODO: если сохраняемых элементов много, следует разбить их сохранение на несколько транзакций
    sql = "BEGIN;\n";

    // Формирование заголовка на вставку данных
    printed = snprintf(sql_operator,
            MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
            s_SQL_INSERT_DATA_ONE_ITEM_HEADER_TEMPLATE,
            HDB_DATA_TABLENAME);
    sql += sql_operator;

    // Генерация SQL-запроса на вставку новых элементов в таблицу данных HDB
    for(HistorizedInfoMap_t::iterator it = historized_map.begin();
        it != historized_map.end();
        it++) {

      // Сохранить только те семплы, которые укладываются в допустимый тип
      if ((*it).second.info.history_type >= store_only_htype) {
        printed = snprintf(sql_operator,
            MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
            s_SQL_INSERT_DATA_ONE_ITEM_VALUES_TEMPLATE,
            (*it).second.datehourm,    // Timestamp
            store_only_htype,          // (*it).second.info.history_type, // sampler_type_t
            (*it).second.info.tag_id,  // TagId
            (*it).second.val,          // VAL
            (*it).second.valid);       // VALID
        assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

        num_samples++;
        sql += sql_operator;
      }
#ifdef VERBOSE
      else LOG(INFO) << "GEV: skip point " << (*it).first << " with type=" << (*it).second.info.history_type
                     << " while storing type=" << store_only_htype;
#endif
    }

    // Затереть последний символ ','
    sql.erase(sql.end() - 1, sql.end());

    sql += ";\nCOMMIT;";
  }

#ifdef VERBOSE
  LOG(INFO) << "SQL: " << sql;
#endif

//  accelerate(true);
  if (num_samples) {
    if (SQLITE_OK != (sql_err_code = sqlite3_exec(m_hist_db, sql.c_str(), 0, 0, &m_hist_err))) {
      LOG(ERROR) << fname << ": code=" << sql_err_code << ", " << m_hist_err;
      LOG(INFO) << "SQL: " << sql;
      sqlite3_free(m_hist_err);
    } else LOG(INFO) << "Successfully insert " << num_samples << " row(s) for history type=" << store_only_htype;
  }
  else {
    LOG(WARNING) << "There are no historized points with type=" << store_only_htype;
  }

//  accelerate(false);

  return 0;
}


// ===========================================================================
// Найти идентификатор тега в НСИ HDB
// Кэш НСИ - в наборе m_actual_hist_points
int Historian::getPointIdFromDictHDB(const char* pname)
{
  map_history_info_t::iterator lookup;
  int id = 0; // по умолчанию - ничего не найдено

  lookup = m_actual_hist_points.find(pname);
  if (lookup != m_actual_hist_points.end())
  {
    // Нашли
    id = (*lookup).second.tag_id;
//    LOG(INFO) << "Tag '" << pname << "', TagId=" << id;
  }

  return id;
}

// ===========================================================================
// Сохранить моментальный срез данных указанного типа.
// Используется ранее составленный список точек, имеющих предысторию (m_actual_hist_points)
// Циклически проверяем этот список, и генерируем предысторию только для точек,
// имеющих глубину хранения менее заданной. К примеру, если htype_to_store == PER_DAY,
// то сохранению подлежат все точки, имеющие sampler_type_t от PER_1_MINUTE до PER_DAY.
//
bool Historian::make_history_samples_by_type(const sampler_type_t htype_to_store, time_t start)
{
  bool result = true;
  //int last_sample_num = 0;
  // Используется в HDB для ссылки между HIST и HIST_DICT,
  // для идентификации Тега (содержится в HIST_DICT) по строке из HIST.
  // m_actual_hist_points - соответствия между тегом и его идентификатором в HIST_DICT
  int point_id; // Находится в m_actual_hist_points по тегу
  const char* fname = "make_history_samples_by_type";
  xdb::AttributeInfo_t info_val;
  xdb::AttributeInfo_t info_valid;
  xdb::Error status;
  HistorizedInfoMap_t historized_map;
  historized_attributes_t historized_info;
  xdb::map_name_id_t::iterator it = m_raw_actual_rtdb_points.begin();
  sampler_type_t previous_htype;

  if (PER_1_MINUTE == htype_to_store) {
      // Подготовим массив значений из БДРВ для занесения в таблицу данных Истории
      // .VAL
      // .VALID
      // .DATEHOURM
      // <HistoryType>
      // <TagId>
      while (it != m_raw_actual_rtdb_points.end()) {

        info_val.name = (*it).first + ".VAL";
        status = m_rtdb_conn->read(&info_val);

        if (status.Ok()) {
          info_valid.name = (*it).first + ".VALID";
          status = m_rtdb_conn->read(&info_valid);

          if (status.Ok())
          {
            // Найти идентификатор тега в НСИ HDB
            if (0 == (point_id = getPointIdFromDictHDB((*it).first.c_str()))) {
                LOG(ERROR) << "Unable to find TagId for historized point " << (*it).first;
                // Пропустим эту сбойную Точку
                continue;
            }
            else {
                result = true;
                // Точку нашли в списке активных точек HDB, загрузим её характеристики
                historized_info.val = info_val.value.fixed.val_double;

                // TEST TEST TEST
                if (!historized_info.val) {
                    historized_info.val = sin(point_id) * 10 + within(11);
                }
                // TEST TEST TEST

                historized_info.valid = info_valid.value.fixed.val_uint8;
                historized_info.datehourm = start;
                historized_info.info.history_type = static_cast<sampler_type_t>((*it).second);
                historized_info.info.tag_id = point_id;
                historized_map.insert(std::pair<const std::string, historized_attributes_t>((*it).first, historized_info));
#ifdef VERBOSE
                LOG(INFO) << "prepare to process: " << (*it).first
                          << " VAL=" << historized_info.val
                          << " VALID=" << (unsigned int)historized_info.valid
                          << " DATEHOURM=" << historized_info.datehourm
                          << " HTYPE=" << historized_info.info.history_type
                          << " TAGID=" << historized_info.info.tag_id;
#endif
            }

          } // Конец блока успешного чтения атрибута VALID

        } // Конец блока успешного чтения атрибута VAL

        it++; // переход к следующему тегу

      } // Конец блока кода чтения всех тегов из списка

  } // Конец загрузки значений для глубины истории PER_1_MINUTE, когда значения читаются из БДРВ
  else {
      // TODO: Чтение в historized_map усредненных значений из HDB
      // для всех точек из списка m_raw_actual_rtdb_points
      switch(htype_to_store) {
        case PER_5_MINUTES: previous_htype = PER_1_MINUTE;  break;
        case PER_HOUR:      previous_htype = PER_5_MINUTES; break;
        case PER_DAY:       previous_htype = PER_HOUR;      break;
        case PER_MONTH:     previous_htype = PER_DAY;       break;
        default:
          LOG(ERROR) << "Unsupported history type:" << htype_to_store;
      }
      if (false == load_samples_list_by_type(previous_htype, start, m_raw_actual_rtdb_points, historized_map)) {
        LOG(ERROR) << "Can't load samples from HDB by history type=" << htype_to_store;
        result = false;
      }
  }

  if (result) {
    // Занести все историзированные значения в HDB
    store_history_samples(historized_map, htype_to_store);
  }

  return result;
}

// ===========================================================================
// Сгенерировать новый отсчет предыстории указанной глубиной на основе агрегирования
// ряда семплов предыдущей глубины.
bool Historian::select_avg_values_from_sample(historized_attributes_t& historized_info,
                                              const sampler_type_t htype_to_load)
{
  const char* fname = "select_avg_values_from_sample";
  sqlite3_stmt* stmt = 0;
  char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  int retcode;
  size_t printed;
  // Для выборки данных используются разные операторы, и для выборки минутных значений
  // выбирается ещё и datehourm, поэтому при разборе ответа нужно учитывать порядок атрибутов.
  int val_idx = 0, valid_idx = 1;
  // Это отметка времени, с которой нужно читать ранее сохраненные семплы. К примеру, пятиминутный
  // семпл состоит из набора пяти одноминутных семплов, полученных за 5 минут до текущего времени.
  time_t past_time = historized_info.datehourm - m_stages[htype_to_load].duration;

  // Загрузка минутных семплов происходит чуть иначе,
  // скорректируем индексы атрибутов в SQL-ответе
  if (PER_1_MINUTE == htype_to_load) {
      // нулевой индекс - неиспользуемый здесь timestamp
      val_idx   = 1;
      valid_idx = 2;
  }

  printed = snprintf(sql_operator,
           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
           ((PER_1_MINUTE == htype_to_load)?
           s_SQL_READ_DATA_ITEM_TEMPLATE : s_SQL_READ_DATA_AVERAGE_ITEM_TEMPLATE),
           HDB_DATA_TABLENAME,
           historized_info.info.tag_id,
           htype_to_load,
           past_time,
           historized_info.datehourm);

  assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

  retcode = sqlite3_prepare(m_hist_db, sql_operator, -1, &stmt, 0);
  if(retcode != SQLITE_OK)
  {
    LOG(ERROR) << fname << ": Could not execute: " << sql_operator;
    return false;
  }

  while(sqlite3_step(stmt) == SQLITE_ROW)
  {

    historized_info.val   = sqlite3_column_double(stmt, val_idx);
    historized_info.valid = sqlite3_column_int(stmt, valid_idx);
#ifdef VERBOSE
    LOG(INFO) << fname << ": SQL=" << sql_operator
              << "; VAL=" << historized_info.val
              << "; VALID=" << (unsigned int)historized_info.valid;
#endif
  }

  return true;
}

// ===========================================================================
// Загрузить из HDB средние значения по всем тегам сразу из списка raw_actual_rtdb_points
bool Historian::load_samples_list_by_type(const sampler_type_t htype_to_load,
                           time_t period_start,
                           xdb::map_name_id_t raw_actual_rtdb_points,
                           HistorizedInfoMap_t& historized_map)
{
  const char *fname = "load_samples_list_by_type";
  bool result = true;
  int last_sample_num = 0;
  time_t period_finish = period_start;
  time_t delta = period_start;
  std::string sql;
  xdb::map_name_id_t::iterator it = m_raw_actual_rtdb_points.begin();
  historized_attributes_t historized_info;

  while (it != m_raw_actual_rtdb_points.end()) {

    // Глубина хранения предыстории этой точки ((*it).second) превышает требуемую глубину
    // (htype_to_load) для выборки данных, значит будем сейчас собирать семплы
    if ((*it).second /* history_type */ >= htype_to_load) {
      last_sample_num++;
      //1 LOG(INFO) << "use tag " << (*it).first << " with orig htype=" << (*it).second << ", current htype=" << htype_to_load;
      // Для данной точки прочитать:
      // 1) среднее значение VAL за период
      // 2) максимальное значение VALID за период
      // Период определяется от начала до конца предыдущего семпла истории (от period_start до delta)

      switch (htype_to_load) {
        case PER_1_MINUTE:
        // история ежеминутной глубины собирается без агрегирования, непосредственно из таблицы данных
        case PER_5_MINUTES:
        case PER_HOUR:
        case PER_DAY:
        case PER_MONTH:
          //1 delta += m_stages[htype_to_load].duration;

          // Отметка времени нового периода равна последней отметки времени периода семпла предыдущего типа.
          // К примеру, отметка времени часовой истории берется равной 12-ой в серии из отметок времени
          // пятиминутной истории.
          historized_info.datehourm = delta;
          historized_info.info.history_type = (*it).second; //htype_to_load;
          historized_info.info.tag_id = getPointIdFromDictHDB((*it).first.c_str());

          if (true == select_avg_values_from_sample(historized_info, htype_to_load)) {
            historized_map.insert(std::pair<const std::string, historized_attributes_t>((*it).first, historized_info));
          }
          else {
            LOG(WARNING) << "Unable to load average sample type " << htype_to_load << " for point " << (*it).second;
          }
          break;

        default:
          LOG(ERROR) << fname << ": unsupported history type: " << htype_to_load;
          return false;
      }

      // Рассчитали окончание периода истории, участвующего в формировании запрашиваемого типа семпла
      period_finish += delta;
    }

    it++;
  } while (false);

  LOG(INFO) << "I " << fname << ": load " << last_sample_num << " history records with type " << htype_to_load;

  return result;
}


// ===========================================================================
int Historian::readValuesFromDataHDB(int         point_id, // Id точки
       sampler_type_t htype,    // Тип читаемой истории
       time_t      start,       // Начало диапазона читаемой истории
       time_t      finish,      // Конец диапазона читаемой истории
       historized_attributes_t* items) // Прочитанные данные семплов
{
  sqlite3_stmt* stmt = 0;
  char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  int retcode;
  size_t printed;
  int row_count = 0;

  assert(items);
  printed = snprintf(sql_operator,
           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
           s_SQL_READ_DATA_ITEM_TEMPLATE,
           HDB_DATA_TABLENAME,
           point_id,
           htype,
           start,
           finish);
  assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

  retcode = sqlite3_prepare(m_hist_db, sql_operator, -1, &stmt, 0);
  if(retcode != SQLITE_OK)
  {
    LOG(ERROR) << "Could not execute SELECT";
    return 0;
  }

  while(sqlite3_step(stmt) == SQLITE_ROW)
  {
    items[row_count].datehourm = sqlite3_column_int(stmt, 0);
    items[row_count].val = sqlite3_column_double(stmt, 1);
    items[row_count].valid = sqlite3_column_int(stmt, 2);
    items[row_count].info.tag_id = point_id;
    items[row_count].info.history_type = htype;

#ifdef VERBOSE
    LOG(INFO) << "SELECT: " << row_count
              << ":" << items[row_count].datehourm
              << ":" << items[row_count].val
              << ":" << (unsigned int)items[row_count].valid;
#endif
    row_count++;
  }
  sqlite3_finalize(stmt);
  LOG(INFO) << "SQL return " << row_count << " row(s): " << sql_operator;

  return row_count;
}


// ===========================================================================
// Если значение attach == true, округления в большую сторону не происходит
time_t Historian::roundTimeToNextEdge(time_t given_time, sampler_type_t htype, bool attach)
{
  time_t round_time = given_time;
  struct tm edge;
  div_t rest;

  localtime_r(&given_time, &edge);
  // все отметки времени проходят по границе минуты
//  edge.tm_sec = 0;

  // Дальнейшее округление зависит от желаемого типа истории
  switch(htype) {
      case PER_1_MINUTE:
        // округления по границы минуты достаточно
        edge.tm_sec = 0;
        if (attach) // притянуть время к следущей минуте?
          edge.tm_min++;
        break;

      case PER_5_MINUTES:
        // округлим до ближайшей минуты, делящейся на 5
        edge.tm_sec = 0;
        rest = div(edge.tm_min,5);
        if (attach && rest.rem)
          edge.tm_min += 5 - rest.rem;
        break;

      case PER_HOUR:
        // округлим до ближайшего часа
        edge.tm_sec = 0;
        edge.tm_min = 0;
        if (attach)
          edge.tm_hour++;
        break;

      case PER_DAY:
        // округлим до ближайшего дня
        edge.tm_sec = 0;
        edge.tm_min = 0;
        edge.tm_hour = 0;
        if (attach)
          edge.tm_mday++;
        break;

      case PER_MONTH:
        // округлим до ближайшего месяца
        edge.tm_sec = 0;
        edge.tm_min = 0;
        edge.tm_hour = 0;
        edge.tm_mday = 1;
        if (attach)
          edge.tm_mon++;
        break;

      case STAGE_NONE:
      case STAGE_LAST:
        std::cout << "Бяка" << std::endl;
        break;
  }

  round_time = mktime(&edge);

  return round_time;
}

// ===========================================================================
// Найти последовательность исторических семплов для точки с указанным именем
// (pname), начинающуюся с момента времени (start), и состоящую из указанного
// количества (n_samples)
bool Historian::load_samples_period_per_tag(const char* pname,
                                            const sampler_type_t htype,
                                            time_t start,
                                            const int n_samples)
{
  bool status = true;
  const char* fname = "load_samples_period_per_tag";
  time_t frame = start;
  int point_id = 0;
  time_t delta;
  time_t finish;
  int read_samples;
  int samples_to_read = n_samples;
  // Индекс массива прочитанных из предыстории значений
  int historized_idx;

  switch (htype) {
    case PER_1_MINUTE:  delta = 60 * 1;  break;
    case PER_5_MINUTES: delta = 60 * 5;  break;
    case PER_HOUR:      delta = 60 * 60; break;
    case PER_DAY:       delta = 60 * 60 * 24; break;
    case PER_MONTH:     delta = 60 * 60 * 24 * 31; break;
    default:
      LOG(ERROR) << fname << ": unsupported history type: " << htype;
      return false;
  }

  if (samples_to_read > MAX_PORTION_SIZE_LOADED_HISTORY) {
    LOG(WARNING) << "Requested size (" << samples_to_read
                 << ") of readed historized items exceeds limit="
                 << MAX_PORTION_SIZE_LOADED_HISTORY;
    samples_to_read = MAX_PORTION_SIZE_LOADED_HISTORY;
  }

  if (0 == (point_id = getPointIdFromDictHDB(pname))) {
    LOG(ERROR) << "Unable to find point " << pname << " in HDB";
    status = false;
   }
   else {
    finish = start + (delta * (samples_to_read - 1));

    // Все по Точке нашли, теперь залезем в таблицу HIST
    // Прочитать из HIST набор значений VAL и VALID, за указанный диапазон дат
    read_samples = readValuesFromDataHDB(point_id,    // Id точки
                            htype,       // Тип читаемой истории
                            start,       // Начало диапазона читаемой истории
                            finish,      // Конец диапазона читаемой истории
                            m_historized); // Прочитанные данные семплов

    if (read_samples) {
      // После чтения необходимо проверить, что последовательность VAL и VALID не нарушена,
      // то есть не имеет разрывов во времени. Разрывы заполняются специальным образом (VALID=?)
      historized_idx = 0;
      // Поскольку при чтении данных можно задавать произвольную дату начала интервала,
      // следует выравнивать начало поискового диапазона до ближайшего корректного.
      // К примеру, если для часовых семплов задано время 12:10, то нужно округлить
      // его до ближайшего часа, до 13:00
      for (frame = roundTimeToNextEdge(start, htype, false); frame <= finish; frame += delta) {

        if (frame == m_historized[historized_idx].datehourm) {
            // Данный семпл для текущего времени имеет сохраненное значение,
            // то есть нет имеет пропуска
            LOG(INFO) << "Предыстория ЕСТЬ " << pname << ": VAL=" << m_historized[historized_idx].val
                      << " VALID=" << (unsigned int)m_historized[historized_idx].valid
                      << " TIME(" << frame << ")=" << ctime(&frame);
            historized_idx++;
        }
        else if (frame < m_historized[historized_idx].datehourm) {
            // Если проверяемый фрейм меньше следующего реально сохраненного семпла, значит
            // был пропуск в накоплении статистики, и следует обозначить "разрыв" предыстории
            LOG(INFO) << "Предыстория НЕТ " << pname << ": VAL=????"
                      << " VALID=INVALID"
                      << " TIME(" << frame << ")=" << ctime(&frame);
        }
        else if (0 == m_historized[historized_idx].datehourm) { // прочитано нулевое время
            // Это значит в HDB не было части данных за это время
            LOG(WARNING) << "Предыстория ПУСТО " << pname
                         << " TIME(" << frame << ")=" << ctime(&frame);
        }
        else { // прочитано значение из середины диапазона?
            // Нарушение логики хранения данных
            LOG(ERROR) << "бяка: Нарушение логики - при выборке данных истории типа " << htype
                       << " за период " << start << ":" << finish
                       << " встретился семпл с отметкой времени "
                       << m_historized[historized_idx].datehourm;
            assert(0 == 1);
        }

        if (!status) {
            LOG(WARNING) << fname << ": can't read '" << pname
                      << "' sample period for " << frame << " sec ";
            // TODO: Пропускать сбойный отсчёт?
        }
      } // Конец цикла проверки непрерывности временнЫх интервалов
    } // Конец блока, если была прочитана хотябы одна запись
  }

  return status;
}


// ===========================================================================
// Обсчитать новые значения из предыдущего цикла и занести их в виде предыстории нового типа
//   Заполнить тестовыми данными таблицу HIST для семплов с тегами из m_actual_hist_points
//   за период с начала теста по его конец.
bool Historian::Make(time_t current)
{
  struct tm edge;
  bool sampling_status = false;
  sampler_type_t now_processing;
  

  // перевести синтетическое время к представлению часы, минуты, секунды
  // для анализа начала отметок формиования предысторий различных глубин
  // 
  localtime_r(&current, &edge);

  // Только на границе минуты
  assert(edge.tm_sec == 0);

  // Получить мгновенные значения и занести их как минутные семплы
  sampling_status = make_history_samples_by_type(PER_1_MINUTE, current);
  LOG(INFO) << "HDB sampling " << ((true == sampling_status)? "SUCCESS" : "FAILURE")
            << ", htype=" << PER_1_MINUTE
            << ", period=" << current;

  // Проверки времени начала цикла должны идти друг за другом, потому что некоторые циклы
  // возникают на одних и тех же границах (к примеру, каждый двеннадцатый пятиминутный и часовой)
  if (!(edge.tm_min % 5)) {
      // Посчитать новый 5-минутный семпл из ранее изготовленных минутных
      sampling_status = make_history_samples_by_type(PER_5_MINUTES, current);
      LOG(INFO) << "HDB sampling " << ((true == sampling_status)? "SUCCESS" : "FAILURE")
                << ", htype=" << PER_5_MINUTES
                << ", period=" << current;
  }

  if (0 == edge.tm_min) {
      sampling_status = make_history_samples_by_type(PER_HOUR, current);
      LOG(INFO) << "HDB sampling " << ((true == sampling_status)? "SUCCESS" : "FAILURE")
                << ", htype=" << PER_HOUR
                << ", period=" << current;
  }

  if ((0 == edge.tm_min) && (0 == edge.tm_hour)) {
      sampling_status = make_history_samples_by_type(PER_DAY, current);
      LOG(INFO) << "HDB sampling " << ((true == sampling_status)? "SUCCESS" : "FAILURE")
                << ", htype=" << PER_DAY
                << ", period=" << current;
  }

  if ((0 == edge.tm_min) && (0 == edge.tm_hour) && (1 == edge.tm_mday)) {
      sampling_status = make_history_samples_by_type(PER_MONTH, current);
      LOG(INFO) << "HDB sampling " << ((true == sampling_status)? "SUCCESS" : "FAILURE")
                << ", htype=" << PER_MONTH
                << ", period=" << current;
  }

  return sampling_status;
}

// ===========================================================================

