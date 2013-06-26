#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "config.h"
#include "helper.hpp"
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
    rc = xdb_broker::XDBService::PK_name::find(t, name, strlen(name), instance);
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

bool XDBDatabaseBrokerImpl::IsServiceCommandEnabled(const Service* srv, const std::string& cmd_name)
{
  bool status = false;
  assert(srv);
  // TODO реализация
  return status;
}


// Деактивировать Обработчик wrk по идентификатору у его Сервиса
bool XDBDatabaseBrokerImpl::RemoveWorker(Worker *wrk)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::RemoveWorker";
  bool          status = false;
  Service      *srv = NULL;
  MCO_RET       rc;
  autoid_t      aid;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;
  uint2         worker_index;

  assert(wrk);
  const char* identity = wrk->GetIDENTITY();

  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    if ((srv = GetServiceById(wrk->GetSERVICE_ID())))
    {
      // Удалось найти Сервис, к которому привязан Обработчик
      rc = xdb_broker::XDBService::SK_ident::find(t, 
            identity, 
            strlen(identity), 
            service_instance, 
            worker_index);
      if (rc) { LogError(rc, fctName, "locating worker failure"); break; }

      // номер искомого Обработчика в спуле есть worker_index
      rc = service_instance.SK_workers_at(worker_index, worker_instance);
      if (rc) { LogError(rc, fctName, "get worker from spool failure"); break; }

      // NB: может быть не удалять, пометить как "неактивен"?
      // rc = worker_instance.state_put(DISARMED);
      // удалить у Сервиса упоминания о данном Обработчике
      rc = service_instance.SK_workers_erase(worker_index);
      if (rc) { LogError(rc, fctName, "worker disarming failure"); break; }
    }
    else
    {
        // Попытка удаления Обработчика несуществующего Сервиса
        LogError(rc, fctName, 
            "try to remove worker for non-existense service");
    }

    status = true;
    break;
  }

  if (false == status)
    mco_trans_rollback(t);
  else
    mco_trans_commit(t);

  return status;
}

