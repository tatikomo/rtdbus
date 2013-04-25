#if !defined XDB_DATABASE_BROKER_HPP
#define XDB_DATABASE_BROKER_HPP
#pragma once

#include "xdb_database.hpp"

class XDBDatabaseBrokerImpl;
class XDBService;
class XDBWorker;

class XDBDatabaseBroker : public XDBDatabase
{
  public:
    XDBDatabaseBroker();
    ~XDBDatabaseBroker();

    bool Open();
    bool AddService(const char*);
    bool RemoveService(const char*);
    bool IsServiceExist(const char*);
    XDBService *GetServiceByName(const char*);
    XDBWorker  *GetWaitingWorkerForService(const char*);
    XDBWorker  *GetWaitingWorkerForService(XDBService*);
    bool AddWaitingWorkerForService(XDBService*, XDBWorker*);

  private:
    XDBDatabaseBrokerImpl *m_impl;
};

#endif
