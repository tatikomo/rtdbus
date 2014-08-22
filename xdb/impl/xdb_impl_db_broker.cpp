#include <new>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h> // exit
#include <stdarg.h>
#include <string.h>

#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"

#if defined DEBUG
# if (EXTREMEDB_VERSION <= 40)
mco_size_t file_writer(void*, const void*, mco_size_t);
# else
mco_size_sig_t file_writer(void*, const void*, mco_size_t);
# endif
#define SETUP_POLICY
#include "mcoxml.h"
#endif

#ifdef __cplusplus
}
#endif

#include "xdb_impl_common.hpp"

#include "helper.hpp"
#include "dat/broker_db.hpp"
#include "xdb_impl_database.hpp"
#include "xdb_impl_db_broker.hpp"
#include "xdb_broker_service.hpp"

using namespace xdb;

/* 
 * Включение динамически генерируемых определений 
 * структуры данных для внутренней базы Брокера.
 *
 * Регенерация осуществляется командой mcocomp 
 * на основе содержимого файла broker.mco
 */
#include "dat/broker_db.h"
#include "dat/broker_db.hpp"

void broker_impl_errhandler(MCO_RET n)
{
    fprintf(stdout, "\neXtremeDB runtime fatal error: %d\n", n);
    exit(-1);
}

/* implement error handler */
void extended_broker_impl_errhandler(MCO_RET errcode, const char* file, int line)
{
  fprintf(stdout, "\neXtremeDB runtime fatal error: %d on line %d of file %s",
          errcode, line, file);
  exit(-1);
}


DatabaseBrokerImpl::DatabaseBrokerImpl(const char* _name) :
    m_service_list(NULL)
{
  Options opt;
  assert(_name);

  // Опции по умолчанию
  setOption(opt, "OF_CREATE",    1);
  setOption(opt, "OF_LOAD_SNAP", 1);
  setOption(opt, "OF_DATABASE_SIZE",   1024 * 1024 * 10);
  setOption(opt, "OF_MEMORYPAGE_SIZE", 1024); // 0..65535
  setOption(opt, "OF_MAP_ADDRESS", 0x20000000);
#if defined USE_EXTREMEDB_HTTP_SERVER
  setOption(opt, "OF_HTTP_PORT", 8081);
#endif

#ifdef DISK_DATABASE
  /* NB: +5 - для ".dbs" и ".log" с завершающим '\0' */
  m_dbsFileName = new char[strlen(m_database->getName()) + 5];
  m_logFileName = new char[strlen(m_database->getName()) + 5];

  strcpy(m_dbsFileName, m_database->getName());
  strcat(m_dbsFileName, ".dbs");

  strcpy(m_logFileName, m_database->getName());
  strcat(m_logFileName, ".log");

  setOption(opt, "OF_DISK_CACHE_SIZE", 1024 * 1024 * 10);
#else
  setOption(opt, "OF_DISK_CACHE_SIZE", 0);
#endif

  m_database = new DatabaseImpl(_name, opt, broker_db_get_dictionary());
}

DatabaseBrokerImpl::~DatabaseBrokerImpl()
{
  delete m_service_list;
  delete m_database;
}

bool DatabaseBrokerImpl::Init()
{
  Error err = m_database->Open();
  return err.Ok();
}

bool DatabaseBrokerImpl::Connect()
{
  Error err = m_database->Connect();
  return err.Ok();
}

bool DatabaseBrokerImpl::Disconnect()
{
  //MCO_RET rc = MCO_S_OK;
  DBState_t state = m_database->State();

#if 0

  switch (state)
  {
    case DB_STATE_UNINITIALIZED:
      LOG(INFO) << "Disconnect from uninitialized state";
    break;

    case DB_STATE_DISCONNECTED:
      LOG(INFO) << "Try to disconnect already diconnected database";
    break;

    case DB_STATE_CONNECTED:
      mco_async_event_release_all(m_database->getDbHandler()/*, MCO_EVENT_newService*/);
      rc = mco_db_disconnect(m_database->getDbHandler());
      // NB: break пропущен специально
    case DB_STATE_ATTACHED:
      assert(m_database);
      rc = mco_db_close(m_database->getName());
      m_database->TransitionToState(DB_STATE_DISCONNECTED);
    break;

    default:
      LOG(INFO) << "Disconnect from unknown state:" << state;
  }

  return (!rc)? true : false;
#else
  return (m_database->Disconnect()).Ok();
#endif
}

DBState_t DatabaseBrokerImpl::State()
{
  return m_database->State();
}

/*
 * Статический метод, вызываемый из runtime базы данных 
 * при создании нового экземпляра XDBService
 */
MCO_RET DatabaseBrokerImpl::new_Service(mco_trans_h /*t*/,
        XDBService *obj,
        MCO_EVENT_TYPE /*et*/,
        void *p)
{
  DatabaseBrokerImpl *self = static_cast<DatabaseBrokerImpl*> (p);
  char name[Service::NameMaxLen + 1];
  MCO_RET rc;
  autoid_t aid;
  Service *service;
//  broker_db::XDBService service_instance;
  bool status = false;

  assert(self);
  assert(obj);

  do
  {
    name[0] = '\0';

    rc = XDBService_name_get(obj, name, Service::NameMaxLen);
    name[Service::NameMaxLen] = '\0';
    if (rc) { LOG(ERROR)<<"Unable to get service's name, rc="<<rc; break; }

    rc = XDBService_autoid_get(obj, &aid);
    if (rc) { LOG(ERROR)<<"Unable to get id of service '"<<name<<"', rc="<<rc; break; }

    service = new Service(aid, name);
    if (!self->m_service_list)
      self->m_service_list = new ServiceList(self->m_database->getDbHandler());

    if (false == (status = self->m_service_list->AddService(service)))
    {
      LOG(WARNING)<<"Unable to add new service into list";
    }
  } while (false);

//  LOG(INFO) << "NEW XDBService "<<obj<<" name '"<<name<<"' self=" << self;

  return MCO_S_OK;
}

/*
 * Статический метод, вызываемый из runtime базы данных 
 * при удалении экземпляра XDBService
 */
MCO_RET DatabaseBrokerImpl::del_Service(mco_trans_h /*t*/,
        XDBService *obj,
        MCO_EVENT_TYPE /*et*/,
        void *p)
{
  DatabaseBrokerImpl *self = static_cast<DatabaseBrokerImpl*> (p);
  char name[Service::NameMaxLen + 1];
  MCO_RET rc;

  assert(self);
  assert(obj);

  do
  {
    name[0] = '\0';

    rc = XDBService_name_get(obj, name, Service::NameMaxLen);
    name[Service::NameMaxLen] = '\0';
    if (rc) { LOG(ERROR)<<"Unable to get service's name, rc="<<rc; break; }

  } while (false);

//  LOG(INFO) << "DEL XDBService "<<obj<<" name '"<<name<<"' self=" << self;

  return MCO_S_OK;
}

MCO_RET DatabaseBrokerImpl::RegisterEvents()
{
  MCO_RET rc;
  mco_trans_h t;

  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) LOG(ERROR) << "Starting transaction, rc=" << rc;

    rc = mco_register_newService_handler(t, 
            &DatabaseBrokerImpl::new_Service, 
            static_cast<void*>(this)
//#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
//            , MCO_AFTER_UPDATE
//#endif
            );

    if (rc) LOG(ERROR) << "Registering event on XDBService creation, rc=" << rc;

    rc = mco_register_delService_handler(t, 
            &DatabaseBrokerImpl::del_Service, 
            static_cast<void*>(this));
    if (rc) LOG(ERROR) << "Registering event on XDBService deletion, rc=" << rc;

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }
  } while(false);

  if (rc)
   mco_trans_rollback(t);

  return rc;
}

Service *DatabaseBrokerImpl::AddService(const std::string& name)
{
  return AddService(name.c_str());
}

Service *DatabaseBrokerImpl::AddService(const char *name)
{
  broker_db::XDBService service_instance = {};
  Service       *srv = NULL;
  MCO_RET        rc;
  mco_trans_h    t;
  autoid_t       aid;

  assert(name);
  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    rc = service_instance.create(t);
    if (rc) { LOG(ERROR) << "Creating instance, rc=" << rc; break; }

    rc = service_instance.name_put(name, static_cast<uint2>(strlen(name)));
    if (rc) { LOG(ERROR) << "Setting '" << name << "' name"; break; }

    rc = service_instance.state_put(REGISTERED);
    if (rc) { LOG(ERROR) << "Setting '" << name << "' state"; break; }

    rc = service_instance.autoid_get(aid);
    if (rc) { LOG(ERROR) << "Getting service "<<name<<" id, rc=" << rc; break; }

    srv = new Service(aid, name);
    srv->SetSTATE(Service::State::REGISTERED);

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }
  } while(false);

  if (rc)
    mco_trans_rollback(t);

  return srv;
}

