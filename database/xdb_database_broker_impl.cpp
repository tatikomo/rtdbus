#include <assert.h>
#include <stdio.h>
#include <glog/logging.h>
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
  MCO_RET rc;
#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
  int ret;
#endif
  XDBDatabase::DBState state = ((XDBDatabase*)m_self)->State();

  LOG(INFO) << "Current state "  << (int)state;
  switch (state)
  {
    case XDBDatabase::DELETED:
      LOG(WARNING) << "State already DELETED";
    break;

    case XDBDatabase::UNINITIALIZED:
    break;

    case XDBDatabase::CONNECTED:
      Disconnect();
      // NB: break пропущен специально!
    case XDBDatabase::DISCONNECTED:
#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
      if (m_metadict_initialized == true)
      {
        ret = mcohv_stop(m_hv);
        LOG(INFO) << "Stopping http server, code=" << ret;

        ret = mcohv_shutdown();
        LOG(INFO) << "Shutdowning http server, code=" << ret;
        free(m_metadict);
        m_metadict_initialized = false;
      }
#endif
      rc = mco_runtime_stop();
      if (rc)
      {
        LOG(ERROR) << "Unable to stop database runtime, code=" << rc;
      }
      //rc_check("Runtime stop", rc);
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
    bool status = false;
    MCO_RET rc;
    mco_runtime_info_t info;

    mco_get_runtime_info(&info);
#if defined DEBUG
    if (!info.mco_save_load_supported)
    {
      LOG(WARNING) << "XML import/export doesn't supported by runtime";
      m_save_to_xml_feature = false;
    }
    else
    {
      m_save_to_xml_feature = true;
    }
//    show_runtime_info("");
#endif
    if (!info.mco_shm_supported)
    {
      LOG(WARNING) << "This program requires shared memory database runtime";
      return false;
    }

    /* Set the error handler to be called from the eXtremeDB runtime if a fatal error occurs */
//    mco_error_set_handler(&errhandler);
    mco_error_set_handler_ex(&extended_errhandler);

    //LOG(INFO) << "User-defined error handler set";
#if 0
    autoid_t aid = 1;
    int64_t  iid = 2;
    unsigned char *p_aid = (unsigned char*)&aid;
    unsigned char *p_iid = (unsigned char*)&iid;
    int i;
    LOG(INFO) << "autoid_t [%lld size=%d], int64_t [%lld size=%d]\n", 
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
    //rc_check("Runtime starting", rc);
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
      LOG(ERROR) << "Unable to initialize UDA metadictionary, rc=" << rc;
      free(m_metadict);
      m_metadict_initialized = false;
    }
    else
    {
      m_metadict_initialized = true;
      rc = mco_metadict_register(m_metadict,
            ((XDBDatabase*)m_self)->DatabaseName(),
            xdb_broker_get_dictionary(), NULL);
      LOG(INFO) << "mco_metadict_register=" << rc;
    }
#endif

    return status;
}


/* NB: Сначала Инициализация (Init), потом Подключение (Connect) */
bool XDBDatabaseBrokerImpl::Connect()
{
  bool status = Init();

  switch (((XDBDatabase*)m_self)->State())
  {
    case XDBDatabase::UNINITIALIZED:
      LOG(WARNING) << "Try connection to uninitialized database " 
        << ((XDBDatabase*)m_self)->DatabaseName();
    break;

    case XDBDatabase::CONNECTED:
      LOG(WARNING) << "Try to re-open database "
        << ((XDBDatabase*)m_self)->DatabaseName();
    break;

    case XDBDatabase::DISCONNECTED:
        status = AttachToInstance();
    break;

    default:
      LOG(WARNING) << "Try to open database '" 
         << ((XDBDatabase*)m_self)->DatabaseName()
         << "' with unknown state " << (int)((XDBDatabase*)m_self)->State();
    break;
  }

  return status;
}

