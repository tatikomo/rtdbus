#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "sqlite3.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_smad_impl.hpp"

// Структура таблицы SMAD
// ---+-------------+-----------+--------------+
//  # |  Имя поля   | Тип поля  | По умолчанию |
// ---+-------------+-----------+--------------+
//  1 | ID          | INTEGER   | generated    |<---+
//  2 | NAME        | TEXT      |              |    |
//  3 | STATE       | INTEGER   | 0            |    |
//  4 | DELEGATION  | INTEGER   | 1            |    |
//  5 | LAST_PROBE  | TIME      |              |    |
//  6 | MODE        | TEXT      |              |    |
// ---+-------------+-----------+--------------+    |
//                                                  |
// Структура таблицы СС                             |
// ---+-------------+-----------+--------------+    |
//  # |  Имя поля   | Тип поля  | По умолчанию |    |
// ---+-------------+-----------+--------------+    |
//  1 | SA_ID       | INTEGER   | NOT NULL     |----+
//  2 | NAME        | TEXT      |              |
//  3 | ADDR        | INTEGER   |              |
//  4 | TYPE        | INTEGER   | 0            |
//  5 | TYPE_SUPPORT| INTEGER   | 0            |
//  6 | Timestamp   | TIME      |              |
//  7 | VALID       | INTEGER   | 0            |
//  8 | I_VAL       | INTEGER   | 0            |
//  9 | G_VAL       | REAL      | 0            |
// 10 | LOW_FAN     | INTEGER   | 0            |
// 11 | HIGH_FAN    | INTEGER   | 0            |
// 12 | LOW_FIS     | REAL      | 0            |
// 13 | HIGH_FIS    | REAL      | 0            |
// ---+-------------+-----------+--------------+

const char *SMAD::s_SQL_CREATE_SMAD_TEMPLATE =
    "CREATE TABLE IF NOT EXISTS %s "
    "("
    "ID INTEGER PRIMARY KEY AUTOINCREMENT," // Уник. идентификатор записи
    "NAME TEXT NOT NULL,"           // Тег СС
    "STATE INTEGER DEFAULT 0,"      // Состояние СС { Недоступно(0)|Инициализация(1)|Работа(2) }
    "DELEGATION INTEGER DEFAULT 1," // Режим управления CC { Местный(1)|Дистанционный(0) }
    "LAST_PROBE INTEGER,"           // Время последнего сеанса работы модуля с Системой Сбора
    "MODE TEXT"                     // Режим работы СС
    ");";
const char *SMAD::s_SQL_INSERT_SMAD_TEMPLATE =
    "INSERT INTO %s (NAME,STATE,DELEGATION,LAST_PROBE,MODE) VALUES ('%s',%d,%d,%ld,'%s');";
const char *SMAD::s_SQL_CREATE_SA_TEMPLATE =
    "CREATE TABLE IF NOT EXISTS %s "
    "("
    "SA_ID INTEGER NOT NULL,"
    "NAME TEXT NOT NULL,"           // Название параметра
    "ADDR INTEGER,"                 // Адрес регистра
    "TYPE INTEGER DEFAULT 0,"               // Тип параметра
    "TYPE_SUPPORT INTEGER DEFAULT 0,"       // Тип обработки
    "RTDBUS_Timestamp INTEGER DEFAULT 0,"   // Время получения данных, выставляется модулем
    "SA_Timestamp INTEGER DEFAULT 0,"       // Время, переданное системой сбора
    "VALID INTEGER DEFAULT 0,"      // Значение Достоверности параметра
    "I_VAL INTEGER DEFAULT 0,"      // (только для дискретов) Значение параметра
    "G_VAL REAL DEFAULT 0,"         // (только для аналогов) Значение параметра
    "LOW_FAN INTEGER DEFAULT 0,"    // (только для аналогов) Нижняя граница инженерного диапазона
    "HIGH_FAN INTEGER DEFAULT 0,"   // (только для аналогов) Верхняя граница инженерного диапазона
    "LOW_FIS REAL DEFAULT 0,"    // (только для аналогов) Нижняя граница физического диапазона
    "HIGH_FIS REAL DEFAULT 0,"   // (только для аналогов) Верхняя граница физического диапазона
    "FACTOR REAL DEFAULT 1,"     // (только для аналогов) Коэффициент масштабирования
    "PRIMARY KEY (SA_ID,ADDR,TYPE_SUPPORT) ON CONFLICT ABORT,"
    "FOREIGN KEY (SA_ID) REFERENCES %s(ID) ON DELETE CASCADE"
    ");";