// Активировать Обработчик wrk, находящийся в спуле своего Сервиса
bool XDBDatabaseBrokerImpl::PushWorker(Worker *wrk)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::PushWorker";
  bool status = false;
  Service      *srv = NULL;
  MCO_RET       rc;
  autoid_t      aid;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;
  uint2         worker_idx = -1;
  uint2         workers_spool_size;
  timer_mark_t  now_time, next_heartbeat_time;
  xdb_broker::timer_mark xdb_next_heartbeat_time;

  assert(wrk);
  const char* wrk_identity = wrk->GetIDENTITY();

    /*
     * Попытаться найти подобного Обработчика в спула Сервиса
     *
     *   Если подобный не найден:
     *      => создать
     *      => наполнить данными
     *      => найти свободное место в спуле
     *          Место найдено:
     *          => поместить в спул
     *          Место не найдено:
     *          => сигнализировать об ошибке
     *
     *   Если подобный найден:
     *      => активировать его
     *
     */
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    if ((srv = GetServiceById(wrk->GetSERVICE_ID())))
    {
      // Удалось найти Сервис, к которому привязан Обработчик,
      rc = xdb_broker::XDBService::SK_ident::find(t,
                wrk_identity,
                strlen(wrk_identity),
                /*OUT*/ service_instance,
                /*OUT*/ worker_idx);

      if (MCO_S_NOTFOUND == rc)
      {
        // TODO 
        // 1. Проверить наличие Сервиса, который указан в 
        // качестве родительского
        // 2. Проверить корректность номера Обработчика в спуле
        if (true == service_instance.is_valid())
        {
          if (-1 == worker_idx)
          {
            // индекс не корректен
          }
        }
#pragma warning "TODO"
        // Если (1) и (!2) - данный Обработчик регистрируется впервые
        //  => Найти в спуле для него свободное место
        // Если (1) и (2) - данный Обработчик уже зарегистрирован, активировать его
        // Иначе - Обработчик содержит некорректные данные
        worker_idx = 1; /* GEV: как пример */
      }
      else
      {
        // Обработчик уже есть в спуле, его индекс найден worker_idx
        if (rc) { LogError(rc, fctName, "locating in worker's spool failure"); break; }
      }

      rc = service_instance.SK_workers_size(workers_spool_size);
      if (rc) { LogError(rc, fctName, "get workers spool size failure"); break; }
      if (!workers_spool_size)
      {
        // требуется первоначальное размещение пула заданного размера
        rc = service_instance.SK_workers_alloc(WAITING_WORKERS_SPOOL_MAXLEN);
        if (rc) { LogError(rc, fctName, "set waiting workers quantity failure"); break; }
        workers_spool_size = WAITING_WORKERS_SPOOL_MAXLEN;
      }

      // номер искомого Обработчика в спуле есть worker_idx
      rc = service_instance.SK_workers_at(worker_idx, worker_instance);
      if (rc) { LogError(rc, fctName, "get worker from spool failure"); break; }
      // NB: пока не будем удалять, пометим как "неактивен"
      // rc = service_instance.SK_workers_erase(worker_idx);

      // удалить у Сервиса упоминания о данном Обработчике
      rc = worker_instance.state_put(ARMED);
      if (rc) { LogError(rc, fctName, "worker arming failure"); break; }

      /* Установить новое значение expiration */
      if (GetTimerValue(now_time))
      {
        next_heartbeat_time.tv_nsec = now_time.tv_nsec;
        next_heartbeat_time.tv_sec = now_time.tv_sec + WORKER_HEARTBEAT_PERIOD_VALUE;
      }
      else { LogError(rc, fctName, "expiration time calculation failure"); break; }

      rc = worker_instance.expiration_write(xdb_next_heartbeat_time);
      if (rc) { LogError(rc, fctName, "worker's set expiration time failure"); break; }
      rc = xdb_next_heartbeat_time.sec_put(next_heartbeat_time.tv_sec);
      if (rc) { LogError(rc, fctName, "expiration time set seconds failure"); break; }
      rc = xdb_next_heartbeat_time.nsec_put(next_heartbeat_time.tv_nsec);
      if (rc) { LogError(rc, fctName, "expiration time set nanoseconds failure"); break; }
      rc = worker_instance.expiration_write(xdb_next_heartbeat_time);
      if (rc) { LogError(rc, fctName, "worker set expiration failure"); break; }

      status = true;
    }
    else
    { // Сервис-владелец не был найден
      LogError(rc, fctName, "workers's owner not found");
      status = false;
    }

    break;
  }

  if (false == status)
    mco_trans_rollback(t);
  else
    mco_trans_commit(t);

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
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;
  Service *service = NULL;
  /* NB: 32 - это размер поля service_name_t из broker.mco */
  char     name_from_db[SERVICE_NAME_MAXLEN+1];
  uint2    name_from_db_length = sizeof(name_from_db)-1;
  uint2    workers_spool_size = 0;
  int      idx, actual_workers = 0;
  autoid_t      Id;
  WorkerState   worker_state;

  rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::XDBService::PK_name::find(t,
            name,
            strlen(name),
            service_instance);

    /* Запись не найдена - нет ошибки */
    if (MCO_S_NOTFOUND == rc) break;

    /* Запись не найдена - есть ошибка - сообщить */
    if (rc) { LogError(rc, fctName, "locating service failure"); break; }

    /* Запись найдена - сконструировать объект на основе данных из БД */
    rc = service_instance.name_get(name_from_db, name_from_db_length);
    if (rc) { LogError(rc, fctName, "get service name failure"); break; }
    rc = service_instance.autoid_get(Id);
    if (rc) { LogError(rc, fctName, "get service id failure"); break; }
    rc = service_instance.SK_workers_size(workers_spool_size);
    if (rc) { LogError(rc, fctName, "get workers spool size failure"); break; }
    for (idx = 0, actual_workers = 0; idx < workers_spool_size; idx++)
    {
      rc = service_instance.SK_workers_at(idx, worker_instance);
      if (rc) { LogError(rc, fctName, "read worker from spool failure"); break; }
      rc = worker_instance.state_get(worker_state);
      if (!rc && (worker_state == ARMED))
        actual_workers++;
    }

    service = new Service(Id, name_from_db);
    break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    mco_trans_commit(t);

  return service;
}

