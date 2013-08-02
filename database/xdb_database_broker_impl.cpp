#include <assert.h>
#include <stdio.h>
#include <stdlib.h> // free
#include <stdarg.h>
#include <string.h>

/* TODO: delete stdio after removing printf() */
#include <stdio.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"
#include "xdb_database_common.h"

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

#include "helper.hpp"
#include "dat/xdb_broker.hpp"
#include "xdb_database_broker.hpp"
#include "xdb_database_broker_impl.hpp"
#include "xdb_database_service.hpp"
#include "xdb_database_worker.hpp"

const unsigned int DATABASE_SIZE = 1024 * 1024 * 1;  // 1M
const unsigned int MEMORY_PAGE_SIZE = DATABASE_SIZE; // Вся БД поместится в одной странице ОЗУ 
const unsigned int MAP_ADDRESS = 0x20000000;

#ifndef MCO_PLATFORM_X64
    static const unsigned int PAGESIZE = 128;
#else 
    static const int unsigned PAGESIZE = 256;
#endif 

#ifdef DISK_DATABASE
const unsigned int DB_DISK_CACHE = (1 * 1024 * 1024);
const unsigned int DB_DISK_PAGE_SIZE = 1024;

    #ifndef DB_LOG_TYPE
        #define DB_LOG_TYPE REDO_LOG
    #endif 
#else 
const unsigned int DB_DISK_CACHE = 0;
const unsigned int DB_DISK_PAGE_SIZE = 0;
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
    XDBDatabaseBroker *self) : m_initialized(false), m_metadict_initialized(false)
{
  assert(self);

  m_self = self;

#ifdef DISK_DATABASE
  const char* name = m_self->DatabaseName();

  m_dbsFileName = new char[strlen(name) + 5];
  m_logFileName = new char[strlen(name) + 5];

  strcpy(m_dbsFileName, name);
  strcat(m_dbsFileName, ".dbs");

  strcpy(m_logFileName, name);
  strcat(m_logFileName, ".log");
#endif
  ((XDBDatabase*)m_self)->TransitionToState(XDBDatabase::UNINITIALIZED);
/*  fprintf(stdout, "\tXDBDatabaseBrokerImpl(%p, %s)\n", (void*)self, name);
  fflush(stdout);*/
}

XDBDatabaseBrokerImpl::~XDBDatabaseBrokerImpl()
{
  const char *fctName = "~XDBDatabaseBrokerImpl";
  MCO_RET rc;
#if EXTREMEDB_VERSION >= 41
  int ret;
#endif
  XDBDatabase::DBState state = ((XDBDatabase*)m_self)->State();

  LogInfo(fctName, "Current state %d", (int)state);
  switch (state)
  {
    case XDBDatabase::DELETED:
      LogWarn(fctName, "State already DELETED (%d)", XDBDatabase::DELETED);
    break;

    case XDBDatabase::UNINITIALIZED:
      //fprintf(stdout, "%s: state UNINITIALIZED\n", fctName);
    break;

    case XDBDatabase::CONNECTED:
      Disconnect();
      // NB: break пропущен специально!
    case XDBDatabase::DISCONNECTED:
#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
      if (m_metadict_initialized == true)
      {
        ret = mcohv_stop(m_hv);
        LogInfo(fctName, "Stopping http server [code=%d]", ret);

        ret = mcohv_shutdown();
        LogInfo(fctName, "Shutdowning http server [code=%d]", ret);
        free(m_metadict);
        m_metadict_initialized = false;
      }
#endif
      rc = mco_runtime_stop();
      rc_check("Runtime stop", rc);
      ((XDBDatabase*)m_self)->TransitionToState(XDBDatabase::DELETED);
    break;
  }

  mco_db_kill(m_self->DatabaseName());

#ifdef DISK_DATABASE
  delete []m_dbsFileName;
  delete []m_logFileName;
#endif

  fflush(stdout);
}

