///////////////////////////////////////////////////////////////////////////////////////////
// Класс представления модуля EGSA
// 2016/04/14
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "sqlite3.h"
#include "glog/logging.h"

// Класс внешней SMAD, используемой EGSA
#include "exchange_smad_ext.hpp"

ExternalSMAD::ExternalSMAD(const char* snap_file)
 : m_db(NULL),
   m_state(STATE_DISCONNECTED),
   m_db_err(NULL),
   m_snapshot_filename(NULL)
{
  m_snapshot_filename = strdup(snap_file);
}

ExternalSMAD::~ExternalSMAD()
{
  int rc;

  if (SQLITE_OK != (rc = sqlite3_close(m_db)))
  LOG(ERROR) << "Closing ExternalSMAD '" << m_snapshot_filename << "': " << rc;

  free(m_snapshot_filename);
}

smad_connection_state_t ExternalSMAD::connect()
{
  const char *fname = "connect";
  char sql_operator[1000 + 1];
  int printed;
  int rc = 0;

  // ------------------------------------------------------------------
  // Открыть файл ExternalSMAD с базой данных SQLite
  rc = sqlite3_open_v2(m_snapshot_filename,
                       &m_db,
                       // Если таблица только что создана, проверить наличие
                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                       "unix-dotfile");
  if (rc == SQLITE_OK)
  {
    LOG(INFO) << fname << ": ExternalSMAD file '" << m_snapshot_filename << "' successfuly opened";
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

#if 0

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
#endif
  }
  else
  {
    LOG(ERROR) << fname << ": opening InternalSMAD file '" << m_snapshot_filename << "' error=" << rc;
    m_state = STATE_ERROR;
  }

  return m_state;
}
