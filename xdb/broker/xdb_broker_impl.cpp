#include <new>
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
#include "xdb_broker_common.h"

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
#include "dat/broker_db.hpp"
#include "xdb_broker.hpp"
#include "xdb_broker_impl.hpp"
#include "xdb_broker_service.hpp"
#include "xdb_broker_worker.hpp"
#include "xdb_broker_letter.hpp"

using namespace xdb;

const unsigned int DATABASE_SIZE = 1024 * 1024 * 1;  // 1Mб
const unsigned int MEMORY_PAGE_SIZE = DATABASE_SIZE; // Вся БД поместится в одной странице ОЗУ 
const unsigned int MAP_ADDRESS = 0x20000000;

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
#include "dat/broker_db.h"
#include "dat/broker_db.hpp"


DatabaseBrokerImpl::DatabaseBrokerImpl(DatabaseBroker *self) : 
    m_initialized(false),
    m_save_to_xml_feature(false),
    m_snapshot_counter(0),
    m_self(self),
    m_service_list(NULL)
#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
    , m_metadict_initialized(false)
#endif
{
  assert(self);

#ifdef DISK_DATABASE
  const char* name = m_self->DatabaseName();

  m_dbsFileName = new char[strlen(name) + 5];
  m_logFileName = new char[strlen(name) + 5];

  strcpy(m_dbsFileName, name);
  strcat(m_dbsFileName, ".dbs");

  strcpy(m_logFileName, name);
  strcat(m_logFileName, ".log");
#endif
  ((Database*)m_self)->TransitionToState(Database::UNINITIALIZED);
}

DatabaseBrokerImpl::~DatabaseBrokerImpl()
{
  MCO_RET rc;
#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
  int ret;
#endif
  Database::DBState state = ((Database*)m_self)->State();

  LOG(INFO) << "Current state "  << (int)state;
  switch (state)
  {
    case Database::DELETED:
      LOG(WARNING) << "State already DELETED";
    break;

    case Database::UNINITIALIZED:
    break;

    case Database::CONNECTED:
      Disconnect();
      // NB: break пропущен специально!
    case Database::DISCONNECTED:
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
      ((Database*)m_self)->TransitionToState(Database::DELETED);
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
bool DatabaseBrokerImpl::Init()
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
      status = ((Database*)m_self)->TransitionToState(Database::DISCONNECTED);
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
            ((Database*)m_self)->DatabaseName(),
            broker_db_get_dictionary(), NULL);
      if (rc)
        LOG(INFO) << "mco_metadict_register=" << rc;
    }
#endif

    return status;
}


/* NB: Сначала Инициализация (Init), потом Подключение (Connect) */
bool DatabaseBrokerImpl::Connect()
{
  bool status = Init();

  switch (((Database*)m_self)->State())
  {
    case Database::UNINITIALIZED:
      LOG(WARNING) << "Try connection to uninitialized database " 
        << ((Database*)m_self)->DatabaseName();
    break;

    case Database::CONNECTED:
      LOG(WARNING) << "Try to re-open database "
        << ((Database*)m_self)->DatabaseName();
    break;

    case Database::DISCONNECTED:
        status = AttachToInstance();
    break;

    default:
      LOG(WARNING) << "Try to open database '" 
         << ((Database*)m_self)->DatabaseName()
         << "' with unknown state " << (int)((Database*)m_self)->State();
    break;
  }

  return status;
}