/* NB: Сначала Инициализация (Init), потом Подключение (Connect) */
bool XDBDatabaseBrokerImpl::Init()
{
    const char *fctName = "XDBDatabaseBrokerImpl::Init";
    bool status = false;
    MCO_RET rc;
    mco_runtime_info_t info;

    mco_get_runtime_info(&info);
#if defined DEBUG
    if (!info.mco_save_load_supported)
    {
      LogWarn(fctName, "XML import/export doesn't supported by runtime");
      m_save_to_xml_feature = false;
    }
    else
    {
      m_save_to_xml_feature = true;
    }
    show_runtime_info("");
#endif
    if (!info.mco_shm_supported)
    {
      LogWarn(fctName, "This program requires shared memory database runtime");
      return false;
    }

    /* Set the error handler to be called from the eXtremeDB runtime if a fatal error occurs */
//    mco_error_set_handler(&errhandler);
    mco_error_set_handler_ex(&extended_errhandler);

//    LogInfo(fctName, "User-defined error handler set");
#if 0
    autoid_t aid = 1;
    int64_t  iid = 2;
    unsigned char *p_aid = (unsigned char*)&aid;
    unsigned char *p_iid = (unsigned char*)&iid;
    int i;
    LogInfo(fctName, "autoid_t [%lld size=%d], int64_t [%lld size=%d]\n", 
        aid, sizeof(autoid_t), iid, sizeof(int64_t));
    for (i=0; i<sizeof(autoid_t); i++)
    {
      printf("aid[%d]: %02X\n", i, p_aid[i]);
    }
    puts("");
    
    for (i=0; i<sizeof(int64_t); i++)
    {
      printf("iid[%d]: %02X\n", i, p_iid[i]);
    }

    iid = 3;
    aid = iid;
    puts("");

    for (i=0; i<sizeof(autoid_t); i++)
    {
      printf("aid[%d]: %02X\n", i, p_aid[i]);
    }

#endif

    rc = mco_runtime_start();
    rc_check("Runtime starting", rc);
    if (!rc)
    {
      status = ((XDBDatabase*)m_self)->TransitionToState(XDBDatabase::DISCONNECTED);
    }

#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
    /* initialize MCOHV */
    mcohv_initialize();

    mco_metadict_size(1, &m_size);
    m_metadict = (mco_metadict_header_t *) malloc(m_size);
    rc = mco_metadict_init (m_metadict, m_size, 0);
    if (rc)
    {
      LogError(rc, fctName, "Unable to initialize UDA metadictionary");
      free(m_metadict);
      m_metadict_initialized = false;
    }
    else
    {
      m_metadict_initialized = true;
      rc = mco_metadict_register(m_metadict,
            ((XDBDatabase*)m_self)->DatabaseName(),
            xdb_broker_get_dictionary(), NULL);
      LogInfo(fctName, "mco_metadict_register=%d", rc);
    }
#endif

    return status;
}


/* NB: Сначала Инициализация (Init), потом Подключение (Connect) */
bool XDBDatabaseBrokerImpl::Connect()
{
  const char *fctName = "XDBDatabaseBrokerImpl::Connect";
  bool status = Init();

  switch (((XDBDatabase*)m_self)->State())
  {
    case XDBDatabase::UNINITIALIZED:
      LogWarn(fctName, "Try to connect to uninitialized database '%s'",
        ((XDBDatabase*)m_self)->DatabaseName());
    break;

    case XDBDatabase::CONNECTED:
      LogWarn(fctName, "Try to database '%s' reopen",
        ((XDBDatabase*)m_self)->DatabaseName());
    break;

    case XDBDatabase::DISCONNECTED:
        status = AttachToInstance();
    break;

    default:
      LogWarn(fctName, "Try to open database '%s' with unknown state %d",
        ((XDBDatabase*)m_self)->DatabaseName(),
        (int)((XDBDatabase*)m_self)->State());
    break;
  }

  return status;
}

bool XDBDatabaseBrokerImpl::Disconnect()
{
  const char *fctName = "XDBDatabaseBrokerImpl::Disconnect";
  MCO_RET rc = MCO_S_OK;
  XDBDatabase::DBState state = ((XDBDatabase*)m_self)->State();

  switch (state)
  {
    case XDBDatabase::UNINITIALIZED:
      LogInfo(fctName, "Disconnect from uninitialized state");
    break;

    case XDBDatabase::DISCONNECTED:
      LogInfo(fctName, "Try to disconnect already diconnected database");
    break;

    case XDBDatabase::CONNECTED:
      assert(m_self);
      rc = mco_db_disconnect(m_db);
      rc_check("Disconnection", rc);

      rc = mco_db_close(m_self->DatabaseName());
      ((XDBDatabase*)m_self)->TransitionToState(XDBDatabase::DISCONNECTED);
      rc_check("Closing", rc);
    break;

    default:
      LogInfo(fctName, "Disconnect from unknown state: %d\n", state);
  }

  return (!rc)? true : false;
}


