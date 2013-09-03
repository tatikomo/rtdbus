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
  m_impl = new XDBDatabaseBrokerImpl(this);
  assert(m_impl);
}

XDBDatabaseBroker::~XDBDatabaseBroker()
{
  assert(m_impl);
  delete m_impl;
}

bool XDBDatabaseBroker::Connect()
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->Connect();
}

Service::State XDBDatabaseBroker::GetServiceState(const Service *srv)
{
  assert(m_impl);
  return m_impl->GetServiceState(srv);
}

Service *XDBDatabaseBroker::GetServiceForWorker(const Worker *wrk)
{
  assert(m_impl);
  return m_impl->GetServiceForWorker(wrk);
}

/* найти в БД и вернуть объект типа Сервис */
Service *XDBDatabaseBroker::GetServiceByName(const std::string& service_name)
{
  return GetServiceByName(service_name.c_str());
}

/* найти в БД и вернуть объект типа Сервис */
Service *XDBDatabaseBroker::GetServiceByName(const char *service_name)
{
  assert(m_impl);

  if (!m_impl) return NULL;
  return m_impl->GetServiceByName(service_name);
}

Service *XDBDatabaseBroker::AddService(const std::string& service_name)
{
  assert(m_impl);

  if (!m_impl) return (Service*)NULL;
  return m_impl->AddService(service_name);
}

Service *XDBDatabaseBroker::AddService(const char *service_name)
{
  assert(m_impl);
  assert(service_name);

  if (!m_impl) return (Service*)NULL;
  return m_impl->AddService(service_name);
}

bool XDBDatabaseBroker::RemoveService(const char *service_name)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->RemoveService(service_name);
}

bool XDBDatabaseBroker::RemoveWorker(Worker *wrk)
{
  assert (wrk);
  if (!m_impl) return false;
  return m_impl->RemoveWorker(wrk);
}

bool XDBDatabaseBroker::PushWorker(Worker *wrk)
{
  assert (wrk);
  if (!m_impl) return false;
  return m_impl->PushWorker(wrk);
}

/* Добавить нового Обработчика в спул Сервиса */
// TODO: delete me
bool XDBDatabaseBroker::PushWorkerForService(Service *srv, Worker *wrk)
{
  assert (wrk);
  if (!m_impl) return false;
  return m_impl->PushWorkerForService(srv, wrk);
}

/* Добавить нового Обработчика в спул Сервиса */
Worker* XDBDatabaseBroker::PushWorkerForService(const std::string& srv_name, const std::string& wrk_ident)
{
  if (!m_impl) return NULL;
  return m_impl->PushWorkerForService(srv_name, wrk_ident);
}

bool XDBDatabaseBroker::IsServiceExist(const char *service_name)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->IsServiceExist(service_name);
}

ServiceList* XDBDatabaseBroker::GetServiceList()
{
  assert(m_impl);

  if (!m_impl) return (ServiceList*) NULL;
  return m_impl->GetServiceList();
}

/* Получить первое ожидающее обработки Сообщение */
bool XDBDatabaseBroker::GetWaitingLetter(
        /* IN  */ Service* srv,
        /* IN  */ Worker* wrk,
        /* OUT */ std::string& header,
        /* OUT */ std::string& body)
{
  assert(srv);
  assert(wrk);
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->GetWaitingLetter(srv, wrk, header, body);
}

bool XDBDatabaseBroker::IsServiceCommandEnabled(
        const Service* srv, 
        const std::string& cmd_name)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->IsServiceCommandEnabled(srv, cmd_name);
}

Service *XDBDatabaseBroker::RequireServiceByName(const char *service_name)
{
  assert(m_impl);

  if (!m_impl) return (Service*)NULL;
  return m_impl->RequireServiceByName(service_name);
}

Service *XDBDatabaseBroker::RequireServiceByName(const std::string& service_name)
{
  assert(m_impl);

  if (!m_impl) return (Service*)NULL;
  return m_impl->RequireServiceByName(service_name);
}

Service *XDBDatabaseBroker::GetServiceById(int64_t _id)
{
  assert(m_impl);
  if (!m_impl) return (Service*)NULL;
  return m_impl->GetServiceById(_id);
}

Worker *XDBDatabaseBroker::PopWorker(const char *service_name)
{
  assert(m_impl);

  if (!m_impl) return (Worker*)NULL;
  return m_impl->PopWorker(service_name);
}

Worker *XDBDatabaseBroker::PopWorker(Service *service)
{
  assert(m_impl);

  if (!m_impl) return (Worker*)NULL;
  return m_impl->PopWorker(service);
}

/* Очистить спул Обработчиков указанного Сервиса */
bool XDBDatabaseBroker::ClearWorkersForService(const char *service_name)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->ClearWorkersForService(service_name);
}

/* поместить сообщение во входящую очередь Службы */
bool XDBDatabaseBroker::PushRequestToService(Service* srv, 
        const std::string& header,
        const std::string& data)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->PushRequestToService(srv, header, data);
}

/* Очистить спул Обработчиков и всех Сервисов */
bool XDBDatabaseBroker::ClearServices()
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->ClearServices();
}

/* Вернуть экземпляр Обработчика из БД. Не найден - вернуть NULL */
Worker *XDBDatabaseBroker::GetWorkerByIdent(const char *ident)
{
  if (!m_impl) return (Worker*) NULL;
  return  m_impl->GetWorkerByIdent(ident);
}

Worker *XDBDatabaseBroker::GetWorkerByIdent(const std::string& ident)
{
  return GetWorkerByIdent(ident.c_str());
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

/* Тестовый API сохранения базы */
void XDBDatabaseBroker::MakeSnapshot(const char *msg)
{
  assert(m_impl);
  return m_impl->MakeSnapshot(msg);
}