// TODO: возможно стоит удалить этот метод-обертку, оставив GetServiceByName
Service *DatabaseBrokerImpl::RequireServiceByName(const char *service_name)
{
  return GetServiceByName(service_name);
}

// TODO: возможно стоит удалить этот метод-обертку, оставив GetServiceByName
Service *DatabaseBrokerImpl::RequireServiceByName(const std::string& service_name)
{
  return GetServiceByName(service_name);
}

/*
 * Удалить запись о заданном сервисе
 * - найти указанную запись в базе
 *   - если найдена, удалить её
 *   - если не найдена, вернуть ошибку
 */
bool DatabaseBrokerImpl::RemoveService(const char *name)
{
  broker_db::XDBService service_instance;
  MCO_RET        rc;
  mco_trans_h    t;

  assert(name);
  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    /* найти запись в таблице сервисов с заданным именем */
    rc = broker_db::XDBService::PK_name::find(t, name, strlen(name), service_instance);
    if (MCO_S_NOTFOUND == rc) 
    { 
        LOG(ERROR) << "Removed service '" << name << "' doesn't exists, rc=" << rc; break;
    }
    if (rc) { LOG(ERROR) << "Unable to find service '" << name << "', rc=" << rc; break; }
    
    rc = service_instance.remove();
    if (rc) { LOG(ERROR) << "Unable to remove service '" << name << "', rc=" << rc; break; }

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }
  } while(false);

  if (rc)
    mco_trans_rollback(t);

  return (MCO_S_OK == rc);
}

bool DatabaseBrokerImpl::IsServiceCommandEnabled(const Service* srv, const std::string& cmd_name)
{
  bool status = false;
  assert(srv);
  // TODO реализация
  assert(1 == 0);
  assert(!cmd_name.empty());
  return status;
}


// Деактивировать Обработчик wrk по идентификатору у его Сервиса
// NB: При этом сам экземпляр из базы не удаляется, а помечается неактивным (DISARMED)
bool DatabaseBrokerImpl::RemoveWorker(Worker *wrk)
{
  MCO_RET       rc = MCO_S_OK;
  mco_trans_h   t;
  broker_db::XDBService service_instance;
  broker_db::XDBWorker  worker_instance = {};

  assert(wrk);
  const char* identity = wrk->GetIDENTITY();

  do
  {
      rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
      if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

      rc = broker_db::XDBWorker::SK_by_ident::find(t,
            identity,
            strlen(identity),
            worker_instance);

      if (rc)
      {
        // Попытка удаления несуществующего Обработчика
        LOG(ERROR) << "Try to disable non-existent worker '"
                   << identity << ", rc=" << rc;
        break;
      }

      // Не удалять, пометить как "неактивен"
      // NB: Неактивные обработчики должны быть удалены в процессе сборки мусора
      rc = worker_instance.state_put(DISARMED);
      if (rc) 
      { 
        LOG(ERROR) << "Disarming worker '" << identity << "', rc=" << rc;
        break;
      }

      wrk->SetSTATE(Worker::State(DISARMED));

      rc = mco_trans_commit(t);
      if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }
  } while(false);

  if (rc)
    mco_trans_rollback(t);

  return (MCO_S_OK == rc);
}

// Активировать Обработчик wrk своего Сервиса
// TODO: рассмотреть необходимость расширенной обработки состояния уже 
// существующего экземпляра. Повторная его регистрация при наличии 
// незакрытой ссылки на Сообщение может говорить о сбое в его обработке.
bool DatabaseBrokerImpl::PushWorker(Worker *wrk)
{
  MCO_RET       rc = MCO_S_OK;
  mco_trans_h   t;
  broker_db::XDBService service_instance;
  broker_db::XDBWorker  worker_instance;
  timer_mark_t  now_time, next_heartbeat_time;
  broker_db::timer_mark xdb_next_heartbeat_time = {};
  autoid_t      srv_aid;
  autoid_t      wrk_aid;

  assert(wrk);
  const char* wrk_identity = wrk->GetIDENTITY();

  do
  {
      rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
      if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

      rc = broker_db::XDBService::autoid::find(t,
                            wrk->GetSERVICE_ID(),
                            service_instance);
      if (rc)
      {
        LOG(ERROR) << "Unable to locate service by id "<<wrk->GetSERVICE_ID()
                   << " for worker "<<wrk->GetIDENTITY();
        break;
      }

      rc = service_instance.autoid_get(srv_aid);
      if (rc) { LOG(ERROR) << "Unable to get autoid for service id "<<wrk->GetSERVICE_ID(); break; }

      // Защита от "дурака" - идентификаторы должны совпадать
      assert(wrk->GetSERVICE_ID() == srv_aid);
      if (wrk->GetSERVICE_ID() != srv_aid)
      {
        LOG(ERROR) << "Database inconsistence: service id ("
                   << wrk->GetSERVICE_ID()<<") for worker "<<wrk->GetIDENTITY()
                   << " did not equal to database value ("<<srv_aid<<")";
      }

      // Проверить наличие Обработчика с таким ident в базе,
      // в случае успеха получив его worker_instance
      rc = broker_db::XDBWorker::SK_by_ident::find(t,
                wrk_identity,
                strlen(wrk_identity),
                worker_instance);

      if (MCO_S_OK == rc) // Экземпляр найден - обновить данные
      {
        if (0 == wrk->GetID()) // Нулевой собственный идентификатор - значит ранее не присваивался
        {
          // Обновить идентификатор
          rc = worker_instance.autoid_get(wrk_aid);
          if (rc) { LOG(ERROR) << "Unable to get worker "<<wrk->GetIDENTITY()<<" id"; break; }
          wrk->SetID(wrk_aid);
        }
        // Сменить ему статус на ARMED и обновить свойства
        rc = worker_instance.state_put(ARMED);
        if (rc) { LOG(ERROR) << "Unable to change '"<< wrk->GetIDENTITY()
                             <<"' worker state, rc="<<rc; break; }

        rc = worker_instance.service_ref_put(srv_aid);
        if (rc) { LOG(ERROR) << "Unable to set '"<< wrk->GetIDENTITY()
                             <<"' with service id "<<srv_aid<<", rc="<<rc; break; }

        /* Установить новое значение expiration */
        // TODO: wrk->CalculateEXPIRATION_TIME();
        if (GetTimerValue(now_time))
        {
          next_heartbeat_time.tv_nsec = now_time.tv_nsec;
          next_heartbeat_time.tv_sec = now_time.tv_sec + (Worker::HeartbeatPeriodValue/1000);
          LOG(INFO) << "Set new expiration time for reactivated worker "<<wrk->GetIDENTITY();
        }
        else { LOG(ERROR) << "Unable to calculate expiration time, rc="<<rc; break; }

        rc = worker_instance.expiration_write(xdb_next_heartbeat_time);
        if (rc) { LOG(ERROR) << "Unable to set worker's expiration time, rc="<<rc; break; }
        rc = xdb_next_heartbeat_time.sec_put(next_heartbeat_time.tv_sec);
        if (rc) { LOG(ERROR) << "Unable to set the expiration seconds, rc="<<rc; break; }
        rc = xdb_next_heartbeat_time.nsec_put(next_heartbeat_time.tv_nsec);
        if (rc) { LOG(ERROR) << "Unable to set expiration time nanoseconds, rc="<<rc; break; }
      }
      else if (MCO_S_NOTFOUND == rc) // Экземпляр не найден, так как ранее не регистрировался
      {
        // Создать новый экземпляр
        rc = worker_instance.create(t);
        if (rc) { LOG(ERROR) << "Creating worker's instance " << wrk->GetIDENTITY() << ", rc="<<rc; break; }

        rc = worker_instance.identity_put(wrk_identity, strlen(wrk_identity));
        if (rc) { LOG(ERROR) << "Put worker's identity " << wrk->GetIDENTITY() << ", rc="<<rc; break; }

        // Первоначальное состояние Обработчика - "ГОТОВ"
        rc = worker_instance.state_put(ARMED);
        if (rc) { LOG(ERROR) << "Unable to change "<< wrk->GetIDENTITY() <<" worker state, rc="<<rc; break; }

        rc = worker_instance.service_ref_put(srv_aid);
        if (rc) { LOG(ERROR) << "Unable to set '"<< wrk->GetIDENTITY()
                             <<"' with service id "<<srv_aid<<", rc="<<rc; break; }

        // Обновить идентификатор
        rc = worker_instance.autoid_get(wrk_aid);
        if (rc) { LOG(ERROR) << "Unable to get worker "<<wrk->GetIDENTITY()<<" id"; break; }
        wrk->SetID(wrk_aid);

        /* Установить новое значение expiration */
        if (GetTimerValue(now_time))
        {
          next_heartbeat_time.tv_nsec = now_time.tv_nsec;
          next_heartbeat_time.tv_sec = now_time.tv_sec + Worker::HeartbeatPeriodValue/1000;
          LOG(INFO) << "Set new expiration time for new worker "<<wrk->GetIDENTITY();
        }
        else { LOG(ERROR) << "Unable to calculate expiration time, rc="<<rc; break; }

        rc = worker_instance.expiration_write(xdb_next_heartbeat_time);
        if (rc) { LOG(ERROR) << "Unable to set worker's expiration time, rc="<<rc; break; }
        rc = xdb_next_heartbeat_time.sec_put(next_heartbeat_time.tv_sec);
        if (rc) { LOG(ERROR) << "Unable to set the expiration seconds, rc="<<rc; break; }
        rc = xdb_next_heartbeat_time.nsec_put(next_heartbeat_time.tv_nsec);
        if (rc) { LOG(ERROR) << "Unable to set expiration time nanoseconds, rc="<<rc; break; }
      }
      else 
      { 
        LOG(ERROR) << "Unable to load worker "<<wrk->GetIDENTITY()<<" data"; 
        break; 
      }

      rc = mco_trans_commit(t);
      if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }

  } while (false);
  
  if (rc)
    mco_trans_rollback(t);

  return (MCO_S_OK == rc);
}
/*
 * Добавить нового Обработчика в спул Сервиса.
 * TODO: рассмотреть необходимость данной функции. Сохранить вариант с std::string& ?
 */