bool XDBDatabaseBrokerImpl::AttachToInstance()
{
  const char *fctName = "XDBDatabaseBrokerImpl::AttachToInstance";
  MCO_RET rc = MCO_S_OK;

#if EXTREMEDB_VERSION >= 40
  /* setup memory device as a shared named memory region */
  m_dev.assignment = MCO_MEMORY_ASSIGN_DATABASE;
  m_dev.size       = DATABASE_SIZE;
  m_dev.type       = MCO_MEMORY_NAMED; /* DB in shared memory */
  sprintf(m_dev.dev.named.name, "%s-db", ((XDBDatabase*)m_self)->DatabaseName());
  m_dev.dev.named.flags = 0;
  m_dev.dev.named.hint  = 0;

  mco_db_params_init (&m_db_params);
  m_db_params.db_max_connections = 10;
  m_db_params.mem_page_size      = MEMORY_PAGE_SIZE; /* set page size for in memory part */
  m_db_params.disk_page_size     = DB_DISK_PAGE_SIZE;/* set page size for persistent storage */
#endif /* EXTREMEDB_VERSION >= 40 */

  /*
   * NB: предварительное удаление экземпляра БД делает бесполезной 
   * последующую попытку подключения к ней.
   * Очистка оставлена в качестве временной меры. В дальнейшем 
   * уже созданный экземпляр БД может использоваться в качестве 
   * persistent-хранилища после аварийного завершения брокера.
   */
  mco_db_kill(((XDBDatabase*)m_self)->DatabaseName());

  /* подключиться к базе данных, предполагая что она создана */
  LogInfo(fctName, "Attaching to instance '%s'",
          ((XDBDatabase*)m_self)->DatabaseName());
  rc = mco_db_connect(((XDBDatabase*)m_self)->DatabaseName(), &m_db);

  /* ошибка - экземпляр базы не найден, попробуем создать её */
  if (MCO_E_NOINSTANCE == rc)
  {
        LogInfo(fctName, "'%s' instance not found, create",
            ((XDBDatabase*)m_self)->DatabaseName());
        /*
         * TODO: Использование mco_db_open() является запрещенным,
         * начиная с версии 4 и старше
         */
#if EXTREMEDB_VERSION >= 40
        rc = mco_db_open_dev(((XDBDatabase*)m_self)->DatabaseName(),
                       xdb_broker_get_dictionary(),
                       &m_dev,
                       1,
                       &m_db_params);
#else
        rc = mco_db_open(((XDBDatabase*)m_self)->DatabaseName(),
                         xdb_broker_get_dictionary(),
                         (void*)MAP_ADDRESS,
                         DATABASE_SIZE + DB_DISK_CACHE,
                         PAGESIZE);
#endif
        if (rc)
        {
          LogError(rc, fctName, "Can't open DB dictionary '%s'",
                ((XDBDatabase*)m_self)->DatabaseName());
          return false;
        }

#ifdef DISK_DATABASE
        LogInfo(fctName, "Disk database '%s' opening", m_dbsFileName);
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
            LogError(rc, fctName, "Error creating disk database");
            return false;
        }
#endif

        /* подключиться к базе данных, т.к. она только что создана */
        LogInfo(fctName, "Connecting to instance '%s'\n", 
            ((XDBDatabase*)m_self)->DatabaseName());
        rc = mco_db_connect(((XDBDatabase*)m_self)->DatabaseName(), &m_db);
  }

  /* ошибка создания экземпляра - выход из системы */
  if (rc)
  {
        LogError(rc, fctName, "Could not attach to instance '%s'",
                ((XDBDatabase*)m_self)->DatabaseName());
        return false;
  }

  rc_check("Connecting", rc);
#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
    mcohv_start(&m_hv, m_metadict, 0, 0);
#endif

  return ((XDBDatabase*)m_self)->TransitionToState(XDBDatabase::CONNECTED);
}

Service *XDBDatabaseBrokerImpl::AddService(const char *name)
{
  const char    *fctName = "XDBDatabaseBrokerImpl::AddService";
  xdb_broker::XDBService instance;
  Service       *srv = NULL;
  MCO_RET        rc;
  mco_trans_h    t;
  autoid_t       aid;
  ServiceState   state;

  assert(name);
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (MCO_S_OK == rc)
  {
    if (rc) { LogError(rc, fctName, "Starting transaction"); break; }

    rc = instance.create(t);
    if (rc) { LogError(rc, fctName, "Creating instance"); break; }

    rc = instance.name_put(name, strlen(name));
    if (rc) { LogError(rc, fctName, "Setting name '%s'", name); break; }

    rc = instance.workers_alloc(Service::WORKERS_SPOOL_MAXLEN);
    if (rc) { LogError(rc, fctName, "Allocating pool of workers"); break; }

    rc = instance.checkpoint();
    if (rc) { LogError(rc, fctName, "Checkpointing"); break; }

    rc = instance.state_get(state);
    if (rc) { LogError(rc, fctName, "Getting service's state"); break; }
    rc = instance.autoid_get(aid);
    if (rc) { LogError(rc, fctName, "Getting service's id"); break; }

    srv = new Service(aid, name);
    srv->SetSTATE((Service::State)state);

    if (rc) { LogError(rc, fctName, "Commitment transaction"); break; }
    break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

  return srv;
}

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
        LogError(rc, fctName, "removed service '%s' doesn't exists", name); break;
    }
    if (rc) { LogError(rc, fctName, "service '%s' searching failure", name); break; }
    
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
  MCO_RET       rc;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;
  uint2         worker_index;

  assert(wrk);
  const char* identity = wrk->GetIDENTITY();

  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
      // Удалось найти Сервис, к которому привязан Обработчик
      rc = xdb_broker::XDBService::SK_ident::find(t,
            identity,
            strlen(identity),
            service_instance,
            worker_index);

      if (MCO_S_NOTFOUND == rc)
      {
        // Попытка удаления Обработчика несуществующего Сервиса
        LogError(rc, fctName, 
            "Try to disable worker '%s' for non-existense service id=%d",
            identity,  wrk->GetSERVICE_ID());
        break;
      }
      if (rc) { LogError(rc, fctName, "Locating worker '%s'", identity); break; }

      // номер искомого Обработчика в спуле есть worker_index