Worker *XDBDatabaseBrokerImpl::LoadWorker(mco_trans_h t,
        autoid_t &srv_aid,
        xdb_broker::XDBWorker& wrk_instance)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::LoadWorker";
  Worker       *worker = NULL;
  MCO_RET       rc = MCO_S_OK;
  char          ident[WORKER_IDENTITY_MAXLEN+1];
  xdb_broker::timer_mark  xdb_expire_time;
  timer_mark_t  expire_time = {0, 0};
  uint4         timer_value;
  WorkerState   state;

  while (rc == MCO_S_OK)
  {
    rc = wrk_instance.identity_get(ident, (uint2)WORKER_IDENTITY_MAXLEN);
    ident[WORKER_IDENTITY_MAXLEN] = '\0';
    if (rc) { LogError(rc, fctName, "get worker's identity failure"); break; }
    rc = wrk_instance.expiration_read(xdb_expire_time);
    if (rc) { LogError(rc, fctName, "get worker's expiration mark failure"); break; }
    rc = wrk_instance.state_get(state);
    if (rc) { LogError(rc, fctName, "get worker's service id failure"); break; }
    rc = xdb_expire_time.sec_get(timer_value); expire_time.tv_sec = timer_value;
    rc = xdb_expire_time.nsec_get(timer_value); expire_time.tv_nsec = timer_value;

    worker = new Worker(ident, srv_aid);
    worker->SetSTATE((Worker::State)state);
    worker->SetEXPIRATION(expire_time);
    break;
  }

  return worker;
}

/* 
 * Вернуть ближайший свободный Обработчик.
 * Побочные эффекты: нет
 */
Worker *XDBDatabaseBrokerImpl::GetWorker(const Service *srv)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::GetWorker";
  MCO_RET       rc;
  bool          status = false;
  mco_trans_h   t;
  mco_cursor_h  csr;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker worker_instance;
  Worker       *worker = NULL;
  WorkerState   worker_state;
  autoid_t      aid;
  timer_mark_t  now_time;
  uint2         workers_spool_size;
  int           idx;

  assert(srv);
  rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    aid = const_cast<Service*>(srv)->GetID();
    rc = xdb_broker::XDBService::autoid::find(t, aid, service_instance);
    if (rc) { LogError(rc, fctName, "get service's instance failure"); break; }
//    GetTimerValue(now_time);

    rc = service_instance.SK_workers_size(workers_spool_size);
    if (rc) { LogError(rc, fctName, "get workers spool size failure"); break; }

    for (idx = 0; idx < workers_spool_size; idx++)
    {
      rc = service_instance.SK_workers_at(idx, worker_instance);
      if (rc) { LogError(rc, fctName, "read worker from spool failure"); break; }
      rc = worker_instance.state_get(worker_state);
      if (!rc && (worker_state == ARMED))
      {
        worker = LoadWorker(t, aid, worker_instance);
        break;
      }
      if (rc) { LogError(rc, fctName, "service's get waiting worker failure"); break; }
    }
    status = true;
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
 * Выбранный экземпляр в базе данных помечается занятым
 */
