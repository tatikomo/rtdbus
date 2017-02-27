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
#include "exchange_smad_int.hpp"

// Структура общей таблицы "SMAD"
// ---+-------------+-----------+--------------+
//  # |  Имя поля   | Тип поля  | По умолчанию |
// ---+-------------+-----------+--------------+
//  1 | ID          | INTEGER   | generated    |<---+
//  2 | NAME        | TEXT      |              |    |
//  3 | NATURE      | INTEGER   | 0            |    |   // Тип СС, ЛПУ, ЦДП,...
//  4 | STATE       | INTEGER   | 0            |    |
//  5 | DELEGATION  | INTEGER   | 1            |    |
//  6 | LAST_PROBE  | TIME      |              |    |   // Когда в последний раз опрашивалась модулем
//  7 | LAST_SENT   | TIME      |              |    |   // Последний раз, когда данные были отправлены клиенту
//  8 | MODE        | TEXT      |              |    |   
// ---+-------------+-----------+--------------+    |
//                                                  |
// Структура таблицы описания параметров СС "DICT"  |
// ---+-------------+-----------+--------------+    |
//  # |  Имя поля   | Тип поля  | По умолчанию |    |
// ---+-------------+-----------+--------------+    |
//  1 | SA_REF      | INTEGER   | NOT NULL     |----+   // Ссылка на таблицу структуры SMAD
//  2 | ID          | INTEGER   | generated    |<---+
//  2 | NAME        | TEXT      |              |    |
//  3 | ADDR        | INTEGER   |              |    |
//  4 | TYPE        | INTEGER   | 0            |    |
//  5 | TYPE_SUPPORT| INTEGER   | 0            |    |
//  6 | Timestamp   | TIME      |              |    |   // TODO: для чего хранить индивидуальное время параметрам?
//  7 | LOW_FAN     | INTEGER   | 0            |    |
//  8 | HIGH_FAN    | INTEGER   | 0            |    |
//  9 | LOW_FIS     | REAL      | 0            |    |
// 10 | HIGH_FIS    | REAL      | 0            |    |
// ---+-------------+-----------+--------------+    |
//                                                  |
// Структура таблицы текущих данных СС "DATA"       |
// ---+-------------+-----------+--------------+    |
//  # |  Имя поля   | Тип поля  | По умолчанию |    |
// ---+-------------+-----------+--------------+    |
//  1 | PARAM_REF   | INTEGER   | NOT NULL     |----+   // Ссылка на таблицу описания
//  2 | Timestamp   | TIME      |              |        // Время получения данных от СС, выставляется модулем
//  3 | Timestamp_SA| TIME      |              |        // Время формирования данных в СС, приходит от СС
//  4 | VALID       | INTEGER   | 0            |        // Достоверность расчётная
//  5 | VALID_SA    | INTEGER   | 0            |        // Достоверность, полученная от СС
//  6 | I_VAL       | INTEGER   | 0            |        // Целочисленное значение параметра
//  7 | G_VAL       | REAL      | 0            |        // Дробное значение параметра
// ---+-------------+-----------+--------------+
// Команда создания таблицы SMAD в случае, если ранее её не было
const char *InternalSMAD::s_SQL_CREATE_SMAD_TEMPLATE =
    "CREATE TABLE IF NOT EXISTS %s "
    "("
    "ID INTEGER PRIMARY KEY,"       // Уник. идентификатор записи
    "NAME TEXT NOT NULL,"           // Тег СС
    "NATURE INTEGER DEFAULT 0,"     // Тип системы - локальной СС, ЛПУ, ЦДП,..
    "STATE INTEGER DEFAULT 0,"      // Состояние СС { Недоступно(0)|Инициализация(1)|Работа(2) }
    "DELEGATION INTEGER DEFAULT 1," // Режим управления CC { Местный(1)|Дистанционный(0) }
    "LAST_PROBE INTEGER,"           // Время последнего сеанса работы модуля с Системой Сбора
    "MODE TEXT"                     // Режим работы СС
    ");";
