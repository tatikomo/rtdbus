#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "dat/xdb_broker.hpp"
#include "xdb_database_broker.hpp"
#include "xdb_database_broker_impl.hpp"
#include "xdb_database_service.hpp"
#include "xdb_database_worker.hpp"

const int   SEGSZ = 1024 * 1024 * 1; // 1M
const int   MAP_ADDRESS = 0x20000000;

#ifndef MCO_PLATFORM_X64
    static const int PAGESIZE = 128;
#else 
    static const int PAGESIZE = 256;
#endif 

#ifdef DISK_DATABASE
    #define DB_DISK_CACHE   (4 * 1024 * 1024)
    #define DB_DISK_PAGE_SIZE 4096

    #ifndef DB_LOG_TYPE
        #define DB_LOG_TYPE REDO_LOG
    #endif 
#else 
    #define DB_DISK_CACHE   0
#endif 

/* 
 * Включение динамически генерируемых определений 
 * структуры данных для внутренней базы Брокера.
 *
 * Регенерация осуществляется командой mcocomp 
 * на основе содержимого файла broker.mco
 */
#include "dat/xdb_broker.h"
#include "dat/xdb_broker.hpp"

XDBDatabaseBrokerImpl::XDBDatabaseBrokerImpl(
    const XDBDatabaseBroker *self, 
    const char *name)
{
  assert(self);
  assert(name);

  m_self = self;
  strncpy(m_name, name, DBNAME_MAXLEN);
  m_name[DBNAME_MAXLEN] = '\0';

#ifdef DISK_DATABASE
  m_dbsFileName = new char[strlen(name) + 5];
  m_logFileName = new char[strlen(name) + 5];

  strcpy(m_dbsFileName, name);
  strcat(m_dbsFileName, ".dbs");

  strcpy(m_logFileName, name);
  strcat(m_logFileName, ".log");
#endif
//  printf("\tXDBDatabaseBrokerImpl(%p, %s)\n", (void*)self, name);
}

XDBDatabaseBrokerImpl::~XDBDatabaseBrokerImpl()
{
  const char *fctName = "~XDBDatabaseBrokerImpl";
  MCO_RET rc;

  assert(m_self);
  rc = mco_db_disconnect(m_db);
  if (rc) { LogError(rc, fctName, "disconnecting failure"); }
  rc = mco_db_close(m_name);
  if (rc) { LogError(rc, fctName, "closing failure"); }

/*  if (kill_after)
      mco_db_kill(m_name);*/

#ifdef DISK_DATABASE
  delete []m_dbsFileName;
  delete []m_logFileName;
#endif

/*  printf("\t~XDBDatabaseBrokerImpl(%p, %s)\n", 
        (void*)m_self,
        ((XDBDatabase*)m_self)->DatabaseName());*/
}

void XDBDatabaseBrokerImpl::LogError(MCO_RET rc, 
            const char *functionName, 
            const char *msg)
{
    const char *empty = "";
    printf("E %s: %s [rc=%d]\n", 
        functionName? functionName : empty,
        msg? msg : empty,
        rc);
}

bool XDBDatabaseBrokerImpl::Open()
{
  switch (((XDBDatabase*)m_self)->State())
  {
    case XDBDatabase::OPENED:
      printf("\tTry to reopen database %s\n",
        ((XDBDatabase*)m_self)->DatabaseName());
    break;

    case XDBDatabase::CONNECTED:
    case XDBDatabase::CLOSED:
        AttachToInstance();
    break;

    case XDBDatabase::DISCONNECTED:
      printf("\tTry to reopen disconnected database %s\n",
        ((XDBDatabase*)m_self)->DatabaseName());
    break;

    default:
      printf("\tUnknown database %s state %d\n",
        ((XDBDatabase*)m_self)->DatabaseName(),
        (int)((XDBDatabase*)m_self)->State());
    break;
  }
}

