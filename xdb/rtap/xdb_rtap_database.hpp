#pragma once
#if !defined XDB_RTAP_DATABASE_IMPL_HPP
#define XDB_RTAP_DATABASE_IMPL_HPP

#include <string>

#include "config.h"
#include "xdb_core_common.h"
#include "xdb_core_error.hpp"

namespace xdb {

class Database;

class RtCoreDatabase
{
  public:
    static const unsigned int DatabaseSize;
    static const unsigned int MemoryPageSize;
    static const unsigned int MapAddress;
#ifdef DISK_DATABASE
    static const int DbDiskCache;
    static const int DbDiskPageSize;
    #ifndef DB_LOG_TYPE
        #define DB_LOG_TYPE REDO_LOG
    #endif 
#else 
    static const int DbDiskCache;
    static const int DbDiskPageSize;
#endif 


    RtCoreDatabase(const char*, BitSet8);
    ~RtCoreDatabase();

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
    // Сохранение данных в указанный файл
    const Error& StoreSnapshot(const char* = NULL);
    const char* getName() { return m_name; };

  private:
    DISALLOW_COPY_AND_ASSIGN(RtCoreDatabase);
    Database *m_impl;

    // TODO перенести все оставшиеся ниже поля в имплементацию
    /*mco_db_h*/ void*     m_db;
    const char*  m_name;
    BitSet8      m_db_access_flags;
#if defined DEBUG
    char  m_snapshot_file_prefix[10];
    bool  m_initialized;
    bool  m_save_to_xml_feature;
    int   m_snapshot_counter;
    int   SaveDbToFile(const char*);
#endif
#ifdef DISK_DATABASE
    char* m_dbsFileName;
    char* m_logFileName;
#endif
#if EXTREMEDB_VERSION >= 40
    /*mco_db_params_t*/ void*   m_db_params;
    /*mco_device_t*/ void*      m_dev;
    /*
     * Internal HttpServer http://localhost:8082/
     */
    /*mco_metadict_header_t*/ void *m_metadict;
    /*mcohv_p*/ void*                m_hv;
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
