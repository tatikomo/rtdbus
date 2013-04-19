#if !defined XDB_DATABASE_BROKER_HPP
#define XDB_DATABASE_BROKER_HPP
#pragma once

#include "xdb_database.hpp"

class XDBDatabaseBrokerImpl;
class XDBService;

class XDBDatabaseBroker : public XDBDatabase
{
  public:
    XDBDatabaseBroker();
    ~XDBDatabaseBroker();

    bool Open();
    bool AddService(char*);
    bool RemoveService(char*);
    bool IsServiceExist(char*);
    XDBService *GetServiceByName(char*);

  private:
    XDBDatabaseBrokerImpl *m_impl;
};

#endif