//      rc = service_instance.workers_at(worker_index, worker_instance);
      /* Due McObject's issue #1182:
       * _at returns read-only instance, but _put returns read-write instance */
      rc = service_instance.workers_put(worker_index, worker_instance);
      if (rc) 
      { 
        LogError(rc, fctName, "Getting worker '%s' at pool's index=%d",
            identity, worker_index); break; 
      }

      // Не удалять, пометить как "неактивен"
      rc = worker_instance.state_put(DISARMED);
      //rc = service_instance.workers_erase(worker_index);
      if (rc) 
      { 
        LogError(rc, fctName, "Disarming worker '%s'", identity); break;
      }

      status = true;
      break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

  return status;
}

// Активировать Обработчик wrk, находящийся в спуле своего Сервиса
bool XDBDatabaseBrokerImpl::PushWorker(Worker *wrk)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::PushWorker";
  bool status = false;
  Service      *srv = NULL;
  MCO_RET       rc;
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
      // Получить service_instance, к которому привязан Обработчик
      rc = xdb_broker::XDBService::autoid::find(t, 
                            srv->GetID(), 
                            service_instance);
      // найти его номер в спуле
#if 1
      // Найти Обработчика по его identity с помощью курсора
      rc = LoadWorkerByIdent(t, srv, wrk);
      worker_idx = wrk->GetINDEX();
#else
      rc = xdb_broker::XDBService::SK_ident::find(t,
                wrk_identity,
                strlen(wrk_identity),
                /*OUT*/ service_instance,
                /*OUT*/ worker_idx);
#endif

      // Если Обработчик не найден, то:
      // 1. service_instance невалиден
      // 2. Обработчик регистрируется в первый раз
      if (MCO_S_NOTFOUND == rc || MCO_S_CURSOR_EMPTY == rc)
      {
#if 0      
        // 
        // NB: Сервис есть (srv), но у него нет указанного Обработчика.
        // Может быть это первая регистрация Обработчика данного Сервиса.
        rc = xdb_broker::XDBService::autoid::find(t, 
                            srv->GetID(), 
                            service_instance);
        if (rc) 
        { 
          LogError(rc, fctName, "Locating service by id=%lld", 
                   srv->GetID()); break; 
        }
#endif

        // Индекс не корректен - данный Обработчик регистрируется впервые
        // Найти в спуле для него свободное место
        if ((uint2)-1 == (worker_idx = LocatingFirstOccurence(service_instance, DISARMED)))
        {
            // Не удалось найти свободного места в спуле
            LogError(rc, fctName, "free space in pool of service '%s' [id=%lld] is over", 
                srv->GetNAME(), srv->GetID()); break;
        }
      }
      else
      {
        // Обработчик уже есть в спуле, его индекс найден (worker_idx)
        if (rc) 
        { 
            LogError(rc, fctName, "locating '%s' in worker's spool failure", 
                wrk_identity);
            break; 
        }
      }

      rc = service_instance.workers_size(workers_spool_size);
      if (rc) { LogError(rc, fctName, "get workers spool size failure"); break; }
      if (!workers_spool_size)
      {
        // требуется первоначальное размещение пула заданного размера
        rc = service_instance.workers_alloc(Service::WORKERS_SPOOL_MAXLEN);
        if (rc) { LogError(rc, fctName, "set waiting workers quantity failure"); break; }
        workers_spool_size = Service::WORKERS_SPOOL_MAXLEN;
      }

      // Номер искомого Обработчика в спуле есть worker_idx
//      rc = service_instance.workers_at(worker_idx, worker_instance);
      rc = service_instance.workers_put(worker_idx, worker_instance);
      // Если Обработчика в состоянии DISARMED по данному индексу не окажется,
      // его следует создать.
      if (MCO_E_EMPTYVECTOREL == rc)
      {
        rc = service_instance.workers_put(worker_idx, worker_instance);
        if (rc) { LogError(rc, fctName, "putting new worker into spool"); break; }
        rc = service_instance.checkpoint();
      }
      if (rc) { LogError(rc, fctName, "getting worker from spool"); break; }