// Шаблон заголовка SQL-выражения занесения данных целочисленного типа в базу
const char *SMAD::s_SQL_INSERT_SA_HEADER_INT_TEMPLATE =
    "INSERT INTO %s (SA_ID,NAME,ADDR,TYPE,TYPE_SUPPORT) VALUES\n";
const char *SMAD::s_SQL_INSERT_SA_VALUES_INT_TEMPLATE =
    "(%d,'%s',%d,%d,%d),\n";

// Шаблон заголовка SQL-выражения занесения данных с плав. запятой в базу
const char *SMAD::s_SQL_INSERT_SA_HEADER_FLOAT_TEMPLATE =
    "INSERT INTO %s (SA_ID,NAME,ADDR,TYPE,TYPE_SUPPORT,LOW_FAN,HIGH_FAN,LOW_FIS,HIGH_FIS,FACTOR) VALUES\n";
const char *SMAD::s_SQL_INSERT_SA_VALUES_FLOAT_TEMPLATE =
    "(%d,'%s',%d,%d,%d,%d,%d,%g,%g,%g),\n";
// Таблица с описанием СС, включенных в эту зону SMAD
const char *SMAD::s_SMAD_DESCRIPTION_TABLENAME = "SMAD";

// ==========================================================================================
SMAD::SMAD(const char *filename)
 : m_db(NULL),
   m_state(STATE_DISCONNECTED),
   m_db_err(NULL),
   m_snapshot_filename(NULL),
   m_sa_name(NULL),
   m_sa_reference(-1)
{
  LOG(INFO) << "Constructor SMAD";
  m_snapshot_filename = strdup(filename);
}

// ==========================================================================================
SMAD::~SMAD()
{
  int rc;

  LOG(INFO) << "Destructor SMAD";

  free(m_snapshot_filename); // NB: именно free(), а не delete
  free(m_sa_name); // NB: именно free(), а не delete

  if (SQLITE_OK != (rc = sqlite3_close(m_db)))
    LOG(ERROR) << "Closing SMAD DB: " << rc;
}

// ==========================================================================================
SMAD::connection_state_t SMAD::connect(const char* sa_name)
{
  const char *fname = "connect";
  char sql_operator[1000 + 1];
  int printed;
  int rc = 0;

  m_sa_name = strdup(sa_name);
  // ------------------------------------------------------------------
  // Открыть файл SMAD с базой данных SQLite
  rc = sqlite3_open_v2(m_snapshot_filename,
                       &m_db,
                       // Если таблица только что создана, проверить наличие
                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                       "unix-dotfile");
  if (rc == SQLITE_OK)
  {
    LOG(INFO) << fname << ": SMAD file '" << m_snapshot_filename << "' successfuly opened";
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
    else LOG(INFO) << "Enable FOREIGN_KEYS support for HDB";


    // Успешное подключение к БД
    // ------------------------------------------------------------------
    // 1) Создать общую таблицу SMAD, если она ранее не существовала
    printed = snprintf(sql_operator, 1000, s_SQL_CREATE_SMAD_TEMPLATE, s_SMAD_DESCRIPTION_TABLENAME);
    assert(printed < 1000);
    if (true == create_table(s_SMAD_DESCRIPTION_TABLENAME, sql_operator))
    {
      // Проверить запись о локальной СС.
      // Если её не было ранее, создать новую запись.
      // Получить идентификатор записи локальной СС в общей таблице.
      if (true == get_sa_reference(m_sa_name, m_sa_reference))
      {
        // ------------------------------------------------------------------
        // Всегда заново создавать таблицу CC, даже если она ранее существовала
        drop_table(m_sa_name);
      }

      // Таблица СС удалена, пересоздать её
      printed = snprintf(sql_operator,
               1000,
               s_SQL_CREATE_SA_TEMPLATE,
               m_sa_name,
               s_SMAD_DESCRIPTION_TABLENAME);
      assert(printed < 1000);
      if (false == create_table(m_sa_name, sql_operator)) {
        LOG(ERROR) << "Creating table " << m_sa_name;
        m_state = STATE_ERROR;
      }
    }
  }
  else
  {
    LOG(ERROR) << fname << ": opening SMAD file '" << m_snapshot_filename << "' error=" << rc;
    m_state = STATE_ERROR;
  }

  return m_state;
}

