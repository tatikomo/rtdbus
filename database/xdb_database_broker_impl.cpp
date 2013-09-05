#include <assert.h>
#include <stdio.h>
#include <glog/logging.h>
#include <stdlib.h> // free
#include <stdarg.h>
#include <string.h>

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

const int DATABASE_SIZE = 1024 * 1024 * 1;  // 1Mб
const int MEMORY_PAGE_SIZE = DATABASE_SIZE; // Вся БД поместится в одной странице ОЗУ 
const int MAP_ADDRESS = 0x20000000;

#ifndef MCO_PLATFORM_X64
    static const unsigned int PAGESIZE = 128;
#else 
    static const int unsigned PAGESIZE = 256;
#endif 

#ifdef DISK_DATABASE
const int DB_DISK_CACHE = (1 * 1024 * 1024);
const int DB_DISK_PAGE_SIZE = 1024;

    #ifndef DB_LOG_TYPE
        #define DB_LOG_TYPE REDO_LOG
    #endif 
#else 
const int DB_DISK_CACHE = 0;
const int DB_DISK_PAGE_SIZE = 0;
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
    XDBDatabaseBroker *self) : m_initialized(false)
#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
    , m_metadict_initialized(false)
#endif
{
  assert(self);

  m_self = self;
  m_service_list = NULL;

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
      delete m_service_list;
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
    mco_error_set_handler(&errhandler);
//    mco_error_set_handler_ex(&extended_errhandler);
    //LOG(INFO) << "User-defined error handler set";
    
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
      if (rc)
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
      mco_async_event_release_all(m_db/*, MCO_EVENT_newService*/);
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

/*
 * Статический метод, вызываемый из runtime базы данных 
 * при создании нового экземпляра XDBService
 */
MCO_RET XDBDatabaseBrokerImpl::new_Service(mco_trans_h t,
        XDBService *obj,
        MCO_EVENT_TYPE et,
        void *p)
{
  XDBDatabaseBrokerImpl *self = static_cast<XDBDatabaseBrokerImpl*> (p);
  char name[Service::NAME_MAXLEN + 1];
  MCO_RET rc;
  autoid_t aid;
  Service *service;
  xdb_broker::XDBService service_instance;
  bool status = false;

  assert(self);
  assert(obj);

  do
  {
    name[0] = '\0';

    rc = XDBService_name_get(obj, name, (uint2)Service::NAME_MAXLEN);
    name[Service::NAME_MAXLEN] = '\0';
    if (rc) { LOG(ERROR)<<"Unable to get service's name, rc="<<rc; break; }

    rc = XDBService_autoid_get(obj, &aid);
    if (rc) { LOG(ERROR)<<"Unable to get id of service '"<<name<<"', rc="<<rc; break; }

    service = new Service(aid, name);
    if (!self->m_service_list)
      self->m_service_list = new ServiceListImpl(self->m_db);

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
MCO_RET XDBDatabaseBrokerImpl::del_Service(mco_trans_h t,
        XDBService *obj,
        MCO_EVENT_TYPE et,
        void *p)
{
  XDBDatabaseBrokerImpl *self = static_cast<XDBDatabaseBrokerImpl*> (p);
  char name[Service::NAME_MAXLEN + 1];
  MCO_RET rc;

  assert(self);
  assert(obj);

  do
  {
    name[0] = '\0';

    rc = XDBService_name_get(obj, name, (uint2)Service::NAME_MAXLEN);
    name[Service::NAME_MAXLEN] = '\0';
    if (rc) { LOG(ERROR)<<"Unable to get service's name, rc="<<rc; break; }

  } while (false);

//  LOG(INFO) << "DEL XDBService "<<obj<<" name '"<<name<<"' self=" << self;

  return MCO_S_OK;
}

MCO_RET XDBDatabaseBrokerImpl::RegisterEvents()
{
  MCO_RET rc;
  mco_trans_h t;

  do
  {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) LOG(ERROR) << "Starting transaction, rc=" << rc;

    rc = mco_register_newService_handler(t, 
            &XDBDatabaseBrokerImpl::new_Service, 
            (void*)this
//#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
//            , MCO_AFTER_UPDATE
//#endif
            );

    if (rc) LOG(ERROR) << "Registering event on XDBService creation, rc=" << rc;

    rc = mco_register_delService_handler(t, 
            &XDBDatabaseBrokerImpl::del_Service, 
            (void*)this);
    if (rc) LOG(ERROR) << "Registering event on XDBService deletion, rc=" << rc;

    rc = mco_trans_commit(t);
  } while(false);

  if (rc)
   mco_trans_rollback(t);

  return rc;
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
# if USE_EXTREMEDB_HTTP_SERVER
  rc = mco_uda_db_open(m_metadict,
                       0,
                       &m_dev,
                       1,
                       &m_db_params,
                       NULL,
                       0);
  if (rc)
    LOG(ERROR) << "Unable to open UDA, rc=" << rc;
# endif

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

  RegisterEvents();

//  rc_check("Connecting", rc);
#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
    m_hv = 0;
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
  xdb_broker::XDBService service_instance;
  Service       *srv = NULL;
  MCO_RET        rc;
  mco_trans_h    t;
  autoid_t       aid;

  assert(name);
  do
  {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    rc = service_instance.create(t);
    if (rc) { LOG(ERROR) << "Creating instance, rc=" << rc; break; }

    rc = service_instance.name_put(name, strlen(name));
    if (rc) { LOG(ERROR) << "Setting '" << name << "' name"; break; }

    rc = service_instance.state_put(REGISTERED);
    if (rc) { LOG(ERROR) << "Setting '" << name << "' state"; break; }

    rc = service_instance.autoid_get(aid);
    if (rc) { LOG(ERROR) << "Getting service's "<<name<<" id, rc=" << rc; break; }

    srv = new Service(aid, name);
    srv->SetSTATE((Service::State)REGISTERED);

    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }
    rc = mco_trans_commit(t);
  } while(false);

  if (rc)
    mco_trans_rollback(t);

  return srv;
}

// TODO: возможно стоит удалить этот метод-обертку, оставив GetServiceByName
Service *XDBDatabaseBrokerImpl::RequireServiceByName(const char *service_name)
{
  return GetServiceByName(service_name);
}

// TODO: возможно стоит удалить этот метод-обертку, оставив GetServiceByName
Service *XDBDatabaseBrokerImpl::RequireServiceByName(const std::string& service_name)
{
  return GetServiceByName(service_name);
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
  xdb_broker::XDBService service_instance;
  MCO_RET        rc;
  mco_trans_h    t;

  assert(name);
  do
  {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::XDBService::PK_name::find(t, name, strlen(name), service_instance);
    if (MCO_S_NOTFOUND == rc) 
    { 
        LOG(ERROR) << "Removed service '" << name << "' doesn't exists, rc=" << rc; break;
    }
    if (rc) { LOG(ERROR) << "Unable to find service '" << name << "', rc=" << rc; break; }
    
    rc = service_instance.remove();
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
  assert(1 == 0);
  return status;
}


// Деактивировать Обработчик wrk по идентификатору у его Сервиса
// NB: При этом сам экземпляр из базы не удаляется, а помечается неактивным (DISARMED)
bool XDBDatabaseBrokerImpl::RemoveWorker(Worker *wrk)
{
  MCO_RET       rc = MCO_S_OK;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;

  assert(wrk);
  const char* identity = wrk->GetIDENTITY();

  do
  {
      rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
      if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

      rc = xdb_broker::XDBWorker::SK_by_ident::find(t,
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
  } while(false);

  if (rc)
    mco_trans_rollback(t);

  return (MCO_S_OK == rc)? true : false;
}

// Активировать Обработчик wrk своего Сервиса
bool XDBDatabaseBrokerImpl::PushWorker(Worker *wrk)
{
  MCO_RET       rc = MCO_S_OK;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;
  timer_mark_t  now_time, next_heartbeat_time;
  xdb_broker::timer_mark xdb_next_heartbeat_time;
  autoid_t      srv_aid;
  autoid_t      wrk_aid;

  assert(wrk);
  const char* wrk_identity = wrk->GetIDENTITY();

  do
  {
      rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
      if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

      rc = xdb_broker::XDBService::autoid::find(t,
                            wrk->GetSERVICE_ID(),
                            service_instance);
      if (rc)
      {
        LOG(ERROR) << "Unable to locate service by id "<<wrk->GetSERVICE_ID()
                   << " for worker "<<wrk_identity;
        break;
      }

      rc = service_instance.autoid_get(srv_aid);
      if (rc) { LOG(ERROR) << "Unable to get autoid for service id "<<wrk->GetSERVICE_ID(); break; }

      // Защита от "дурака" - идентификаторы должны совпадать
      assert(wrk->GetSERVICE_ID() == srv_aid);
      if (wrk->GetSERVICE_ID() != srv_aid)
      {
        LOG(ERROR) << "Database inconsistence: service id ("
                   << wrk->GetSERVICE_ID()<<") for worker "<<wrk_identity
                   << " did not equal to database value ("<<srv_aid<<")";
      }

      // Проверить наличие Обработчика с таким ident в базе,
      // в случае успеха получив его worker_instance
      rc = xdb_broker::XDBWorker::SK_by_ident::find(t,
                wrk_identity,
                strlen(wrk_identity),
                worker_instance);

      if (!rc) // Экземпляр найден - обновить данные
      {
        if (!wrk->GetID()) // Нулевой собственный идентификатор - значит ранее не присваивался
        {
          // Обновить идентификатор
          rc = worker_instance.autoid_get(wrk_aid);
          if (rc) { LOG(ERROR) << "Unable to get worker "<<wrk_identity<<" id"; break; }
          wrk->SetID(wrk_aid);
        }
        // Сменить ему статус на ARMED и обновить свойства
        rc = worker_instance.state_put(ARMED);
        if (rc) { LOG(ERROR) << "Unable to change '"<< wrk_identity
                             <<"' worker state, rc="<<rc; break; }

        rc = worker_instance.service_ref_put(srv_aid);
        if (rc) { LOG(ERROR) << "Unable to set '"<< wrk_identity
                             <<"' with service id "<<srv_aid<<", rc="<<rc; break; }

        /* Установить новое значение expiration */
        // TODO: wrk->CalculateEXPIRATION_TIME();
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
      }
      else if (MCO_S_NOTFOUND == rc) // Экземпляр не найден, так как ранее не регистрировался
      {
        // Создать новый экземпляр
        rc = worker_instance.create(t);
        if (rc) { LOG(ERROR) << "Creating worker's instance " << wrk_identity << ", rc="<<rc; break; }

        rc = worker_instance.identity_put(wrk_identity, strlen(wrk_identity));
        if (rc) { LOG(ERROR) << "Put worker's identity " << wrk_identity << ", rc="<<rc; break; }

        // Первоначальное состояние Обработчика - "ГОТОВ"
        rc = worker_instance.state_put(ARMED);
        if (rc) { LOG(ERROR) << "Unable to change "<< wrk_identity <<" worker state, rc="<<rc; break; }

        rc = worker_instance.service_ref_put(srv_aid);
        if (rc) { LOG(ERROR) << "Unable to set '"<< wrk_identity
                             <<"' with service id "<<srv_aid<<", rc="<<rc; break; }

        // Обновить идентификатор
        rc = worker_instance.autoid_get(wrk_aid);
        if (rc) { LOG(ERROR) << "Unable to get worker "<<wrk_identity<<" id"; break; }
        wrk->SetID(wrk_aid);

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
      }
      else 
      { 
        LOG(ERROR) << "Unable to load worker "<<wrk_identity<<" data"; 
        break; 
      }

      mco_trans_commit(t);

  } while (false);
  
  if (rc)
    mco_trans_rollback(t);

  return (MCO_S_OK == rc);
}
/*
 * Добавить нового Обработчика в спул Сервиса.
 * TODO: рассмотреть необходимость данной функции. Сохранить вариант с std::string& ?
 */
bool XDBDatabaseBrokerImpl::PushWorkerForService(const Service *srv, Worker *wrk)
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

  mco_trans_rollback(t);

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

#if 0
/*
 * Загрузить данные Обработчика в ранее созданный вручную экземпляр
 */
MCO_RET XDBDatabaseBrokerImpl::LoadWorkerByIdent(
        /* IN */ mco_trans_h t,
        /* INOUT */ Worker *wrk)
{
  MCO_RET rc = MCO_S_OK;
  mco_cursor_t csr;
  xdb_broker::XDBWorker worker_instance;
  uint2 idx;
  char name[SERVICE_NAME_MAXLEN+1];

  assert(wrk);

  const char *identity = wrk->GetIDENTITY();

  do
  {
    rc = xdb_broker::XDBWorker::SK_by_ident::find(t,
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

  return rc;
}
#endif

// Загрузить данные Обработчика, основываясь на его идентификаторе
MCO_RET XDBDatabaseBrokerImpl::LoadWorker(mco_trans_h t,
        /* IN  */ xdb_broker::XDBWorker& wrk_instance,
        /* OUT */ Worker*& worker)
{
  MCO_RET       rc = MCO_S_OK;
  char          ident[Worker::IDENTITY_MAXLEN + 1];
  xdb_broker::timer_mark  xdb_expire_time;
  timer_mark_t  expire_time = {0, 0};
  uint4         timer_value;
  WorkerState   state;
  autoid_t      wrk_aid;
  autoid_t      srv_aid;

  do
  {
    rc = wrk_instance.autoid_get(wrk_aid);
    if (rc) { LOG(ERROR) << "Unable to get worker id"; break; }

    rc = wrk_instance.identity_get(ident, (uint2)Worker::IDENTITY_MAXLEN);
    if (rc) { LOG(ERROR) << "Unable to get identity for worker id "<<wrk_aid; break; }

    rc = wrk_instance.service_ref_get(srv_aid);
    if (rc) { LOG(ERROR) << "Unable to get service id for worker "<<ident; break; }

    ident[Worker::IDENTITY_MAXLEN] = '\0';
    if (rc) { LOG(ERROR)<<"Unable to get worker's identity for service id "<<srv_aid<<", rc="<<rc; break; }

    rc = wrk_instance.expiration_read(xdb_expire_time);
    if (rc) { LOG(ERROR)<<"Unable to get worker '"<<ident<<"' expiration mark, rc="<<rc; break; }

    rc = wrk_instance.state_get(state);
    if (rc) { LOG(ERROR)<<"Unable to get worker '"<<ident<<"' state, rc="<<rc; break; }

    rc = xdb_expire_time.sec_get(timer_value); expire_time.tv_sec = timer_value;
    rc = xdb_expire_time.nsec_get(timer_value); expire_time.tv_nsec = timer_value;

    worker = new Worker(srv_aid, ident, wrk_aid);
    worker->SetSTATE((Worker::State)state);
    worker->SetEXPIRATION(expire_time);
    /* Состояние объекта полностью соответствует хранимому в БД */
    worker->SetVALID();
    LOG(INFO) << "Load Worker('" << worker->GetIDENTITY()
              << "' id=" << worker->GetID() 
              << " srv_id=" << worker->GetSERVICE_ID() 
              << " state=" << worker->GetSTATE();
  } while(false);

  return rc;
}

/* 
 * Вернуть ближайший свободный Обработчик в состоянии ARMED.
 * Побочные эффекты: создается экземпляр Worker, его удаляет вызвавшая сторона
 *
 * TODO: возвращать следует наиболее "старый" экземпляр, временная отметка 
 * которого раньше всех остальных экземпляров с этим же состоянием.
 */
Worker *XDBDatabaseBrokerImpl::PopWorker(const Service *srv)
{
  Worker       *worker = NULL;
  autoid_t      service_aid;

  assert(srv);
  service_aid = const_cast<Service*>(srv)->GetID();
  worker = GetWorkerByState(service_aid, ARMED);

  return worker;
}

Worker *XDBDatabaseBrokerImpl::PopWorker(const std::string& service_name)
{
  Service      *service = NULL;
  Worker       *worker = NULL;

  if (NULL != (service = GetServiceByName(service_name)))
  {
    autoid_t  service_aid = service->GetID();
    worker = GetWorkerByState(service_aid, ARMED);
  }

  return worker;
}

Service *XDBDatabaseBrokerImpl::GetServiceForWorker(const Worker *wrk)
{
  // TODO реализация
  assert(1 == 0);
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

  mco_trans_rollback(t);

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
Worker* XDBDatabaseBrokerImpl::GetWorkerByState(autoid_t& service_id,
                       WorkerState searched_worker_state)
{
  MCO_RET      rc = MCO_S_OK;
  Worker      *worker = NULL;
  mco_trans_h  t;
  WorkerState  worker_state;
  mco_cursor_t csr;
  bool         awaiting_worker_found = false;
  xdb_broker::XDBWorker    worker_instance;
  int          cmp_result;

  do
  {
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = xdb_broker::XDBWorker::SK_by_serv_id::cursor(t, &csr);
    if (MCO_S_CURSOR_EMPTY == rc) // если индекс еще не содержит ни одной записи
      break;
    if (rc) { LOG(ERROR)<< "Unable to get cursor, rc="<<rc; break; }

    for (rc = xdb_broker::XDBWorker::SK_by_serv_id::search(t, &csr, MCO_EQ, service_id);
         (rc == MCO_S_OK) || (false == awaiting_worker_found);
         rc = mco_cursor_next(t, &csr)) 
    {
       rc = xdb_broker::XDBWorker::SK_by_serv_id::compare(t,
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
        LOG(ERROR) << "Найден Обработчик "<<worker->GetIDENTITY()
                   <<" для другого Сервиса ("
                   << worker->GetSERVICE_ID()<<" != "<<service_id<<")";
    }
  } while(false);

  mco_trans_rollback(t);

  return worker;
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

  do
  {
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::XDBWorker::SK_by_ident::find(t,
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

/* Получить первое ожидающее обработки Сообщение */
bool XDBDatabaseBrokerImpl::GetWaitingLetter(
        /* IN  */ Service* srv,
        /* IN  */ Worker* wrk, /* GEV: зачем здесь Worker? Он должен быть закреплен за сообщением после передачи */
        /* OUT */ std::string& header,
        /* OUT */ std::string& body)
{
  bool status = false;
  mco_trans_h t;
  MCO_RET rc = MCO_S_OK;
  mco_cursor_t csr;
  xdb_broker::XDBLetter letter_instance;
  autoid_t  aid;
  uint2     sz;
  char*     header_buffer = NULL;
  char*     body_buffer = NULL;

  assert(srv);
  assert(wrk);

  do
  {
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc="<<rc; break; }

    rc = xdb_broker::XDBLetter::SK_by_state_for_serv::cursor(t, &csr);
    if (MCO_S_CURSOR_EMPTY == rc) 
        break; /* В индексе нет ни одной записи */
    if (rc) 
    {
      LOG(ERROR)<<"Unable to initialize cursor for '"<<srv->GetNAME()<<"', rc="<<rc;
      break;
    }

    rc = xdb_broker::XDBLetter::SK_by_state_for_serv::search(t, &csr, MCO_EQ, srv->GetID(), UNASSIGNED); 
    // Если курсор пуст, вернуться
    if (MCO_S_CURSOR_EMPTY == rc)
      break;
    if (rc) { LOG(ERROR) << "Unable to get Letters list cursor, rc="<<rc; break; }

    rc = mco_cursor_first(t, &csr);
    if (rc) 
    { 
        LOG(ERROR) << "Unable to point at first item in Letters list cursor, rc="<<rc;
        break;
    }

#if 0
    while (rc == MCO_S_OK)
    {
      rc = letter_instance.from_cursor(t, &csr);
      if (rc) 
      { 
        LOG(ERROR) << "Unable to get item from Letters cursor, rc="<<rc; 
      }

      rc = letter_instance.name_get(name, (uint2)Service::NAME_MAXLEN);
      if (rc) { LOG(ERROR) << "Getting service name, rc="<<rc; break; }

      rc = letter_instance.autoid_get(aid);
      if (rc) { LOG(ERROR) << "Getting ID of service '"<<name<<"', rc=" << rc; break; }

      if (false == AddService(name, aid))
        LOG(ERROR) << "Unable to add new service '"<<name<<"':"<<aid<<" into list";

      rc = mco_cursor_next(t, &csr);
    }
#else
    // Достаточно получить первый элемент, функция будет вызываться до опустошения содержимого курсора 
    rc = letter_instance.from_cursor(t, &csr);
    if (rc) { LOG(ERROR) << "Unable to get item from Letters cursor, rc="<<rc; break; }

    rc = letter_instance.autoid_get(aid);
    if (rc) { LOG(ERROR) << "Getting letter' id, rc=" << rc; break; }

    rc = letter_instance.header_size(sz);
    if (rc) { LOG(ERROR) << "Getting header size for letter id"<<aid<<", rc=" << rc; break; }
    header_buffer = new char[sz + 1];

    rc = letter_instance.header_get(header_buffer, sizeof(*header_buffer), sz);
    if (rc) { LOG(ERROR) << "Getting header for letter id"<<aid<<", rc=" << rc; break; }
    header_buffer[sz] = '\0';

    rc = letter_instance.body_size(sz);
    if (rc) { LOG(ERROR) << "Getting message body size for letter id"<<aid<<", rc=" << rc; break; }
    body_buffer = new char[sz + 1];

    rc = letter_instance.body_get(body_buffer, sizeof(*body_buffer), sz);
    if (rc) { LOG(ERROR) << "Getting message for letter id"<<aid<<", rc=" << rc; break; }
    body_buffer[sz] = '\0';

    header.assign(header_buffer);
    body.assign(body_buffer);

    delete []header_buffer;
    delete []body_buffer;

#endif
  } while(false);

  mco_trans_rollback(t);

  return status;
}

Service::State XDBDatabaseBrokerImpl::GetServiceState(const Service *srv)
{
  assert (srv);
  Service::State state = Service::DISABLED;
  // TODO реализация
  assert(1 == 0);
  // Проверить состояние самого Сервиса и ее подчиненных Обработчиков
  return state;
}

Worker *XDBDatabaseBrokerImpl::GetWorkerByIdent(const std::string& ident)
{
  // TODO: Реализовать для использования локализованных названий сервисов
  //
  return GetWorkerByIdent(ident.c_str());
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

  /*
   * Поместить новое сообщение letter в очередь сервиса srv.
   * Сообщение будет обработано одним из Обработчиков Сервиса.
   * Первоначальное состояние: READY
   * Состояние после успешной передачи Обработчику: PROCESSING
   * Состояние после получения ответа: DONE_OK или DONE_FAIL
   */
bool XDBDatabaseBrokerImpl::PushRequestToService(Service *srv,
            const std::string& header,
            const std::string& body)
{
  MCO_RET      rc;
  mco_trans_h  t;
  xdb_broker::XDBLetter  letter_instance;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;
  autoid_t     aid;
  bool         status = false;
  xdb_broker::timer_mark   mark;

  assert (srv);

  /*
   * 1. Получить id Службы
   * 2. Поместить сообщение и его заголовок в таблицу XDBLetter 
   * с состоянием UNASSIGNED.
   * Далее, после передачи сообщения Обработчику, он вносит свой id
   */

  do
  {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = letter_instance.create(t);
    if (rc)
    { 
        LOG(ERROR)<<"Unable to create letter for service '"
                  <<srv->GetNAME()<<"', rc="<<rc;
        break; 
    }

    rc = letter_instance.service_put(srv->GetID());

    rc = letter_instance.autoid_get(aid);
/*
 * Идентификатор Обработчика запишется только после 
 * успешной передачи сообщения этому Обработчику
 */
//    rc = worker_put( autoid_t value );

    rc = letter_instance.state_put(UNASSIGNED); // ASSIGNED - после передачи Обработчику

    rc = letter_instance.expiration_write(mark);

    rc = letter_instance.header_put(header.c_str(), header.size());

    rc = letter_instance.body_put(body.c_str(), body.size());
    if (rc)
    { 
        LOG(ERROR)<<"Unable to set letter attributes for service '"
                  <<srv->GetNAME()<<"', rc="<<rc;
        break; 
    }

    mco_trans_commit(t);
    status = true;
    LOG(INFO) << "PushRequestToService '"<<srv->GetNAME()<<"' id="<<aid;
  } while(false);

  if (rc)
    mco_trans_rollback(t);

  return status;
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

ServiceList* XDBDatabaseBrokerImpl::GetServiceList()
{
  if (!m_service_list)
    m_service_list = new ServiceListImpl(m_db);
  else
  {
    if (!m_service_list->refresh())
      LOG(WARNING) << "Services list is not updated";
  }

  return m_service_list;
}


ServiceListImpl::ServiceListImpl(mco_db_h _db)
{
  m_array = new Service*[MAX_SERVICES_ENTRY];
  memset(m_array, '\0', sizeof(m_array)); // NB: подразумевается что 0 и NULL равны
  m_size = 0;
  m_db = _db;
  assert(m_db);
}

ServiceListImpl::~ServiceListImpl()
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

Service* ServiceListImpl::first()
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

Service* ServiceListImpl::last()
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

Service* ServiceListImpl::next()
{
  Service *srv = NULL;

  assert(m_array);
  if (m_array && (m_current_index < m_size))
  {
    srv = m_array[m_current_index++];
  }

  return srv;
}

Service* ServiceListImpl::prev()
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
const int ServiceListImpl::size()
{
  return m_size;
}

// Добавить новый Сервис, определенный своим именем и идентификатором
bool ServiceListImpl::AddService(const char* name, int64_t id)
{
  Service *srv;

  if (m_size >= MAX_SERVICES_ENTRY)
    return false;
 
  srv = new Service(id, name);
  m_array[m_size++] = srv;
  
  LOG(INFO) << "EVENT AddService '"<<srv->GetNAME()<<"' id="<<srv->GetID();
  return true;
}

// Добавить новый Сервис, определенный объектом Service
bool ServiceListImpl::AddService(const Service* service)
{
  assert(service);
  Service *srv = const_cast<Service*>(service);
  if (m_size >= MAX_SERVICES_ENTRY)
    return false;

  LOG(INFO) << "EVENT AddService '"<<srv->GetNAME()<<"' id="<<srv->GetID();
  m_array[m_size++] = srv;

  return true;
}

// Удалить Сервис по его имени
bool ServiceListImpl::RemoveService(const char* name)
{
  LOG(INFO) << "EVENT DelService '"<<name<<"'";
  return false;
}

// Удалить Сервис по его идентификатору
bool ServiceListImpl::RemoveService(const int64_t id)
{
  LOG(INFO) << "EVENT AddService with id="<<id;
  return false;
}

// Перечитать список Сервисов из базы данных
bool ServiceListImpl::refresh()
{
  bool status = false;
  mco_trans_h t;
  MCO_RET rc = MCO_S_OK;
  mco_cursor_t csr;
  xdb_broker::XDBService service_instance;
  char name[Service::NAME_MAXLEN + 1];
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
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc="<<rc; break; }

    rc = xdb_broker::XDBService::list_cursor(t, &csr);
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

      rc = service_instance.name_get(name, (uint2)Service::NAME_MAXLEN);
      if (rc) { LOG(ERROR) << "Getting service name, rc="<<rc; break; }

      rc = service_instance.autoid_get(aid);
      if (rc) { LOG(ERROR) << "Getting ID of service '"<<name<<"', rc=" << rc; break; }

      if (false == AddService(name, aid))
        LOG(ERROR) << "Unable to add new service '"<<name<<"':"<<aid<<" into list";

      rc = mco_cursor_next(t, &csr);
    }

  } while(false);

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