// Шаблон занесения данных в таблицу SMAD
const char *InternalSMAD::s_SQL_INSERT_SMAD_TEMPLATE =
    "INSERT INTO %s (NAME,NATURE,STATE,DELEGATION,LAST_PROBE,MODE) VALUES ('%s',%d,%d,%d,%ld,'%s');";
// Команда создания таблицы НСИ СС в случае, если ранее её не было
const char *InternalSMAD::s_SQL_CREATE_SA_TEMPLATE =
    "CREATE TABLE IF NOT EXISTS %s "
    "("
    "ID INTEGER PRIMARY KEY,"     // Уник. идентификатор записи
    "SA_REF INTEGER NOT NULL,"
    "NAME TEXT NOT NULL,"         // Название параметра
    "ADDR INTEGER,"               // Адрес регистра
    "TYPE INTEGER DEFAULT 0,"     // Тип параметра
    "TYPE_SUPPORT INTEGER DEFAULT 0,"       // Тип обработки
    "Timestamp INTEGER DEFAULT 0,"// Время какое-то. Неястно пока для чего, но пусть будет.
    "LOW_FAN INTEGER DEFAULT 0,"  // (только для аналогов) Нижняя граница инженерного диапазона
    "HIGH_FAN INTEGER DEFAULT 0," // (только для аналогов) Верхняя граница инженерного диапазона
    "LOW_FIS REAL DEFAULT 0,"     // (только для аналогов) Нижняя граница физического диапазона
    "HIGH_FIS REAL DEFAULT 0,"    // (только для аналогов) Верхняя граница физического диапазона
    "FACTOR REAL DEFAULT 1,"      // (только для аналогов) Коэффициент масштабирования
//    "PRIMARY KEY (SA_REF,ADDR,TYPE_SUPPORT) ON CONFLICT ABORT,"
    "FOREIGN KEY (SA_REF) REFERENCES %s(ID) ON DELETE CASCADE"
    ");";
// Шаблон заголовка SQL-выражения занесения данных целочисленного типа в базу
const char *InternalSMAD::s_SQL_INSERT_SA_HEADER_INT_TEMPLATE =
    "INSERT INTO %s (SA_REF,NAME,ADDR,TYPE,TYPE_SUPPORT) VALUES\n";
// Шаблон тела SQL-выражения занесения данных целочисленного типа в базу
const char *InternalSMAD::s_SQL_INSERT_SA_VALUES_INT_TEMPLATE =
//    SA_REF
//    |     NAME
//    |     |   ADDR
//    |     |   |  TYPE
//    |     |   |  |  TYPE_SUPPORT
//    |     |   |  |  |
    "(%lld,'%s',%d,%d,%d),\n";

// Шаблон заголовка SQL-выражения занесения данных с плав. запятой в базу
const char *InternalSMAD::s_SQL_INSERT_SA_HEADER_FLOAT_TEMPLATE =
    "INSERT INTO %s (SA_REF,NAME,ADDR,TYPE,TYPE_SUPPORT,LOW_FAN,HIGH_FAN,LOW_FIS,HIGH_FIS,FACTOR) VALUES\n";
// Шаблон тела SQL-выражения занесения данных с плав. запятой в базу
const char *InternalSMAD::s_SQL_INSERT_SA_VALUES_FLOAT_TEMPLATE =
//    SA_REF
//    |     NAME
//    |     |   ADDR
//    |     |   |  TYPE
//    |     |   |  |  TYPE_SUPPORT
//    |     |   |  |  |  LOW_FAN
//    |     |   |  |  |  |  HIGH_FAN
//    |     |   |  |  |  |  |  LOW_FIS
//    |     |   |  |  |  |  |  |  HIGH_FIS
//    |     |   |  |  |  |  |  |  |  FACTOR
//    |     |   |  |  |  |  |  |  |  |
    "(%lld,'%s',%d,%d,%d,%d,%d,%g,%g,%g),\n";

