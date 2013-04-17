#include <assert.h>

#include "xdb_database_broker.hpp"
#include "xdb_database_broker_impl.hpp"

const char *database_name = "BrokerDB";

XDBDatabaseBroker::XDBDatabaseBroker() : XDBDatabase(database_name)
{
  m_impl = new XDBDatabaseBrokerImpl(this, database_name);
  assert(m_impl);
}

XDBDatabaseBroker::~XDBDatabaseBroker()
{
  assert(m_impl);
  delete m_impl;
}

bool XDBDatabaseBroker::Open()
{
  bool status = false;
  assert(m_impl);

  return m_impl->Open();
}

bool XDBDatabaseBroker::AddService(char *service_name)
{
  bool status = false;
  assert(m_impl);

  return m_impl->AddService(service_name);
}

bool XDBDatabaseBroker::RemoveService(char *service_name)
{
  bool status = false;
  assert(m_impl);

  return m_impl->RemoveService(service_name);
}

bool XDBDatabaseBroker::IsServiceExist(char *service_name)
{
  bool status = false;
  assert(m_impl);

  return m_impl->IsServiceExist(service_name);
}