bool XDBDatabaseBrokerImpl::Disconnect()
{
  MCO_RET rc = MCO_S_OK;
  XDBDatabase::DBState state = ((XDBDatabase*)m_self)->State();

  switch (state)
  {
    case XDBDatabase::UNINITIALIZED:
      LOG(INFO) << "Disconnect from uninitialized state";
    break;

    case XDBDatabase::DISCONNECTED:
      LOG(INFO) << "Try to disconnect already diconnected database";
    break;

    case XDBDatabase::CONNECTED:
      assert(m_self);
      rc = mco_db_disconnect(m_db);
//      rc_check("Disconnection", rc);

      rc = mco_db_close(m_self->DatabaseName());
      ((XDBDatabase*)m_self)->TransitionToState(XDBDatabase::DISCONNECTED);
//      rc_check("Closing", rc);
    break;

    default:
      LOG(INFO) << "Disconnect from unknown state:" << state;
  }

  return (!rc)? true : false;
}


bool XDBDatabaseBrokerImpl::AttachToInstance()
{
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
  LOG(INFO) << "Attaching to '" << ((XDBDatabase*)m_self)->DatabaseName() << "' instance";
  rc = mco_db_connect(((XDBDatabase*)m_self)->DatabaseName(), &m_db);

  /* ошибка - экземпляр базы не найден, попробуем создать её */
  if (MCO_E_NOINSTANCE == rc)
  {
        LOG(INFO) << ((XDBDatabase*)m_self)->DatabaseName() << " instance not found, create";
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
          LOG(ERROR) << "Can't open DB dictionary '"
                << ((XDBDatabase*)m_self)->DatabaseName()
                << "', rc=" << rc;
          return false;
        }

#ifdef DISK_DATABASE
        LOG(INFO) << "Opening '" << m_dbsFileName << "' disk database";
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
            LOG(ERROR) << "Error creating disk database, rc=" << rc;
            return false;
        }
#endif

        /* подключиться к базе данных, т.к. она только что создана */
        LOG(INFO) << "Connecting to instance " << ((XDBDatabase*)m_self)->DatabaseName(); 
        rc = mco_db_connect(((XDBDatabase*)m_self)->DatabaseName(), &m_db);
  }

  /* ошибка создания экземпляра - выход из системы */
  if (rc)
  {
        LOG(ERROR) << "Unable attaching to instance '" 
            << ((XDBDatabase*)m_self)->DatabaseName() 
            << "' with code " << rc;
        return false;
  }

//  rc_check("Connecting", rc);
#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
    mcohv_start(&m_hv, m_metadict, 0, 0);
#endif

  return ((XDBDatabase*)m_self)->TransitionToState(XDBDatabase::CONNECTED);
}

Service *XDBDatabaseBrokerImpl::AddService(const std::string& name)
{
  return AddService(name.c_str());
}

Service *XDBDatabaseBrokerImpl::AddService(const char *name)
{
  xdb_broker::XDBService instance;
  Service       *srv = NULL;
  MCO_RET        rc;
  mco_trans_h    t;
  autoid_t       aid;
  ServiceState   state;

  assert(name);
  do
  {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    rc = instance.create(t);
    if (rc) { LOG(ERROR) << "Creating instance, rc=" << rc; break; }

    rc = instance.name_put(name, strlen(name));
    if (rc) { LOG(ERROR) << "Setting name '" << name << "'"; break; }

    rc = instance.workers_alloc(Service::WORKERS_SPOOL_MAXLEN);
    if (rc) { LOG(ERROR) << "Allocating pool of workers, rc=" << rc; break; }

    rc = instance.checkpoint();
    if (rc) { LOG(ERROR) << "Checkpointing, rc=" << rc; break; }

    rc = instance.state_get(state);
    if (rc) { LOG(ERROR) << "Getting service's state, rc=" << rc; break; }
    rc = instance.autoid_get(aid);
    if (rc) { LOG(ERROR) << "Getting service's id, rc=" << rc; break; }

    srv = new Service(aid, name);
    srv->SetSTATE((Service::State)state);

    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }
  } while(false);

  if (rc)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

  return srv;
}

Service *XDBDatabaseBrokerImpl::RequireServiceByName(const char *service_name)
{
  Service *service = NULL;

  if (NULL == (service = GetServiceByName(service_name)))
  {
  }
  return service;
}

Service *XDBDatabaseBrokerImpl::RequireServiceByName(const std::string& service_name)
{
  Service *service = NULL;

  if (NULL == (service = GetServiceByName(service_name)))
  {
  }
  return service;
}