//      rc = service_instance.workers_put(worker_idx, worker_instance); /* issue #1182 */
      if (rc) { LogError(rc, fctName, "putting worker into spool"); break; }

      rc = worker_instance.identity_put(wrk_identity, strlen(wrk_identity));
      if (rc) { LogError(rc, fctName, "Put worker's identity '%s'", wrk_identity); break; }

      // Первоначальное состояние Обработчика - "ЗАНЯТ"
      rc = worker_instance.state_put(ARMED);
      if (rc) { LogError(rc, fctName, "worker's inprocess set failure"); break; }

      /* Установить новое значение expiration */
      if (GetTimerValue(now_time))
      {
        next_heartbeat_time.tv_nsec = now_time.tv_nsec;
        next_heartbeat_time.tv_sec = now_time.tv_sec + Worker::HEARTBEAT_PERIOD_VALUE;
      }
      else { LogError(rc, fctName, "expiration time calculation failure"); break; }

      rc = worker_instance.expiration_write(xdb_next_heartbeat_time);
      if (rc) { LogError(rc, fctName, "worker's set expiration time failure"); break; }
      rc = xdb_next_heartbeat_time.sec_put(next_heartbeat_time.tv_sec);
      if (rc) { LogError(rc, fctName, "expiration time set seconds failure"); break; }
      rc = xdb_next_heartbeat_time.nsec_put(next_heartbeat_time.tv_nsec);
      if (rc) { LogError(rc, fctName, "expiration time set nanoseconds failure"); break; }
//      rc = worker_instance.expiration_write(xdb_next_heartbeat_time);
//      if (rc) { LogError(rc, fctName, "worker set expiration failure"); break; }

      status = true;
    }
    else
    { // Сервис-владелец не был найден
      LogError(rc, fctName, "workers's owner not found");
      status = false;
    }

    break;
  }

  // Удалим Сервис, переданный сюда из GetServiceById
  delete srv;

  if (rc || false == status)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

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
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
//  xdb_broker::XDBWorker  worker_instance;
  Service *service = NULL;
  /* NB: 32 - это размер поля service_name_t из broker.mco */
//  uint2    workers_spool_size = 0;
//  int      idx, actual_workers = 0;
  autoid_t      Id;
//  WorkerState   worker_state;

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
    rc = service_instance.autoid_get(Id);
    if (rc) { LogError(rc, fctName, "get service's id failure"); break; }

#if 0 
NB: В настоящее время объект Сервис не содержит в себе количества своих Обработчиков
    rc = service_instance.workers_size(workers_spool_size);
    if (rc) { LogError(rc, fctName, "get workers spool size failure"); break; }
    for (idx = 0, actual_workers = 0; idx < workers_spool_size; idx++)
    {
      rc = service_instance.workers_at(idx, worker_instance);
      if (rc) { LogError(rc, fctName, "read worker from spool failure"); break; }
      rc = worker_instance.state_get(worker_state);
      if (!rc && (worker_state == ARMED))
        actual_workers++;
    }
#endif

    service = LoadService(t, Id, service_instance);
    break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

  return service;
}

Service *XDBDatabaseBrokerImpl::LoadService(mco_trans_h t,
        autoid_t &aid,
        xdb_broker::XDBService& instance)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::LoadService";
  Service      *service = NULL;
  MCO_RET       rc = MCO_S_OK;
  char          name[Service::NAME_MAXLEN + 1];
  xdb_broker::timer_mark  xdb_expire_time;
  timer_mark_t  expire_time = {0, 0};
  uint4         timer_value;
  ServiceState  state;

  while (rc == MCO_S_OK)
  {
    rc = instance.name_get(name, (uint2)Service::NAME_MAXLEN);
    name[Service::NAME_MAXLEN] = '\0';
    if (rc) { LogError(rc, fctName, "get service's name failure"); break; }
/*    rc = instance.expiration_read(xdb_expire_time);
    if (rc) { LogError(rc, fctName, "get worker's expiration mark failure"); break; }*/
    rc = instance.state_get(state);
    if (rc) { LogError(rc, fctName, "get worker's service id failure"); break; }
/*    rc = xdb_expire_time.sec_get(timer_value); expire_time.tv_sec = timer_value;
    rc = xdb_expire_time.nsec_get(timer_value); expire_time.tv_nsec = timer_value;*/

    service = new Service(aid, name);
    service->SetSTATE((Service::State)state);
    /* Состояние объекта полностью соответствует хранимому в БД */
    service->SetVALID();
    break;
  }

  return service;
}

/*
 * Загрузить данные Обработчика в ранее созданный вручную экземпляр
 * 1. Найти родительский Сервис
 * 2. Пройти по спулу Обработчиков в поиске того, чей identity нам задан
 */
MCO_RET XDBDatabaseBrokerImpl::LoadWorkerByIdent(
        /* IN */ mco_trans_h t,
        /* IN */ Service *srv,
        /* INOUT */ Worker *wrk)
{
  const char *fctName = "XDBDatabaseBrokerImpl::LoadWorkerByIdent";
  MCO_RET rc = MCO_S_OK;
  mco_cursor_t csr;
  int idx;

  assert(srv);
  assert(wrk);

  const char *identity = wrk->GetIDENTITY();

  while (!rc)
  {
    rc = xdb_broker::XDBService::SK_ident::cursor(t, &csr);
    if (MCO_S_CURSOR_EMPTY == rc) 
        break; /* В индексе нет ни одной записи */
    if (rc) 
    {
      LogError(rc, fctName, 
            "Unable initialize searching cursor for '%s'", identity);
      break;
    }

    idx = 0;
    while (!rc)
    {
      rc = xdb_broker::XDBService::SK_ident::search(t, &csr, MCO_EQ, identity, strlen(identity));
      if (rc) 
      { 
        LogError(rc, fctName, "Unable searching '%s'", identity); 
        break;
      }

      if (MCO_S_OK == rc)
      {
        //LogInfo(fctName, "Found: '%s'", identity);
        wrk->SetINDEX(idx);
        break;
      }
      idx++;
    }

    if (rc)
    {
        // Это может быть нормальным поведением
        // LogInfo(fctName, "NOT Found: '%s'", identity);
        wrk->SetINDEX(-1);
    }

    break;
  }

  return rc;
}