Worker *XDBDatabaseBrokerImpl::PopWorker(const char *name)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::PopWorker";
  MCO_RET       rc;
  bool          status = false;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker worker_instance;
  uint2         workers_spool_size;
  Worker *worker = NULL;
  WorkerState    worker_state;
  autoid_t       aid;
  int            idx;

  assert(name);
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    if (rc) { LogError(rc, fctName, "transaction starting failure"); break; }

    rc = xdb_broker::XDBService::PK_name::find(t, name, strlen(name), service_instance);
    if (rc) { LogError(rc, fctName, "service locating by name failure"); break; }
    rc = service_instance.autoid_get(aid);
    if (rc) { LogError(rc, fctName, "get service's id failure"); break; }

    rc = service_instance.SK_workers_size(workers_spool_size);
    if (rc) { LogError(rc, fctName, "get workers spool size failure"); break; }

    for (idx = 0; idx < workers_spool_size; idx++)
    {
      rc = service_instance.SK_workers_at(idx, worker_instance);
      if (rc) { LogError(rc, fctName, "read worker from spool failure"); break; }
      rc = worker_instance.state_get(worker_state);
      if (!rc && (worker_state == ARMED))
      {
        /* TODO сменить состояние Обработчика на "ЗАНЯТ" */
        rc = worker_instance.state_put(IN_PROCESS);
        if (rc) { LogError(rc, fctName, "change worker state failure"); break; }
        break;
      }
      if (rc) { LogError(rc, fctName, "service's get waiting worker failure"); break; }
    }

    status = true;
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
 * Выбранный экземпляр в базе данных удаляется из списка ожидающих.
 * Вызывающая сторона должна удалить полученный экземпляр Worker
 */
Worker *XDBDatabaseBrokerImpl::PopWorker(
        Service *service)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::PopWorker";
  MCO_RET       rc;
  mco_trans_h   t;
  xdb_broker::XDBService    service_instance;
  xdb_broker::XDBWorker     worker_instance;
  WorkerState   worker_state;
  uint2         workers_waiting_count = 0;
  autoid_t      aid = 0;
  Worker       *worker = NULL;
  uint2         awaiting_worker_idx = 0;
  char          identity[WORKER_IDENTITY_MAXLEN+1];

  assert(service);
  rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    rc = xdb_broker::XDBService::autoid::find(t, aid, service_instance);
    if (rc) { LogError(rc, fctName, "locating service failure"); break; }

    /* Есть ли Обработчики для данного Сервиса? */
    rc = service_instance.SK_workers_size(workers_waiting_count);
    if (rc) { LogError(rc, fctName, 
            "get service's waiting workers queue size failure"); break; }
    
    if (!workers_waiting_count)
    {
       /* Спул Обработчиков пуст - вернуться */
       LogError(rc, fctName, "waiting workers spool is empty");
       break;
    }

    if (-1 == (awaiting_worker_idx = LocatingFirstOccurence(service_instance, ARMED)))
    {
       /* Нет свободных Обработчиков - вернуться */
       LogError(rc, fctName, "no one waiting workers in spool");
       break;
    }

    rc = service_instance.SK_workers_at(awaiting_worker_idx, worker_instance);
    if (rc) { LogError(rc, fctName, 
            "get service's first awaiting worker failure"); break; }
    worker = LoadWorker(t, aid, worker_instance);
    break;
  }

  if (!worker)
  {
    LogError(rc, fctName, NULL);
    mco_trans_rollback(t);
  }
  else
    mco_trans_commit(t);

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
  rc = xdb_broker::XDBService::PK_name::find(t, name, strlen(name), instance);
  mco_trans_commit(t);

  return (MCO_S_NOTFOUND == rc)? false:true;
}

/*
 * Поиск в спуле данного Сервиса индекса Обработчика,
 * находящегося в состоянии state
 */