/*
 * Удалить запись о заданном сервисе
 * - найти указанную запись в базе
 *   - если найдена, удалить её
 *   - если не найдена, вернуть ошибку
 */
bool XDBDatabaseBrokerImpl::RemoveService(const char *name)
{
  bool        status = false;
  xdb_broker::XDBService instance;
  MCO_RET        rc;
  mco_trans_h    t;

  assert(name);
  do
  {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::XDBService::PK_name::find(t, name, strlen(name), instance);
    if (MCO_S_NOTFOUND == rc) 
    { 
        LOG(ERROR) << "Removed service '" << name << "' doesn't exists, rc=" << rc; break;
    }
    if (rc) { LOG(ERROR) << "Unable to find service '" << name << "', rc=" << rc; break; }
    
    rc = instance.remove();
    if (rc) { LOG(ERROR) << "Unable to remove service '" << name << "', rc=" << rc; break; }

    status = true;
  } while(false);

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
  bool          status = false;
  MCO_RET       rc;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;
  uint2         worker_index;

  assert(wrk);
  const char* identity = wrk->GetIDENTITY();

  do 
  {
      rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
      if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

      // Удалось найти Сервис, к которому привязан Обработчик
      rc = xdb_broker::XDBService::SK_ident::find(t,
            identity,
            strlen(identity),
            service_instance,
            worker_index);

      if (MCO_S_NOTFOUND == rc)
      {
        // Попытка удаления Обработчика несуществующего Сервиса
        LOG(ERROR) << "Try to disable worker '" << identity 
            << "' for non-existense service id=" << wrk->GetSERVICE_ID() 
            << ", rc=" << rc;
        break;
      }
      if (rc) 
      {
        LOG(ERROR) << "Locating worker '" << identity << "', rc=" << rc; 
        break;
      }

      // номер искомого Обработчика в спуле есть worker_index
//      rc = service_instance.workers_at(worker_index, worker_instance);
      /* Due McObject's issue #1182:
       * _at returns read-only instance, but _put returns read-write instance */
      rc = service_instance.workers_put(worker_index, worker_instance);
      if (rc) 
      { 
        LOG(ERROR) << "Getting worker '" << identity 
            << "' at pool's index=" << worker_index << ", rc=" << rc;
        break;
      }

      // Не удалять, пометить как "неактивен"
      rc = worker_instance.state_put(DISARMED);
      //rc = service_instance.workers_erase(worker_index);
      if (rc) 
      { 
        LOG(ERROR) << "Disarming worker '" << identity << "', rc=" << rc;
        break;
      }

      status = true;
  } while(false);

  if (rc)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

  return status;
}

// Активировать Обработчик wrk, находящийся в спуле своего Сервиса
bool XDBDatabaseBrokerImpl::PushWorker(Worker *wrk)
{
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
  do
  {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

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
          LOG(ERROR) << "Unable to locate service by id=" << srv->GetID() << ", rc=" << rc;
          break; 
        }
#endif

        // Индекс не корректен - данный Обработчик регистрируется впервые
        // Найти в спуле для него свободное место
        if ((uint2)-1 == (worker_idx = LocatingFirstOccurence(service_instance, DISARMED)))
        {
            // Не удалось найти свободного места в спуле
            LOG(ERROR) << "No free space in service '" << srv->GetNAME()
                << "' pool with id=" << srv->GetID() << ", rc=" << rc;
            break;
        }
      }
      else
      {
        // Обработчик уже есть в спуле, его индекс найден (worker_idx)
        if (rc) 
        { 
            LOG(ERROR) << "Unable to locating '" << wrk_identity 
                << "' in worker's pool, rc=" << rc;
            break; 
        }
      }

      rc = service_instance.workers_size(workers_spool_size);
      if (rc)
      {
        LOG(ERROR) << "Unable to get workers spool size, rc=" << rc;
        break;
      }
      if (!workers_spool_size)
      {
        // требуется первоначальное размещение пула заданного размера
        rc = service_instance.workers_alloc(Service::WORKERS_SPOOL_MAXLEN);
        if (rc) 
        { 
          LOG(ERROR) << "Unable to set waiting workers quantity, rc=" << rc;
          break;
        }
        workers_spool_size = Service::WORKERS_SPOOL_MAXLEN;
      }

      // Номер искомого Обработчика в спуле есть worker_idx
      rc = service_instance.workers_put(worker_idx, worker_instance);
      // Если Обработчика в состоянии DISARMED по данному индексу не окажется,
      // его следует создать.
      if (MCO_E_EMPTYVECTOREL == rc)
      {
        rc = service_instance.workers_put(worker_idx, worker_instance);
        if (rc) { LOG(ERROR) << "Putting new worker into spool, rc="<<rc; break; }
        rc = service_instance.checkpoint();
      }
      if (rc) { LOG(ERROR) << "Getting worker from spool, rc="<<rc; break; }