bool DatabaseBrokerImpl::PushWorkerForService(const Service *srv, Worker *wrk)
{
  assert(srv);
  assert(wrk);

  return false;
}

/*
 * Добавить нового Обработчика в спул Сервиса.
 * Сервис srv должен быть уже зарегистрированным в БД;
 * Экземпляр Обработчика wrk в БД еще не содержится;
 */
Worker* DatabaseBrokerImpl::PushWorkerForService(const std::string& service_name, const std::string& wrk_identity)
{
  bool status = false;
  Service *srv = NULL;
  Worker  *wrk = NULL;

  if (NULL == (srv = GetServiceByName(service_name)))
  {
    // Сервис не зарегистрирован в БД => попытка его регистрации
    if (NULL == (srv = AddService(service_name)))
    {
      LOG(WARNING) << "Unable to register new service " << service_name.c_str();
    }
    else status = true;
  }
  else status = true;

  // Сервис действительно существует => прописать ему нового Обработчика
  if (true == status)
  {
    wrk = new Worker(srv->GetID(), const_cast<char*>(wrk_identity.c_str()), 0);

    if (false == (status = PushWorker(wrk)))
    {
      LOG(WARNING) << "Unable to register worker '" << wrk_identity.c_str()
          << "' for service '" << service_name.c_str() << "'";
    }
  }
  delete srv;
  return wrk;
}

Service *DatabaseBrokerImpl::GetServiceByName(const std::string& name)
{
  return GetServiceByName(name.c_str());
}

/*
 * Найти в базе данных запись о Сервисе по его имени
 * @return Новый объект, представляющий Сервис
 * Вызываюшая сторона должна сама удалить объект возврата
 */
Service *DatabaseBrokerImpl::GetServiceByName(const char* name)
{
  MCO_RET       rc;
  mco_trans_h   t;
  broker_db::XDBService service_instance;
  Service *service = NULL;
  autoid_t      service_id;

  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    /* найти запись в таблице сервисов с заданным именем */
    rc = broker_db::XDBService::PK_name::find(t,
            name,
            strlen(name),
            service_instance);

    /* Запись не найдена - нет ошибки */
    if (MCO_S_NOTFOUND == rc) break;

    /* Запись не найдена - есть ошибка - сообщить */
    if (rc) { LOG(ERROR)<<"Unable to locating service '"<<name<<"', rc="<<rc; break; }

    /* Запись найдена - сконструировать объект на основе данных из БД */
    rc = service_instance.autoid_get(service_id);
    if (rc) { LOG(ERROR)<<"Unable to get service's id for '"<<name<<"', rc="<<rc; break; }

    service = LoadService(service_id, service_instance);
  } while(false);

  mco_trans_rollback(t);

  return service;
}

Service *DatabaseBrokerImpl::LoadService(
        autoid_t &aid,
        broker_db::XDBService& instance)
{
  Service      *service = NULL;
  MCO_RET       rc = MCO_S_OK;
  char          name[Service::NameMaxLen + 1];
  ServiceState  state;

  do
  {
    rc = instance.name_get(name, Service::NameMaxLen);
    name[Service::NameMaxLen] = '\0';
    if (rc) { LOG(ERROR)<<"Unable to get service's name, rc="<<rc; break; }
    rc = instance.state_get(state);
    if (rc) { LOG(ERROR)<<"Unable to get service id for "<<name; break; }

    service = new Service(aid, name);
    service->SetSTATE((Service::State)state);
    /* Состояние объекта полностью соответствует хранимому в БД */
    service->SetVALID();
  } while (false);

  return service;
}

#if 0
/*
 * Загрузить данные Обработчика в ранее созданный вручную экземпляр
 */
MCO_RET DatabaseBrokerImpl::LoadWorkerByIdent(
        /* IN */ mco_trans_h t,
        /* INOUT */ Worker *wrk)
{
  MCO_RET rc = MCO_S_OK;
  mco_cursor_t csr;
  broker_db::XDBWorker worker_instance;
  uint2 idx;
  char name[SERVICE_NAME_MAXLEN+1];

  assert(wrk);

  const char *identity = wrk->GetIDENTITY();

  do
  {
    rc = broker_db::XDBWorker::SK_by_ident::find(t,
                identity,
                strlen(identity),
                worker_instance);
    if (rc) 
    { 
//      LOG(ERROR)<<"Unable to locate '"<<identity<<"' in service id "<<service_aid<<", rc="<<rc;
      wrk->SetINDEX(-1);
      break;
    }
    wrk->SetINDEX(idx);
    if (!wrk->GetSERVICE_NAME()) // Имя Сервиса не было заполнено
    {
      rc = service_instance.name_get(name, SERVICE_NAME_MAXLEN);
      if (!rc)
        wrk->SetSERVICE_NAME(name);
    }

  } while(false);
#if EXTREMEDB_VERSION > 40
  mco_cursor_close(t, &csr);
#endif

  return rc;
}
#endif

// Загрузить данные Обработчика, основываясь на его идентификаторе
MCO_RET DatabaseBrokerImpl::LoadWorker(mco_trans_h /*t*/,
        /* IN  */ broker_db::XDBWorker& wrk_instance,
        /* OUT */ Worker*& worker)
{
  MCO_RET       rc = MCO_S_OK;
  char          ident[IDENTITY_MAXLEN + 1];
  broker_db::timer_mark  xdb_expire_time;
  timer_mark_t  expire_time = {0, 0};
  uint4         timer_value;
  WorkerState   state;
  autoid_t      wrk_aid;
  autoid_t      srv_aid;
  autoid_t      letter_aid;

  do
  {
    rc = wrk_instance.autoid_get(wrk_aid);
    if (rc) { LOG(ERROR) << "Unable to get worker id"; break; }

    rc = wrk_instance.identity_get(ident, static_cast<uint2>(IDENTITY_MAXLEN));
    if (rc) { LOG(ERROR) << "Unable to get identity for worker id "<<wrk_aid; break; }

    rc = wrk_instance.service_ref_get(srv_aid);
    if (rc) { LOG(ERROR) << "Unable to get service id for worker "<<ident; break; }

    rc = wrk_instance.letter_ref_get(letter_aid);
    if (rc) { LOG(ERROR) << "Unable to get letter id for worker "<<ident; break; }

    ident[IDENTITY_MAXLEN] = '\0';
    if (rc)
    { 
        LOG(ERROR)<<"Unable to get worker's identity for service id "
                  <<srv_aid<<", rc="<<rc;
        break;
    }

    rc = wrk_instance.expiration_read(xdb_expire_time);
    if (rc) { LOG(ERROR)<<"Unable to get worker '"<<ident<<"' expiration mark, rc="<<rc; break; }

    rc = wrk_instance.state_get(state);
    if (rc) { LOG(ERROR)<<"Unable to get worker '"<<ident<<"' state, rc="<<rc; break; }

    rc = xdb_expire_time.sec_get(timer_value); expire_time.tv_sec = timer_value;
    rc = xdb_expire_time.nsec_get(timer_value); expire_time.tv_nsec = timer_value;

    worker = new Worker(srv_aid, ident, wrk_aid, letter_aid);
    worker->SetSTATE(static_cast<Worker::State>(state));
    worker->SetEXPIRATION(expire_time);
    /* Состояние объекта полностью соответствует хранимому в БД */
    worker->SetVALID();
/*    LOG(INFO) << "Load Worker('" << worker->GetIDENTITY()
              << "' id=" << worker->GetID() 
              << " srv_id=" << worker->GetSERVICE_ID() 
              << " state=" << worker->GetSTATE();*/
  } while(false);

  return rc;
}

