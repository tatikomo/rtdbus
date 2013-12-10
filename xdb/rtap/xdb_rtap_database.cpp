#include <assert.h>
#include <bitset>
#include <stdio.h> // snprintf

#include "glog/logging.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"
#include "xdb_rtap_common.h"
#include "xdb_core_common.h"

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
#include "xdb_core_base.hpp"
#include "xdb_rtap_application.hpp"
#include "xdb_rtap_database.hpp"
#include "xdb_rtap_connection.hpp"

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


/* Stream writer with prototype mco_stream_write */
mco_size_sig_t file_writer( void *stream_handle /* FILE *  */, const void *from, mco_size_t nbytes )
{
  FILE *f = (FILE *)stream_handle;
  mco_size_sig_t nbs;

  nbs = fwrite( from, 1, nbytes, f );
  return nbs;
}

/* Stream reader with prototype mco_stream_read */
mco_size_sig_t file_reader( void *stream_handle /* FILE *  */,  /* OUT */void *to, mco_size_t max_nbytes )
{
  FILE *f = (FILE *)stream_handle;
  mco_size_sig_t nbs;

  nbs = fread( to, 1, max_nbytes, f );
  return nbs;
}


RtCoreDatabase::RtCoreDatabase(const char* _name, BitSet8 _o_flags) :
  Database(_name),
  m_db_access_flags(_o_flags),
  m_initialized(false),
  m_save_to_xml_feature(false),
  m_snapshot_counter(0)
{
  TransitionToState(Database::UNINITIALIZED);

  LOG(INFO) << "Constructor database " << DatabaseName();
  // TODO: Проверить флаги открытия базы данных
  // TRUNCATE и LOAD_SNAP не могут быть установленными одновременно
  if (m_db_access_flags.test(OF_POS_CREATE))
    LOG(INFO) << "Creates database if it not exist";
  if (m_db_access_flags.test(OF_POS_TRUNCATE))
    LOG(INFO) << "Truncate database";
  if (m_db_access_flags.test(OF_POS_RDWR))
    LOG(INFO) << "Mount database in read/write mode";
  if (m_db_access_flags.test(OF_POS_READONLY))
    LOG(INFO) << "Mount database in read-only mode";
  if (m_db_access_flags.test(OF_POS_LOAD_SNAP))
    LOG(INFO) << "Load database' content from snapshot";
}

RtCoreDatabase::~RtCoreDatabase()
{
  MCO_RET rc;
#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
  int ret;
#endif
  Database::DBState state = State();

  LOG(INFO) << "Current state " << (int)state;
  switch (state)
  {
    case Database::CLOSED:
      LOG(WARNING) << "State already CLOSED";
    break;

    case Database::UNINITIALIZED:
    break;

    case Database::CONNECTED:
      // перед завершением работы сделать снапшот
      MakeSnapshot();
    case Database::ATTACHED:
      Disconnect();
      // NB: break пропущен специально!
    case Database::INITIALIZED:
      // NB: break пропущен специально!
    case Database::DISCONNECTED:
#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
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
      TransitionToState(Database::CLOSED);
    break;
  }

  mco_db_kill(DatabaseName());

#ifdef DISK_DATABASE
  delete []m_dbsFileName;
  delete []m_logFileName;
#endif

  LOG(INFO) << "Destructor database " << DatabaseName();
}

/* NB: Сначала Инициализация runtime (Init), потом Подключение (Connect) */
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
    if (rc)
    {
      status = TransitionToState(Database::UNINITIALIZED);
    }
    else
    {

#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
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
        if (rc) LOG(INFO) << "mco_metadict_register=" << rc;
      }
#endif

      status = TransitionToState(Database::INITIALIZED);
    }

    return status;
}