int XDBDatabaseBrokerImpl::LocatingFirstOccurence(
    xdb_broker::XDBService &service_instance,
    WorkerState            searched_worker_state)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::LocatingFirstOccurence";
  MCO_RET      rc;
  xdb_broker::XDBWorker     worker_instance;
  WorkerState  worker_state;
  bool         awaiting_worker_found = false;
  uint2        workers_waiting_count = 0;
  autoid_t     aid = 0;
  Worker      *worker = NULL;
  int          awaiting_worker_idx = -1;

  while (MCO_S_OK == rc)
  {
    /* Есть ли в Обработчики для данного Сервиса? */
    rc = service_instance.SK_workers_size(workers_waiting_count);
    if (rc) { LogError(rc, fctName, 
            "get service's waiting workers queue size failure"); break; }
    
    /* 
     * Поскольку вектор элементов есть структура с постоянным количеством элементов,
     * требуется в цикле проверить все его элементы. Первое ненулевое значение autoid
     * и будет являться искомым идентификатором Обработчика
     */
    for (awaiting_worker_idx = 0, aid = 0;
         awaiting_worker_idx < workers_waiting_count;
         awaiting_worker_idx++)
    {
      rc = service_instance.SK_workers_at(awaiting_worker_idx, worker_instance);
      if (rc)
      {
        // выйти из поиска первого ожидающего зарегистрированного Обработчика
        awaiting_worker_found = false;
        if (rc) { LogError(rc, fctName, "get service's awaiting worker failure"); break; }
      }
      else // Найден Обработчик, проверить его состояние
      {
        rc = worker_instance.state_get(worker_state);
        if (rc) { LogError(rc, fctName, "get awaiting worker's state failure"); break; }

        // Проверить статус Обработчика - должен быть равен заданному
        if (worker_state == searched_worker_state)
        {
          awaiting_worker_found = true;
          break;
        }
      }
    }

    if (false == awaiting_worker_found)
    {
        /* нет ни одного Обработчика - у всех другое состояние */
        LogError(rc, fctName, "no one waiting workers find");
        break;
    }

    break;
  }

  return awaiting_worker_idx;
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
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;
  Worker  *worker = NULL;
#if 0
  /* NB: WORKER_IDENTITY_MAXLEN - это размер поля identity_t из broker.mco */
  char     identity_from_db[WORKER_IDENTITY_MAXLEN+1];
  uint2    identity_from_db_length = sizeof(identity_from_db)-1;
#endif
  autoid_t self_id;
  xdb_broker::timer_mark xdb_expiration;
  uint4 timer_value;
  timer_mark_t expiration;
  uint2        worker_idx;
  autoid_t     service_aid;
  WorkerState  state;

  rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::XDBService::SK_ident::find(t,
                ident,
                strlen(ident),
                service_instance,
                worker_idx);

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
    printf("XDBDatabaseBrokerImpl::GetWorkerByIdent('%s' %d)\n",
           ident, strlen(ident));
#endif
    rc = service_instance.autoid_get(service_aid);
    if (rc) { LogError(rc, fctName, "worker's get service id failure"); break; }
    rc = service_instance.SK_workers_at(worker_idx, worker_instance);
    if (rc) { LogError(rc, fctName, "get worker's instance failure"); break; }

    rc = worker_instance.expiration_read(xdb_expiration);
    if (rc) { LogError(rc, fctName, "worker's get expiration failure"); break; }
    rc = worker_instance.state_get(state);
    if (rc) { LogError(rc, fctName, "worker's state read failure"); break; }

    worker = new Worker(ident, service_aid);
    worker->SetSTATE((Worker::State)state);
    xdb_expiration.sec_get(timer_value); expiration.tv_sec = timer_value;
    xdb_expiration.nsec_get(timer_value); expiration.tv_nsec = timer_value;
    worker->SetEXPIRATION(expiration);
    status = true;
    break;
  }

  if (true == status)
    mco_trans_rollback(t);
  else
    mco_trans_commit(t);

  return worker;
}

Service::State XDBDatabaseBrokerImpl::GetServiceState(const Service *srv)
{
  assert (srv);
  Service::State state = Service::DISABLED;

  // TODO реализация
  // Проверить состояние самого Сервиса и ее подчиненных Обработчиков
  return state;
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