// Таблица с накапливаемыми значениями параметров
const char *InternalSMAD::s_SQL_CREATE_SA_DATA_TEMPLATE =
    "CREATE TABLE IF NOT EXISTS %s"
    "("
    "PARAM_REF INTEGER NOT NULL,"   // Ссылка на таблицу с описателями параметров СС
    "Timestamp INTEGER DEFAULT 0,"  // Время получения данных, выставляется модулем
    "Timestamp_ACQ INTEGER DEFAULT 0,"   // Время, переданное системой сбора
    "VALID INTEGER DEFAULT 0,"      // Расчётное значение Достоверности параметра
    "VALID_ACQ INTEGER DEFAULT 0,"  // Значение Достоверности параметра, полученное от СС
    "I_VAL INTEGER DEFAULT 0,"      // (только для дискретов) Значение параметра
    "G_VAL REAL DEFAULT 0,"         // (только для аналогов) Значение параметра
    "PRIMARY KEY (PARAM_REF,Timestamp) ON CONFLICT ABORT,"
    "FOREIGN KEY (PARAM_REF) REFERENCES %s(ID) ON DELETE CASCADE"
    ");";

// Шаблон заголовка SQL-выражения занесения целочисленных данных в базу
const char *InternalSMAD::s_SQL_INSERT_SA_DATA_HEADER_INT_TEMPLATE =
    "INSERT INTO %s (PARAM_REF,Timestamp,VALID,VALID_ACQ,I_VAL) VALUES\n";
const char *InternalSMAD::s_SQL_INSERT_SA_DATA_VALUES_INT_TEMPLATE =
    //PARAM_REF
    //|    Timestamp
    //|    |   VALID
    //|    |   |  VALID_ACQ
    //|    |   |  |  I_VALUE
    //|    |   |  |  |
    "(%lld,%ld,%d,%d,%d);";

// Шаблон заголовка SQL-выражения занесения данных с плав. запятой в базу
const char *InternalSMAD::s_SQL_INSERT_SA_DATA_HEADER_FLOAT_TEMPLATE =
    "INSERT INTO %s (PARAM_REF,Timestamp,VALID,VALID_ACQ,G_VAL) VALUES\n";
const char *InternalSMAD::s_SQL_INSERT_SA_DATA_VALUES_FLOAT_TEMPLATE =
    //PARAM_REF
    //|    Timestamp
    //|    |   VALID
    //|    |   |  VALID_ACQ
    //|    |   |  |  G_VALUE
    //|    |   |  |  |
    "(%lld,%ld,%d,%d,%g);";

// Выбрать ссылку параметра из справочной таблицы указанной системы сбора
const char *InternalSMAD::s_SQL_SELECT_SA_DICT_REF =
    "SELECT ID FROM %s WHERE SA_REF=%lld AND NAME='%s';";
// Выбрать количество записей об указанной системе сбора из конфигурации SMAD
const char *InternalSMAD::s_SQL_SELECT_SMAD_SA_COUNT =
    "SELECT COUNT(*) FROM %s WHERE NAME='%s';";
// Прочитать идентификатор искомой СС из таблицы SMAD
const char *InternalSMAD::s_SQL_SELECT_SA_ID_FROM_SMAD =
    "SELECT ID FROM %s WHERE NAME='%s';";
// Удалить данные очищаемой СС из аккумуляторной таблицы по идентификатору
const char *InternalSMAD::s_SQL_DELETE_SA_DATA_BY_REF =
    "DELETE FROM %s WHERE DATA.PARAM_REF IN (SELECT ID FROM DICT WHERE SA_REF=%lld);";
// Удалить данные очищаемой СС из аккумуляторной таблицы по её имени
const char *InternalSMAD::s_SQL_DELETE_SA_DATA_BY_NAME =
    "DELETE FROM DATA WHERE DATA.PARAM_REF IN (SELECT DICT.ID FROM DICT,SMAD WHERE SMAD.NAME='%s' and DICT.SA_REF=SMAD.ID);";
// Удалить НСИ очищаемой СС по её идентификатору
const char *InternalSMAD::s_SQL_DELETE_SA_DICT_BY_REF =
    "DELETE FROM %s WHERE DICT.SA_REF=%lld;";
// Удалить НСИ очищаемой СС по её имени
const char *InternalSMAD::s_SQL_DELETE_SA_DICT_BY_NAME =
    "DELETE FROM DICT WHERE DICT.SA_REF=(SELECT SMAD.ID FROM SMAD WHERE SMAD.NAME='%s');";