//      rc = service_instance.workers_put(worker_idx, worker_instance); /* issue #1182 */
      if (rc) { LOG(ERROR) << "Putting worker into spool, rc="<<rc; break; }

      rc = worker_instance.identity_put(wrk_identity, strlen(wrk_identity));
      if (rc) { LOG(ERROR) << "Put worker's identity " << wrk_identity << ", rc="<<rc; break; }

      // Первоначальное состояние Обработчика - "ЗАНЯТ"
      rc = worker_instance.state_put(ARMED);
      if (rc) { LOG(ERROR) << "Unable to change worker's "<< worker_idx <<" state, rc="<<rc; break; }

      /* Установить новое значение expiration */
      if (GetTimerValue(now_time))
      {
        next_heartbeat_time.tv_nsec = now_time.tv_nsec;
        next_heartbeat_time.tv_sec = now_time.tv_sec + Worker::HEARTBEAT_PERIOD_VALUE;
      }
      else { LOG(ERROR) << "Unable to calculate expiration time, rc="<<rc; break; }

      rc = worker_instance.expiration_write(xdb_next_heartbeat_time);
      if (rc) { LOG(ERROR) << "Unable to set worker's expiration time, rc="<<rc; break; }
      rc = xdb_next_heartbeat_time.sec_put(next_heartbeat_time.tv_sec);
      if (rc) { LOG(ERROR) << "Unable to set the expiration seconds, rc="<<rc; break; }
      rc = xdb_next_heartbeat_time.nsec_put(next_heartbeat_time.tv_nsec);
      if (rc) { LOG(ERROR) << "Unable to set expiration time nanoseconds, rc="<<rc; break; }

      status = true;
    }
    else
    { // Сервис-владелец не был найден
      LOG(ERROR) << "Unable to find worker's service with id=" << wrk->GetSERVICE_ID();
      status = false;
    }

  } while(false);

  // Удалим Сервис, переданный сюда из GetServiceById
  delete srv;

  if (rc || false == status)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

  return status;
}
/*
 * Добавить нового Обработчика в спул Сервиса.
 * TODO: рассмотреть необходимость данной функции. Сохранить вариант с std::string& ?
 */
bool XDBDatabaseBrokerImpl::PushWorkerForService(const Service *srv, Worker *wrk)
{
  return false;
}

/*
 * Добавить нового Обработчика в спул Сервиса.
 * Сервис srv должен быть уже зарегистрированным в БД;
 * Экземпляр Обработчика wrk в БД еще не содержится;
 */