bool XDBDatabaseBrokerImpl::AttachToInstance()
{
    MCO_RET rc;

    /* подключиться к базе данных, предполагая что она создана */
    printf("\nattaching to instance '%s'\n",
           ((XDBDatabase*)m_self)->DatabaseName());
    rc = mco_db_connect(((XDBDatabase*)m_self)->DatabaseName(),
            &m_db);

    /* ошибка - экземпляр базы не найден, попробуем создать её */
    if (rc == MCO_E_NOINSTANCE)
    {
        printf("'%s' instance not found, create\n",
            ((XDBDatabase*)m_self)->DatabaseName());
        /*
         * TODO: Использование mco_db_open() является запрещенным,
         * начиная с версии 4 и старше
         */
#if EXTREMEDB_VER >= 40
        rc = mco_db_open_dev(((XDBDatabase*)m_self)->DatabaseName(),
                         xdb_broker_get_dictionary(),
                         (void*)MAP_ADDRESS,
                         SEGSZ + DB_DISK_CACHE,
                         PAGESIZE);
#else
        rc = mco_db_open(((XDBDatabase*)m_self)->DatabaseName(),
                         xdb_broker_get_dictionary(),
                         (void*)MAP_ADDRESS,
                         SEGSZ + DB_DISK_CACHE,
                         PAGESIZE);
#endif

#ifdef DISK_DATABASE
        printf("Disk database '%s' opening\n", m_dbsFileName);
        rc = mco_disk_open(((XDBDatabase*)m_self)->DatabaseName(),
                           m_dbsFileName,
                           m_logFileName, 
                           0, 
                           DB_DISK_CACHE, 
                           DB_DISK_PAGE_SIZE,
                           MCO_INFINITE_DATABASE_SIZE,
                           DB_LOG_TYPE);

        if (rc != MCO_S_OK && rc != MCO_ERR_DISK_ALREADY_OPENED)
        {
            printf("\nerror creating disk database");
            return false;
        }
#endif

        /* подключиться к базе данных, т.к. она только что создана */
        printf("connecting to instance '%s'\n", 
            ((XDBDatabase*)m_self)->DatabaseName());
        rc = mco_db_connect(((XDBDatabase*)m_self)->DatabaseName(), &m_db);
    }

    /* ошибка создания экземпляра - выход из системы */
    if (rc)
    {
        printf("\nCould not create instance: %d\n", rc);
        return false;
    }

    ((XDBDatabase*)m_self)->TransitionToState(XDBDatabase::OPENED);
    return true;
}

bool XDBDatabaseBrokerImpl::AddService(const char *name)
{
  const char    *fctName = "XDBDatabaseBrokerImpl::AddService";
  bool           status = false;
  xdb_broker::XDBService instance;
  MCO_RET        rc;
  mco_trans_h    t;

  assert(name);
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    if (rc) { LogError(rc, fctName, "transaction starting failure"); break; }

    rc = instance.create(t);
    if (rc) { LogError(rc, fctName, "instance creating failure"); break; }

    rc = instance.name_put(name, strlen(name));
    if (rc) { LogError(rc, fctName, "name setting failure"); break; }

    rc = instance.checkpoint();
    if (rc) { LogError(rc, fctName, "checkpointing failure"); break; }

    rc = mco_trans_commit(t);
    if (rc) { LogError(rc, fctName, "transaction commitment failure"); break; }

    status = true;
    break;
  }

  if (rc)
    mco_trans_rollback(t);

  return status;
}

#if 0
  XDBService *service = NULL;
  if (NULL != (service = GetServiceByName(name)))
  {
      oid.id = service->GetID();
      rc = instance.create(t, &oid);
      if (rc) { LogError(rc, fctName, "creating failure"); break; }
      rc = instance.remove();
      if (rc) { LogError(rc, fctName, "removing failure"); break; }
      delete service;
  }
#endif

/*
 * Удалить запись о заданном сервисе
 * - найти указанную запись в базе
 *   - если найдена, удалить её
 *   - если не найдена, вернуть ошибку
 */
bool XDBDatabaseBrokerImpl::RemoveService(const char *name)
{
  const char *fctName = "XDBDatabaseBrokerImpl::RemoveService";
  bool        status = false;
  xdb_broker::XDBService instance;
  MCO_RET        rc;
  mco_trans_h    t;

  assert(name);
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    if (rc) { LogError(rc, fctName, "transaction starting failure"); break; }

    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::XDBService::pair::find(t, name, strlen(name), instance);
    if (MCO_S_NOTFOUND == rc) 
    { 
        LogError(rc, fctName, "removed service doesn't exists"); break;
    }
    if (rc) { LogError(rc, fctName, "searching failure"); break; }
    
    rc = instance.remove();
    if (rc) { LogError(rc, fctName, "removing service failure"); break; }

    status = true;
    break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    mco_trans_commit(t);

  return status;
}