// Обновить время последнего успешного обмена с указанной СС
const char *InternalSMAD::s_SQL_UPDATE_SMAD_SA_LASTPROBE =
    "UPDATE OR ABORT %s SET LAST_PROBE=%ld WHERE ID=%lld;";
// Проверить существование таблицы с указанным именем
// NB: Ключевое слово "table" должно быть строчным, регистр имеет значение
const char *InternalSMAD::s_SQL_CHECK_TABLE_TEMPLATE =
    "SELECT count(*) FROM sqlite_master WHERE type='table' AND NAME='%s';";
// Название таблицы с описанием систем сбора, включенных в эту зону SMAD
const char *InternalSMAD::s_SMAD_DESCRIPTION_TABLENAME = "SMAD";
// Название таблицы с описанием параметров, принадлежащих данной СС
const char *InternalSMAD::s_SA_DICT_TABLENAME = "DICT";
// Название таблицы, аккумулирующей данные от всех систем сбора этой зоны SMAD
const char *InternalSMAD::s_SA_DATA_TABLENAME = "DATA";

// ==========================================================================================
InternalSMAD::InternalSMAD(const char* sa_name, gof_t_SacNature sa_nature, const char *filename)
 : m_db(NULL),
   m_state(STATE_DISCONNECTED),
   m_sa_nature(sa_nature),
   m_db_err(NULL),
   m_snapshot_filename(NULL),
   m_sa_name(NULL),
   m_sa_reference(0)
{
  LOG(INFO) << "Constructor InternalSMAD";
  m_sa_name = strdup(sa_name);
  m_snapshot_filename = strdup(filename);
}

// ==========================================================================================
InternalSMAD::~InternalSMAD()
{
  int rc;

  LOG(INFO) << "Destructor InternalSMAD";

  free(m_snapshot_filename); // NB: именно free(), а не delete
  free(m_sa_name); // NB: именно free(), а не delete

  if (SQLITE_OK != (rc = sqlite3_close(m_db)))
    LOG(ERROR) << "Closing InternalSMAD DB: " << rc;
}

// ==========================================================================================
// Подключиться к InternalSMAD в режиме выгрузки данных, справочные таблицы не создаются
// Используется модулем EGSA
smad_connection_state_t InternalSMAD::attach(const char* sa_name)
{
  const char *fname = "attach";
  int rc = 0;


  // ------------------------------------------------------------------
  // Открыть файл InternalSMAD с базой данных SQLite
  rc = sqlite3_open_v2(m_snapshot_filename,
                       &m_db,
                       // Если таблица только что создана, проверить наличие
                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                       "unix-dotfile");
  if (rc == SQLITE_OK)
  {
    LOG(INFO) << fname << ": InternalSMAD file '" << m_snapshot_filename << "' successfuly opened";
    m_state = STATE_OK;
  }
  else {
    LOG(ERROR) << fname << ": opening InternalSMAD file '" << m_snapshot_filename << "' error=" << rc;
    m_state = STATE_ERROR;
  }

  // NB: Подключение EGSA происходит до запуска модулей систем сбора,
  // в результате справочные таблицы к этому моменту еще не созданы.
  // Это необходимо учитывать при попытках чтения данных из подключенного SMAD,
  // и если идентификатор m_sa_reference равен нулю, попытаться его прочитать заново.
  // Если после этого идентификатор все еще равен нулю, значит модуль системы сброра
  // еще не запустился.
  if ((STATE_OK == m_state) && (true == get_sa_reference(m_sa_name, m_sa_reference)))
  {
    LOG(INFO) << "OK Attach to SA '" << sa_name << "'";
    m_state = STATE_OK;
  }
  else {
    LOG(ERROR) << "FAIL attach to SA '" << sa_name << "'";
    m_state = STATE_DISCONNECTED;
  }
  return m_state;
}