Worker* XDBDatabaseBrokerImpl::PushWorkerForService(const std::string& service_name, const std::string& wrk_identity)
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
    wrk = new Worker(wrk_identity.c_str(), srv->GetID());

    if (false == (status = PushWorker(wrk)))
    {
      LOG(WARNING) << "Unable to register worker '" << wrk_identity.c_str()
          << "' for service '" << service_name.c_str() << "'";
    }
  }

  return wrk;
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
  MCO_RET       rc;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  Service *service = NULL;
  autoid_t      service_id;

  do
  {
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::XDBService::PK_name::find(t,
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

  if (rc)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

  return service;
}

Service *XDBDatabaseBrokerImpl::LoadService(
        autoid_t &aid,
        xdb_broker::XDBService& instance)
{
  Service      *service = NULL;
  MCO_RET       rc = MCO_S_OK;
  char          name[Service::NAME_MAXLEN + 1];
  ServiceState  state;

  do
  {
    rc = instance.name_get(name, (uint2)Service::NAME_MAXLEN);
    name[Service::NAME_MAXLEN] = '\0';
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
  MCO_RET rc = MCO_S_OK;
  mco_cursor_t csr;
  int idx;

  assert(srv);
  assert(wrk);

  const char *identity = wrk->GetIDENTITY();

  do
  {
    rc = xdb_broker::XDBService::SK_ident::cursor(t, &csr);
    if (MCO_S_CURSOR_EMPTY == rc) 
        break; /* В индексе нет ни одной записи */
    if (rc) 
    {
      LOG(ERROR)<<"Unable to initialize cursor for '"<<identity<<"', rc="<<rc;
      break;
    }

    idx = 0;
    while (!rc)
    {
      rc = xdb_broker::XDBService::SK_ident::search(t, &csr, MCO_EQ, identity, strlen(identity));
      if (rc) 
      { 
        LOG(ERROR)<<"Unable to search '"<<identity<<"', rc="<<rc; 
        break;
      }

      if (MCO_S_OK == rc)
      {
        wrk->SetINDEX(idx);
        break;
      }
      idx++;
    }

    if (rc)
    {
        // Это может быть нормальным поведением
        wrk->SetINDEX(-1);
    }

  } while(false);

  return rc;
}

Worker *XDBDatabaseBrokerImpl::LoadWorker(mco_trans_h t,
        autoid_t &srv_aid,
        xdb_broker::XDBWorker& wrk_instance,
        uint2 index_in_spool)
{
  Worker       *worker = NULL;
  MCO_RET       rc = MCO_S_OK;
  char          ident[Worker::IDENTITY_MAXLEN + 1];
  xdb_broker::timer_mark  xdb_expire_time;
  timer_mark_t  expire_time = {0, 0};
  uint4         timer_value;
  WorkerState   state;

  do
  {
    rc = wrk_instance.identity_get(ident, (uint2)Worker::IDENTITY_MAXLEN);
    ident[Worker::IDENTITY_MAXLEN] = '\0';
    if (rc) { LOG(ERROR)<<"Unable to get worker's identity for service id "<<srv_aid<<", rc="<<rc; break; }
    rc = wrk_instance.expiration_read(xdb_expire_time);
    if (rc) { LOG(ERROR)<<"Unable to get worker '"<<ident<<"' expiration mark, rc="<<rc; break; }
    rc = wrk_instance.state_get(state);
    if (rc) { LOG(ERROR)<<"Unable to get worker '"<<ident<<"' state, rc="<<rc; break; }
    rc = xdb_expire_time.sec_get(timer_value); expire_time.tv_sec = timer_value;
    rc = xdb_expire_time.nsec_get(timer_value); expire_time.tv_nsec = timer_value;

    worker = new Worker(ident, srv_aid);
    worker->SetSTATE((Worker::State)state);
    worker->SetEXPIRATION(expire_time);
    worker->SetINDEX(index_in_spool);
    /* Состояние объекта полностью соответствует хранимому в БД */
    worker->SetVALID();
    LOG(INFO) << "New Worker(id='" << ident
              << "' aid=" << srv_aid 
              << " state=" << (int)state
              << " spool_idx=" << index_in_spool << ")";
  } while(false);

  return worker;
}

/* 
 * Вернуть ближайший свободный Обработчик.
 * Побочные эффекты: нет
 */
Worker *XDBDatabaseBrokerImpl::GetWorker(const Service *srv)
{
  MCO_RET       rc;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker worker_instance;
  Worker       *worker = NULL;
  WorkerState   worker_state;
  autoid_t      aid;
  uint2         workers_spool_size;
  uint2         idx;

  assert(srv);
  do
  {
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Unable to start transaction, rc="<<rc; break; }

    aid = const_cast<Service*>(srv)->GetID();
    rc = xdb_broker::XDBService::autoid::find(t, aid, service_instance);
    if (rc) { LOG(ERROR)<<"Unable to get service's instance, rc="<<rc; break; }

    rc = service_instance.workers_size(workers_spool_size);
    if (rc) { LOG(ERROR)<<"Unable to get workers spool size, rc="<<rc; break; }

    for (idx = 0; idx < workers_spool_size; idx++)
    {
      rc = service_instance.workers_at(idx, worker_instance);
      if (rc) { LOG(ERROR)<<"Unable to read worker from spool, rc="<<rc; break; }
      rc = worker_instance.state_get(worker_state);
      if (!rc && (worker_state == ARMED))
      {
        worker = LoadWorker(t, aid, worker_instance, idx);
        break;
      }
      if (rc) { LOG(ERROR)<<"Unable to get service's waiting worker, rc="<<rc; break; }
    }
  } while(false);

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
  MCO_RET       rc;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker worker_instance;
  uint2         workers_spool_size = 0;
  uint2         awaiting_worker_idx = -1;
  Worker *worker = NULL;
  autoid_t      aid;

  assert(name);
  do
  {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = xdb_broker::XDBService::PK_name::find(t, name, strlen(name), service_instance);
    if (rc) { LOG(ERROR)<< "Locating service by name '"<<name<<"', rc="<<rc; break; }
    rc = service_instance.autoid_get(aid);
    if (rc) { LOG(ERROR)<< "Get id for service "<<name<<", rc="<<rc; break; }

    rc = service_instance.workers_size(workers_spool_size);
    if (rc) { LOG(ERROR)<< "Get workers spool size for service '"<<name<<"', rc="<<rc; break; }
    if (!workers_spool_size)
    {
       /* Спул Обработчиков пуст - вернуться */
       LOG(WARNING) << "Waiting workers pool is empty";
       break;
    }

    if ((uint2)-1 == (awaiting_worker_idx = LocatingFirstOccurence(service_instance, ARMED)))
    {
       /* Нет свободных Обработчиков - вернуться */
       LOG(WARNING) << "No one waiting workers in '"<<name<<"' spool";
       break;
    }

    rc = service_instance.workers_put(awaiting_worker_idx, worker_instance); // issue #1182
    if (rc) { LOG(ERROR)<< "Get service's first awaiting worker, rc="<<rc; break; }
    if (rc)
    {
      LOG(ERROR)<< "making worker instance at "<<awaiting_worker_idx<<" as read-write, rc="<<rc;
      break;
    }

    /* Сменить состояние Обработчика на "ЗАНЯТ" */
    rc = worker_instance.state_put(IN_PROCESS);
    if (rc) { LOG(ERROR)<< "Changing worker state, rc="<<rc; break; }

    worker = LoadWorker(t, aid, worker_instance, awaiting_worker_idx);
  } while(false);

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
  MCO_RET       rc;
  mco_trans_h   t;
  xdb_broker::XDBService    service_instance;
  xdb_broker::XDBWorker     worker_instance;
  uint2         workers_spool_size = 0;
  autoid_t      aid = 0;
  Worker       *worker = NULL;
  uint2         awaiting_worker_idx = 0;

  assert(service);
  do
  {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = xdb_broker::XDBService::autoid::find(t, service->GetID(), service_instance);
    if (rc) { LOG(ERROR)<<"Locating service id="<<service->GetID()<<" failure, rc="<<rc; break; }

    /* Есть ли Обработчики для данного Сервиса? */
    rc = service_instance.workers_size(workers_spool_size);
    if (rc) { LOG(ERROR)<<"Get service's waiting workers queue size, rc="<<rc; break; }
    
    if (!workers_spool_size)
    {
       /* Спул Обработчиков пуст - вернуться */
       LOG(WARNING) << "Waiting workers spool is empty";
       break;
    }

    if ((uint2)-1 == (awaiting_worker_idx = LocatingFirstOccurence(service_instance, ARMED)))
    {
       /* Нет свободных Обработчиков - вернуться */
       LOG(WARNING) << "No one waiting workers in spool";
       break;
    }

    rc = service_instance.workers_put(awaiting_worker_idx, worker_instance); /* issue #1182 */
    if (rc) { LOG(ERROR)<<"Get service's first awaiting worker, rc="<<rc; break; }
    if (rc) 
    {
        LOG(ERROR)<< "Making worker instance at "<<awaiting_worker_idx
                <<" as read-write, rc="<<rc; break; 
    }

    aid = service->GetID();

    /* Сменить состояние Обработчика на "ЗАНЯТ" */
    rc = worker_instance.state_put(IN_PROCESS);
    if (rc) { LOG(ERROR)<< "Change worker state, rc="<<rc; break; }

    worker = LoadWorker(t, aid, worker_instance, awaiting_worker_idx);
  } while(false);

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

  do
  {
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = xdb_broker::XDBService::autoid::find(t, aid, service_instance);
    if (rc) { LOG(ERROR)<<"Locating service by id="<<_id<<", rc="<<rc; break; }

    service = LoadService(aid, service_instance);
  } while(false);

  if (rc)
    mco_trans_rollback(t);
  else
    rc = mco_trans_commit(t);

  return service;
}

/*
 * Вернуть признак существования Сервиса с указанным именем в БД
 */
bool XDBDatabaseBrokerImpl::IsServiceExist(const char *name)
{
  MCO_RET       rc;
  mco_trans_h   t;
  xdb_broker::XDBService instance;

  assert(name);
  do
  {
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = xdb_broker::XDBService::PK_name::find(t, name, strlen(name), instance);
    // Если объект не найден - это не ошибка
    if (rc == MCO_S_NOTFOUND) break;
    if (rc) { LOG(WARNING)<< "Locating service instance by name '"<<name<<"', rc="<<rc; break; }
  } while(false);

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
  MCO_RET      rc = MCO_S_OK;
  xdb_broker::XDBWorker     worker_instance;
  WorkerState  worker_state;
  bool         awaiting_worker_found = false;
  uint2        workers_spool_size = 0;
  uint2        awaiting_worker_idx;

  do
  {
    /* Есть ли в Обработчики для данного Сервиса? */
    rc = service_instance.workers_size(workers_spool_size);
    if (rc) { LOG(ERROR)<<"Get service's waiting workers queue size,rc="<<rc; break; }
    
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
        if (rc) { LOG(ERROR)<<"Get service's awaiting worker, rc="<<rc; break; }
      }
      else // Найден Обработчик, проверить его состояние
      {
        rc = worker_instance.state_get(worker_state);
        if (rc) { LOG(ERROR)<< "Get awaiting worker's state, rc="<<rc; break; }

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
        LOG(INFO) << "No one waiting workers find";
        break;
    }

  } while(false);

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
  MCO_RET      rc;
  mco_trans_h  t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;
  Worker      *worker = NULL;
  uint2        worker_idx;
  autoid_t     service_aid;

  do
  {
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::XDBService::SK_ident::find(t,
                ident,
                strlen(ident),
                service_instance,
                worker_idx);

    /* Запись не найдена - нет ошибки */
    if (MCO_S_NOTFOUND == rc) break;

    /* Запись не найдена - есть ошибка - сообщить */
    if (rc) { LOG(ERROR)<<"Worker location, rc="<<rc; break; }

    /* Запись найдена - сконструировать объект на основе данных из БД */
    rc = service_instance.autoid_get(service_aid);
    if (rc) { LOG(ERROR)<< "Getting worker's service id, rc="<<rc; break; }
    rc = service_instance.workers_at(worker_idx, worker_instance);
    if (rc) { LOG(ERROR)<< "Getting worker's instance, rc="<<rc; break; }

    worker = LoadWorker(t, service_aid, worker_instance, worker_idx);
  } while(false);

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

    LOG(INFO) << "Export DB to " << fname;
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
        LOG(ERROR)<< "Unable to set xml policy, rc="<<rc;
      }
#endif
      rc = mco_db_xml_export(t, f, file_writer);

      /* revert xml policy */
#ifdef SETUP_POLICY
      mco_xml_set_policy(t, &op);
#endif

      if (rc != MCO_S_OK)
      {
         LOG(ERROR)<< "Exporting, rc="<<rc;
         mco_trans_rollback(t);
      }
      else
      {
         mco_trans_commit(t);
      }
    }
    else
    {
      LOG(ERROR)<< "Opening transaction, rc="<<rc;
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