// ==========================================================================================
int SMAD::push(const sa_parameter_info_t& info)
{
  int status = -1;

  LOG(INFO) << "Push " << info.name;
  return status;
}

// ==========================================================================================
int SMAD::purge()
{
  int status = -1;

  LOG(INFO) << "Purge";
  return status;
}

// ===========================================================================
// Создать соответствующую таблицу, если она ранее не существовала
bool SMAD::create_table(const char* table_name, const char* create_template)
{
  const char* fname = "create_table";
  bool created = true;
  int printed;
  std::string sql;
  char sql_operator[1000 + 1];

  // Создать таблицу данных
  printed = snprintf(sql_operator,
                     1000,
                     create_template,
                     table_name);
  assert(printed < 1000);

  sql = "BEGIN;\n";
  sql += sql_operator;
  sql += "\nCOMMIT;";

  if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
      LOG(ERROR) << fname << ": Creating table '" << table_name
                 << "': " << m_db_err;
      created = false;
      sqlite3_free(m_db_err);
      LOG(ERROR) << sql;
  }
  else LOG(INFO) << "Table '" << table_name << "' successfully created, sql size=" << printed;

  return created;
}

// ==========================================================================================
bool SMAD::get_sa_reference(char* system_name, int& reference)
{
  const char *fname = "get_sa_reference";
  char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];
  int printed;
  int count = 1;
  bool complete = false;
  sqlite3_stmt* stmt = 0;

  do
  {
    reference = 0;
    printed = snprintf(sql_operator,
                       MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                       "SELECT COUNT(*) FROM %s WHERE NAME='%s';",
                       s_SMAD_DESCRIPTION_TABLENAME,
                       system_name);
    assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

    printed = sqlite3_prepare(m_db, sql_operator, -1, &stmt, 0);
    if (printed != SQLITE_OK) {
      LOG(ERROR) << fname << ": Could not select count() from "
                 << s_SMAD_DESCRIPTION_TABLENAME << ", code="<< printed;
      break;
    }

    // Поместить словарь тегов в actual_hist_points
    while(sqlite3_step(stmt) == SQLITE_ROW) {
      count = sqlite3_column_int(stmt, 0);
#if (VERBOSE > 2)
      LOG(INFO) << "count=" << reference << " for " << system_name;
#endif
    }
    sqlite3_finalize(stmt);

    if (!count) {
      // В таблице SMAD нет записи об искомой СС, завести её
      printed = snprintf(sql_operator,
                         MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                         s_SQL_INSERT_SMAD_TEMPLATE,
                         s_SMAD_DESCRIPTION_TABLENAME,
                         system_name,
                         0,
                         1,
                         time(0),
                         "TEST");
      assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

      if (sqlite3_exec(m_db, sql_operator, 0, 0, &m_db_err)) {
        LOG(ERROR) << fname << ": Unable to insert " << system_name
                   << " info: " << m_db_err;
        sqlite3_free(m_db_err);
      }
      else LOG(INFO) << "Insert into " << s_SMAD_DESCRIPTION_TABLENAME
                     << " new " << system_name << " record";
    }

    // Прочитать запись об искомой СС
    printed = snprintf(sql_operator,
                       MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                       "SELECT ID FROM %s WHERE NAME='%s';",
                       s_SMAD_DESCRIPTION_TABLENAME,
                       system_name);
    assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

    printed = sqlite3_prepare(m_db, sql_operator, -1, &stmt, 0);
    if (printed != SQLITE_OK) {
      LOG(ERROR) << fname << ": Could not select from "
                 << s_SMAD_DESCRIPTION_TABLENAME << ", code="<< printed;
      break;
    }

    while(sqlite3_step(stmt) == SQLITE_ROW) {
        reference = sqlite3_column_int(stmt, 0);
#if (VERBOSE > 2)
        LOG(INFO) << "find ref=" << reference << " for " << system_name;
#endif
     }

     LOG(INFO) << system_name << " ref=" << reference
               << " in " << s_SMAD_DESCRIPTION_TABLENAME;
     complete = true;

  } while(false);

  sqlite3_finalize(stmt);

  if (!complete) {
    LOG(ERROR) << "Failure locating " << system_name << " from '"
               << s_SMAD_DESCRIPTION_TABLENAME << "'";
  }

  return complete;
}

