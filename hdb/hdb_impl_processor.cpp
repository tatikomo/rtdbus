/*
 *
 */
#include <iostream>
#include <algorithm>    // std::sort
#include <vector>       // std::vector

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>       // sin

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

#include "sqlite3.h"

// Массив переходов от текущего типа собирателя к следующему
const state_pair_t m_stages[xdb::PERIOD_LAST + 1] = {
  // текущая стадия (current)
  // |                     предыдущая стадия (prev)
  // |                     |                    следующая стадия (next)
  // |                     |                    |                       анализируемых отсчетов
  // |                     |                    |                       | предыдущей стадии (num_prev_samples)
  // |                     |                    |                       |   длительность стадии в секундах
  // |                     |                    |                       |   | (duration)
  // |                     |                    |                       |   |
  { xdb::PERIOD_NONE,      xdb::PERIOD_NONE,     xdb::PERIOD_NONE,      0,  0},
  { xdb::PERIOD_1_MINUTE,  xdb::PERIOD_NONE,     xdb::PERIOD_5_MINUTES, 1,  60},
  { xdb::PERIOD_5_MINUTES, xdb::PERIOD_1_MINUTE, xdb::PERIOD_HOUR,      5,  300},
  { xdb::PERIOD_HOUR,      xdb::PERIOD_5_MINUTES,xdb::PERIOD_DAY,      12,  3600},
  { xdb::PERIOD_DAY,       xdb::PERIOD_HOUR,     xdb::PERIOD_MONTH,    24,  86400},
  { xdb::PERIOD_MONTH,     xdb::PERIOD_DAY,      xdb::PERIOD_LAST,     30,  2592000}, // для 30 дней
  { xdb::PERIOD_LAST,      xdb::PERIOD_MONTH,    xdb::PERIOD_LAST,      0,  0}
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

/*
 * http://www.sqlite.org/lang_conflict.html
 *
ROLLBACK

    When an applicable constraint violation occurs, the ROLLBACK resolution algorithm
    aborts the current SQL statement with an SQLITE_CONSTRAINT error and rolls back
    the current transaction. If no transaction is active (other than the implied
    transaction that is created on every command) then the ROLLBACK resolution
    algorithm works the same as the ABORT algorithm.
ABORT

    When an applicable constraint violation occurs, the ABORT resolution algorithm
    aborts the current SQL statement with an SQLITE_CONSTRAINT error and backs out
    any changes made by the current SQL statement; but changes caused by prior SQL
    statements within the same transaction are preserved and the transaction remains
    active. This is the default behavior and the behavior specified by the SQL standard.
FAIL

    When an applicable constraint violation occurs, the FAIL resolution algorithm aborts
    the current SQL statement with an SQLITE_CONSTRAINT error. But the FAIL resolution
    does not back out prior changes of the SQL statement that failed nor does it end the
    transaction. For example, if an UPDATE statement encountered a constraint violation
    on the 100th row that it attempts to update, then the first 99 row changes are
    preserved but changes to rows 100 and beyond never occur.
IGNORE

    When an applicable constraint violation occurs, the IGNORE resolution algorithm skips
    the one row that contains the constraint violation and continues processing subsequent
    rows of the SQL statement as if nothing went wrong. Other rows before and after the
    row that contained the constraint violation are inserted or updated normally. No error
    is returned when the IGNORE conflict resolution algorithm is used.
REPLACE

    When a UNIQUE or PRIMARY KEY constraint violation occurs, the REPLACE algorithm
    deletes pre-existing rows that are causing the constraint violation prior to inserting
    or updating the current row and the command continues executing normally. If a NOT NULL
    constraint violation occurs, the REPLACE conflict resolution replaces the NULL value
    with the default value for that column, or if the column has no default value, then
    the ABORT algorithm is used. If a CHECK constraint violation occurs, the REPLACE
    conflict resolution algorithm always works like ABORT.

    When the REPLACE conflict resolution strategy deletes rows in order to satisfy a
    constraint, delete triggers fire if and only if recursive triggers are enabled.

    The update hook is not invoked for rows that are deleted by the REPLACE conflict
    resolution strategy. Nor does REPLACE increment the change counter. The exceptional
    behaviors defined in this paragraph might change in a future release.
*/

// HIST_DICT table
// ---+-------------+-----------+--------------+
// #  |  Имя поля   | Тип поля  | По умолчанию |
// ---+-------------+-----------+--------------+
// 1  | TagId       | INTEGER   | generated    |<-------+   уник. идентификатор записи
// 2  | Tag         | TEXT      |              |    //  |   название тега точки
// 3  | Depth       | INTEGER   | 1            |    //  |   макс. глубина хранения предыстории
// ---+-------------+-----------+--------------+    //  |   (= макс. значение HistoryType из БДРВ)
const char *HistoricImpl::s_SQL_CREATE_TABLE_DICT_TEMPLATE=//|
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
const char *HistoricImpl::s_SQL_CREATE_TABLE_DATA_TEMPLATE =
        "CREATE TABLE IF NOT EXISTS %s"
        "("
        "Timestamp INTEGER,"
        "HistoryType integer default 1,"
        "TagId INTEGER NOT NULL,"
        "VAL REAL,"
        "VALID INTEGER DEFAULT 0,"
//        "PRIMARY KEY (Timestamp,HistoryType,TagId) ON CONFLICT ABORT,"
        "PRIMARY KEY (Timestamp,HistoryType,TagId) ON CONFLICT REPLACE,"
        "FOREIGN KEY (TagID) REFERENCES %s(TagId) ON DELETE CASCADE"
        ");";

const char *HistoricImpl::s_SQL_CREATE_INDEX_DATA_TEMPLATE =
        "CREATE INDEX IF NOT EXISTS TimestampIndex "
        "ON %s (\"Timestamp\" ASC);";

const char *HistoricImpl::s_SQL_CHECK_TABLE_TEMPLATE =
        "SELECT count(*) FROM sqlite_master WHERE type='TABLE' AND NAME='%s';";
const char *HistoricImpl::s_SQL_INSERT_DICT_HEADER_TEMPLATE = "INSERT INTO %s (Tag, Depth) VALUES\n";
const char *HistoricImpl::s_SQL_INSERT_DICT_VALUES_TEMPLATE = "('%s',%d),\n";
const char *HistoricImpl::s_SQL_SELECT_DICT_TEMPLATE = "SELECT TagId,Tag,Depth FROM %s;";
const char *HistoricImpl::s_SQL_DELETE_DICT_TEMPLATE = "DELETE FROM %s WHERE TagId=%d;";
const char *HistoricImpl::s_SQL_INSERT_DATA_ONE_ITEM_HEADER_TEMPLATE =
        "INSERT INTO %s (Timestamp,HistoryType,TagId,VAL,VALID) VALUES\n";
const char *HistoricImpl::s_SQL_INSERT_DATA_ONE_ITEM_VALUES_TEMPLATE = "(%ld,%d,%d,%g,%d),\n";
// Выбрать набор семплов определенного типа для указанного Тега в указанный период времени
const char *HistoricImpl::s_SQL_READ_DATA_ITEM_TEMPLATE =
        "SELECT Timestamp,VAL,VALID FROM %s "
        "WHERE TagId=%d "
        "AND HistoryType=%d "
        "AND Timestamp>=%d AND Timestamp<=%d;";
const char *HistoricImpl::s_SQL_READ_DATA_AVERAGE_ITEM_TEMPLATE =
        "SELECT AVG(VAL),MAX(VALID) FROM %s "
        "WHERE TagId=%d "
        "AND HistoryType=%d "
        // > && <= потому, что интересующий временной интервал начинается после своего начала,
        // иначе в выборку попадает на один семпл больше
        "AND Timestamp>%d AND Timestamp<=%d;";

// ===========================================================================
// Создать экземпляр Историка
// Первый параметр - указатель на экземпляр среды БДРВ
//   Может быть NULL в случае, когда требуется доступ только к HDB
// Второй параметр - указатель на название снимка HDB с историей параметров
HistoricImpl::HistoricImpl(xdb::RtEnvironment* rtdb_env, const char* hdb_name) :
  m_rtdb_env(rtdb_env),
  m_rtdb_conn(NULL),
  m_hist_db(NULL),
  m_hist_err(NULL),
  m_history_db_connected(false),
  m_rtdb_connected(false)
{
  if (hdb_name && (strlen(hdb_name) > SERVICE_NAME_MAXLEN)) {
    LOG(WARNING) << "Given HDB env name length is exceed limit: " << SERVICE_NAME_MAXLEN;
  }

  if (hdb_name && hdb_name[0]) {
    strncpy(m_history_db_filename, hdb_name, SERVICE_NAME_MAXLEN);
    m_history_db_filename[SERVICE_NAME_MAXLEN] = '\0';
  }
  else strcpy(m_history_db_filename, HISTORY_DB_FILENAME);
}

// ===========================================================================
HistoricImpl::~HistoricImpl()
{
  Disconnect();
}

// ===========================================================================
// Подключение к БДРВ и к HDB
// Происходит выгрузка списка точек, имеющих предысторию, из БДРВ.
// Если экземпляр создаётся из нескольких нитей, загрузка точек из БДРВ может быть
// многократной, так как каждая нить будет создавать подключение к БДРВ. Чтобы
// этого избежать, во второстепенных нитях первым параметром передается NULL,
// тогда подключение к БДРВ не требуется, и структура HDB не будет изменяться.
bool HistoricImpl::Connect()
{
  bool status = false;
  int dict_size = 0;

  // Подключиться к БДРВ, если необходимо
  if (m_rtdb_env) {
    LOG(INFO) << "Prepare to attach to RTDB and HDB after";
    //
    if (true == (status = rtdb_connect()))
    {
      // Получить из БДРВ перечень точек, имеющих предысторию
      if (true == rtdb_get_info_historized_points())
      {
        // Подключиться к БД Предыстории
        if (true == history_db_connect())
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
  }
  else { // подключаться к БДРВ не требуется, только к HDB
    LOG(INFO) << "Prepare to attach to HDB only";
    if (true == (status = history_db_connect())) {
      // Инициализируем словарь на основе уже существующих данных
      dict_size = getPointsFromDictHDB(m_actual_hist_points);
      LOG(INFO) << "Existing points dictionary size=" << dict_size;
    }
  }

  return status;
}

// ===========================================================================
bool HistoricImpl::Disconnect()
{
  if (m_rtdb_connected) {
    if (true == rtdb_disconnect()) {
      LOG(INFO) << "Disconnection from RTDB";
    }
    else {
      LOG(INFO) << "Unable to disconnect from RTDB";
    }
  }

  history_db_disconnect();

  return ((m_rtdb_connected == false) && (m_history_db_connected == false));
}

// ===========================================================================
// Создать индексы, если они ранее не существовали
bool HistoricImpl::createIndexes()
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
bool HistoricImpl::createTable(const char* table_name)
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
                       HDB_DATA_TABLENAME,
                       HDB_DICT_TABLENAME);
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
      created = false;
      sqlite3_free(m_hist_err);
    }
    else LOG(INFO) << "HDB table '" << table_name << "' successfully created";
  }

  return created;
}