bool RtCoreDatabase::Connect()
{
  bool status;

  if (INITIALIZED != State())
    status = Init();

  switch (State())
  {
    case Database::UNINITIALIZED:
      LOG(WARNING)<<"Try to connect to uninitialized database "<<DatabaseName();
    break;

    case Database::ATTACHED:
    case Database::CONNECTED:
      LOG(WARNING)<<"Try to re-open database "<<DatabaseName();
    break;

    case Database::INITIALIZED:
        status = ConnectToInstance();
        // переход в состояние ATTACHED
    break;

    case Database::DISCONNECTED:
      LOG(WARNING)<<"Try to connect to disconnected database "<<DatabaseName();
    break;

    default:
      LOG(WARNING) << "Try to open database '" << DatabaseName()
         << "' with unknown state " << (int)State();
    break;
  }

  return status;
}

// Подключиться к базе данных.
// NB: Нельзя открывать БД, это выполняется классом RtDbConnection
//
// Если указан флаг O_TRUNCATE:
//  - попробовать удалить предыдующий экземпляр mco_db_kill()
//  - создать новый экземпляр mco_db_open()
// Если указан флаг O_CREATE
//  - при ошибке подключения к БД попробовать создать новый экземпляр
bool RtCoreDatabase::ConnectToInstance()
{
  MCO_RET rc = MCO_S_OK;
  bool status = false;

  // Создание нового экземпляра БД (mco_db_open)
  if (false == (status = Create()))
  {
    LOG(ERROR)<<"Unable attach to database "<<DatabaseName();
    return status;
  }

  LOG(INFO) << "Connecting to instance " << DatabaseName(); 
  rc = mco_db_connect(DatabaseName(), &m_db);
  if (rc)
  {
    // экземпляр базы не найден, и допускается его создание
    if ((MCO_E_NOINSTANCE == rc) && (true == m_db_access_flags.test(OF_POS_CREATE)))
    {
      // Требуется создать экземпляр, если его еще нет
      status = Create();
    }
    else
    {
      LOG(ERROR) << "Unable connect to " << DatabaseName() << ", rc="<<rc;
      if (false == TransitionToState(Database::DISCONNECTED))
      {
        LOG(WARNING) << "Unable transition from "<< State() << " to DISCONECTED";
      }
      status = false;
    }
  }
  else
  {
     // Если требуется удалить старый экземпляр БД
     if (true == m_db_access_flags.test(OF_POS_TRUNCATE))
     {
       /*
        * NB: предварительное удаление экземпляра БД делает бесполезной 
        * последующую попытку подключения к ней.
        * Очистка оставлена в качестве временной меры. В дальнейшем 
        * уже созданный экземпляр БД может использоваться в качестве 
        * persistent-хранилища после аварийного завершения.
        */
       rc = mco_db_clean(m_db);
       if (rc)
       {
         LOG(ERROR) << "Unable truncate database "<<DatabaseName();
       }
      // mco_db_kill(DatabaseName());
    }
 
    if (false == (status = TransitionToState(Database::CONNECTED)))
    {
      LOG(WARNING) << "Unable transition from "<< State() << " to CONNECTED";
    }
    else
    {
      RegisterEvents();
#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
      m_hv = 0;
      mcohv_start(&m_hv, m_metadict, 0, 0);
#endif
    }
  }
  return status;
}

bool RtCoreDatabase::MakeSnapshot()
{
  FILE* fbak;
  bool status = false;
  MCO_RET rc = MCO_S_OK;
  char fname[20];

#if EXTREMEDB_VERSION >= 40
  do
  {
    strcpy(fname, DatabaseName());
    strcat(fname, ".snap");

    /* Backup database */
    if (NULL != (fbak = fopen(fname, "wb")))
    {
      rc = mco_inmem_save((void *)fbak, file_writer, m_db);
      if (rc)
      {
        LOG(ERROR) << "Unable to save "<<DatabaseName()<<" snapshot into "<<fname;
      }
      fclose(fbak);
      status = true;
    }
    else
    {
      LOG(ERROR) << "Can't open output file for streaming";
      status = false;
    }
  } while(false);
#else
#warning "RtCoreDatabase::MakeSnapshot is disabled"
#endif

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

    case Database::CONNECTED: // база подключена и открыта
      rc = mco_async_event_release_all(m_db);
      if (rc != MCO_S_OK && rc != MCO_S_EVENT_RELEASED)
      {
        LOG(ERROR)<<"Unable to release "<<DatabaseName()<<" events, rc="<<rc;
      }

      rc = mco_db_disconnect(m_db);
      if (rc) { LOG(ERROR)<<"Unable to disconnect from "<<DatabaseName()<<", rc="<<rc; }

      // NB: break пропущен специально
    case Database::ATTACHED:  // база подключена
      rc = mco_db_close(DatabaseName());
      if (rc) { LOG(ERROR)<<"Unable to close "<<DatabaseName()<<", rc="<<rc; }

      TransitionToState(Database::DISCONNECTED);
    break;

    default:
      LOG(INFO) << "Disconnect from unknown state:" << state;
  }

  return (!rc)? true : false;
}