Worker *XDBDatabaseBrokerImpl::LoadWorker(mco_trans_h t,
        autoid_t &srv_aid,
        xdb_broker::XDBWorker& wrk_instance,
        uint2 index_in_spool)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::LoadWorker";
  Worker       *worker = NULL;
  MCO_RET       rc = MCO_S_OK;
  char          ident[Worker::IDENTITY_MAXLEN + 1];
  xdb_broker::timer_mark  xdb_expire_time;
  timer_mark_t  expire_time = {0, 0};
  uint4         timer_value;
  WorkerState   state;

  while (rc == MCO_S_OK)
  {
    rc = wrk_instance.identity_get(ident, (uint2)Worker::IDENTITY_MAXLEN);
    ident[Worker::IDENTITY_MAXLEN] = '\0';
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
    worker->SetINDEX(index_in_spool);
    /* Состояние объекта полностью соответствует хранимому в БД */
    worker->SetVALID();
    LogInfo(fctName, "New Worker(id='%s' aid=%d state=%d spool_idx=%d)", 
            ident, srv_aid, (int)state, index_in_spool);
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
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker worker_instance;
  Worker       *worker = NULL;
  WorkerState   worker_state;
  autoid_t      aid;
  timer_mark_t  now_time;
  uint2         workers_spool_size;
  uint2         idx;

  assert(srv);
  rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    aid = const_cast<Service*>(srv)->GetID();
    rc = xdb_broker::XDBService::autoid::find(t, aid, service_instance);
    if (rc) { LogError(rc, fctName, "get service's instance failure"); break; }
//    GetTimerValue(now_time);

    rc = service_instance.workers_size(workers_spool_size);
    if (rc) { LogError(rc, fctName, "get workers spool size failure"); break; }

    for (idx = 0; idx < workers_spool_size; idx++)
    {
      rc = service_instance.workers_at(idx, worker_instance);
      if (rc) { LogError(rc, fctName, "read worker from spool failure"); break; }
      rc = worker_instance.state_get(worker_state);
      if (!rc && (worker_state == ARMED))
      {
        worker = LoadWorker(t, aid, worker_instance, idx);
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
    rc = mco_trans_commit(t);

  return worker;
}


/* 
 * Вернуть ближайший Обработчик, находящийся в состоянии ARMED.
 *
 * Побочные эффекты:
 *   Выбранный экземпляр в базе данных не удаляется, а 
 *   помечается занятым (IN_PROCESS)
 */
Worker *XDBDatabaseBrokerImpl::PopWorker(const char *name)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::PopWorker";
  MCO_RET       rc;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker worker_instance;
  uint2         workers_spool_size = 0;
  uint2         awaiting_worker_idx = -1;
  Worker *worker = NULL;
  WorkerState   worker_state;
  autoid_t      aid;
  int           idx;

  assert(name);
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    if (rc) { LogError(rc, fctName, "Starting transaction"); break; }

    rc = xdb_broker::XDBService::PK_name::find(t, name, strlen(name), service_instance);
    if (rc) { LogError(rc, fctName, "locating service by name '%s'", name); break; }
    rc = service_instance.autoid_get(aid);
    if (rc) { LogError(rc, fctName, "get service's id"); break; }

    rc = service_instance.workers_size(workers_spool_size);
    if (rc) { LogError(rc, fctName, "get workers spool size"); break; }
    if (!workers_spool_size)
    {
       /* Спул Обработчиков пуст - вернуться */
       LogWarn(fctName, "waiting workers pool is empty");
       break;
    }

    if ((uint2)-1 == (awaiting_worker_idx = LocatingFirstOccurence(service_instance, ARMED)))
    {
       /* Нет свободных Обработчиков - вернуться */
       LogWarn(fctName, "no one waiting workers in spool");
       break;
    }

//    rc = service_instance.workers_at(awaiting_worker_idx, worker_instance);
    rc = service_instance.workers_put(awaiting_worker_idx, worker_instance);
    if (rc) { LogError(rc, fctName, 
            "get service's first awaiting worker failure"); break; }
//    rc = service_instance.workers_put(awaiting_worker_idx, worker_instance); /* issue #1182 */
    if (rc)
    {
      LogError(rc, fctName,
               "making worker instance at %d as read-write",
               awaiting_worker_idx); 
      break;
    }

    /* Сменить состояние Обработчика на "ЗАНЯТ" */
    rc = worker_instance.state_put(IN_PROCESS);
    if (rc) { LogError(rc, fctName, "changing worker state"); break; }

    worker = LoadWorker(t, aid, worker_instance, awaiting_worker_idx);
    break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

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
  uint2         workers_spool_size = 0;
  autoid_t      aid = 0;
  Worker       *worker = NULL;
  uint2         awaiting_worker_idx = 0;
  char          identity[Worker::IDENTITY_MAXLEN+1];

  assert(service);
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    rc = xdb_broker::XDBService::autoid::find(t, service->GetID(), service_instance);
    if (rc) { LogError(rc, fctName, "locating service id=%lld failure", service->GetID()); break; }

    /* Есть ли Обработчики для данного Сервиса? */
    rc = service_instance.workers_size(workers_spool_size);
    if (rc) { LogError(rc, fctName, 
            "get service's waiting workers queue size failure"); break; }
    
    if (!workers_spool_size)
    {
       /* Спул Обработчиков пуст - вернуться */
       LogWarn(fctName, "waiting workers spool is empty");
       break;
    }

    if ((uint2)-1 == (awaiting_worker_idx = LocatingFirstOccurence(service_instance, ARMED)))
    {
       /* Нет свободных Обработчиков - вернуться */
       LogWarn(fctName, "no one waiting workers in spool");
       break;
    }

//    rc = service_instance.workers_at(awaiting_worker_idx, worker_instance);
    rc = service_instance.workers_put(awaiting_worker_idx, worker_instance);
    if (rc) { LogError(rc, fctName, 
            "get service's first awaiting worker failure"); break; }
//    rc = service_instance.workers_put(awaiting_worker_idx, worker_instance); /* issue #1182 */
    if (rc) { LogError(rc, fctName, "making worker instance at %d as read-write", awaiting_worker_idx); break; }

    aid = service->GetID();

    /* Сменить состояние Обработчика на "ЗАНЯТ" */
    rc = worker_instance.state_put(IN_PROCESS);
    if (rc) { LogError(rc, fctName, "change worker state failure"); break; }

    worker = LoadWorker(t, aid, worker_instance, awaiting_worker_idx);
    break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

  return worker;
}

Service *XDBDatabaseBrokerImpl::GetServiceForWorker(const Worker *wrk)
{
  // TODO: реализация
  return NULL;
}


Service *XDBDatabaseBrokerImpl::GetServiceById(int64_t _id)
{
  autoid_t      aid = _id;
  mco_trans_h   t;
  MCO_RET       rc;
  Service      *service = NULL;
  xdb_broker::XDBService service_instance;
  const char   *fctName = "XDBDatabaseBrokerImpl::GetServiceById";

  rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    rc = xdb_broker::XDBService::autoid::find(t, aid, service_instance);
    if (rc) { LogError(rc, fctName, "locating service failure"); break; }

    service = LoadService(t, aid, service_instance);
    break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

  return service;
}

bool XDBDatabaseBrokerImpl::IsServiceExist(const char *name)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::IsServiceExist";
  MCO_RET       rc;
  mco_trans_h   t;
  xdb_broker::XDBService instance;

  assert(name);
  while (true)
  {
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LogError(rc, fctName, "Starting transaction"); break; }

    rc = xdb_broker::XDBService::PK_name::find(t, name, strlen(name), instance);
    if (rc) { LogError(rc, fctName, "Locating service instance by name '%s'", name); break; }

    break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

  return (MCO_S_NOTFOUND == rc)? false:true;
}