/* 
 * Вернуть ближайший свободный Обработчик в состоянии ARMED (по умолчанию).
 * Побочные эффекты: создается экземпляр Worker, его удаляет вызвавшая сторона
 *
 * TODO: возвращать следует наиболее "старый" экземпляр, временная отметка 
 * которого раньше всех остальных экземпляров с этим же состоянием.
 */
Worker *DatabaseBrokerImpl::PopWorker(const Service *srv, WorkerState state)
{
  Worker       *worker = NULL;
  autoid_t      service_aid;

  assert(srv);
  service_aid = const_cast<Service*>(srv)->GetID();
  worker = GetWorkerByState(service_aid, state);

  return worker;
}

Worker *DatabaseBrokerImpl::PopWorker(const std::string& service_name, WorkerState state)
{
  Service      *service = NULL;
  Worker       *worker = NULL;

  if (NULL != (service = GetServiceByName(service_name)))
  {
    autoid_t  service_aid = service->GetID();
    delete service;
    worker = GetWorkerByState(service_aid, state);
  }

  return worker;
}

Service *DatabaseBrokerImpl::GetServiceForWorker(const Worker *wrk)
{
//  Service *service = NULL;
  assert (wrk);
  return GetServiceById(const_cast<Worker*>(wrk)->GetSERVICE_ID());
}


Service *DatabaseBrokerImpl::GetServiceById(int64_t _id)
{
  autoid_t      aid = _id;
  mco_trans_h   t;
  MCO_RET       rc;
  Service      *service = NULL;
  broker_db::XDBService service_instance;

  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = broker_db::XDBService::autoid::find(t, aid, service_instance);
    if (rc) { LOG(ERROR)<<"Locating service by id="<<_id<<", rc="<<rc; break; }

    service = LoadService(aid, service_instance);
  } while(false);

  mco_trans_rollback(t);

  return service;
}

/*
 * Вернуть признак существования Сервиса с указанным именем в БД
 */
bool DatabaseBrokerImpl::IsServiceExist(const char *name)
{
  MCO_RET       rc;
  mco_trans_h   t;
  broker_db::XDBService instance;

  assert(name);
  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = broker_db::XDBService::PK_name::find(t, name, strlen(name), instance);
    // Если объект не найден - это не ошибка
    if (rc == MCO_S_NOTFOUND) break;

    if (rc) { LOG(WARNING)<< "Locating service instance by name '"<<name<<"', rc="<<rc; }
  } while(false);

  mco_trans_rollback(t);

  return (MCO_S_NOTFOUND == rc)? false:true;
}

/*
 * Поиск Обработчика, находящегося в состоянии state.
 *
 * TODO: рассмотреть возможность поиска экземпляра, имеющего 
 * самую раннюю отметку времени из всех остальных конкурентов.
 * Это необходимо для более равномерного распределения нагрузки.
 */
Worker* DatabaseBrokerImpl::GetWorkerByState(autoid_t& service_id,
                       WorkerState searched_worker_state)
{
  MCO_RET      rc = MCO_S_OK;
  Worker      *worker = NULL;
  mco_trans_h  t;
  WorkerState  worker_state;
  mco_cursor_t csr;
  bool         awaiting_worker_found = false;
  broker_db::XDBWorker    worker_instance;
  int          cmp_result;

  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = broker_db::XDBWorker::SK_by_serv_id::cursor(t, &csr);
    if (MCO_S_CURSOR_EMPTY == rc) // если индекс еще не содержит ни одной записи
      break;
    if (rc) { LOG(ERROR)<< "Unable to get cursor, rc="<<rc; break; }

    for (rc = broker_db::XDBWorker::SK_by_serv_id::search(t, &csr, MCO_EQ, service_id);
         (rc == MCO_S_OK) || (false == awaiting_worker_found);
         rc = mco_cursor_next(t, &csr)) 
    {
       rc = broker_db::XDBWorker::SK_by_serv_id::compare(t,
                &csr,
                service_id,
                cmp_result);
       if (rc == MCO_S_OK && cmp_result == 0)
       {
         rc = worker_instance.from_cursor(t, &csr);
         // сообщим об ошибке и попробуем продолжить
         if (rc) { LOG(ERROR)<<"Unable to load worker from cursor, rc="<<rc; continue; }
         
         rc = worker_instance.state_get(worker_state);
         if (rc) { LOG(ERROR)<< "Unable to get worker state from cursor, rc="<<rc; continue; }

         if (searched_worker_state == worker_state)
         {
           awaiting_worker_found = true;
           break; // нашли что искали, выйдем из цикла
         } 
       }
       if (rc)
         break;
    }
    rc = MCO_S_CURSOR_END == rc ? MCO_S_OK : rc;

    if (MCO_S_OK == rc && awaiting_worker_found)
    {
      // У нас есть id Сервиса и Обработчика -> получить все остальное
      rc = LoadWorker(t, worker_instance, worker);
      if (rc) { LOG(ERROR)<< "Unable to load worker instance, rc="<<rc; break; }
      if (worker->GetSERVICE_ID() != service_id)
        LOG(ERROR) << "Founded Worker '"<<worker->GetIDENTITY()
                   <<"' is registered for another Service, "
                   << worker->GetSERVICE_ID()<<" != "<<service_id;
    }
  } while(false);

#if EXTREMEDB_VERSION > 40
  mco_cursor_close(t, &csr);
#endif
  mco_trans_rollback(t);

  return worker;
}

/* Очистить спул Обработчиков указанной Службы
 *
 * Удалить все незавершенные Сообщения, привязанные к Обработчикам Сервиса.
 * Удалить всех Обработчиков Сервиса.
 */
bool DatabaseBrokerImpl::ClearWorkersForService(const Service *service)
{
    Worker *wrk;

    assert(service);

    while ((wrk = PopWorker(service, OCCUPIED)))
    {
//        if (m_verbose) 
        {
          LOG(INFO) << "Purge worker " << wrk->GetIDENTITY()
                    << " for service "<< const_cast<Service*>(service)->GetNAME();
        }

        ReleaseLetterFromWorker(wrk);

        RemoveWorker(wrk);

        delete wrk;
    }

    return true;
}

/* Очистить спул Обработчиков и всех Служб */
bool DatabaseBrokerImpl::ClearServices()
{
  Service  *srv;
  bool      status = false;

  /*
   * Пройтись по списку Сервисов
   *      Для каждого вызвать ClearWorkersForService()
   *      Удалить Сервис
   */
  srv = m_service_list->first();
  while (srv)
  {
    if (true == (status = ClearWorkersForService(srv)))
    {
      status = RemoveService(srv->GetNAME());
    }
    else
    {
      LOG(ERROR) << "Unable to clear resources for service '"<<srv->GetNAME()<<"'";
    }

    srv = m_service_list->next();
  }

  return status;
}

/* Вернуть NULL или Обработчика по его идентификационной строке */
/*
 * Найти в базе данных запись об Обработчике по его имени
 * @return Новый объект, представляющий Обработчика
 * Вызываюшая сторона должна сама удалить объект возврата
 */
Worker *DatabaseBrokerImpl::GetWorkerByIdent(const char *ident)
{
  MCO_RET      rc;
  mco_trans_h  t;
  broker_db::XDBService service_instance;
  broker_db::XDBWorker  worker_instance;
  Worker      *worker = NULL;

  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    /* найти запись в таблице сервисов с заданным именем */
    rc = broker_db::XDBWorker::SK_by_ident::find(t,
                ident,
                strlen(ident),
                worker_instance);

    /* Запись не найдена - нет ошибки */
    if (MCO_S_NOTFOUND == rc) break;

    /* Запись не найдена - есть ошибка - сообщить */
    if (rc) { LOG(ERROR)<<"Worker '"<<ident<<"' location, rc="<<rc; break; }

    /* Запись найдена - сконструировать объект на основе данных из БД */
    rc = LoadWorker(t, worker_instance, worker);
    if (rc) { LOG(ERROR)<<"Unable to load worker instance"; break; }

  } while(false);

  mco_trans_rollback(t);

  return worker;
}