bool XDBDatabaseBrokerImpl::RemoveWorker(Worker *wrk)
{
  bool status = false;
  Service *srv = NULL;

  assert(wrk);

  if (srv = GetServiceById(wrk->GetSERVICE_ID()))
  {
    /* TODO удалить Обработчик из списка своей Службы */
  }
  return status;
}


Service *XDBDatabaseBrokerImpl::GetServiceByName(const std::string& name)
{
  return GetServiceByName(name.c_str());
}

/*
 * Найти в базе данных запись о Сервисе по его имени
 * @return Новый объект, представляющий Сервис
 * Вызываюшая сторона должна сама удалить объект возврата
 */
Service *XDBDatabaseBrokerImpl::GetServiceByName(const char* name)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::GetServiceByName";
  MCO_RET       rc;
  bool          status = false;
  mco_trans_h   t;
  xdb_broker::XDBService instance;
  Service *service = NULL;
  /* NB: 32 - это размер поля service_name_t из broker.mco */
  char     name_from_db[32+1];
  uint2    name_from_db_length = sizeof(name_from_db)-1;
  autoid_t id;
  uint2         workers_waiting_count = 0;

  rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::XDBService::pair::find(t, name, strlen(name), instance);

    /* Запись не найдена - нет ошибки */
    if (MCO_S_NOTFOUND == rc) break;

    /* Запись не найдена - есть ошибка - сообщить */
    if (rc) { LogError(rc, fctName, "\t\tlocating service failure"); break; }

    /* Запись найдена - сконструировать объект на основе данных из БД */
    rc = instance.name_get(name_from_db, name_from_db_length);
    if (rc) { LogError(rc, fctName, "\t\tget service name failure"); break; }
    rc = instance.autoid_get(id);
    if (rc) { LogError(rc, fctName, "\t\tget service id failure"); break; }
    rc = instance.waiting_size(workers_waiting_count);

    service = new Service(id, name_from_db);
    service->SetWaitingWorkersCount(workers_waiting_count);
    break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    mco_trans_commit(t);

  return service;
}

/* 
 * Вернуть ближайший свободный Обработчик.
 * Побочные эффекты: нет
 */
Worker *XDBDatabaseBrokerImpl::GetWorkerForService(const Service *srv)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::GetWorkerForService";
  MCO_RET       rc;
  bool          status = false;
  mco_trans_h   t;
  xdb_broker::XDBService instance;
  Worker       *worker = NULL;
  xdb_broker_oid oid;

  assert(srv);
  rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    rc = instance.waiting_at((uint2)0, oid);
    if (rc) { LogError(rc, fctName, "service's get waiting worker failure"); break; }

    break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    mco_trans_commit(t);

  return worker;
}


/* 
 * Вернуть ближайший свободный Обработчик.
 *
 * Побочные эффекты:
 * Выбранный экземпляр в базе данных удаляется из списка ожидающих
 */
Worker *XDBDatabaseBrokerImpl::PopWorkerForService(const char *name)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::PopWorkerForService";
  MCO_RET       rc;
  bool          status = false;
  mco_trans_h   t;
  xdb_broker::XDBService instance;
  xdb_broker_oid oid;

  assert(name);
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  if (rc)
  {
    LogError(rc, fctName, NULL);
    mco_trans_rollback(t);
    return NULL;
  }

  rc = instance.waiting_at((uint2)0, oid);
  // TODO: удалить экземпляр из базы

  mco_trans_commit(t);
  return NULL;
}

/* 
 * Вернуть ближайший свободный Обработчик.
 *
 * Побочные эффекты:
 * Выбранный экземпляр в базе данных удаляется из списка ожидающих.
 * Вызывающая сторона должна удалить полученный экземпляр Worker
 */