/*
 * Поиск в спуле данного Сервиса индекса Обработчика,
 * находящегося в состоянии state.
 *
 * NB: При поиске Обработчиков в состоянии DISARMED функция может возвращать 
 * индекс Обработчика в спуле, на месте которого еще не создан ни один Обработчик 
 * (ошибка MCO_E_EMPTYVECTOREL). 
 * Следует при дальнейшем взятии Обработчика по вернувшемуся индексу предусмотреть
 * этот случай.
 */
uint2 XDBDatabaseBrokerImpl::LocatingFirstOccurence(
    xdb_broker::XDBService &service_instance,
    WorkerState            searched_worker_state)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::LocatingFirstOccurence";
  MCO_RET      rc = MCO_S_OK;
  xdb_broker::XDBWorker     worker_instance;
  WorkerState  worker_state;
  bool         awaiting_worker_found = false;
  uint2        workers_spool_size = 0;
  Worker      *worker = NULL;
  uint2        awaiting_worker_idx;

  while (MCO_S_OK == rc)
  {
    /* Есть ли в Обработчики для данного Сервиса? */
    rc = service_instance.workers_size(workers_spool_size);
    if (rc) { LogError(rc, fctName, 
            "get service's waiting workers queue size failure"); break; }
    
    /* 
     * Поскольку вектор элементов есть структура с постоянным количеством элементов,
     * требуется в цикле проверить все его элементы. Первое ненулевое значение autoid
     * и будет являться искомым идентификатором Обработчика
     */
    for (awaiting_worker_idx = 0;
         awaiting_worker_idx < workers_spool_size;
         awaiting_worker_idx++)
    {
      rc = service_instance.workers_at(awaiting_worker_idx, worker_instance);
      // прочитали пустой элемент спула
      // TODO это нормально на свежесозданном Сервисе. 
      if (MCO_E_EMPTYVECTOREL == rc)
      {
        if (DISARMED == searched_worker_state)
        {
          // Если ищем DISARMED Обработчик, 
          // то пустой элемент нам тоже подойдет
          awaiting_worker_found = true;
          break;
        }
        // в остальных случаях ищем дальше
        continue;
      }

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
        LogInfo(fctName, "no one waiting workers find");
        break;
    }

    break;
  }

  return (false == awaiting_worker_found)? -1 : awaiting_worker_idx;
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
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;
  Worker  *worker = NULL;
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
    if (rc) { LogError(rc, fctName, "Worker location"); break; }

    /* Запись найдена - сконструировать объект на основе данных из БД */
    rc = service_instance.autoid_get(service_aid);
    if (rc) { LogError(rc, fctName, "Getting worker's service id"); break; }
    rc = service_instance.workers_at(worker_idx, worker_instance);
    if (rc) { LogError(rc, fctName, "Getting worker's instance"); break; }

    worker = LoadWorker(t, service_aid, worker_instance, worker_idx);
    break;
  }

  if (rc)
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