#if 0
/* Получить первое ожидающее обработки Сообщение */
bool DatabaseBrokerImpl::GetWaitingLetter(
        /* IN  */ Service* srv,
        /* IN  */ Worker* wrk, /* GEV: зачем здесь Worker? Он должен быть закреплен за сообщением после передачи */
        /* OUT */ std::string& header,
        /* OUT */ std::string& body)
{
  mco_trans_h t;
  MCO_RET rc = MCO_S_OK;
  mco_cursor_t csr;
  broker_db::XDBLetter letter_instance;
  autoid_t  aid;
  uint2     sz;
  char*     header_buffer = NULL;
  char*     body_buffer = NULL;

  assert(srv);
  assert(wrk);

  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc="<<rc; break; }

    rc = broker_db::XDBLetter::SK_by_state_for_serv::cursor(t, &csr);
    if (MCO_S_CURSOR_EMPTY == rc) 
        break; /* В индексе нет ни одной записи */
    if (rc) 
    {
      LOG(ERROR)<<"Unable to initialize cursor for '"<<srv->GetNAME()<<"', rc="<<rc;
      break;
    }

    rc = broker_db::XDBLetter::SK_by_state_for_serv::search(t,
                &csr,
                MCO_EQ,
                srv->GetID(),
                UNASSIGNED); 
    // Вернуться, если курсор пуст
    if (MCO_S_CURSOR_EMPTY == rc)
      break;
    if (rc) { LOG(ERROR) << "Unable to get Letters list cursor, rc="<<rc; break; }

    rc = mco_cursor_first(t, &csr);
    if (rc) 
    { 
        LOG(ERROR) << "Unable to point at first item in Letters list cursor, rc="<<rc;
        break;
    }

    // Достаточно получить первый элемент,
    // функция будет вызываться до опустошения содержимого курсора 
    rc = letter_instance.from_cursor(t, &csr);
    if (rc) { LOG(ERROR) << "Unable to get item from Letters cursor, rc="<<rc; break; }

    rc = letter_instance.autoid_get(aid);
    if (rc) { LOG(ERROR) << "Getting letter' id, rc=" << rc; break; }

    rc = letter_instance.header_size(sz);
    if (rc) { LOG(ERROR) << "Getting header size for letter id"<<aid<<", rc=" << rc; break; }
    header_buffer = new char[sz + 1];

    rc = letter_instance.header_get(header_buffer, sizeof(*header_buffer), sz);
    if (rc) { LOG(ERROR) << "Getting header for letter id"<<aid<<", rc=" << rc; break; }
//    header_buffer[sz] = '\0';

    rc = letter_instance.body_size(sz);
    if (rc) { LOG(ERROR) << "Getting message body size for letter id"<<aid<<", rc=" << rc; break; }
    body_buffer = new char[sz + 1];

    rc = letter_instance.body_get(body_buffer, sizeof(*body_buffer), sz);
    if (rc) { LOG(ERROR) << "Getting message for letter id"<<aid<<", rc=" << rc; break; }
//    body_buffer[sz] = '\0';

    header.assign(header_buffer);
    body.assign(body_buffer);

    delete []header_buffer;
    delete []body_buffer;

  } while(false);

#if EXTREMEDB_VERSION > 40
  mco_cursor_close(t, &csr);
#endif
  mco_trans_rollback(t);

  return (MCO_S_OK == rc);
}
#endif

MCO_RET DatabaseBrokerImpl::LoadLetter(mco_trans_h /*t*/,
    /* IN  */broker_db::XDBLetter& letter_instance,
    /* OUT */xdb::Letter*& letter)
{
  MCO_RET rc = MCO_S_OK;
  autoid_t     aid;
  autoid_t     service_aid, worker_aid;
  uint2        header_sz, data_sz, sz;
  timer_mark_t expire_time = {0, 0};
  broker_db::timer_mark xdb_expire_time;
  LetterState  state;
  broker_db::XDBWorker worker_instance;
  char        *header_buffer = NULL;
  char        *body_buffer = NULL;
  char         reply_buffer[IDENTITY_MAXLEN + 1];
  uint4        timer_value;

  do
  {
    rc = letter_instance.autoid_get(aid);
    if (rc) { LOG(ERROR) << "Getting letter' id, rc=" << rc; break; }

    rc = letter_instance.service_ref_get(service_aid);
    if (rc) { LOG(ERROR) << "Getting letter's ("<<aid<<") service id, rc=" << rc; break; }
    rc = letter_instance.worker_ref_get(worker_aid);
    if (rc) { LOG(ERROR) << "Getting letter's ("<<aid<<") worker id, rc=" << rc; break; }

    // Выгрузка Заголовка Сообщения
    rc = letter_instance.header_size(header_sz);
    if (rc) { LOG(ERROR) << "Getting header size for letter id"<<aid<<", rc=" << rc; break; }
    header_buffer = new char[header_sz + 1];
    rc = letter_instance.header_get(header_buffer, header_sz, sz);
    if (rc)
    {
        LOG(ERROR) << "Getting header for letter id "<<aid<<", size="<<sz<<", rc=" << rc;
        break;
    }
    header_buffer[header_sz] = '\0';
    std::string header(header_buffer, header_sz);

    // Выгрузка тела Сообщения
    rc = letter_instance.body_size(data_sz);
    if (rc)
    {
        LOG(ERROR) << "Getting message body size for letter id"<<aid<<", rc=" << rc;
        break;
    }
    body_buffer = new char[data_sz + 1];

    rc = letter_instance.body_get(body_buffer, data_sz, sz);
    if (rc)
    {
        LOG(ERROR) << "Getting data for letter id "<<aid<<", size="<<sz<<", rc=" << rc;
        break;
    }
    body_buffer[data_sz] = '\0';
    std::string body(body_buffer, data_sz);

#if 0
    // Привязка к Обработчику уже была выполнена
    if (worker_aid)
    {
      /* По идентификатору Обработчика прочитать его IDENTITY */
      rc = broker_db::XDBWorker::autoid::find(t, worker_aid, worker_instance);
      if (rc)
      {
        LOG(ERROR) << "Locating assigned worker with id="<<worker_aid
                   <<" for letter id "<<aid<<", rc=" << rc; 
        break;
      }
      worker_instance.identity_get(reply_buffer, (uint2)IDENTITY_MAXLEN);
      if (rc) { LOG(ERROR) << "Unable to get identity for worker id="<<worker_aid<<", rc="<<rc; break; }
    }
#else
    rc = letter_instance.origin_get(reply_buffer, static_cast<uint2>(IDENTITY_MAXLEN));
    if (rc) { LOG(ERROR) << "Getting reply address for letter id "<<aid<<", rc=" << rc; break; }
#endif

    rc = letter_instance.expiration_read(xdb_expire_time);
    if (rc) { LOG(ERROR)<<"Unable to get expiration mark for letter id="<<aid<<", rc="<<rc; break; }
    rc = xdb_expire_time.sec_get(timer_value);  expire_time.tv_sec = timer_value;
    rc = xdb_expire_time.nsec_get(timer_value); expire_time.tv_nsec = timer_value;

    rc = letter_instance.state_get(state);
    if (rc) { LOG(ERROR)<<"Unable to get state for letter id="<<aid; break; }

    assert(letter == NULL);
    try
    {
      letter = new xdb::Letter(reply_buffer, header, body);

      letter->SetID(aid);
      letter->SetSERVICE_ID(service_aid);
      letter->SetWORKER_ID(worker_aid);
      letter->SetEXPIRATION(expire_time);
      letter->SetSTATE(static_cast<Letter::State>(state));
      // Все поля заполнены
      letter->SetVALID();
    }
    catch (std::bad_alloc &ba)
    {
      LOG(ERROR) << "Unable to allocaling letter instance (head size="
                 << header_sz << " data size=" << data_sz << "), "
                 << ba.what();
    }

  } while(false);

  delete[] header_buffer;
  delete[] body_buffer;
  return rc;
}

