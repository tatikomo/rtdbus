#if !defined XDB_RTAP_DATABASE_IMPL_HPP
#define XDB_RTAP_DATABASE_IMPL_HPP
#pragma once

#include <string>
#include <bitset>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#include "mco.h"
#if EXTREMEDB_VERSION >= 40 && USE_EXTREMEDB_HTTP_SERVER
#include "mcohv.h"
#endif
}
#endif

#include "xdb_core_base.hpp" // class Database

namespace xdb
{

typedef std::bitset<8> BitSet8;

// Позиции бит в флагах, передаваемых конструктору
typedef enum
{
  OF_POS_CREATE   = 1, // создать БД в случае, если это не было сделано ранее
  OF_POS_READONLY = 2, // открыть в режиме "только для чтения"
  OF_POS_RDWR     = 3, // открыть в режиме "чтение|запись" (по умолчанию)
  OF_POS_TRUNCATE = 4, // открыть пустую базу, удалив данные в существующем экземпляре
  OF_POS_LOAD_SNAP= 5  // открыть базу, заполнив ее данными из последнего снапшота
} FlagPos_t;

class RtCoreDatabase : public Database
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
    bool Init();
    // создание нового экземпяра (с возможностью удаления старого) mco_db_open
    bool Create();
    // открытие подключения с помощью mco_db_connect
    bool Connect();
    // Останов рантайма
    bool Disconnect();
    // Загрузка данных из указанного файла
    bool LoadSnapshotFromFile(const char*);

  private:
    DISALLOW_COPY_AND_ASSIGN(RtCoreDatabase);
    mco_db_h     m_db;
    const char*  m_name;
    BitSet8      m_db_access_flags;
#if defined DEBUG
    char  m_snapshot_file_prefix[10];
    bool  m_initialized;
    bool  m_save_to_xml_feature;
    int   m_snapshot_counter;
    MCO_RET SaveDbToFile(const char*);
#endif
#ifdef DISK_DATABASE
    char* m_dbsFileName;
    char* m_logFileName;
#endif
#if EXTREMEDB_VERSION >= 40
    mco_db_params_t   m_db_params;
    mco_device_t      m_dev;
#  if USE_EXTREMEDB_HTTP_SERVER  
    /*
     * Internal HttpServer http://localhost:8082/
     */
    mco_metadict_header_t *m_metadict;
    mcohv_p                m_hv;
    unsigned int           m_size;
#  endif  
    bool                   m_metadict_initialized;
#endif
    /*
     * Зарегистрировать все обработчики событий, заявленные в БД
     */
    MCO_RET RegisterEvents();
    bool    ConnectToInstance();
    bool    LoadFromSnapshot();
    bool    MakeSnapshot();
};

}

#endif