// ===========================================================================
void HistoricImpl::enable_features()
{
  const char* fname = "enable_features";

  if (true == m_history_db_connected) {
    // Установить кодировку UTF8
    if (sqlite3_exec(m_hist_db, "PRAGMA ENCODING = \"UTF-8\"", 0, 0, &m_hist_err)) {
      LOG(ERROR) << fname << ": Unable to set UTF-8 support for HDB: " << m_hist_err;
      sqlite3_free(m_hist_err);
    }
    else LOG(INFO) << "Enable UTF-8 support for HDB";

    // Включить поддержку внешних ключей
    if (sqlite3_exec(m_hist_db, "PRAGMA FOREIGN_KEYS = 1", 0, 0, &m_hist_err)) {
      LOG(ERROR) << fname << ": Unable to set FOREIGN_KEYS support for HDB: " << m_hist_err;
      sqlite3_free(m_hist_err);
    }
    else LOG(INFO) << "Enable FOREIGN_KEYS support for HDB";
  }
}

// ===========================================================================
bool HistoricImpl::history_db_connect()
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

    if (m_rtdb_connected) {

      //  Включить поддержку UTF8 и внешних ключей
      enable_features();

      // Проверить структуру только что открытой БД.
      // Для этого получить из БДРВ список всех параметров ТИ, для которых указано сохранение истории.
      // Если Историческая база пуста - создать ей новую структуру на основе данных из БДРВ
      // Если Историческая база не пуста, требуется сравнение новой и старой версий списка параметров.
      status = createTable(HDB_DICT_TABLENAME);

      if (true == (status = createTable(HDB_DATA_TABLENAME)))
        status = createIndexes();
    }
  }
  else
  {
    LOG(ERROR) << fname << ": Opening HDB '" << m_history_db_filename
               << "':  " << sqlite3_errmsg(m_hist_db);
    status = false;
  }

  return status;
}