Letter* DatabaseBrokerImpl::GetWaitingLetter(/* IN */ Service* srv)
{
  mco_trans_h  t;
  MCO_RET rc = MCO_S_OK;
  mco_cursor_t csr;
  broker_db::XDBLetter letter_instance = {};
  Letter      *letter = NULL;
  char        *header_buffer = NULL;
  char        *body_buffer = NULL;
  int          cmp_result = 0;

  assert(srv);

  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc="<<rc; break; }

    rc = broker_db::XDBLetter::SK_by_state_for_serv::cursor(t, &csr);
    if (MCO_S_CURSOR_EMPTY == rc) 
        break; /* В индексе нет ни одной записи */
    if (rc) 
    {
      LOG(ERROR)<<"Unable to initialize cursor for '"<<srv->GetNAME()
                <<"', rc="<<rc;
      break;
    }

    assert(letter == NULL);

    for (rc = broker_db::XDBLetter::SK_by_state_for_serv::search(t,
                                    &csr,
                                    MCO_EQ,
                                    srv->GetID(),
                                    UNASSIGNED);
         (rc == MCO_S_OK);
         rc = mco_cursor_next(t, &csr)) 
    {
       if ((MCO_S_NOTFOUND == rc) || (MCO_S_CURSOR_EMPTY == rc))
         break;

       rc = broker_db::XDBLetter::SK_by_state_for_serv::compare(t,
                &csr,
                srv->GetID(),
                UNASSIGNED,
                cmp_result);
       if (rc == MCO_S_OK && cmp_result == 0)
       {
         // Достаточно получить первый элемент,
         // функция будет вызываться до опустошения содержимого курсора 
         rc = letter_instance.from_cursor(t, &csr);
         if (rc) { LOG(ERROR) << "Unable to get item from Letters cursor, rc="<<rc; break; }

         rc = LoadLetter(t, letter_instance, letter);
         if (rc) { LOG(ERROR) << "Unable to read Letter instance, rc="<<rc; break; }

         assert(letter->GetSTATE() == xdb::Letter::UNASSIGNED);
         break;
       } // if database OK and item was found
    } // for each elements of cursor

  } while(false);

  delete []header_buffer;
  delete []body_buffer;

#if EXTREMEDB_VERSION > 40
  mco_cursor_close(t, &csr);
#endif
  mco_trans_rollback(t);

  return letter;
}

/*
 * Назначить Сообщение letter в очередь Обработчику worker
 * Сообщение готовится к отправке (вызов zmsg.send() вернул управление)
 *
 * Для Обработчика:
 * 1. Задать в Worker.letter_ref ссылку на Сообщение
 * 2. Заполнить Worker.expiration
 * 3. Изменить Worker.state с ARMED на OCCUPIED
 *
 * Для Сообщения:
 * 1. Задать в Letter.worker_ref ссылку на Обработчик
 * 2. Заполнить Letter.expiration
 * 3. Изменить Letter.state с UNASSIGNED на ASSIGNED
 */
bool DatabaseBrokerImpl::AssignLetterToWorker(Worker* worker, Letter* letter)
{
  mco_trans_h   t;
  MCO_RET       rc;
  broker_db::XDBLetter  letter_instance;
  broker_db::XDBWorker  worker_instance;
//  Worker::State worker_state;
//  Letter::State letter_state;
  autoid_t      worker_aid;
  autoid_t      letter_aid;
  timer_mark_t  exp_time;
  broker_db::timer_mark xdb_exp_worker_time;
  broker_db::timer_mark xdb_exp_letter_time;

  assert(worker);
  assert(letter);

  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = broker_db::XDBLetter::autoid::find(t, letter->GetID(), letter_instance);
    /* Запись не найдена - нет ошибки */
    if (MCO_S_NOTFOUND == rc) break;
    /* Запись не найдена - есть ошибка - сообщить */
    if (rc) { LOG(ERROR)<<"Unable to locate Letter with id="<<letter->GetID()<<", rc="<<rc; break; }

    rc = broker_db::XDBWorker::autoid::find(t, worker->GetID(), worker_instance);
    /* Запись не найдена - есть ошибка - сообщить */
    if (rc) { LOG(ERROR)<<"Unable to locate Worker with id="<<worker->GetID()<<", rc="<<rc; break; }

    /* ===== Обработчик и Сообщение действительно есть в БД =============== */

    rc = worker_instance.autoid_get(worker_aid);
    if (rc) { LOG(ERROR)<<"Unable to get id for Worker '"<<worker->GetIDENTITY()<<"', rc="<<rc; break; }

    rc = letter_instance.autoid_get(letter_aid);
    if (rc) { LOG(ERROR)<<"Unable to get id for Letter id='"<<letter->GetID()<<", rc="<<rc; break; }

    assert(worker_aid == worker->GetID());
    assert(letter_aid == letter->GetID());

    if (worker_aid != worker->GetID())
      LOG(ERROR) << "Id mismatch for worker "<< worker->GetIDENTITY()
                 <<" with id="<<worker_aid<<" ("<<worker->GetID()<<")";

    if (letter_aid != letter->GetID())
      LOG(ERROR) << "Id mismatch for letter id "<< letter->GetID();

    /* ===== Идентификаторы переданных Обработчика и Сообщения cовпадают со значениями из БД == */

    if (GetTimerValue(exp_time))
    {
      exp_time.tv_sec += Letter::ExpirationPeriodValue;
    }
    else { LOG(ERROR) << "Unable to calculate expiration time, rc="<<rc; break; }

    letter_instance.expiration_write(xdb_exp_letter_time);
    xdb_exp_letter_time.sec_put(exp_time.tv_sec);
    xdb_exp_letter_time.nsec_put(exp_time.tv_nsec);

    worker_instance.expiration_write(xdb_exp_worker_time);
    xdb_exp_worker_time.sec_put(exp_time.tv_sec);
    xdb_exp_worker_time.nsec_put(exp_time.tv_nsec);

    if (mco_get_last_error(t))
    {
      LOG(ERROR) << "Unable to set expiration time";
      break;
    }

    /* ===== Установлены ограничения времени обработки и для Сообщения, и для Обработчика == */
    
    // TODO проверить корректность преобразования
    rc = letter_instance.state_put(static_cast<LetterState>(ASSIGNED));
    if (rc) 
    {
      LOG(ERROR)<<"Unable changing state to "<<ASSIGNED<<" (ASSIGNED) from "
                <<letter->GetSTATE()<<" for letter with id="
                <<letter->GetID()<<", rc="<<rc;
      break; 
    }
    letter->SetSTATE(Letter::ASSIGNED);
    letter->SetWORKER_ID(worker->GetID());

    // TODO проверить корректность преобразования
    rc = worker_instance.state_put(static_cast<WorkerState>(OCCUPIED));
    if (rc) 
    { 
      LOG(ERROR)<<"Worker '"<<worker->GetIDENTITY()<<"' changing state OCCUPIED ("
                <<OCCUPIED<<"), rc="<<rc; break; 
    }
    worker->SetSTATE(Worker::OCCUPIED);

    /* ===== Установлены новые значения состояний Сообщения и Обработчика == */

    rc = worker_instance.letter_ref_put(letter->GetID());
    if (rc) 
    { 
        LOG(ERROR)<<"Unable to set letter ref "<<letter->GetID()
            <<" for worker '"<<worker->GetIDENTITY()<<"', rc="<<rc; 
        break; 
    }

    // Обновить идентификатор Обработчика, до этого он должен был быть равным 
    // id Сообщения, для соблюдения уникальности индекса SK_by_worker_id
    rc = letter_instance.worker_ref_put(worker->GetID());
    if (rc) 
    { 
        LOG(ERROR)<<"Unable to set worker ref "<<worker->GetID()
            <<" for letter id="<<letter->GetID()<<", rc="<<rc; 
        break; 
    }
#if 0
#warning "Здесь должен быть идентификатор Клиента, инициировавшего запрос, а не идентификатор Обработчика"
    rc = letter_instance.origin_put(letter->GetREPLY_TO(), strlen(letter->GetREPLY_TO()));
    if (rc)
    {
        LOG(ERROR) << "Set origin '"<<letter->GetREPLY_TO()<<"' for letter id="
                   <<letter->GetID()<<", rc=" << rc; 
        break;
    }
#endif

    LOG(INFO) << "Assign letter_id:"<<letter->GetID()<<" to wrk_id:"<<worker->GetID()<<" srv_id:"<<worker->GetSERVICE_ID();
    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }
  } while(false);

  if (rc)
    mco_trans_rollback(t);

  return (MCO_S_OK == rc);
}

/*
 * Удалить обработанное Worker-ом сообщение
 * Вызывается из Брокера по получении REPORT от Обработчика
 * TODO: рассмотреть возможность ситуаций, когда полученная квитанция не совпадает с хранимым Сообщением
 */