Worker *XDBDatabaseBrokerImpl::PopWorkerForService(
        Service *service)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::PopWorkerForService";
  MCO_RET       rc;
  bool          status = false;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;
  xdb_broker_oid service_oid;
  uint2         workers_waiting_count = 0;
  autoid_t      worker_aid;
  Worker       *worker = NULL;
  char          identity[WORKER_IDENTITY_MAXLEN+1];

  assert(service);
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    rc = xdb_broker::XDBService::autoid::find(t, service->GetID(), service_instance);
    if (rc) { LogError(rc, fctName, "locating service failure"); break; }
    /* Есть ли в спуле Сервиса Обработчики? */
    rc = service_instance.waiting_size(workers_waiting_count);
    if (!workers_waiting_count)
    {
       /* Обработчиков нет - вернуться */
       mco_trans_commit(t);
       status = true;
       break;
    }

    rc = service_instance.waiting_at((uint2)0, service_oid);
    if (rc) { LogError(rc, fctName, "get service's 0-waiting worker failure"); break; }
    /* Получить по service_oid экземпляр Обработчика */
    worker_aid = service_oid.id;
    rc = xdb_broker::XDBWorker::autoid::find(t, worker_aid, worker_instance);
    if (rc) { LogError(rc, fctName, "locating worker by id failure"); break; }
    rc =  worker_instance.identity_get(identity, (uint2) sizeof(identity)-1);
    if (rc) { LogError(rc, fctName, "get worker's identity failure"); break; }
    worker = new Worker((int64_t)worker_aid,
                        (const char*)identity,
                        (int64_t)service_oid.id);

    mco_trans_commit(t);
    status = true;
    break;
  }

  if (rc)
  {
    LogError(rc, fctName, NULL);
    status = false;
    mco_trans_rollback(t);
  }

  return worker;
}

Service *XDBDatabaseBrokerImpl::GetServiceForWorker(const Worker *wrk)
{
  // TODO: реализация
  return NULL;
}


Service *XDBDatabaseBrokerImpl::GetServiceById(int64_t _id)
{
  // TODO: реализация
  return NULL;
}

bool XDBDatabaseBrokerImpl::IsServiceExist(const char *name)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::IsServiceExist";
  MCO_RET       rc;
  bool          status = false;
  mco_trans_h   t;
  xdb_broker::XDBService instance;

  assert(name);
  rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  if (rc)
  {
    LogError(rc, fctName, NULL);
    mco_trans_rollback(t);
    return false;
  }
  rc = xdb_broker::XDBService::pair::find(t, name, strlen(name), instance);
  mco_trans_commit(t);

  return (MCO_S_NOTFOUND == rc)? false:true;
}

bool XDBDatabaseBrokerImpl::PushWorkerForService(
        Service *service,
        Worker  *worker)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::PushWorkerForService";
  MCO_RET       rc;
  bool          status = false;
  mco_trans_h   t;
  uint2         waiting_size = 0;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker worker_instance;
  mco_time      expire_time;
  timer_unit    beg_time = 0, end_time = 0;
  xdb_broker_oid oid;
  autoid_t  aid;

  assert(service);
  assert(worker);

  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    rc = xdb_broker::XDBService::autoid::find(t, service->GetID(), service_instance);
    if (rc) { LogError(rc, fctName, "locating service failure"); break; }

    /* Создать нового Обработчика */
    rc = worker_instance.create(t);
    if (rc) { LogError(rc, fctName, "worker's creating failure"); break; }
    rc = worker_instance.identity_put(
            worker->GetIDENTITY(),
            strlen(worker->GetIDENTITY()));
    if (rc) { LogError(rc, fctName, "worker's identity assign failure"); break; }
    oid.id = service->GetID();
    rc = worker_instance.service_put(oid);
    if (rc) { LogError(rc, fctName, "worker's service name assign failure"); break; }
    beg_time = mco_system_get_current_time();
    end_time = beg_time + 100; /* TODO GEV это тест! - неясно как соотносятся time_t и mco_time */
    rc = worker_instance.expiration_put(end_time);
    if (rc) { LogError(rc, fctName, "worker's set expiration time failure"); break; }
    rc = worker_instance.checkpoint();
    if (rc) { LogError(rc, fctName, "worker's checkpoint failure"); break; }
    rc = worker_instance.autoid_get(aid);
    if (rc) { LogError(rc, fctName, "worker's get autoid failure"); break; }
    /* Обработчик создан */

    /* Внести нового Обработчика в спул Сервиса */
    rc = service_instance.waiting_size(waiting_size);
    if (rc) { LogError(rc, fctName, "get waiting workers quantity failure"); break; }
    rc = service_instance.waiting_alloc(waiting_size+1);
    if (rc) { LogError(rc, fctName, "set waiting workers quantity failure"); break; }

    oid.id = aid; /* aid Обработчика */
    rc = service_instance.waiting_put(waiting_size, oid);
    if (rc) { LogError(rc, fctName, "insert new waiting worker failure"); break; }

    rc = mco_trans_commit(t);
    if (rc) { LogError(rc, fctName, "transaction commit failure"); break; }
    status = true;
    break;
  }

  if (rc)
  {
    LogError(rc, fctName, NULL);
    status = false;
    mco_trans_rollback(t);
  }

  return status;
}