// ===========================================================================
bool HistoricImpl::rtdb_connect()
{
  if (m_rtdb_env && (NULL != (m_rtdb_conn = m_rtdb_env->getConnection()))) {
      m_rtdb_connected = true;
  }

  LOG(INFO) << "Connection to RTDB: " << m_rtdb_connected;
  return (m_rtdb_conn != NULL);
}

// ===========================================================================
bool HistoricImpl::rtdb_disconnect()
{
  delete m_rtdb_conn;
  m_rtdb_connected = false;

  return true;
}

// ===========================================================================
bool HistoricImpl::history_db_disconnect()
{
  int status;

  if (m_history_db_connected) {
    if (SQLITE_OK == (status = sqlite3_close(m_hist_db))) {
      m_history_db_connected = false;
      LOG(INFO) << "Disconnection from HDB";
    }
    else LOG(ERROR) << "Unable to disconnect from HDB: status=" << status;
  }
  else LOG(ERROR) << "Try to close not connected history database";

  return (false == m_history_db_connected);
}

// ===========================================================================
void HistoricImpl::accelerate(bool speedup)
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
    LOG(ERROR) << fname << ": Unable to change JOURNAL_MODE: " << m_hist_err;
    sqlite3_free(m_hist_err);
  }
  else LOG(INFO) << "JOURNAL_MODE is changed";

  if (sqlite3_exec(m_hist_db, sql_operator_synchronous, 0, 0, &m_hist_err)) {
    LOG(ERROR) << fname << ": Unable to change SYNCHRONOUS mode: " << m_hist_err;
    sqlite3_free(m_hist_err);
  }
  else LOG(INFO) << "SYNCHRONOUS mode is changed";
}