bool DatabaseBrokerImpl::Disconnect()
{
  MCO_RET rc = MCO_S_OK;
  Database::DBState state = ((Database*)m_self)->State();

  switch (state)
  {
    case Database::UNINITIALIZED:
      LOG(INFO) << "Disconnect from uninitialized state";
    break;

    case Database::DISCONNECTED:
      LOG(INFO) << "Try to disconnect already diconnected database";
    break;

    case Database::CONNECTED:
      assert(m_self);
      mco_async_event_release_all(m_db/*, MCO_EVENT_newService*/);
      rc = mco_db_disconnect(m_db);
//      rc_check("Disconnection", rc);

      rc = mco_db_close(m_self->DatabaseName());
      ((Database*)m_self)->TransitionToState(Database::DISCONNECTED);
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
MCO_RET DatabaseBrokerImpl::new_Service(mco_trans_h /*t*/,
        XDBService *obj,
        MCO_EVENT_TYPE /*et*/,
        void *p)
{
  DatabaseBrokerImpl *self = static_cast<DatabaseBrokerImpl*> (p);
  char name[Service::NAME_MAXLEN + 1];
  MCO_RET rc;
  autoid_t aid;
  Service *service;
  broker_db::XDBService service_instance;
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
MCO_RET DatabaseBrokerImpl::del_Service(mco_trans_h /*t*/,
        XDBService *obj,
        MCO_EVENT_TYPE /*et*/,
        void *p)
{
  DatabaseBrokerImpl *self = static_cast<DatabaseBrokerImpl*> (p);
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

MCO_RET DatabaseBrokerImpl::RegisterEvents()
{
  MCO_RET rc;
  mco_trans_h t;

  do
  {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) LOG(ERROR) << "Starting transaction, rc=" << rc;

    rc = mco_register_newService_handler(t, 
            &DatabaseBrokerImpl::new_Service, 
            (void*)this
//#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
//            , MCO_AFTER_UPDATE
//#endif
            );

    if (rc) LOG(ERROR) << "Registering event on XDBService creation, rc=" << rc;

    rc = mco_register_delService_handler(t, 
            &DatabaseBrokerImpl::del_Service, 
            (void*)this);
    if (rc) LOG(ERROR) << "Registering event on XDBService deletion, rc=" << rc;

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }
  } while(false);

  if (rc)
   mco_trans_rollback(t);

  return rc;
}

bool DatabaseBrokerImpl::AttachToInstance()
{
  MCO_RET rc = MCO_S_OK;

#if EXTREMEDB_VERSION >= 40
  /* setup memory device as a shared named memory region */
  m_dev.assignment = MCO_MEMORY_ASSIGN_DATABASE;
  m_dev.size       = DATABASE_SIZE;
  m_dev.type       = MCO_MEMORY_NAMED; /* DB in shared memory */
  sprintf(m_dev.dev.named.name, "%s-db", ((Database*)m_self)->DatabaseName());
  m_dev.dev.named.flags = 0;
  m_dev.dev.named.hint  = 0;

  mco_db_params_init (&m_db_params);
  m_db_params.db_max_connections = 10;
  /* set page size for in memory part */
  m_db_params.mem_page_size      = static_cast<uint2>(MEMORY_PAGE_SIZE);
  /* set page size for persistent storage */
  m_db_params.disk_page_size     = DB_DISK_PAGE_SIZE;
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
  mco_db_kill(((Database*)m_self)->DatabaseName());

  /* подключиться к базе данных, предполагая что она создана */
  LOG(INFO) << "Attaching to '" << ((Database*)m_self)->DatabaseName() << "' instance";
  rc = mco_db_connect(((Database*)m_self)->DatabaseName(), &m_db);

  /* ошибка - экземпляр базы не найден, попробуем создать её */
  if (MCO_E_NOINSTANCE == rc)
  {
        LOG(INFO) << ((Database*)m_self)->DatabaseName() << " instance not found, create";
        /*
         * TODO: Использование mco_db_open() является запрещенным,
         * начиная с версии 4 и старше
         */

#if EXTREMEDB_VERSION >= 40
        rc = mco_db_open_dev(((Database*)m_self)->DatabaseName(),
                       broker_db_get_dictionary(),
                       &m_dev,
                       1,
                       &m_db_params);
#else
        rc = mco_db_open(((Database*)m_self)->DatabaseName(),
                         broker_db_get_dictionary(),
                         (void*)MAP_ADDRESS,
                         DATABASE_SIZE + DB_DISK_CACHE,
                         PAGESIZE);
#endif
        if (rc)
        {
          LOG(ERROR) << "Can't open DB dictionary '"
                << ((Database*)m_self)->DatabaseName()
                << "', rc=" << rc;
          return false;
        }

#ifdef DISK_DATABASE
        LOG(INFO) << "Opening '" << m_dbsFileName << "' disk database";
        rc = mco_disk_open(((Database*)m_self)->DatabaseName(),
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
        LOG(INFO) << "Connecting to instance " << ((Database*)m_self)->DatabaseName(); 
        rc = mco_db_connect(((Database*)m_self)->DatabaseName(), &m_db);
  }

  /* ошибка создания экземпляра - выход из системы */
  if (rc)
  {
        LOG(ERROR) << "Unable attaching to instance '" 
            << ((Database*)m_self)->DatabaseName() 
            << "' with code " << rc;
        return false;
  }

  RegisterEvents();
//  MCO_TRANS_ISOLATION_LEVEL isolation_level = mco_trans_set_default_isolation_level(m_db, MCO_REPEATABLE_READ);
//  LOG(INFO) << "Change default transaction isolation level to MCO_REPEATABLE_READ, "<<isolation_level;


//  rc_check("Connecting", rc);
#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
    m_hv = 0;
    mcohv_start(&m_hv, m_metadict, 0, 0);
#endif

  return ((Database*)m_self)->TransitionToState(Database::CONNECTED);
}

Service *DatabaseBrokerImpl::AddService(const std::string& name)
{
  return AddService(name.c_str());
}

Service *DatabaseBrokerImpl::AddService(const char *name)
{
  broker_db::XDBService service_instance;
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
    if (rc) { LOG(ERROR) << "Getting service "<<name<<" id, rc=" << rc; break; }

    srv = new Service(aid, name);
    srv->SetSTATE((Service::State)REGISTERED);

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
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
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
  broker_db::XDBWorker  worker_instance;

  assert(wrk);
  const char* identity = wrk->GetIDENTITY();

  do
  {
      rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
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
  broker_db::timer_mark xdb_next_heartbeat_time;
  autoid_t      srv_aid;
  autoid_t      wrk_aid;

  assert(wrk);
  const char* wrk_identity = wrk->GetIDENTITY();

  do
  {
      rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
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
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
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
  char          ident[WORKER_IDENTITY_MAXLEN + 1];
  broker_db::timer_mark  xdb_expire_time;
  timer_mark_t  expire_time = {0, 0};
  uint4         timer_value;
  WorkerState   state;
  autoid_t      wrk_aid;
  autoid_t      srv_aid;

  do
  {
    rc = wrk_instance.autoid_get(wrk_aid);
    if (rc) { LOG(ERROR) << "Unable to get worker id"; break; }

    rc = wrk_instance.identity_get(ident, (uint2)WORKER_IDENTITY_MAXLEN);
    if (rc) { LOG(ERROR) << "Unable to get identity for worker id "<<wrk_aid; break; }

    rc = wrk_instance.service_ref_get(srv_aid);
    if (rc) { LOG(ERROR) << "Unable to get service id for worker "<<ident; break; }

    ident[WORKER_IDENTITY_MAXLEN] = '\0';
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

    worker = new Worker(srv_aid, ident, wrk_aid);
    worker->SetSTATE((Worker::State)state);
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
 * Вернуть ближайший свободный Обработчик в состоянии ARMED.
 * Побочные эффекты: создается экземпляр Worker, его удаляет вызвавшая сторона
 *
 * TODO: возвращать следует наиболее "старый" экземпляр, временная отметка 
 * которого раньше всех остальных экземпляров с этим же состоянием.
 */
Worker *DatabaseBrokerImpl::PopWorker(const Service *srv)
{
  Worker       *worker = NULL;
  autoid_t      service_aid;

  assert(srv);
  service_aid = const_cast<Service*>(srv)->GetID();
  worker = GetWorkerByState(service_aid, ARMED);

  return worker;
}

Worker *DatabaseBrokerImpl::PopWorker(const std::string& service_name)
{
  Service      *service = NULL;
  Worker       *worker = NULL;

  if (NULL != (service = GetServiceByName(service_name)))
  {
    autoid_t  service_aid = service->GetID();
    delete service;
    worker = GetWorkerByState(service_aid, ARMED);
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
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
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
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
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
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
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


bool DatabaseBrokerImpl::ClearWorkersForService(const char *name)
{
    /* TODO: Очистить спул Обработчиков указанной Службы */

    /*
     * Удалить все незавершенные Сообщения, привязанные к Обработчикам Сервиса
     * Удалить всех Обработчиков Сервиса
     */
    assert(name);
    return false;
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
    if (true == (status = ClearWorkersForService(srv->GetNAME())))
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
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
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
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
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
  char         reply_buffer[WORKER_IDENTITY_MAXLEN + 1];
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
      worker_instance.identity_get(reply_buffer, (uint2)WORKER_IDENTITY_MAXLEN);
      if (rc) { LOG(ERROR) << "Unable to get identity for worker id="<<worker_aid<<", rc="<<rc; break; }
    }
#else
    rc = letter_instance.origin_get(reply_buffer, (uint2)WORKER_IDENTITY_MAXLEN);
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
      letter->SetSTATE((Letter::State)state);
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
  broker_db::XDBLetter letter_instance;
  Letter      *letter = NULL;
  char        *header_buffer = NULL;
  char        *body_buffer = NULL;
  int          cmp_result = 0;

  assert(srv);

  do
  {
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
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
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
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
    rc = letter_instance.state_put((LetterState)ASSIGNED);
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
    rc = worker_instance.state_put((WorkerState)OCCUPIED);
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
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR)<<"Starting transaction, rc="<<rc; break; }

    rc = broker_db::XDBLetter::SK_by_worker_id::find(t, worker->GetID(), letter_instance);
    if (rc)
    {
      LOG(ERROR) << "Unable to find letter assigned to worker "<<worker->GetIDENTITY();
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
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
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
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
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

  assert(worker);
  if (!worker)
    return false;

  const char* ident = worker->GetIDENTITY();

  do
  {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
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
    rc = worker_instance.state_put((WorkerState)new_state);
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
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
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

#if defined DEBUG
/* Тестовый API сохранения базы */
void DatabaseBrokerImpl::MakeSnapshot(const char* msg)
{
  static char file_name[50];

  if (false == m_initialized)
  {
    m_snapshot_counter = 0;
    strcpy(m_snapshot_file_prefix, "snap");
    m_initialized = true;
  }

  sprintf(file_name, "%s.%03d.%s",
          m_snapshot_file_prefix,
          ++m_snapshot_counter,
          (NULL == msg)? "xdb" : msg);

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

MCO_RET DatabaseBrokerImpl::SaveDbToFile(const char* fname)
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
         rc = mco_trans_commit(t);
         if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }
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
void DatabaseBrokerImpl::MakeSnapshot(const char*)
{
  return;
}
#endif

ServiceList* DatabaseBrokerImpl::GetServiceList()
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


ServiceListImpl::ServiceListImpl(mco_db_h _db) :
    m_current_index(0),
    m_db(_db),
    m_size(0)
{
  m_array = new Service*[MAX_SERVICES_ENTRY];
  memset(m_array, '\0', sizeof(m_array)); // NB: подразумевается что 0 и NULL равны
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
int ServiceListImpl::size() const
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
  
//  LOG(INFO) << "EVENT AddService '"<<srv->GetNAME()<<"' id="<<srv->GetID();
  return true;
}

// Добавить новый Сервис, определенный объектом Service
bool ServiceListImpl::AddService(const Service* service)
{
  assert(service);
  Service *srv = const_cast<Service*>(service);
  if (m_size >= MAX_SERVICES_ENTRY)
    return false;

//  LOG(INFO) << "EVENT AddService '"<<srv->GetNAME()<<"' id="<<srv->GetID();
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
  LOG(INFO) << "EVENT DelService with id="<<id;
  return false;
}

// Перечитать список Сервисов из базы данных
bool ServiceListImpl::refresh()
{
  bool status = false;
  mco_trans_h t;
  MCO_RET rc = MCO_S_OK;
  mco_cursor_t csr;
  broker_db::XDBService service_instance;
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

      rc = service_instance.name_get(name, (uint2)Service::NAME_MAXLEN);
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


