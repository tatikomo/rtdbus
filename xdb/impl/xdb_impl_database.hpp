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
#include "mcouda.h"  // mco_metadict_header_t
#include "mcohv.h"   // mcohv_p
}
#endif

#include "helper.hpp"
#include "xdb_impl_common.hpp"
#include "xdb_impl_error.hpp"

namespace xdb {

class DatabaseImpl
{
  public:
    // Обеспечить доступ к внутренностям БД для реализации специфики БД Брокера
    friend class DatabaseBrokerImpl;

    DatabaseImpl(const char*, const Options&, mco_dictionary_h);
    ~DatabaseImpl();

    // Инициализация рантайма
    const Error& Init();
    // создание нового экземпяра (с возможностью удаления старого) mco_db_open
    const Error& Create();
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
    // Получить имя базы данных
    const char* getName() { return m_name; };
    // Сменить текущее состояние на новое
    const Error& TransitionToState(DBState_t);
    // Вернуть последнюю ошибку
    const Error& getLastError() const;
    // Вернуть признак отсутствия ошибки
    bool  ifErrorOccured() const;
    // Установить новое состояние ошибки
    void  setError(ErrorType_t);
    // Сбросить ошибку
    void  clearError();
    // Вернуть текущее состояние БД
    DBState_t State() const;

  protected:
    mco_db_h getDbHandler();
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

#ifdef DISK_DATABASE
    char* m_dbsFileName;
    char* m_logFileName;
#endif

#if EXTREMEDB_VERSION >= 40
    mco_db_params_t   *m_db_params;
    mco_device_t      *m_dev;
    /*
     * Internal HttpServer http://localhost:8082/
     */
    mco_metadict_header_t *m_metadict;
    mcohv_p                m_hv;
    unsigned int           m_size;
    bool                   m_metadict_initialized;
#endif
    /*
     * Зарегистрировать все обработчики событий, заявленные в БД
     */
    const Error& RegisterEvents();
    const Error& ConnectToInstance();
};

} // namespace xdb

#endif