// ===========================================================================
// TODO: вызвать эту функцию только один раз для каждого возможного экземпляра
bool HistoricImpl::tune_dictionaries()
{
  bool result = true;
  const char* fname = "tune_dictionaries";
  int dict_size = 0;
  // Разница между старым и новым наборами тегов Исторической БД
  map_history_info_t need_to_create_hist_points;
  map_history_info_t need_to_delete_hist_points;

  LOG(INFO) << fname << ": START";
  // Установить щадящий режим занесения данных, отключив сброс журнала на диск
  accelerate(true);

  // Загрузить в m_actual_hist_points содержимое таблицы данных HDB
  assert(m_actual_hist_points.empty());
  dict_size = getPointsFromDictHDB(m_actual_hist_points);

  // Синхронизация при старте баз данных HDB и RTDB
  // 1) найти и удалить точки, присутствующие в HDB, но отсутствующие в БДРВ
  // 2) найти и создать точки, отсутствующие в HDB, но присутствующие в БДРВ

  LOG(INFO) << fname << ": Select " << dict_size << " actual history points";
  if (!dict_size) {
    // HDB пуста, значит можно без проверки скопировать все из RTDB
    need_to_create_hist_points = m_actual_rtdb_points;
    need_to_delete_hist_points.clear();
  }
  else {
    // В снимке HDB уже присутствуют какие-то Точки.
    // Найти разницу между списками m_actual_rtdb_points и m_actual_hist_points,
    // включив в результат все то, что есть в первом списке, но нет во втором.
    calcDifferences(m_actual_rtdb_points, m_actual_hist_points, need_to_create_hist_points);
    calcDifferences(m_actual_hist_points, m_actual_rtdb_points, need_to_delete_hist_points);
  }

  LOG(INFO) << fname << ": size of need_to_create_rtdb_points=" << need_to_create_hist_points.size();
  LOG(INFO) << fname << ": size of need_to_delete_rtdb_points=" << need_to_delete_hist_points.size();

  // Сразу после этого заполнить массив m_actual_hist_points актуальной информацией
  createPointsDictHDB(need_to_create_hist_points);
  getPointsFromDictHDB(m_actual_hist_points);

  deletePointsDictHDB(need_to_delete_hist_points);

  accelerate(false);
  LOG(INFO) << fname << ": STOP " << result;

  return result;
}

// ===========================================================================
// В результат (третий параметр) идет только тот тег, который присутствует
// в первом наборе, но отсутствует во втором.
int HistoricImpl::calcDifferences(map_history_info_t& whole,  /* входящий общий набор тегов */
                    map_history_info_t& part,   /* входящий пересекающий набор тегов */
                    map_history_info_t& result) /* исходящий набор пересечения */
{

  for (map_history_info_t::const_iterator it = whole.begin();
       it != whole.end();
       ++it)
  {
    // Найти Тег из первого множества во втором множестве
    const map_history_info_t::const_iterator lookup = part.find((*it).first);
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
int HistoricImpl::getPointsFromDictHDB(map_history_info_t &actual_hist_points)
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
#if (VERBOSE > 2)
      LOG(INFO) << "load actual hist id=" << info.tag_id << " '" << tag << "' htype=" << info.history_type;
#endif
      actual_hist_points.insert(std::pair<std::string, history_info_t>(tag, info));
    }

    LOG(INFO) << "Succesfully load " << actual_hist_points.size() << " actual HDB tag(s)";
    complete = true;

  } while (false);

  sqlite3_finalize(stmt);

  if (!complete) {
    LOG(ERROR) << "Failure loading from table '" << HDB_DICT_TABLENAME << "' of HDB";
  }

  return actual_hist_points.size();
}

// ===========================================================================
// Создать НСИ HIST_DICT на основе связки "Тег" <-> { "идентификатор тега","глубина истории" }
int HistoricImpl::createPointsDictHDB(map_history_info_t& points_to_create)
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

    for (map_history_info_t::const_iterator it = points_to_create.begin();
         it != points_to_create.end();
         ++it)
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
      LOG(ERROR) << fname << ": executing SQL: " << m_hist_err;
      sqlite3_free(m_hist_err);
    } else LOG(INFO) << fname << ": Create " << idx_last_item << " point(s)";

#if (VERBOSE > 8)
    LOG(INFO) << fname << ": SQL: " << sql;
#endif
  } // Конец блока, если были данные для вставки

  return idx;
}

// ===========================================================================
// Удалить данные из таблицы HIST с заданными идентификаторами
int HistoricImpl::deletePointsDataHDB(map_history_info_t& points_to_delete)
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

    for (map_history_info_t::const_iterator it = points_to_delete.begin();
         it != points_to_delete.end();
         ++it)
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