// ==========================================================================================
// Подключиться к InternalSMAD, создав все необходимые таблицы
// Используется модулем СС
smad_connection_state_t InternalSMAD::connect()
{
  const char *fname = "connect";
  char sql_operator[1000 + 1];
  int printed;
  int rc = 0;

  // ------------------------------------------------------------------
  // Открыть файл InternalSMAD с базой данных SQLite
  rc = sqlite3_open_v2(m_snapshot_filename,
                       &m_db,
                       // Если таблица только что создана, проверить наличие
                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                       "unix-dotfile");
  if (rc == SQLITE_OK)
  {
    LOG(INFO) << fname << ": InternalSMAD file '" << m_snapshot_filename << "' successfuly opened";
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


    // Успешное подключение к БД
    // ------------------------------------------------------------------
    // 1) Создать общую таблицу SMAD, если она ранее не существовала
    printed = snprintf(sql_operator, 1000,
                       s_SQL_CREATE_SMAD_TEMPLATE,
                       s_SMAD_DESCRIPTION_TABLENAME);
    assert(printed < 1000);
    if (true == create_table(s_SMAD_DESCRIPTION_TABLENAME, sql_operator))
    {
      // Проверить запись о локальной СС.
      // Если её не было ранее, создать новую запись.
      // Получить идентификатор записи локальной СС в общей таблице SMAD
      if (true == get_sa_reference(m_sa_name, m_sa_reference))
      {
        // ------------------------------------------------------------------
        // Всегда заново создавать данные CC, даже если они ранее существовали.
        // Первичной информацией должен быть конфигурационный файл, иначе возможна
        // ситуация с рассинхронизацией структуры БД InternalSMAD и конфигурационного файла.
        drop_dict(m_sa_name, m_sa_reference);
      }

      // Попытка создать таблицу НСИ "DICT", если она была удалена ранее
      printed = snprintf(sql_operator,
               1000,
               s_SQL_CREATE_SA_TEMPLATE,
               s_SA_DICT_TABLENAME,             // Название таблицы = "DICT"
               s_SMAD_DESCRIPTION_TABLENAME);   // Название таблицы SMAD, содержащую внешние ссылки
      assert(printed < 1000);
      if (false == create_table(s_SA_DICT_TABLENAME, sql_operator)) {
        LOG(ERROR) << "Creating table " << s_SA_DICT_TABLENAME;
        m_state = STATE_ERROR;
      }
      else {
        // Создать таблицу "DATA" с аккумулируемыми данными от СС
        sprintf(sql_operator,
                s_SQL_CREATE_SA_DATA_TEMPLATE,
                s_SA_DATA_TABLENAME,    // Название таблицы-аккумулятора "DATA"
                s_SA_DICT_TABLENAME);   // Название таблицы НСИ "DICT", содержащую внешние ссылки
        if (true == create_table(s_SA_DATA_TABLENAME, sql_operator)) {
          LOG(INFO) << "Creating accumulative data table '"
                    << s_SA_DATA_TABLENAME << "' is OK";
        }
        else {
          LOG(INFO) << "Creating accumulative data table '"
                    << s_SA_DATA_TABLENAME << "' is FAIL";
        }
      }
    }
  }
  else
  {
    LOG(ERROR) << fname << ": opening InternalSMAD file '" << m_snapshot_filename << "' error=" << rc;
    m_state = STATE_ERROR;
  }

  return m_state;
}

// ------------------------------------------------------------------
// Выгрузить данные параметров, имещих флаг модификации с предыдущего опроса
// Этот список затем используется для сброса флага модификации
int InternalSMAD::pop(sa_parameters_t& params)
{
  int rc = NOK;
  sa_parameter_info_t test;

  LOG(INFO) << "Read last acquired parameters";

  params.clear();

  strcpy(test.name, "TEST");
  test.address = 101;
  test.type = SA_PARAM_TYPE_TM;
  test.g_value = 3.141592;
  test.valid = 1;

  params.push_back(test);
  rc = OK;

  return rc;
}

