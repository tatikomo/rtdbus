#if !defined XDB_DATABASE_BROKER_HPP
#define XDB_DATABASE_BROKER_HPP
#pragma once

#include "xdb_database.hpp"

class XDBDatabaseBrokerImpl;

class XDBDatabaseBroker : public XDBDatabase
{
  public:
    XDBDatabaseBroker();
    ~XDBDatabaseBroker();

    bool Open();
    bool AddService(char *service_name);
    bool RemoveService(char *service_name);
    bool IsServiceExist(char *service_name);

  private:
    XDBDatabaseBrokerImpl *m_impl;
};

#endif
