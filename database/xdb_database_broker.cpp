#include <assert.h>

#include "xdb_database_broker.hpp"
#include "xdb_database_broker_impl.hpp"
#include "xdb_database_service.hpp"
#include "xdb_database_worker.hpp"

class Service;
class Worker;

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

Service *XDBDatabaseBroker::GetServiceForWorker(const Worker *wrk)
{
  assert(m_impl);
  return m_impl->GetServiceForWorker(wrk);
}

/* найти в БД и вернуть объект типа Сервис */
Service *XDBDatabaseBroker::GetServiceByName(const char *service_name)
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

bool XDBDatabaseBroker::RemoveWorker(Worker *wrk)
{
  if (!m_impl) return false;
  return m_impl->RemoveWorker(wrk);
}

bool XDBDatabaseBroker::IsServiceExist(const char *service_name)
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->IsServiceExist(service_name);
}

Service *XDBDatabaseBroker::GetServiceById(int64_t _id)
{
  assert(m_impl);
  return m_impl->GetServiceById(_id);
}

Worker *XDBDatabaseBroker::PopWorkerForService(const char *service_name)
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return 0;
  return m_impl->PopWorkerForService(service_name);
}

Worker *XDBDatabaseBroker::PopWorkerForService(Service *service)
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return 0;
  return m_impl->PopWorkerForService(service);
}

bool XDBDatabaseBroker::PushWorkerForService(Service *service, Worker *worker)
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->PushWorkerForService(service, worker);
}

/* Очистить спул Обработчиков указанного Сервиса */
bool XDBDatabaseBroker::ClearWorkersForService(const char *service_name)
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->ClearWorkersForService(service_name);
}

/* Очистить спул Обработчиков и всех Сервисов */
bool XDBDatabaseBroker::ClearServices()
{
  bool status = false;
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->ClearServices();
}
 
void XDBDatabaseBroker::EnableServiceCommand(const std::string &srv_name, 
        const std::string& command)
{
  assert(m_impl);
  return m_impl->EnableServiceCommand(srv_name, command);
}

void XDBDatabaseBroker::EnableServiceCommand(const Service *srv, 
        const std::string& command)
{
  assert(m_impl);
  return m_impl->EnableServiceCommand(srv, command);
}

void XDBDatabaseBroker::DisableServiceCommand(const std::string &srv_name, 
        const std::string& command)
{
  assert(m_impl);
  return m_impl->DisableServiceCommand(srv_name, command);
}

void XDBDatabaseBroker::DisableServiceCommand(const Service *srv, 
        const std::string& command)
{
  assert(m_impl);
  return m_impl->DisableServiceCommand(srv, command);
}