// ------------------------------------------------------------------
// Всегда заново создавать данные CC, даже если они ранее существовали.
// Первичной информацией должен быть конфигурационный файл, иначе возможна
// ситуация с рассинхронизацией структуры БД InternalSMAD и конфигурационного файла.
// Удалить записи о параметрах данной СС из DATA, затем удалить НСИ из DICT
int InternalSMAD::drop_dict(const char* sa_name, uint64_t sa_ref)
{
  const char* fname = "drop_dict";
  int rc = NOK;
  int printed;
  std::string sql;
  char sql_operator[1000 + 1];

  LOG(INFO) << "Drop all '" << sa_name << "' info with SMAD.reference=" << sa_ref;

  sql = "BEGIN;";

  if (table_existance(s_SA_DATA_TABLENAME)) {
    printed = snprintf(sql_operator, 1000,
                     s_SQL_DELETE_SA_DATA_BY_REF,
                     s_SA_DATA_TABLENAME,
                     sa_ref
                    );
    assert(printed < 1000);
    sql += "\n";
    sql += sql_operator;
  }
  else {
    LOG(WARNING) << "Try delete from unexistent table " << s_SA_DATA_TABLENAME;
  }

  if (table_existance(s_SA_DICT_TABLENAME)) {
    printed = snprintf(sql_operator, 1000,
                       s_SQL_DELETE_SA_DICT_BY_REF,
                       s_SA_DICT_TABLENAME,
                       sa_ref
                      );
    assert(printed < 1000);
    sql += "\n";
    sql += sql_operator;
  }
  else {
    LOG(WARNING) << "Try delete from unexistent table " << s_SA_DICT_TABLENAME;
  }

  sql += "\nCOMMIT;";

  if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
        LOG(ERROR) << fname << ": clear all info for SA '" << sa_name
                   << "': " << m_db_err << " (" << sql << ")";
        sqlite3_free(m_db_err);
  }
  else {
    rc = OK;
#if (VERBOSE >= 4)
    LOG(INFO) << fname << ": SUCCESS SQL (" << sql << ")";
#endif
  }

/*
 * select DATA.TIMESTAMP,DATA.PARAM_REF,DICT.NAME from DATA,DICT,SMAD
 * where DICT.SA_REF = SMAD.ID
 * and DATA.PARAM_REF IN (SELECT ID FROM DICT WHERE SA_REF=DICT.SA_REF AND SA_REF=1)
 * and SMAD.NAME = 'BI4500';
*/
  return rc;
}

// ==========================================================================================
// Обновить отметку времени в таблице SMAD
int InternalSMAD::update_last_acq_info()
{
  const char* fname = "update_last_acq_info";
  std::string sql = "BEGIN;\n";
  char sql_operator[1000 + 1];
  int printed;
  int status = NOK;

  printed = snprintf(sql_operator, 1000,
                     s_SQL_UPDATE_SMAD_SA_LASTPROBE,
                     s_SMAD_DESCRIPTION_TABLENAME,
                     time(0),
                     m_sa_reference);
  assert(printed < 1000);

  sql += sql_operator;
  sql += "\nCOMMIT;";

  if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
    LOG(ERROR) << fname << ": update last acquisition time - " << m_db_err << " (" << sql << ")";
    sqlite3_free(m_db_err);
  }
  else status = OK;

  return status;
}