bool DatabaseBrokerImpl::ReleaseLetterFromWorker(Worker* worker)
{
  mco_trans_h    t;
  MCO_RET        rc;
  broker_db::XDBLetter letter_instance;

  assert(worker);

  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = broker_db::XDBLetter::SK_by_worker_id::find(t, worker->GetID(), letter_instance);
    if (rc)
    {
      LOG(WARNING) << "Unable to find any letter assigned to worker "<<worker->GetIDENTITY();
      break;
    }

    // GEV : start test
    autoid_t letter_aid, worker_aid, service_aid;
    rc = letter_instance.autoid_get(letter_aid);
    rc = letter_instance.worker_ref_get(worker_aid);
    rc = letter_instance.service_ref_get(service_aid);
    assert(worker_aid == worker->GetID());
    assert(service_aid == worker->GetSERVICE_ID());
    // GEV : end test
    //
    rc = letter_instance.remove();
    if (rc)
    {
      LOG(ERROR) << "Unable to remove letter assigned to worker "<<worker->GetIDENTITY();
      break;
    }

    // Сменить состояние Обработчика на ARMED
    if (false == SetWorkerState(worker, xdb::Worker::ARMED))
    {
      LOG(ERROR) << "Unable to set worker '"<<worker->GetIDENTITY()<<"' into ARMED";
      break;
    }
    worker->SetSTATE(xdb::Worker::ARMED);

    LOG(INFO) << "Release letter_id:"<<letter_aid<<" from wrk_id:"<<worker_aid<<" srv_id:"<<service_aid;
    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

  } while(false);

  if (rc)
  {
    LOG(ERROR) << "Unable to find the letter needs to be released for worker '"
               <<worker->GetIDENTITY()<<"'";
    mco_trans_rollback(t);
  }

  return (MCO_S_OK == rc);
}

// Найти экземпляр Сообщения по паре Сервис/Обработчик
// xdb::Letter::SK_wrk_srv - найти экземпляр по service_id и worker_id
Letter* DatabaseBrokerImpl::GetAssignedLetter(Worker* worker)
{
//  mco_trans_h   t;
//  MCO_RET       rc;
  Letter* letter = NULL;
  broker_db::XDBLetter letter_instance;

  assert(worker);

#if 0
  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = broker_db::XDBLetter::SK_by_worker_id::find(t,
            worker->GetID(),
            letter_instance);
    /* Запись не найдена - нет ошибки */
    if (MCO_S_NOTFOUND == rc) break;
    /* Запись не найдена - есть ошибка - сообщить */
    if (rc)
    {
        LOG(ERROR)<<"Unable to locate Letter assigned to worker "<<worker->GetIDENTITY()<<", rc="<<rc;
        break;
    }

    rc = LoadLetter(t, letter_instance, letter);

    if (rc)
    {
      LOG(ERROR)<<"Unable to instantiating Letter assigned to worker "<<worker->GetIDENTITY()<<", rc="<<rc;
      break;
    }

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }
  } while(false);

  if (rc)
    mco_trans_rollback(t);

#else
#endif
  return letter;
}

// Изменить состояние Сообщения
// NB: Функция не удаляет экземпляр из базы, а только помечает новым состоянием!
bool DatabaseBrokerImpl::SetLetterState(Letter* letter, Letter::State _new_state)
{
  mco_trans_h   t;
  MCO_RET       rc;
  broker_db::XDBLetter  letter_instance;

  assert(letter);
  if (!letter)
    return false;

  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = broker_db::XDBLetter::autoid::find(t, letter->GetID(), letter_instance);
    /* Запись не найдена - нет ошибки */
    if (MCO_S_NOTFOUND == rc) break;
    /* Запись не найдена - есть ошибка - сообщить */
    if (rc) { LOG(ERROR)<<"Unable to locate Letter with id="<<letter->GetID()<<", rc="<<rc; break; }

    // TODO проверить корректность преобразования из Letter::State в LetterState 
    rc = letter_instance.state_put((LetterState)_new_state);
    if (rc) 
    {
      LOG(ERROR)<<"Unable changing state to "<<_new_state<<" from "<<letter->GetSTATE()
                <<" for letter with id="<<letter->GetID()<<", rc="<<rc;
      break; 
    }
    letter->SetSTATE(_new_state);

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }

  } while(false);

  if (rc)
    mco_trans_rollback(t);

  return (MCO_S_OK == rc);
}

bool DatabaseBrokerImpl::SetWorkerState(Worker* worker, Worker::State new_state)
{
  mco_trans_h t;
  MCO_RET rc;
  broker_db::XDBWorker  worker_instance;
  WorkerState state;

  assert(worker);
  if (!worker)
    return false;

  const char* ident = worker->GetIDENTITY();

  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    /* найти запись в таблице сервисов с заданным именем */
    rc = broker_db::XDBWorker::SK_by_ident::find(t,
                ident,
                strlen(ident),
                worker_instance);

    /* Запись не найдена - нет ошибки */
    if (MCO_S_NOTFOUND == rc) break;
    /* Запись не найдена - есть ошибка - сообщить */
    if (rc) { LOG(ERROR)<<"Worker '"<<ident<<"' location, rc="<<rc; break; }

    // TODO: проверить совместимость WorkerState между Worker::State
    switch(new_state)
    {
      case DISARMED:
        state = DISARMED;
      break;

      case ARMED:
        state = ARMED;
        // Удалить ссылку на ранее используемый экземпляр Letter
        rc = worker_instance.letter_ref_put(0);
        LOG(INFO)<<"Worker '"<<ident<<"' release old letter " <<  worker->GetLETTER_ID();
        if (rc) { LOG(ERROR)<<"Worker '"<<ident<<"' release old letter, rc="<<rc; break; }
        worker->SetLETTER_ID(0);
      break;

      case INIT:
        state = INIT;
      break;

      case SHUTDOWN:
        state = SHUTDOWN;
      break;

      case OCCUPIED:
        state = OCCUPIED;
      break;

      case EXPIRED:
        state = EXPIRED;
      break;

      default:
        state = DISARMED;
        LOG(ERROR) << "Worker '" << ident << "' have unknown state " << new_state;
    }

    rc = worker_instance.state_put(state);
    if (rc) { LOG(ERROR)<<"Worker '"<<ident<<"' changing state to "<<new_state<<", rc="<<rc; break; }
    worker->SetSTATE(new_state);

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }
  } while (false);

  if (rc)
    mco_trans_rollback(t);

  return (MCO_S_OK == rc);
}

Worker *DatabaseBrokerImpl::GetWorkerByIdent(const std::string& ident)
{
  // TODO: Реализовать для использования локализованных названий сервисов
  //
  return GetWorkerByIdent(ident.c_str());
}

void DatabaseBrokerImpl::EnableServiceCommand(
        const Service *srv, 
        const std::string &command)
{
  assert(srv);
  assert(!command.empty());
  assert (1 == 0);
}

void DatabaseBrokerImpl::EnableServiceCommand(
        const std::string &srv_name, 
        const std::string &command)
{
  assert(!srv_name.empty());
  assert(!command.empty());
  assert (1 == 0);
}

void DatabaseBrokerImpl::DisableServiceCommand(
        const std::string &srv_name, 
        const std::string &command)
{
  assert(!srv_name.empty());
  assert(!command.empty());
  assert (1 == 0);
}

void DatabaseBrokerImpl::DisableServiceCommand(
        const Service *srv, 
        const std::string &command)
{
  assert(srv);
  assert(!command.empty());
  assert (1 == 0);
}

/* TODO: deprecated */
bool DatabaseBrokerImpl::PushRequestToService(Service *srv,
            const std::string& reply_to,
            const std::string& header,
            const std::string& body)
{
  assert(srv);
  assert(!reply_to.empty());
  assert(!header.empty());
  assert(!body.empty());
  assert (1 == 0);
}

/*
 * Поместить новое сообщение letter в очередь сервиса srv.
 * Сообщение будет обработано одним из Обработчиков Сервиса.
 * Первоначальное состояние: READY
 * Состояние после успешной передачи Обработчику: PROCESSING
 * Состояние после получения ответа: DONE_OK или DONE_FAIL
 */