// ===========================================================================
void SMAD::accelerate(bool speedup)
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
                           journal_MEMORY);
        //                   journal_OFF);
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

// ==========================================================================================
bool SMAD::drop_table(const char* table_name)
{
  const char* fname = "drop_table";
  bool dropped = true;
  int printed;
  std::string sql;
  char sql_operator[MAX_BUFFER_SIZE_FOR_SQL_COMMAND + 1];

  // Создать таблицу данных
  printed = snprintf(sql_operator,
                     MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                     "DROP TABLE IF EXISTS %s;",
                     table_name);
  assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

  sql = "BEGIN;\n";
  sql += sql_operator;
  sql += "\nCOMMIT;";

  if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
      LOG(ERROR) << fname << ": Dropping table '" << table_name
                 << "': " << m_db_err;
      dropped = false;
      sqlite3_free(m_db_err);
      LOG(ERROR) << sql;
  }
  else LOG(INFO) << "Table '" << table_name << "' successfully dropped";

  return dropped;
}

// ==========================================================================================
int SMAD::setup_parameter(const sa_parameter_info_t& info)
{
  int rc = -1;
  char sql_operator[1000 + 1];
  std::string sql;
  const char *fname = "setup_smad_parameter";
  int printed;

  LOG(INFO) << "setup parameter: "
            << info.name << ":"
            << info.address << ":"
            << info.type << ":"
            << info.support << ":"
            << info.low_fan << ":"
            << info.high_fan << ":"
            << info.low_fis << ":"
            << info.high_fis;

  // Начать транзакцию
  sql = "BEGIN;\n";

  switch(info.type) {
    case MBUS_PARAM_TYPE_TS:
    case MBUS_PARAM_TYPE_TSA:
    case MBUS_PARAM_TYPE_AL:
    case MBUS_PARAM_TYPE_SA:
      printed = snprintf(sql_operator,
                         1000,
                         s_SQL_INSERT_SA_HEADER_INT_TEMPLATE,
                         m_sa_name);
      assert (printed < 1000);
      sql += sql_operator;

      printed = snprintf(sql_operator,
                         1000,
                         s_SQL_INSERT_SA_VALUES_INT_TEMPLATE,
                         m_sa_reference,
                         info.name,
                         info.address,
                         info.type,
                         info.support);
      assert (printed < 1000);
      sql += sql_operator;
      break;

    case MBUS_PARAM_TYPE_TM:
      printed = snprintf(sql_operator,
                         1000,
                         s_SQL_INSERT_SA_HEADER_FLOAT_TEMPLATE,
                         m_sa_name);
      assert (printed < 1000);
      sql += sql_operator;

      printed = snprintf(sql_operator,
                         1000,
                         s_SQL_INSERT_SA_VALUES_FLOAT_TEMPLATE,
                         m_sa_reference,
                         info.name,
                         info.address,
                         info.type,
                         info.support,
                         info.low_fan,
                         info.high_fan,
                         info.low_fis,
                         info.high_fis,
                         info.factor);
      assert (printed < 1000);
      sql += sql_operator;
      break;

    default:
      LOG(ERROR) << fname << ": unknown type: " << info.type;
  }

  // Затереть последние два символа ',' и '\n'
  sql.erase(sql.end() - 2, sql.end());

  // Закрыть транзакцию
  sql += ";\nCOMMIT;";

  if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
      LOG(ERROR) << fname << ": executing SQL: " << m_db_err << ": " << sql;
      sqlite3_free(m_db_err);
  }
  else {
    LOG(INFO) << fname << ": Create point " << info.name;
    rc = 0; // успешно занеслось в БД
  }

#if (VERBOSE > 3)
  LOG(INFO) << fname << ": SQL: " << sql;
#endif

  return rc;
}

// ==========================================================================================