#if defined DEBUG
/* Тестовый API сохранения базы */
void XDBDatabaseBrokerImpl::MakeSnapshot(const char* msg)
{
  static char file_name[50];

  if (false == m_initialized)
  {
    m_snapshot_counter = 0;
    strcpy(m_snapshot_file_prefix, "snap");
    m_initialized = true;
  }

  sprintf(file_name, "%s.%s.%03d",
          m_snapshot_file_prefix,
          (NULL == msg)? "xdb" : msg,
          ++m_snapshot_counter);

  //fprintf(stdout, "Make snapshot into %s file\n", file_name);

  if (true == m_save_to_xml_feature) 
    SaveDbToFile(file_name);
}

#if (EXTREMEDB_VERSION<=40)
mco_size_t file_writer(void* stream_handle, const void* from, mco_size_t nbytes)
#else
mco_size_sig_t file_writer(void* stream_handle, const void* from, mco_size_t nbytes)
#endif
{
    FILE* f = (FILE*)stream_handle;
    int nbs = fwrite(from, 1, nbytes, f);
    return nbs;
} /* ========================================================================= */

MCO_RET XDBDatabaseBrokerImpl::SaveDbToFile(const char* fname)
{
  const char *fctName = "XDBDatabaseBrokerImpl::SaveDbToFile";
  MCO_RET rc = MCO_S_OK;
  mco_xml_policy_t op, np;
  mco_trans_h t;
  FILE* f;

    /* setup policy */
    np.blob_coding = MCO_TEXT_BASE64;
    np.encode_lf = MCO_YES;
    np.encode_nat = MCO_NO; /*MCO_YES ЯаШТХФХв Ъ зШбЫЮТЮЩ ЪЮФШаЮТЪХ агббЪШе СгЪТ */
    np.encode_spec = MCO_YES;
    np.float_format = MCO_FLOAT_FIXED;
    np.ignore_field = MCO_YES;
    np.indent = MCO_YES; /*or MCO_NO*/
    np.int_base = MCO_NUM_DEC;
    np.quad_base = MCO_NUM_HEX; /* other are invalid */
    np.text_coding = MCO_TEXT_ASCII;
    np.truncate_sp = MCO_YES;
    np.use_xml_attrs = MCO_NO; //YES;
    np.ignore_autoid = MCO_NO;
    np.ignore_autooid = MCO_NO;

    LogInfo(fctName, "Database export to %s", fname);
    f = fopen(fname, "wb");

    /* export content of the database to a file */
#ifdef SETUP_POLICY
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_HIGH, &t);
#else
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_HIGH, &t);
#endif

    if (rc == MCO_S_OK)
    {
       /* setup XML subsystem*/
#ifdef SETUP_POLICY
      mco_xml_get_policy(t, &op);
      rc = mco_xml_set_policy(t, &np);
      if (MCO_S_OK != rc)
      {
        LogError(rc, fctName, "Unable to set xml policy");
      }
#endif
      rc = mco_db_xml_export(t, f, file_writer);

      /* revert xml policy */
#ifdef SETUP_POLICY
      mco_xml_set_policy(t, &op);
#endif

      if (rc != MCO_S_OK)
      {
         LogError(rc, fctName, "Exporting");
         mco_trans_rollback(t);
      }
      else
      {
         mco_trans_commit(t);
      }
    }
    else
    {
      LogError(rc, fctName, "Opening transaction");
    }

    fclose(f);

  return rc;
} /* ========================================================================= */

#else
void XDBDatabaseBrokerImpl::MakeSnapshot(const char*)
{
  return;
}
#endif