#if (VERBOSE > 8)
    LOG(INFO) << fname << ": SQL=" << sql;
#endif
  }

  return idx;
}

// ===========================================================================
// Удалить данные из таблицы HIST_DICT с заданными идентификаторами
int HistoricImpl::deletePointsDictHDB(map_history_info_t& points_to_delete)
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

    for (map_history_info_t::const_iterator it = points_to_delete.begin();
         it != points_to_delete.end();
         ++it)
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

#if (VERBOSE > 8)
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
bool HistoricImpl::rtdb_get_info_historized_points()
{
  bool status = false;
  history_info_t info;

  // К этому моменту список точек из БДРВ еще д.б. пуст
  assert(!m_actual_rtdb_points.size());

#if (HIST_DATABASE_LOCALIZATION == LOCAL)
  status = local_rtdb_get_info_historized_points(m_raw_actual_rtdb_points);
#elif (HIST_DATABASE_LOCALIZATION == REMOTE)
  status = remote_rtdb_get_info_historized_points(m_raw_actual_rtdb_points);
#else
#error "Unknown HIST localization (neither LOCAL nor REMOTE)"
#endif

  // Сконвертировать данные из формата словаря БДРВ "Тег" <-> "Глубина истории"
  // в формат "Тег" <-> { "Глубина истории", "Идентификатор тега в HDB" }
  for (xdb::map_name_id_t::const_iterator it = m_raw_actual_rtdb_points.begin();
       it != m_raw_actual_rtdb_points.end();
       ++it)
  {
     info.tag_id = 0; // пустое значение, т.к. из БДРВ приходит только глубина истории и тег.
     info.history_type = (*it).second;
#if (VERBOSE > 5)
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
#if (HIST_DATABASE_LOCALIZATION == LOCAL)
bool HistoricImpl::local_rtdb_get_info_historized_points(xdb::map_name_id_t& points)
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
#if (HIST_DATABASE_LOCALIZATION == REMOTE)
bool HistoricImpl::remote_rtdb_get_info_historized_points(xdb::map_name_id_t& points)
{
  bool status = false;
  LOG(FATAL) << "Not implemented";
  assert (0 == 1);
  return status;
}
#endif

// ===========================================================================
// Занести все историзированные значения в HDB
int HistoricImpl::store_history_samples(HistorizedInfoMap_t& historized_map, xdb::sampler_type_t store_only_htype)
{
  static const char* fname = "store_history_samples";
  static char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  int num_samples = 0; // Индекс элемента в списке на сохранение
  int sql_err_code;
  std::string sql;
  int printed;
  int rc = OK;
  int estimated_sql_operator_size;
  // "(%ld,%d,%d,%g,%d),"
  // strlen("(9999999999,9,1000000,1234567.89,1),\n") = 37
  // strlen(заголовок SQL-запроса) = 100

  if (historized_map.size() > 0) { // Есть данные к сохранению

    estimated_sql_operator_size = ((historized_map.size() * 37) + 100);
    // Уменьшим потребление памяти, оставив ровно столько, сколько нужно для хранения
    sql.resize(estimated_sql_operator_size);

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
    for(HistorizedInfoMap_t::const_iterator it = historized_map.begin();
        it != historized_map.end();
        ++it)
    {
      // Сохранить только те семплы, которые укладываются в допустимый тип
      if ((*it).second.info.history_type >= store_only_htype)
      {
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
#if (VERBOSE > 3)
      else LOG(INFO) << "Skip point " << (*it).first
                     << " with type=" << (*it).second.info.history_type
                     << " while storing type=" << store_only_htype;
#endif
    }

    // Затереть последние два символа ',' и '\n'
    sql.erase(sql.end() - 2, sql.end());

    sql += ";\nCOMMIT;";
  }

#if (VERBOSE > 8)
  LOG(INFO) << "SQL: " << sql;
#endif

  LOG(INFO) << "estimated_sql_size=" << estimated_sql_operator_size
            << " real_sql_size=" << sql.size()
            << " for " << historized_map.size() << " point(s)";

//  accelerate(true);
  if (num_samples) {
    if (SQLITE_OK != (sql_err_code = sqlite3_exec(m_hist_db, sql.c_str(), 0, 0, &m_hist_err))) {
      LOG(ERROR) << fname << ": code=" << sql_err_code << ", " << m_hist_err;
      sqlite3_free(m_hist_err);
#if (VERBOSE > 7)
      LOG(ERROR) << fname << ": SQL: " << sql;
#endif
      rc = NOK;
    } else LOG(INFO) << "Successfully insert " << num_samples << " row(s) for history type=" << store_only_htype;
  }
  else {
    LOG(WARNING) << "There are no historized points with type=" << store_only_htype;
  }

//  accelerate(false);

  return rc;
}


// ===========================================================================
// Найти идентификатор тега в НСИ HDB
// Кэш НСИ - в наборе m_actual_hist_points
int HistoricImpl::getPointIdFromDictHDB(const char* pname)
{
  int id = 0; // по умолчанию - ничего не найдено

//  LOG(INFO) << "GEV: getPointIdFromDictHDB '" << pname << "', actual_hist_points.size=" << m_actual_hist_points.size();
  const map_history_info_t::const_iterator lookup = m_actual_hist_points.find(pname);
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
// имеющих глубину хранения менее заданной. К примеру, если htype_to_store == PERIOD_DAY,
// то сохранению подлежат все точки, имеющие sampler_type_t от PERIOD_1_MINUTE до PERIOD_DAY.
//
bool HistoricImpl::make_history_samples_by_type(const xdb::sampler_type_t htype_to_store, time_t start)
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
  xdb::map_name_id_t::const_iterator it = m_raw_actual_rtdb_points.begin();
  //xdb::sampler_type_t previous_htype;

  if (xdb::PERIOD_1_MINUTE == htype_to_store) {
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
                LOG(ERROR) << fname << ": Unable to find TagId for historized point " << (*it).first;
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
                historized_info.info.history_type = static_cast<xdb::sampler_type_t>((*it).second);
                historized_info.info.tag_id = point_id;
                historized_map.insert(std::pair<const std::string, historized_attributes_t>((*it).first, historized_info));
#if (VERBOSE > 5)
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

        ++it; // переход к следующему тегу

      } // Конец блока кода чтения всех тегов из списка

  } // Конец загрузки значений для глубины истории PERIOD_1_MINUTE, когда значения читаются из БДРВ
  else {
      // previous_htype = m_stages[htype_to_store].prev;
      // TODO: Чтение в historized_map усредненных значений из HDB
      // для всех точек из списка m_raw_actual_rtdb_points
      if (false == load_samples_list_by_type(m_stages[htype_to_store].prev, start, m_raw_actual_rtdb_points, historized_map)) {
        LOG(ERROR) << fname << ": Can't load samples from HDB by history type=" << htype_to_store;
        result = false;
      }
  }

  if (result) {
    // Занести все историзированные значения в HDB
    if (OK != store_history_samples(historized_map, htype_to_store))
      LOG(ERROR) << fname << ": Unable to save history sample type: " << htype_to_store;
  }

  return result;
}

// ===========================================================================
// Сгенерировать новый отсчет предыстории указанной глубиной на основе агрегирования
// ряда семплов предыдущей глубины.
bool HistoricImpl::select_avg_values_from_sample(historized_attributes_t& historized_info,
                                              const xdb::sampler_type_t htype_to_load)
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
  if (xdb::PERIOD_1_MINUTE == htype_to_load) {
      // нулевой индекс - неиспользуемый здесь timestamp
      val_idx   = 1;
      valid_idx = 2;
  }

  printed = snprintf(sql_operator,
           MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
           ((xdb::PERIOD_1_MINUTE == htype_to_load)?
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
    sqlite3_finalize(stmt);
    return false;
  }

  while(sqlite3_step(stmt) == SQLITE_ROW)
  {

    historized_info.val   = sqlite3_column_double(stmt, val_idx);
    historized_info.valid = sqlite3_column_int(stmt, valid_idx);
#if (VERBOSE > 8)
    LOG(INFO) << fname << ": SQL=" << sql_operator
              << "; VAL=" << historized_info.val
              << "; VALID=" << (unsigned int)historized_info.valid;
#endif
  }
  sqlite3_finalize(stmt);

  return true;
}

// ===========================================================================
// Загрузить из HDB средние значения по всем тегам сразу из списка raw_actual_rtdb_points
bool HistoricImpl::load_samples_list_by_type(const xdb::sampler_type_t htype_to_load,
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
  xdb::map_name_id_t::const_iterator it = m_raw_actual_rtdb_points.begin();
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
        case xdb::PERIOD_1_MINUTE:
        // история ежеминутной глубины собирается без агрегирования, непосредственно из таблицы данных
        case xdb::PERIOD_5_MINUTES:
        case xdb::PERIOD_HOUR:
        case xdb::PERIOD_DAY:
        case xdb::PERIOD_MONTH:
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

    ++it;
  }

  LOG(INFO) << "I " << fname << ": load " << last_sample_num << " history records with type " << htype_to_load;

  return result;
}


// ===========================================================================
// Код возврата - количество прочитанных данных
int HistoricImpl::readValuesFromDataHDB(int         point_id, // Id точки
       xdb::sampler_type_t htype,    // Тип читаемой истории
       time_t      start,       // Начало диапазона читаемой истории
       time_t      finish,      // Конец диапазона читаемой истории
       historized_attributes_t* items) // Прочитанные данные семплов
{
  sqlite3_stmt* stmt = 0;
  char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
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

  if (SQLITE_OK != sqlite3_prepare(m_hist_db, sql_operator, -1, &stmt, 0)) {
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

#if (VERBOSE > 7)
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
time_t HistoricImpl::roundTimeToNextEdge(time_t given_time, xdb::sampler_type_t htype, bool attach)
{
  time_t round_time = given_time;
  struct tm edge;
  div_t rest;

  localtime_r(&given_time, &edge);
  // все отметки времени проходят по границе минуты
//  edge.tm_sec = 0;

  // Дальнейшее округление зависит от желаемого типа истории
  switch(htype) {
      case xdb::PERIOD_1_MINUTE:
        // округления по границы минуты достаточно
        edge.tm_sec = 0;
        if (attach) // притянуть время к следущей минуте?
          edge.tm_min++;
        break;

      case xdb::PERIOD_5_MINUTES:
        // округлим до ближайшей минуты, делящейся на 5
        edge.tm_sec = 0;
        rest = div(edge.tm_min,5);
        if (attach && rest.rem)
          edge.tm_min += 5 - rest.rem;
        break;

      case xdb::PERIOD_HOUR:
        // округлим до ближайшего часа
        edge.tm_sec = 0;
        edge.tm_min = 0;
        if (attach)
          edge.tm_hour++;
        break;

      case xdb::PERIOD_DAY:
        // округлим до ближайшего дня
        edge.tm_sec = 0;
        edge.tm_min = 0;
        edge.tm_hour = 0;
        if (attach)
          edge.tm_mday++;
        break;

      case xdb::PERIOD_MONTH:
        // округлим до ближайшего месяца
        edge.tm_sec = 0;
        edge.tm_min = 0;
        edge.tm_hour = 0;
        edge.tm_mday = 1;
        if (attach)
          edge.tm_mon++;
        break;

      case xdb::PERIOD_NONE:
      case xdb::PERIOD_LAST:
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
int HistoricImpl::load_samples_period_per_tag(const char* pname,
                                           bool& existance, // признак наличия точки в HDB
                                           const xdb::sampler_type_t htype,
                                           time_t start,
                                           historized_attributes_t *output_historized,
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
  int idx;
  // количество загруженных в выходной массив отсчетов
  int loaded_history_num = 0;
  historized_attributes_t internal_historized[MAX_PORTION_SIZE_LOADED_HISTORY];

  if ((xdb::PERIOD_1_MINUTE >= htype) && (htype <= xdb::PERIOD_MONTH)) {
    delta = m_stages[htype].duration;
  }
  else {
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
    existance = false;
    status = false;
  }
  else {
    existance = true;
    finish = start + (delta * (samples_to_read - 1));
    memset(internal_historized, '\0', sizeof(historized_attributes_t) * MAX_PORTION_SIZE_LOADED_HISTORY);

    // Все по Точке нашли, теперь залезем в таблицу HIST
    // Прочитать из HIST набор значений VAL и VALID, за указанный диапазон дат
    read_samples = readValuesFromDataHDB(point_id,    // Id точки
                            htype,       // Тип читаемой истории
                            start,       // Начало диапазона читаемой истории
                            finish,      // Конец диапазона читаемой истории
                            internal_historized); // Прочитанные данные семплов

    if (read_samples) {
      // После чтения необходимо проверить, что последовательность VAL и VALID не нарушена,
      // то есть не имеет разрывов во времени. Разрывы заполняются специальным образом (VALID=?)
      idx = 0;
      loaded_history_num = 0;
      // Поскольку при чтении данных можно задавать произвольную дату начала интервала,
      // следует выравнивать начало поискового диапазона до ближайшего корректного.
      // К примеру, если для часовых семплов задано время 12:10, то нужно округлить
      // его до ближайшего часа, до 13:00
      for (frame = roundTimeToNextEdge(start, htype, false); frame <= finish; frame += delta) {

        if (frame == internal_historized[idx].datehourm) {
          // Данный семпл для текущего времени имеет сохраненное значение, то есть нет имеет пропуска
#if (VERBOSE > 3)
          LOG(INFO) << "Предыстория ЕСТЬ " << pname << ": VAL=" << internal_historized[idx].val
                    << " VALID=" << (unsigned int)internal_historized[idx].valid
                    << " TIME(" << frame << ")=" << ctime(&frame);
#endif
          // Предыстория есть - считается
          output_historized[loaded_history_num++] = internal_historized[idx++];
        }
        else if (frame < internal_historized[idx].datehourm) {
            // Если проверяемый фрейм меньше следующего реально сохраненного семпла, значит
            // был пропуск в накоплении статистики, и следует обозначить "разрыв" предыстории
#if (VERBOSE > 3)
            LOG(INFO) << "Предыстория НЕТ " << pname << ": VAL=????"
                      << " VALID=INVALID"
                      << " TIME(" << frame << ")=" << ctime(&frame);
#endif
            // предыстории нет, заполним дыру недостоверными значениями
            output_historized[loaded_history_num].valid = (char)0;
            output_historized[loaded_history_num].val = 0.0;
            output_historized[loaded_history_num].datehourm = frame;
            loaded_history_num++;
        }
        else if (0 == internal_historized[idx].datehourm) { // прочитано нулевое время
            // Это значит в HDB не было части данных за это время
#if (VERBOSE > 3)
            LOG(WARNING) << "Предыстория ПУСТО " << pname
                         << " TIME(" << frame << ")=" << ctime(&frame);
#endif
            output_historized[loaded_history_num].valid = (char)0;
            output_historized[loaded_history_num].val = -1; // GEV: -1 как специальное значение
            output_historized[loaded_history_num].datehourm = frame;
            loaded_history_num++;
        }
        else { // прочитано значение из середины диапазона?
            // Нарушение логики хранения данных
            LOG(ERROR) << "Нарушение логики - при выборке данных истории типа " << htype
                       << " для текущего фрейма " << frame
                       << " за период " << start << ":" << finish
                       << " встретился семпл с отметкой времени "
                       << internal_historized[idx].datehourm;
            output_historized[loaded_history_num].valid = (char)0;
            output_historized[loaded_history_num].val = -2; // GEV: -2 как специальное значение
            output_historized[loaded_history_num].datehourm = frame;
            loaded_history_num++;
            //1 assert(0 == 1);
        }

        if (!status) {
            LOG(WARNING) << fname << ": can't read '" << pname
                      << "' sample period for " << frame << " sec ";
            // TODO: Пропускать сбойный отсчёт?
        }
      } // Конец цикла проверки непрерывности временнЫх интервалов
    } // Конец блока, если была прочитана хотябы одна запись
  }

  return loaded_history_num;
}

// ===========================================================================
// Обсчитать новые значения из предыдущего цикла и занести их в виде предыстории нового типа
//   Заполнить тестовыми данными таблицу HIST для семплов с тегами из m_actual_hist_points
//   за период с начала теста по его конец.
bool HistoricImpl::Make(time_t current)
{
  const char *fname = "Make";
  struct tm edge;
  bool sampling_status = false;

  assert(m_rtdb_env != NULL);
  assert(m_rtdb_conn != NULL);

  // перевести синтетическое время к представлению часы, минуты, секунды
  // для анализа начала отметок формиования предысторий различных глубин
  localtime_r(&current, &edge);

  LOG(INFO) << fname << ": start at " << current << ", tm_sec = " << edge.tm_sec;
  // Только на границе минуты
  assert(edge.tm_sec == 0);

  // Получить мгновенные значения и занести их как минутные семплы
  sampling_status = make_history_samples_by_type(xdb::PERIOD_1_MINUTE, current);
  LOG(INFO) << "HDB sampling " << ((true == sampling_status)? "SUCCESS" : "FAILURE")
            << ", htype=" << xdb::PERIOD_1_MINUTE
            << ", period=" << current;

  // Проверки времени начала цикла должны идти друг за другом, потому что некоторые циклы
  // возникают на одних и тех же границах (к примеру, каждый двеннадцатый пятиминутный и часовой)
  if (!(edge.tm_min % 5)) {
      // Посчитать новый 5-минутный семпл из ранее изготовленных минутных
      sampling_status = make_history_samples_by_type(xdb::PERIOD_5_MINUTES, current);
      LOG(INFO) << "HDB sampling " << ((true == sampling_status)? "SUCCESS" : "FAILURE")
                << ", htype=" << xdb::PERIOD_5_MINUTES
                << ", period=" << current;
  }

  if (0 == edge.tm_min) {
      sampling_status = make_history_samples_by_type(xdb::PERIOD_HOUR, current);
      LOG(INFO) << "HDB sampling " << ((true == sampling_status)? "SUCCESS" : "FAILURE")
                << ", htype=" << xdb::PERIOD_HOUR
                << ", period=" << current;
  }

  if ((0 == edge.tm_min) && (0 == edge.tm_hour)) {
      sampling_status = make_history_samples_by_type(xdb::PERIOD_DAY, current);
      LOG(INFO) << "HDB sampling " << ((true == sampling_status)? "SUCCESS" : "FAILURE")
                << ", htype=" << xdb::PERIOD_DAY
                << ", period=" << current;
  }

  if ((0 == edge.tm_min) && (0 == edge.tm_hour) && (1 == edge.tm_mday)) {
      sampling_status = make_history_samples_by_type(xdb::PERIOD_MONTH, current);
      LOG(INFO) << "HDB sampling " << ((true == sampling_status)? "SUCCESS" : "FAILURE")
                << ", htype=" << xdb::PERIOD_MONTH
                << ", period=" << current;
  }

  return sampling_status;
}

// ===========================================================================

