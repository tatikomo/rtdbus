#include <assert.h>

#include "xdb_database_broker.hpp"
#include "xdb_database_broker_impl.hpp"
//#include "xdb_database_service.hpp"

class XDBService;
class XDBWorker;

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

  if (!m_impl) return false;
  return m_impl->Open();
}

/* найти в БД и вернуть объект типа Сервис */
XDBService *XDBDatabaseBroker::GetServiceByName(const char *service_name)
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->GetServiceByName(service_name);
}

bool XDBDatabaseBroker::AddService(const char *service_name)
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->AddService(service_name);
}

bool XDBDatabaseBroker::RemoveService(const char *service_name)
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->RemoveService(service_name);
}

bool XDBDatabaseBroker::IsServiceExist(const char *service_name)
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->IsServiceExist(service_name);
}

XDBWorker *XDBDatabaseBroker::GetWaitingWorkerForService(const char *service_name)
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return 0;
  return m_impl->GetWaitingWorkerForService(service_name);
}

XDBWorker *XDBDatabaseBroker::GetWaitingWorkerForService(XDBService *service)
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return 0;
  return m_impl->GetWaitingWorkerForService(service);
}

bool XDBDatabaseBroker::AddWaitingWorkerForService(XDBService *service, XDBWorker *worker)
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->AddWaitingWorkerForService(service, worker);
}

