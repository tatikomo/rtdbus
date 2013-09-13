#include <assert.h>

#include "xdb_database_broker.hpp"
#include "xdb_database_broker_impl.hpp"
#include "xdb_database_service.hpp"
#include "xdb_database_worker.hpp"

using namespace xdb;

class Service;
class Worker;

const char *database_name = "BrokerDB";

DatabaseBroker::DatabaseBroker() : Database(database_name)
{
  m_impl = new DatabaseBrokerImpl(this);
  assert(m_impl);
}

DatabaseBroker::~DatabaseBroker()
{
  assert(m_impl);
  delete m_impl;
}

bool DatabaseBroker::Connect()
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->Connect();
}

bool DatabaseBroker::SetWorkerState(xdb::Worker* worker, xdb::Worker::State new_state)
{
  assert(m_impl);
  return m_impl->SetWorkerState(worker, new_state);
}

xdb::Service *DatabaseBroker::GetServiceForWorker(const xdb::Worker *wrk)
{
  assert(m_impl);
  return m_impl->GetServiceForWorker(wrk);
}

/* найти в БД и вернуть объект типа Сервис */
xdb::Service *DatabaseBroker::GetServiceByName(const std::string& service_name)
{
  return GetServiceByName(service_name.c_str());
}

/* найти в БД и вернуть объект типа Сервис */
xdb::Service *DatabaseBroker::GetServiceByName(const char *service_name)
{
  assert(m_impl);

  if (!m_impl) return NULL;
  return m_impl->GetServiceByName(service_name);
}

xdb::Service *DatabaseBroker::AddService(const std::string& service_name)
{
  assert(m_impl);

  if (!m_impl) return (xdb::Service*)NULL;
  return m_impl->AddService(service_name);
}

xdb::Service *DatabaseBroker::AddService(const char *service_name)
{
  assert(m_impl);
  assert(service_name);

  if (!m_impl) return (xdb::Service*)NULL;
  return m_impl->AddService(service_name);
}

bool DatabaseBroker::RemoveService(const char *service_name)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->RemoveService(service_name);
}

bool DatabaseBroker::RemoveWorker(xdb::Worker *wrk)
{
  assert (wrk);
  if (!m_impl) return false;
  return m_impl->RemoveWorker(wrk);
}

bool DatabaseBroker::PushWorker(xdb::Worker *wrk)
{
  assert (wrk);
  if (!m_impl) return false;
  return m_impl->PushWorker(wrk);
}

/* Добавить нового Обработчика в спул Сервиса */
// TODO: delete me
bool DatabaseBroker::PushWorkerForService(xdb::Service *srv, xdb::Worker *wrk)
{
  assert (wrk);
  if (!m_impl) return false;
  return m_impl->PushWorkerForService(srv, wrk);
}

/* Добавить нового Обработчика в спул Сервиса */
xdb::Worker* DatabaseBroker::PushWorkerForService(const std::string& srv_name, const std::string& wrk_ident)
{
  if (!m_impl) return NULL;
  return m_impl->PushWorkerForService(srv_name, wrk_ident);
}

bool DatabaseBroker::IsServiceExist(const char *service_name)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->IsServiceExist(service_name);
}

ServiceList* DatabaseBroker::GetServiceList()
{
  assert(m_impl);

  if (!m_impl) return (ServiceList*) NULL;
  return m_impl->GetServiceList();
}

/* Получить первое ожидающее обработки Сообщение */
Letter* DatabaseBroker::GetWaitingLetter(xdb::Service* srv)
{
  assert(m_impl);

  if (!m_impl) return NULL;
  return m_impl->GetWaitingLetter(srv);
}

bool DatabaseBroker::IsServiceCommandEnabled(
        const xdb::Service* srv, 
        const std::string& cmd_name)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->IsServiceCommandEnabled(srv, cmd_name);
}

bool DatabaseBroker::AssignLetterToWorker(xdb::Worker* worker, Letter* letter)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->AssignLetterToWorker(worker, letter);
}

xdb::Service *DatabaseBroker::RequireServiceByName(const char *service_name)
{
  assert(m_impl);

  if (!m_impl) return (xdb::Service*)NULL;
  return m_impl->RequireServiceByName(service_name);
}

xdb::Service *DatabaseBroker::RequireServiceByName(const std::string& service_name)
{
  assert(m_impl);

  if (!m_impl) return (xdb::Service*)NULL;
  return m_impl->RequireServiceByName(service_name);
}

xdb::Service *DatabaseBroker::GetServiceById(int64_t _id)
{
  assert(m_impl);
  if (!m_impl) return (xdb::Service*)NULL;
  return m_impl->GetServiceById(_id);
}

xdb::Worker *DatabaseBroker::PopWorker(const std::string& service_name)
{
  assert(m_impl);

  if (!m_impl) return (xdb::Worker*)NULL;
  return m_impl->PopWorker(service_name);
}

xdb::Worker *DatabaseBroker::PopWorker(xdb::Service *service)
{
  assert(m_impl);

  if (!m_impl) return (xdb::Worker*)NULL;
  return m_impl->PopWorker(service);
}

/* Очистить спул Обработчиков указанного Сервиса */
bool DatabaseBroker::ClearWorkersForService(const char *service_name)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->ClearWorkersForService(service_name);
}

/* поместить сообщение во входящую очередь Службы */
bool DatabaseBroker::PushRequestToService(xdb::Service* srv, 
        const std::string& header,
        const std::string& data)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->PushRequestToService(srv, header, data);
}

/* поместить сообщение во входящую очередь Службы */
bool DatabaseBroker::PushRequestToService(xdb::Service* srv, Letter* letter)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->PushRequestToService(srv, letter);
}

/* Очистить спул Обработчиков и всех Сервисов */
bool DatabaseBroker::ClearServices()
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->ClearServices();
}

/* Вернуть экземпляр Обработчика из БД. Не найден - вернуть NULL */
xdb::Worker *DatabaseBroker::GetWorkerByIdent(const char *ident)
{
  if (!m_impl) return (xdb::Worker*) NULL;
  return  m_impl->GetWorkerByIdent(ident);
}

xdb::Worker *DatabaseBroker::GetWorkerByIdent(const std::string& ident)
{
  return GetWorkerByIdent(ident.c_str());
}
 
void DatabaseBroker::EnableServiceCommand(const std::string &srv_name, 
        const std::string& command)
{
  assert(m_impl);
  return m_impl->EnableServiceCommand(srv_name, command);
}

void DatabaseBroker::EnableServiceCommand(const xdb::Service *srv, 
        const std::string& command)
{
  assert(m_impl);
  return m_impl->EnableServiceCommand(srv, command);
}

void DatabaseBroker::DisableServiceCommand(const std::string &srv_name, 
        const std::string& command)
{
  assert(m_impl);
  return m_impl->DisableServiceCommand(srv_name, command);
}

void DatabaseBroker::DisableServiceCommand(const xdb::Service *srv, 
        const std::string& command)
{
  assert(m_impl);
  return m_impl->DisableServiceCommand(srv, command);
}

/* Тестовый API сохранения базы */
void DatabaseBroker::MakeSnapshot(const char *msg)
{
  assert(m_impl);
  return m_impl->MakeSnapshot(msg);
}