bool DatabaseBrokerImpl::PushRequestToService(Service *srv, Letter *letter)
{

  MCO_RET      rc;
  mco_trans_h  t;
  broker_db::XDBLetter  letter_instance;
  broker_db::XDBService service_instance;
  broker_db::XDBWorker  worker_instance;
  autoid_t     aid;
  timer_mark_t what_time;
  broker_db::timer_mark   mark;

  assert (srv);
  assert(letter);

  /*
   * 1. Получить id Службы
   * 2. Поместить сообщение и его заголовок в таблицу XDBLetter 
   * с состоянием UNASSIGNED.
   * Далее, после передачи сообщения Обработчику, он вносит свой id
   */
  do
  {
    rc = mco_trans_start(m_database->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = letter_instance.create(t);
    if (rc)
    { 
        LOG(ERROR)<<"Unable to create new letter for service '"
                  <<srv->GetNAME()<<"', rc="<<rc;
        break; 
    }

    rc = letter_instance.autoid_get(aid);
    if (rc) { LOG(ERROR) << "Get letter id, rc="<<rc; break; }
    letter->SetID(aid);

    rc = letter_instance.service_ref_put(srv->GetID());
    if (rc) { LOG(ERROR) << "Set reference to service id="<<srv->GetID(); break; }
    letter->SetSERVICE_ID(srv->GetID());

    /*
     * NB: для сохранения уникальности ключа SK_by_worker_id нельзя оставлять пустым 
     * поле worker_ref_id, хотя оно и должно будет заполниться в AssignLetterToWorker.
     * Поэтому сейчас в него запишем идентификатор Letter, т.к. он уникален.
     */
    rc = letter_instance.worker_ref_put(aid);
    if (rc) { LOG(ERROR) << "Set temporary reference to unexistance worker id="<<aid; break; }
    // NB: в базе - не ноль, в экземпляре класса - 0. Может возникнуть путаница.
    letter->SetWORKER_ID(0);

    rc = letter_instance.state_put(UNASSIGNED); // ASSIGNED - после передачи Обработчику
    if (rc) { LOG(ERROR) << "Set state to UNASSIGNED("<<UNASSIGNED<<"), rc="<<rc; break; }
    letter->SetSTATE(xdb::Letter::UNASSIGNED);

    rc = letter_instance.expiration_write(mark);
    if (rc) { LOG(ERROR) << "Set letter expiration, rc="<<rc; break; }
    if (GetTimerValue(what_time))
    {
      mark.nsec_put(what_time.tv_nsec);
      what_time.tv_sec += Worker::HeartbeatPeriodValue/1000; // из мсек в сек
      mark.sec_put(what_time.tv_sec);
      letter->SetEXPIRATION(what_time);
    }
    else 
    {
      LOG(WARNING) << "Unable to calculate expiration time"; 
      mark.nsec_put(0);
      mark.sec_put(0);
    }

    rc = letter_instance.header_put(letter->GetHEADER().data(), letter->GetHEADER().size());
    if (rc) { LOG(ERROR) << "Set message header, rc="<<rc; break; }
    rc = letter_instance.body_put(letter->GetDATA().data(), letter->GetDATA().size());
    if (rc) { LOG(ERROR) << "Set data, rc="<<rc; break; }
    rc = letter_instance.origin_put(letter->GetREPLY_TO(), strlen(letter->GetREPLY_TO()));
    if (rc) { LOG(ERROR) << "Set field REPLY to '"<<letter->GetREPLY_TO()<<"', rc="<<rc; break; }
    if (mco_get_last_error(t))
    { 
        LOG(ERROR)<<"Unable to set letter attributes for service '"
                  <<srv->GetNAME()<<"', rc="<<rc;
        break; 
    }
    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }

    LOG(INFO) << "PushRequestToService '"<<srv->GetNAME()<<"' id="<<aid;
  } while(false);

  if (rc)
    mco_trans_rollback(t);

  return (MCO_S_OK == rc);
}

/* Тестовый API сохранения базы */
void DatabaseBrokerImpl::MakeSnapshot(const char* msg)
{
  m_database->SaveAsXML(NULL, msg);
}

ServiceList* DatabaseBrokerImpl::GetServiceList()
{
  if (!m_service_list)
    m_service_list = new ServiceList(m_database->getDbHandler());

  if (!m_service_list->refresh())
    LOG(WARNING) << "Services list is not updated";

  return m_service_list;
}


ServiceList::ServiceList(mco_db_h _db) :
    m_current_index(0),
    m_db_handler(_db),
    m_size(0)
{
  m_array = new Service*[MAX_SERVICES_ENTRY];
  memset(m_array, '\0', MAX_SERVICES_ENTRY*sizeof(Service*));
  assert(m_db_handler);
}

ServiceList::~ServiceList()
{
  if ((m_size) && (m_array))
  {
    for (int i=0; i<m_size; i++)
    {
      delete m_array[i];
    }
  }
  delete []m_array;
}

Service* ServiceList::first()
{
  Service *srv = NULL;

  assert(m_array);

  if (m_array)
  {
    m_current_index = 0;
    srv = m_array[m_current_index];
  }

  if (m_size)
    m_current_index++; // если список не пуст, передвинуться на след элемент

  return srv;
}

Service* ServiceList::last()
{
  Service *srv = NULL;

  assert(m_array);
  if (m_array)
  {
    if (!m_size)
      m_current_index = m_size; // Если список пуст
    else
      m_current_index = m_size - 1;

    srv = m_array[m_current_index];
  }

  return srv;
}

Service* ServiceList::next()
{
  Service *srv = NULL;

  assert(m_array);
  if (m_array && (m_current_index < m_size))
  {
    srv = m_array[m_current_index++];
  }

  return srv;
}

Service* ServiceList::prev()
{
  Service *srv = NULL;

  assert(m_array);
  if (m_array && (m_size < m_current_index))
  {
    srv = m_array[m_current_index--];
  }

  return srv;
}


// Получить количество зарегистрированных объектов
int ServiceList::size() const
{
  return m_size;
}

// Добавить новый Сервис, определенный своим именем и идентификатором
bool ServiceList::AddService(const char* name, int64_t id)
{
  Service *srv;

  if (m_size >= MAX_SERVICES_ENTRY)
    return false;
 
  srv = new Service(id, name);
  m_array[m_size++] = srv;
  
//  LOG(INFO) << "EVENT AddService '"<<srv->GetNAME()<<"' id="<<srv->GetID();
  return true;
}

// Добавить новый Сервис, определенный объектом Service
bool ServiceList::AddService(const Service* service)
{
  assert(service);
  if (m_size >= MAX_SERVICES_ENTRY)
    return false;

//  LOG(INFO) << "EVENT AddService '"<<service->GetNAME()<<"' id="<<service->GetID();
  m_array[m_size++] = const_cast<Service*>(service);

  return true;
}

// Удалить Сервис по его имени
bool ServiceList::RemoveService(const char* name)
{
  LOG(INFO) << "EVENT DelService '"<<name<<"'";
  return false;
}

// Удалить Сервис по его идентификатору
bool ServiceList::RemoveService(const int64_t id)
{
  LOG(INFO) << "EVENT DelService with id="<<id;
  return false;
}

// Перечитать список Сервисов из базы данных
bool ServiceList::refresh()
{
  bool status = false;
  mco_trans_h t;
  MCO_RET rc = MCO_S_OK;
  mco_cursor_t csr;
  broker_db::XDBService service_instance;
  char name[Service::NameMaxLen + 1];
  autoid_t aid;

  //delete m_array;
  for (int i=0; i < m_size; i++)
  {
    // Удалить экземпляр Service, хранящийся по ссылке
    delete m_array[i];
    m_array[i] = NULL;  // NULL как признак свободного места
  }
  m_size = 0;

  do
  {
    rc = mco_trans_start(m_db_handler, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc="<<rc; break; }

    rc = broker_db::XDBService::list_cursor(t, &csr);
    // Если курсор пуст, вернуться
    if (MCO_S_CURSOR_EMPTY == rc)
      break;
    if (rc) { LOG(ERROR) << "Unable to get Services list cursor, rc="<<rc; break; }

    rc = mco_cursor_first(t, &csr);
    if (rc) 
    { 
        LOG(ERROR) << "Unable to point at first item in Services list cursor, rc="<<rc;
        break;
    }

    while (rc == MCO_S_OK)
    {
      rc = service_instance.from_cursor(t, &csr);
      if (rc) 
      { 
        LOG(ERROR) << "Unable to get item from Services list cursor, rc="<<rc; 
      }

      rc = service_instance.name_get(name, Service::NameMaxLen);
      if (rc) { LOG(ERROR) << "Getting service name, rc="<<rc; break; }

      rc = service_instance.autoid_get(aid);
      if (rc) { LOG(ERROR) << "Getting ID of service '"<<name<<"', rc=" << rc; break; }

      if (false == AddService(name, aid))
        LOG(ERROR) << "Unable to add new service '"<<name<<"':"<<aid<<" into list";

      rc = mco_cursor_next(t, &csr);
    }

  } while(false);

#if EXTREMEDB_VERSION > 40
  mco_cursor_close(t, &csr);
#endif
  mco_trans_rollback(t);

  switch(rc)
  {
    case MCO_S_CURSOR_END:
    case MCO_S_CURSOR_EMPTY:
      status = true;
      break;

    default:
      status = false;
  }

  return status;
}


