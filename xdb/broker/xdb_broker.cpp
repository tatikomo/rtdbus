#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "xdb_impl_error.hpp"
#include "xdb_impl_db_broker.hpp"
#include "xdb_impl_common.hpp"
#include "xdb_broker.hpp"
#include "xdb_broker_service.hpp"
#include "xdb_broker_worker.hpp"
#include "msg_message.hpp"

using namespace xdb;

const char *database_name = "broker_db";

DatabaseBroker::DatabaseBroker()
{
  m_impl = new DatabaseBrokerImpl(database_name);
}

DatabaseBroker::~DatabaseBroker()
{
  delete m_impl;
}

bool DatabaseBroker::Connect()
{
  return m_impl->Connect();
}

bool DatabaseBroker::Disconnect()
{
  return m_impl->Disconnect();
}

int DatabaseBroker::State() const
{
  return static_cast<xdb::DBState_t>(m_impl->State());
}

// Получить свою точку подключения
const char* DatabaseBroker::getEndpoint() const
{
  return m_impl->getEndpoint();
}

// Получить точку подключения для указанного Сервиса
const char* DatabaseBroker::getEndpoint(const std::string& service_name) const
{
  return m_impl->getEndpoint(service_name);
}

bool DatabaseBroker::SetWorkerState(Worker* worker, Worker::State new_state)
{
  assert(m_impl);
  return m_impl->SetWorkerState(worker, new_state);
}

// Найти экземпляр Сообщения по его Обработчику
Letter* DatabaseBroker::GetAssignedLetter(Worker* worker)
{
  return m_impl->GetAssignedLetter(worker);
}

bool DatabaseBroker::SetLetterState(Letter* letter, Letter::State new_state)
{
  return m_impl->SetLetterState(letter, new_state);
}

Service *DatabaseBroker::GetServiceForWorker(const Worker *wrk)
{
  return m_impl->GetServiceForWorker(wrk);
}

/* найти в БД и вернуть объект типа Сервис */
Service *DatabaseBroker::GetServiceByName(const std::string& service_name)
{
  return GetServiceByName(service_name.c_str());
}

/* найти в БД и вернуть объект типа Сервис */
Service *DatabaseBroker::GetServiceByName(const char *service_name)
{
  if (!m_impl) return NULL;
  return m_impl->GetServiceByName(service_name);
}

// Обновить состояние экземпляра в БД
bool DatabaseBroker::Update(Service* srv)
{
  assert(m_impl);
  return m_impl->Update(srv);
}

// Обновить состояние экземпляра в БД
bool DatabaseBroker::Update(Worker* wrk)
{
  assert(m_impl);
  return m_impl->Update(wrk);
}

Service *DatabaseBroker::AddService(const std::string& service_name)
{
  if (!m_impl) return (Service*)NULL;
  return m_impl->AddService(service_name);
}

Service *DatabaseBroker::AddService(const char *service_name)
{
  assert(service_name);

  if (!m_impl) return (Service*)NULL;
  return m_impl->AddService(service_name);
}

bool DatabaseBroker::RemoveService(const char *service_name)
{
  assert(service_name);

  if (!m_impl) return false;
  return m_impl->RemoveService(service_name);
}

bool DatabaseBroker::RemoveWorker(Worker *wrk)
{
  assert (wrk);

  if (!m_impl) return false;
  return m_impl->RemoveWorker(wrk);
}

bool DatabaseBroker::PushWorker(Worker *wrk)
{
  assert (wrk);

  if (!m_impl) return false;
  return m_impl->PushWorker(wrk);
}

/* Добавить нового Обработчика в спул Сервиса */
// TODO: delete me
bool DatabaseBroker::PushWorkerForService(Service *srv, Worker *wrk)
{
  assert (wrk);
  if (!m_impl) return false;
  return m_impl->PushWorkerForService(srv, wrk);
}

/* Добавить нового Обработчика в спул Сервиса */
Worker* DatabaseBroker::PushWorkerForService(const std::string& srv_name, const std::string& wrk_ident)
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
Letter* DatabaseBroker::GetWaitingLetter(Service* srv)
{
  assert(m_impl);

  if (!m_impl) return NULL;
  return m_impl->GetWaitingLetter(srv);
}

/* Очистить сообщение после получения квитанции о завершении от Обработчика */
bool DatabaseBroker::ReleaseLetterFromWorker(Worker* worker)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->ReleaseLetterFromWorker(worker);
}


bool DatabaseBroker::IsServiceCommandEnabled(
        const Service* srv, 
        const std::string& cmd_name)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->IsServiceCommandEnabled(srv, cmd_name);
}

bool DatabaseBroker::AssignLetterToWorker(Worker* worker, Letter* letter)
{
  assert(m_impl);

  if (!m_impl) return false;
  // TODO: проверить, возможно только для команды MDPW_DISCONNECT
  if (!letter) return false;
  return m_impl->AssignLetterToWorker(worker, letter);
}

Service *DatabaseBroker::RequireServiceByName(const char *service_name)
{
  assert(m_impl);

  if (!m_impl) return (Service*)NULL;
  return m_impl->RequireServiceByName(service_name);
}

Service *DatabaseBroker::RequireServiceByName(const std::string& service_name)
{
  assert(m_impl);

  if (!m_impl) return (Service*)NULL;
  return m_impl->RequireServiceByName(service_name);
}

Service *DatabaseBroker::GetServiceById(int64_t _id)
{
  assert(m_impl);
  if (!m_impl) return (Service*)NULL;
  return m_impl->GetServiceById(_id);
}

Worker *DatabaseBroker::PopWorker(const std::string& service_name)
{
  assert(m_impl);

  if (!m_impl) return (Worker*)NULL;
  return m_impl->PopWorker(service_name);
}

Worker *DatabaseBroker::PopWorker(const Service *service)
{
  assert(m_impl);

  if (!m_impl) return (Worker*)NULL;
  return m_impl->PopWorker(service);
}

/* Очистить спул Обработчиков указанного Сервиса */
bool DatabaseBroker::ClearWorkersForService(const Service *service)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->ClearWorkersForService(service);
}

/* поместить сообщение во входящую очередь Службы */
bool DatabaseBroker::PushRequestToService(Service* srv,
        const std::string& reply_to,
        const std::string& header,
        const std::string& data)
{
  assert(m_impl);

  if (!m_impl) return false;
  return m_impl->PushRequestToService(srv, reply_to, header, data);
}

/* поместить сообщение во входящую очередь Службы */
bool DatabaseBroker::PushRequestToService(Service* srv, Letter* letter)
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
Worker *DatabaseBroker::GetWorkerByIdent(const char *ident)
{
  if (!m_impl) return (Worker*) NULL;
  return  m_impl->GetWorkerByIdent(ident);
}

Worker *DatabaseBroker::GetWorkerByIdent(const std::string& ident)
{
  return GetWorkerByIdent(ident.c_str());
}
 
void DatabaseBroker::EnableServiceCommand(const std::string &srv_name, 
        const std::string& command)
{
  assert(m_impl);
  return m_impl->EnableServiceCommand(srv_name, command);
}

void DatabaseBroker::EnableServiceCommand(const Service *srv, 
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

void DatabaseBroker::DisableServiceCommand(const Service *srv, 
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
