#include <assert.h>
#include <stdio.h> // snprintf

#include "glog/logging.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"
#include "xdb_rtap_common.h"
#include "xdb_core_common.h"
#include "xdb_core_base.hpp"

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

#include "dat/rtap_db.hpp"
#include "xdb_rtap_application.hpp"
#include "xdb_rtap_database.hpp"

using namespace xdb;

const unsigned int RtCoreDatabase::DatabaseSize = 1024 * 1024 * 10; // 10Mb
// Вся БД поместится в одной странице ОЗУ 
const unsigned int RtCoreDatabase::MemoryPageSize = RtCoreDatabase::DatabaseSize;
const unsigned int MapAddress = 0x20000000;
#ifdef DISK_DATABASE
const int RtCoreDatabase::DbDiskCache = RtCoreDatabase::DatabaseSize;
const int RtCoreDatabase::DbDiskPageSize = 1024;
    #ifndef DB_LOG_TYPE
        #define DB_LOG_TYPE REDO_LOG
    #endif 
#else 
const int RtCoreDatabase::DbDiskCache = 0;
const int RtCoreDatabase::DbDiskPageSize = 0;
#endif 


RtCoreDatabase::RtCoreDatabase(const char* _name) :
  Database(_name),
  m_initialized(false),
  m_save_to_xml_feature(false),
  m_snapshot_counter(0)
{
  TransitionToState(Database::UNINITIALIZED);

  LOG(INFO) << "Constructor database " << DatabaseName();
}

RtCoreDatabase::~RtCoreDatabase()
{
  MCO_RET rc;
#if (EXTREMEDB_VERSION >= 41) && USE_EXTREMEDB_HTTP_SERVER
  int ret;
#endif
  Database::DBState state = State();

  LOG(INFO) << "Current state " << (int)state;
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
    case Database::INITIALIZED:
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
      rc = mco_runtime_stop();
      if (rc)
      {
        LOG(ERROR) << "Unable to stop database runtime, code=" << rc;
      }
      TransitionToState(Database::DELETED);
    break;
  }

  mco_db_kill(DatabaseName());

#ifdef DISK_DATABASE
  delete []m_dbsFileName;
  delete []m_logFileName;
#endif

  LOG(INFO) << "Destructor database " << DatabaseName();
}

/* NB: Сначала Инициализация (Init), потом Подключение (Connect) */
bool RtCoreDatabase::Init()
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
    if (!rc)
    {
      status = TransitionToState(Database::DISCONNECTED);
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
            DatabaseName(),
            rtap_db_get_dictionary(), NULL);
      if (rc)
        LOG(INFO) << "mco_metadict_register=" << rc;
    }
#endif

    return status;
}


/* NB: Сначала Инициализация (Init), потом Подключение (Connect) */
bool RtCoreDatabase::Connect()
{
  bool status = Init();

  switch (State())
  {
    case Database::UNINITIALIZED:
      LOG(WARNING) << "Try connection to uninitialized database " << DatabaseName();
    break;

    case Database::CONNECTED:
      LOG(WARNING) << "Try to re-open database "
        << DatabaseName();
    break;

    case Database::DISCONNECTED:
        status = AttachToInstance();
    break;

    default:
      LOG(WARNING) << "Try to open database '" << DatabaseName()
         << "' with unknown state " << (int)State();
    break;
  }

  return status;
}

bool RtCoreDatabase::Disconnect()
{
  MCO_RET rc = MCO_S_OK;
  Database::DBState state = State();

  switch (state)
  {
    case Database::UNINITIALIZED:
      LOG(INFO) << "Disconnect from uninitialized state";
    break;

    case Database::DISCONNECTED:
      LOG(INFO) << "Try to disconnect already diconnected database";
    break;

    case Database::CONNECTED:
      mco_async_event_release_all(m_db);
      rc = mco_db_disconnect(m_db);

      rc = mco_db_close(DatabaseName());
      TransitionToState(Database::DISCONNECTED);
    break;

    default:
      LOG(INFO) << "Disconnect from unknown state:" << state;
  }

  return (!rc)? true : false;
}

bool RtCoreDatabase::AttachToInstance()
{
  MCO_RET rc = MCO_S_OK;

#if EXTREMEDB_VERSION >= 40
  /* setup memory device as a shared named memory region */
  m_dev.assignment = MCO_MEMORY_ASSIGN_DATABASE;
  m_dev.size       = DatabaseSize;
  m_dev.type       = MCO_MEMORY_NAMED; /* DB in shared memory */
  sprintf(m_dev.dev.named.name, "%s-db", DatabaseName());
  m_dev.dev.named.flags = 0;
  m_dev.dev.named.hint  = 0;

  mco_db_params_init (&m_db_params);
  m_db_params.db_max_connections = 10;
  /* set page size for in memory part */
  m_db_params.mem_page_size      = static_cast<uint2>(MemoryPageSize);
  /* set page size for persistent storage */
  m_db_params.disk_page_size     = DbDiskPageSize;
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
  mco_db_kill(DatabaseName());

  /* подключиться к базе данных, предполагая что она создана */
  LOG(INFO) << "Attaching to '" << DatabaseName() << "' instance";
  rc = mco_db_connect(DatabaseName(), &m_db);

  /* ошибка - экземпляр базы не найден, попробуем создать её */
  if (MCO_E_NOINSTANCE == rc)
  {
        LOG(INFO) << DatabaseName() << " instance not found, create";
        /*
         * TODO: Использование mco_db_open() является запрещенным,
         * начиная с версии 4 и старше
         */

#if EXTREMEDB_VERSION >= 40
        rc = mco_db_open_dev(DatabaseName(),
                       rtap_db_get_dictionary(),
                       &m_dev,
                       1,
                       &m_db_params);
#else
        rc = mco_db_open(DatabaseName(),
                         rtap_db_get_dictionary(),
                         (void*)MapAddress,
                         DatabaseSize + DbDiskCache,
                         PAGESIZE);
#endif
        if (rc)
        {
          LOG(ERROR) << "Can't open DB dictionary '"
                << DatabaseName()
                << "', rc=" << rc;
          return false;
        }

#ifdef DISK_DATABASE
        LOG(INFO) << "Opening '" << m_dbsFileName << "' disk database";
        rc = mco_disk_open(DatabaseName(),
                           m_dbsFileName,
                           m_logFileName, 
                           0, 
                           DbDiskCache, 
                           DbDiskPageSize,
                           MCO_INFINITE_DATABASE_SIZE,
                           DB_LOG_TYPE);

        if (rc != MCO_S_OK && rc != MCO_ERR_DISK_ALREADY_OPENED)
        {
            LOG(ERROR) << "Error creating disk database, rc=" << rc;
            return false;
        }
#endif

        /* подключиться к базе данных, т.к. она только что создана */
        LOG(INFO) << "Connecting to instance " << DatabaseName(); 
        rc = mco_db_connect(DatabaseName(), &m_db);
  }

  /* ошибка создания экземпляра - выход из системы */
  if (rc)
  {
        LOG(ERROR) << "Unable attaching to instance '" 
            << DatabaseName() << "' with code " << rc;
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

  return TransitionToState(Database::CONNECTED);
}

MCO_RET RtCoreDatabase::RegisterEvents()
{
  MCO_RET rc;
  mco_trans_h t;

#if 0
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
#endif

  if (rc)
   mco_trans_rollback(t);

  return rc;
}