// NB: нужно выполнить до mco_db_connect
bool RtCoreDatabase::LoadFromSnapshot()
{
  bool status = false;
  MCO_RET rc = MCO_S_OK;
  FILE* fbak;
  char fname[20];

#if EXTREMEDB_VERSION >= 40
  do
  {
    if (rc)
    {
      LOG(ERROR) << "Unable to clean '"<<DatabaseName()<<"' database content";
      break;
    }

    strcpy(fname, DatabaseName());
    strcat(fname, ".snap");

    if (NULL == (fbak = fopen(fname, "rb")))
    {
      LOG(ERROR) << "Unable to open snapshot file "<<fname;
      break;
    }

    rc = mco_inmem_load((void *)fbak,
                        file_reader,
                        DatabaseName(),
                        rtap_db_get_dictionary(),
                        &m_dev,
                        1,
                        &m_db_params);
    if (rc)
    {
      LOG(ERROR) << "Unable to read data from snapshot file, rc="<<rc;
      status = false;
    }
    else
    {
      status = true;
    }
    fclose(fbak);

  } while (false);
#endif

  return status;
}


// Создать базу данных с помощью mco_db_open
bool RtCoreDatabase::Create()
{
  MCO_RET rc = MCO_S_OK;
  bool is_loaded_from_snapshot = false;

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

  if (true == m_db_access_flags.test(OF_POS_LOAD_SNAP))
  {
    // Внутри mco_inmem_load вызывается mco_db_open_dev
    if (false == LoadFromSnapshot())
    {
      LOG(ERROR) << "Unable to restore content from snapshot";
      is_loaded_from_snapshot = false;
    }
  }

  // База данных не была ранее открыта
  if (false == is_loaded_from_snapshot)
  {
    rc = mco_db_open_dev(DatabaseName(),
                       rtap_db_get_dictionary(),
                       &m_dev,
                       1,
                       &m_db_params);
  }

#else /* EXTREMEDB_VERSION >= 40 */

   rc = mco_db_open(DatabaseName(),
                       rtap_db_get_dictionary(),
                       (void*)MapAddress,
                       DatabaseSize + DbDiskCache,
                       PAGESIZE);

#endif /* EXTREMEDB_VERSION >= 40 */

   if (rc)
   {
     // Если мы пытаемся открыть уже созданную БД,
     // и при этом флаги открытия не говорят, что нам 
     // нужно было ее создавать - это не ошибка
     if (MCO_E_INSTANCE_DUPLICATE == rc)
     {
       LOG(INFO) << "Database '"<<DatabaseName()<<"' already created";
     }
     else
     {
       LOG(ERROR) << "Can't open DB dictionary '"
                << DatabaseName()
                << "', rc=" << rc;
       return false;
     }
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
  return TransitionToState(Database::ATTACHED);
}


MCO_RET RtCoreDatabase::RegisterEvents()
{
  MCO_RET rc;
  mco_trans_h t;

  do
  {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) LOG(ERROR) << "Starting transaction, rc=" << rc;

#if 0
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
#endif

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }
  } while(false);

  if (rc)
   mco_trans_rollback(t);

  return rc;
}
