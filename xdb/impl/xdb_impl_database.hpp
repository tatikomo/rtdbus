#pragma once
#ifndef XDB_IMPL_DATABASE_IMPL_HPP
#define XDB_IMPL_DATABASE_IMPL_HPP

#include <string>
#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#include "mco.h"
# if (EXTREMEDB_VERSION >= 40)
#include "mcouda.h"  // mco_metadict_header_t
#include "mcohv.h"   // mcohv_p
# endif
#define SETUP_POLICY
#include "mcoxml.h"
}
#endif

#include "helper.hpp"
#include "xdb_impl_common.hpp"
#include "xdb_impl_error.hpp"

namespace xdb {

class DatabaseImpl
{
  public:
    DatabaseImpl(const char*, const Options&, mco_dictionary_h);
    ~DatabaseImpl();

    // Инициализация рантайма
    const Error& Init();
    // создание нового экземпяра (с возможностью удаления старого) mco_db_open
    const Error& Open();
    // открытие подключения с помощью mco_db_connect
    const Error& Connect();
    // Останов рантайма
    const Error& Disconnect();
    // Загрузка данных из указанного файла
    const Error& LoadSnapshot(const char* = NULL);
    // Сохранение данных в указанный файл в двоичном виде
    const Error& StoreSnapshot(const char* = NULL);
    // Сохранение данных в указанный файл в виде XML
    const Error& SaveAsXML(const char* = NULL, const char* = NULL);
    // Восстановление содержимого БД из указанного XML файла
    const Error& LoadFromXML(const char* = NULL);
    // Получить имя базы данных
    const char* getName() { return m_name; };
    // Сменить текущее состояние на новое
    const Error& TransitionToState(DBState_t);
    // Вернуть последнюю ошибку
    const Error& getLastError() const;
    // Вернуть признак отсутствия ошибки
    bool  ifErrorOccured() const;
    // Установить новое состояние ошибки
    void  setError(ErrorCode_t);
    // Сбросить ошибку
    void  clearError();
    // Вернуть текущее состояние БД
    DBState_t State() const;
    // Управление работой БДРВ
    const Error& Control(rtDbCq&);
    // Интерфейс управления БД - Контроль Точек
    const Error& Query(rtDbCq&);
    // Интерфейс управления БД - Контроль выполнения
    const Error& Config(rtDbCq&);

    mco_db_h getDbHandler();

  protected:
    unsigned int m_snapshot_counter;
    unsigned int m_DatabaseSize;
    unsigned short m_MemoryPageSize;
    unsigned int m_MapAddress;
    unsigned int m_DbDiskCache;
    unsigned int m_DbDiskPageSize;
#ifdef DISK_DATABASE
    char* m_dbsFileName;
    char* m_logFileName;
    #ifndef DB_LOG_TYPE
        #define DB_LOG_TYPE REDO_LOG
    #endif
#endif

    unsigned int getSnapshotCounter();

  private:
    DISALLOW_COPY_AND_ASSIGN(DatabaseImpl);
    DBState_t    m_state;
    Error        m_last_error;
    char         m_name[DBNAME_MAXLEN+1];
    mco_db_h     m_db;
    mco_dictionary_h m_dict;
    Options      m_db_access_flags;
    bool         m_save_to_xml_feature;
    BitSet8      m_flags;
    static       int m_count;

#ifdef DISK_DATABASE
    char* m_dbsFileName;
    char* m_logFileName;
#endif

    // Успешность загрузки снимка БД (приоритет: двоичный, xml)
    bool m_snapshot_loaded;

#if EXTREMEDB_VERSION >= 40
    mco_db_params_t    m_db_params;
    mco_device_t       m_dev;
#if defined USE_EXTREMEDB_HTTP_SERVER
    /*
     * Internal HttpServer http://localhost:8082/
     */
    mco_metadict_header_t *m_metadict;
    mcohv_interface_def_t  m_intf;
    mcohv_p                m_hv;
    unsigned int           m_size;
    bool                   m_metadict_initialized;
#endif

#endif
    /*
     * Зарегистрировать все обработчики событий, заявленные в БД
     */
    const Error& RegisterEvents();
    const Error& ConnectToInstance();
    // Установка общих значимых полей для эксорта/импорта данных в XML
    void setupPolicy(mco_xml_policy_t&);
};

} // namespace xdb

#endif