// ==========================================================================================
// Занести указанное значение в InternalSMAD до тех пор, пока верхний уровень не заберет его,
// или не будет превышено время хранения в InternalSMAD.
// Предполагается, что вызывающий контекст переводит режим фиксации траназакции в "MEMORY".
int InternalSMAD::push(const sa_parameter_info_t& info)
{
  const char* fname = "push";
  int status = NOK;
  std::string sql;
  char sql_operator[1000 + 1];

  // Занести текущее значение в БД, как новую строку
  LOG(INFO) << "Push " << info.name << " with ref=" << info.ref;

  sql = "BEGIN;\n";

  switch(info.type) {
    case SA_PARAM_TYPE_TM:
      // Подготовили заголовок
      status = snprintf(sql_operator,
                         1000,
                         // INSERT INTO DATA (PARAM_REF,Timestamp,VALID,VALID_ACQ,I_VAL) VALUES
                         s_SQL_INSERT_SA_DATA_HEADER_FLOAT_TEMPLATE,
                         s_SA_DATA_TABLENAME);
      assert(status < 1000);
      sql += sql_operator;

      status = snprintf(sql_operator,
                         1000,
                         s_SQL_INSERT_SA_DATA_VALUES_FLOAT_TEMPLATE,
                            //  PARAM_REF
                            //  |   Timestamp_ACQ
                            //  |   |   VALID
                            //  |   |   |  VALID_ACQ
                            //  |   |   |  |  G_VAL
                            //  |   |   |  |  |
                            // (%ld,%ld,%d,%d,%g)
                         info.ref,
                         info.rtdbus_timestamp,
                         info.valid,
                         info.valid_acq,
                         info.g_value
                         );
      assert(status < 1000);
      sql += sql_operator;
      sql += "\nCOMMIT;";
      break;

    case SA_PARAM_TYPE_AL:
    case SA_PARAM_TYPE_TS:
    case SA_PARAM_TYPE_TSA:
      // Подготовили заголовок
      status = snprintf(sql_operator,
                         1000,
                         // INSERT INTO DATA (PARAM_REF,Timestamp,VALID,VALID_ACQ,G_VAL) VALUES
                         s_SQL_INSERT_SA_DATA_HEADER_INT_TEMPLATE,
                         s_SA_DATA_TABLENAME);
      assert(status < 1000);
      sql += sql_operator;

      status = snprintf(sql_operator,
                         1000,
                         s_SQL_INSERT_SA_DATA_VALUES_INT_TEMPLATE,
                            //  PARAM_REF
                            //  |   Timestamp_ACQ
                            //  |   |   VALID
                            //  |   |   |  VALID_ACQ
                            //  |   |   |  |  I_VAL
                            //  |   |   |  |  |
                            // (%ld,%ld,%d,%d,%d)
                         info.ref,
                         info.rtdbus_timestamp,
                         info.valid,
                         info.valid_acq,
                         info.i_value
                         );
      assert(status < 1000);
      sql += sql_operator;
      sql += "\nCOMMIT;";

      break;

    default:
      LOG(ERROR) << "Бяка";
      assert(0 == 1);
  }

  if (sqlite3_exec(m_db, sql.c_str(), 0, 0, &m_db_err)) {
      LOG(ERROR) << fname << ": parameter '" << info.name
                 << "': " << m_db_err << " (" << sql << ")";
      sqlite3_free(m_db_err);
  }
  else status = OK;
  
  return status;
}

// ==========================================================================================
int InternalSMAD::purge(time_t since)
{
  int status = NOK;

  LOG(INFO) << "Purge since " << since;

  // TODO: удалить данные, принадлежащие текущей системе сбора, за интервал
  // от указанного момента до текущего времени.
  return status;
}

// ===========================================================================
// Создать соответствующую таблицу, если она ранее не существовала
bool InternalSMAD::create_table(const char* table_name, const char* create_template)
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
                 << "' if not exist: " << m_db_err;
      created = false;
      sqlite3_free(m_db_err);
      LOG(ERROR) << sql;
  }
  else LOG(INFO) << "Table '" << table_name << "' creates OK (if not exist), sql size=" << printed;

  return created;
}

// ===========================================================================
bool InternalSMAD::table_existance(const char *table_name)
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

  retcode = sqlite3_prepare(m_db, sql_operator, -1, &stmt, 0);
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

