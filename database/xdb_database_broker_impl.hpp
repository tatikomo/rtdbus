#if !defined XDB_DATABASE_BROKER_IMPL_HPP
#define XDB_DATABASE_BROKER_IMPL_HPP
#pragma once

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "xdb_database_broker.hpp"

class XDBService;

/* Фактическая реализация функциональности Базы Данных для Брокера */
class XDBDatabaseBrokerImpl
{
  public:
    XDBDatabaseBrokerImpl(const XDBDatabaseBroker*, const char*);
    ~XDBDatabaseBrokerImpl();
    bool Open();
    bool AddService(const char*);
    bool RemoveService(const char*);
    bool IsServiceExist(const char*);
    XDBService *GetServiceByName(const char*);

  private:
    const XDBDatabaseBroker *m_self;
    void  LogError(MCO_RET, const char*, const char*);

    mco_db_h db;
    bool AttachToInstance();
#ifdef DISK_DATABASE
    char* m_dbsFileName;
    char* m_logFileName;
#endif
};

#endif