bool XDBDatabaseBrokerImpl::ClearWorkersForService(const char *name)
{
    /* TODO: Очистить спул Обработчиков указанной Службы */
    return false;
}

bool XDBDatabaseBrokerImpl::ClearServices()
{
    /* TODO: Очистить спул Обработчиков и всех Служб */
    return false;
}

/* Вернуть NULL или Обработчика по его идентификационной строке */
/*
 * Найти в базе данных запись об Обработчике по его имени
 * @return Новый объект, представляющий Обработчика
 * Вызываюшая сторона должна сама удалить объект возврата
 */
Worker *XDBDatabaseBrokerImpl::GetWorkerByIdent(const char *ident)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::GetWorkerByIdent";
  MCO_RET       rc;
  bool          status = false;
  mco_trans_h   t;
  xdb_broker::XDBWorker instance;
  Worker  *worker = NULL;
#if 0
  /* NB: WORKER_IDENTITY_MAXLEN - это размер поля identity_t из broker.mco */
  char     identity_from_db[WORKER_IDENTITY_MAXLEN+1];
  uint2    identity_from_db_length = sizeof(identity_from_db)-1;
#endif
  autoid_t self_id;
  mco_time expiration;
  xdb_broker_oid service_oid;

  rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::XDBWorker::PK_ident::find(t,
                ident,
                strlen(ident),
                instance);

    /* Запись не найдена - нет ошибки */
    if (MCO_S_NOTFOUND == rc) break;

    /* Запись не найдена - есть ошибка - сообщить */
    if (rc) { LogError(rc, fctName, "worker location failure"); break; }

    /* Запись найдена - сконструировать объект на основе данных из БД */
#if 0
    rc = instance.identity_get(identity_from_db, identity_from_db_length);
    if (rc) { LogError(rc, fctName, "worker's get identity failure"); break; }
#else
    /* IDENTITY у нас уже есть - это ident */
#endif
    rc = instance.autoid_get(self_id);
    if (rc) { LogError(rc, fctName, "worker's get id failure"); break; }
    rc = instance.service_get(service_oid);
    if (rc) { LogError(rc, fctName, "worker's get service id failure"); break; }
    rc = instance.expiration_get(expiration);
    if (rc) { LogError(rc, fctName, "worker's get expiration failure"); break; }

    worker = new Worker(self_id, ident, service_oid.id);
    assert(worker);
    worker->SetEXPIRATION(expiration);
    status = true;
    break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    mco_trans_commit(t);


  return worker;
}


Worker *XDBDatabaseBrokerImpl::GetWorkerByIdent(const std::string& ident)
{
    return NULL;
}

void XDBDatabaseBrokerImpl::EnableServiceCommand(
        const Service *srv, 
        const std::string &command)
{
  assert(srv);
}

void XDBDatabaseBrokerImpl::EnableServiceCommand(
        const std::string &srv_name, 
        const std::string &command)
{
}

void XDBDatabaseBrokerImpl::DisableServiceCommand(
        const std::string &srv_name, 
        const std::string &command)
{
}

void XDBDatabaseBrokerImpl::DisableServiceCommand(
        const Service *srv, 
        const std::string &command)
{
  assert(srv);
}