// ==========================================================================================
bool InternalSMAD::get_sa_reference(char* system_name, uint64_t& reference)
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
    // Выбрать количество записей об указанной системе сбора из конфигурации SMAD
    printed = snprintf(sql_operator,
                       MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                       s_SQL_SELECT_SMAD_SA_COUNT,
                       s_SMAD_DESCRIPTION_TABLENAME,
                       system_name);
    assert(printed < MAX_BUFFER_SIZE_FOR_SQL_COMMAND);

    printed = sqlite3_prepare(m_db, sql_operator, -1, &stmt, 0);
    if (printed != SQLITE_OK) {
      LOG(ERROR) << fname << ": Could not get count of " << system_name
                 << " from " << s_SMAD_DESCRIPTION_TABLENAME << ", code="<< printed;
      break;
    }

    // Поместить словарь тегов в actual_hist_points
    while(sqlite3_step(stmt) == SQLITE_ROW) {
      count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (0 == count) {
      // В таблице SMAD нет записи об искомой СС, завести её
      printed = snprintf(sql_operator,
                         MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                         s_SQL_INSERT_SMAD_TEMPLATE,
                         s_SMAD_DESCRIPTION_TABLENAME,
                         system_name,
                         m_sa_nature,
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
    else if (1 == count) {
      // Нормальная ситуация, одна запись для СС
      LOG(INFO) << "SA " << system_name << " is found";
    }
    else {
      LOG(ERROR) << "Detected multiple records (" << count << ") for SA " << system_name;
      assert(0 == 1);
    }

    // Прочитать идентификатор искомой СС из таблицы SMAD
    printed = snprintf(sql_operator,
                       MAX_BUFFER_SIZE_FOR_SQL_COMMAND,
                       s_SQL_SELECT_SA_ID_FROM_SMAD,
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
        reference = sqlite3_column_int64(stmt, 0);
#if (VERBOSE > 2)
        LOG(INFO) << system_name << " reference=" << reference;
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
void InternalSMAD::accelerate(bool speedup)
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

  if (speedup) {
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
  }
  else {
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
bool InternalSMAD::drop_table(const char* table_name)
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
int InternalSMAD::setup_parameter(sa_parameter_info_t& info)
{
  int rc = NOK;
  char sql_operator[1000 + 1];
  std::string sql;
  const char *fname = "setup_parameter";
  int printed;
  sqlite3_stmt* stmt = 0;

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
    case SA_PARAM_TYPE_TS:
    case SA_PARAM_TYPE_TSA:
    case SA_PARAM_TYPE_AL:
    case SA_PARAM_TYPE_SA:
      printed = snprintf(sql_operator,
                         1000,
                         s_SQL_INSERT_SA_HEADER_INT_TEMPLATE,
                         s_SA_DICT_TABLENAME);
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

    case SA_PARAM_TYPE_TM:
      printed = snprintf(sql_operator,
                         1000,
                         s_SQL_INSERT_SA_HEADER_FLOAT_TEMPLATE,
                         s_SA_DICT_TABLENAME);
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
    rc = OK; // успешно занеслось в БД
  }

#if (VERBOSE > 3)
  LOG(INFO) << fname << ": SQL: " << sql;
#endif

  // ----------------------------------------------------------------
  // TODO: Прочитать значение идентификатора, и занести в словарь
  // info.ref = "SELECT ID FROM _DICT WHERE SA_REF=<Ссылка на СС> AND NAME=<TAG>"
  printed = snprintf(sql_operator,
                     1000,
    // SELECT ID FROM DICT WHERE SA_REF=%lld AND NAME='%s'
                     s_SQL_SELECT_SA_DICT_REF,
                     s_SA_DICT_TABLENAME,
                     m_sa_reference,
                     info.name);
  assert (printed < 1000);
  info.ref = 0;

  printed = sqlite3_prepare(m_db, sql_operator, -1, &stmt, 0);
  if (printed != SQLITE_OK) {
    LOG(ERROR) << fname << ": select parameter reference: "
               << " (" << sql_operator << ")";
  }
  else {
    // Поместить словарь тегов в actual_hist_points
    while(sqlite3_step(stmt) == SQLITE_ROW) {
      info.ref = sqlite3_column_int64(stmt, 0);
//#if (VERBOSE > 2)
      LOG(INFO) << m_sa_name << ":" << info.name << " reference=" << info.ref;
//#endif
    }
    sqlite3_finalize(stmt);

    if (0 == info.ref) {
      // В таблице SMAD не обнаружена запись об искомой точке в справочнике, хотя
      // мы её только что занесли...
      LOG(ERROR) << m_sa_name << ":" << info.name << " reference not found (" << sql_operator << ")";
      assert(0 == 1);
    }
    else {
      LOG(INFO) << fname << ": Select point '" << info.name << "' reference=" << info.ref;
      rc = OK; // успешно прочитали ссылку
    }
  }

  return rc;
}

// ==========================================================================================



