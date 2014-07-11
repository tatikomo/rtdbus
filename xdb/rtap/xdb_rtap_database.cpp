#include <assert.h>
#include <bitset>
#include <stdio.h> // snprintf
#include <stdlib.h> // exit

#include "glog/logging.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"
#include "xdb_rtap_common.h"

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

#include "xdb_core_common.h"
#include "xdb_core_base.hpp"
#include "xdb_rtap_database.hpp"
#include "dat/rtap_db.hpp"

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

void impl_errhandler(MCO_RET n)
{
    fprintf(stdout, "\neXtremeDB runtime fatal error: %d\n", n);
    exit(-1);
}

/* implement error handler */
void extended_impl_errhandler(MCO_RET errcode, const char* file, int line)
{
  fprintf(stdout, "\neXtremeDB runtime fatal error: %d on line %d of file %s",
          errcode, line, file);
  exit(-1);
}


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
  m_db_access_flags(_o_flags),
  m_initialized(false),
  m_save_to_xml_feature(false),
  m_snapshot_counter(0)
{
  m_db_params = new mco_db_params_t;
  m_dev = new mco_device_t;
#if USE_EXTREMEDB_HTTP_SERVER
  m_metadict = new mco_metadict_header_t;
#else
  m_metadict = NULL;
#endif

  m_impl = new Database(_name);
  m_impl->TransitionToState(DB_STATE_UNINITIALIZED);

  LOG(INFO) << "Constructor database " << m_impl->DatabaseName();
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
  DBState state = m_impl->State();

  LOG(INFO) << "Current state " << (int)state;
  switch (state)
  {
    case DB_STATE_CLOSED:
      LOG(WARNING) << "State already CLOSED";
    break;

    case DB_STATE_UNINITIALIZED:
    break;

    case DB_STATE_CONNECTED:
      // перед завершением работы сделать снапшот
      StoreSnapshot(NULL);
      // NB: break пропущен специально!
    case DB_STATE_ATTACHED:
      Disconnect();
      // NB: break пропущен специально!
    case DB_STATE_INITIALIZED:
      // NB: break пропущен специально!
    case DB_STATE_DISCONNECTED:
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
      m_impl->TransitionToState(DB_STATE_CLOSED);
    break;
  }

  mco_db_kill(m_impl->DatabaseName());

#ifdef DISK_DATABASE
  delete []m_dbsFileName;
  delete []m_logFileName;
#endif

  delete static_cast<mco_db_params_t*>(m_db_params);
  delete static_cast<mco_device_t*>(m_dev);
  delete static_cast<mco_metadict_header_t*>(m_metadict);

  LOG(INFO) << "Destructor database " << m_impl->DatabaseName();
}

/* NB: Сначала Инициализация runtime (Init), потом Подключение (Connect) */
const Error& RtCoreDatabase::Init()
{
    MCO_RET rc;
    mco_runtime_info_t info;

    m_impl->clearError();
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
      m_impl->setError(rtE_RUNTIME_FATAL);
      return m_impl->getLastError();
    }

    /* Set the error handler to be called from the eXtremeDB runtime if a fatal error occurs */
    mco_error_set_handler(&impl_errhandler);
//    mco_error_set_handler_ex(&extended_impl_errhandler);
    //LOG(INFO) << "User-defined error handler set";
    
    rc = mco_runtime_start();
    if (rc)
    {
      m_impl->TransitionToState(DB_STATE_UNINITIALIZED);
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
              m_impl->DatabaseName(),
              rtap_db_get_dictionary(), NULL);
        if (rc) LOG(INFO) << "mco_metadict_register=" << rc;
      }
#endif

      m_impl->TransitionToState(DB_STATE_INITIALIZED);
    }

    return m_impl->getLastError();
}

const Error& RtCoreDatabase::Connect()
{
  m_impl->clearError();

  if (DB_STATE_INITIALIZED != m_impl->State())
    Init();

  switch (m_impl->State())
  {
    case DB_STATE_UNINITIALIZED:
      LOG(WARNING)<<"Try to connect to uninitialized database "<<m_impl->DatabaseName();
    break;

    case DB_STATE_ATTACHED:
    case DB_STATE_CONNECTED:
      LOG(WARNING)<<"Try to re-open database "<<m_impl->DatabaseName();
    break;

    case DB_STATE_INITIALIZED:
        ConnectToInstance();
        // переход в состояние ATTACHED
    break;

    case DB_STATE_DISCONNECTED:
      LOG(WARNING)<<"Try to connect to disconnected database "<<m_impl->DatabaseName();
    break;

    default:
      LOG(WARNING) << "Try to open database '" << m_impl->DatabaseName()
         << "' with unknown state " << (int)m_impl->State();
    break;
  }

  return m_impl->getLastError();
}

// Подключиться к базе данных.
// NB: Нельзя открывать БД, это выполняется классом RtConnection
//
// Если указан флаг O_TRUNCATE:
//  - попробовать удалить предыдующий экземпляр mco_db_kill()
//  - создать новый экземпляр mco_db_open()
// Если указан флаг O_CREATE
//  - при ошибке подключения к БД попробовать создать новый экземпляр
const Error& RtCoreDatabase::ConnectToInstance()
{
  MCO_RET rc = MCO_S_OK;

  m_impl->clearError();

  // Создание нового экземпляра БД (mco_db_open)
  if ((Create()).Ok())
  {
    LOG(ERROR)<<"Unable attach to database "<<m_impl->DatabaseName();
    return m_impl->getLastError();
  }

  LOG(INFO) << "Connecting to instance " << m_impl->DatabaseName(); 
  rc = mco_db_connect(m_impl->DatabaseName(), static_cast<mco_db_h*>(m_db));
  if (rc)
  {
    // экземпляр базы не найден, и допускается его создание
    if ((MCO_E_NOINSTANCE == rc) && (true == m_db_access_flags.test(OF_POS_CREATE)))
    {
      // Требуется создать экземпляр, если его еще нет
      Create();
    }
    else
    {
      LOG(ERROR) << "Unable connect to " << m_impl->DatabaseName() << ", rc="<<rc;
      m_impl->TransitionToState(DB_STATE_DISCONNECTED);
    }
  }
  else
  {
     m_impl->TransitionToState(DB_STATE_ATTACHED);

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
       rc = mco_db_clean(static_cast<mco_db_h>(m_db));
       if (rc)
       {
         LOG(ERROR) << "Unable truncate database "<<m_impl->DatabaseName();
       }
      // mco_db_kill(m_impl->DatabaseName());
    }
 
    if (!rc)
    {
      RegisterEvents();
#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
      m_hv = 0;
      mcohv_start(&m_hv, m_metadict, 0, 0);
#endif
    }
  }
  return m_impl->getLastError();
}

const Error& RtCoreDatabase::StoreSnapshot(const char* given_file_name)
{
  FILE* fbak;
  MCO_RET rc = MCO_S_OK;
  char fname[40];

  m_impl->clearError();
#if EXTREMEDB_VERSION >= 40
  do
  {
    if (given_file_name)
    {
      if (strlen(given_file_name) > sizeof(fname))
      {
        LOG(ERROR) << "Given file name for XML snapshot storing is exceed limits ("<<sizeof(fname)<<")";
      }
      snprintf(fname, sizeof(fname), "%s.snap", given_file_name);
    }
    else
    {
      snprintf(fname, sizeof(fname), "%s.snap", m_impl->DatabaseName());
    }
    fname[sizeof(fname)] = '\0';

    /* Backup database */
    if (NULL != (fbak = fopen(fname, "wb")))
    {
      rc = mco_inmem_save((void *)fbak, file_writer, static_cast<mco_db_h>(m_db));
      if (rc)
      {
        LOG(ERROR) << "Unable to save "<<m_impl->DatabaseName()<<" snapshot into "<<fname;
      }
      fclose(fbak);
    }
    else
    {
      LOG(ERROR) << "Can't open output file for streaming";
      m_impl->setError(rtE_SNAPSHOT_WRITE);
    }
  } while(false);
#else
#warning "RtCoreDatabase::StoreSnapshot is disabled"
#endif

  return m_impl->getLastError();
}

const Error& RtCoreDatabase::Disconnect()
{
  MCO_RET rc = MCO_S_OK;
  DBState state = m_impl->State();

  m_impl->clearError();

  switch (state)
  {
    case DB_STATE_UNINITIALIZED:
      LOG(INFO) << "Disconnect from uninitialized state";
    break;

    case DB_STATE_DISCONNECTED:
      LOG(INFO) << "Try to disconnect already diconnected database";
    break;

    case DB_STATE_CONNECTED: // база подключена и открыта
      rc = mco_async_event_release_all(static_cast<mco_db_h>(m_db));
      if (rc != MCO_S_OK && rc != MCO_S_EVENT_RELEASED)
      {
        LOG(ERROR)<<"Unable to release "<<m_impl->DatabaseName()<<" events, rc="<<rc;
      }

      rc = mco_db_disconnect(static_cast<mco_db_h>(m_db));
      if (rc) { LOG(ERROR)<<"Unable to disconnect from "<<m_impl->DatabaseName()<<", rc="<<rc; }

      // NB: break пропущен специально
    case DB_STATE_ATTACHED:  // база подключена
      rc = mco_db_close(m_impl->DatabaseName());
      if (rc) { LOG(ERROR)<<"Unable to close "<<m_impl->DatabaseName()<<", rc="<<rc; }

      m_impl->TransitionToState(DB_STATE_DISCONNECTED);
    break;

    default:
      LOG(INFO) << "Disconnect from unknown state:" << state;
  }

  if (rc)
    m_impl->setError(rtE_DB_NOT_DISCONNECTED);

  return m_impl->getLastError();
}

// NB: нужно выполнить до mco_db_connect
const Error& RtCoreDatabase::LoadSnapshot(const char *given_file_name)
{
  MCO_RET rc = MCO_S_OK;
  FILE* fbak;
  char fname[40];

  m_impl->clearError();

#if EXTREMEDB_VERSION >= 40
  do
  {
    // TODO: Очистить предыдущее содержимое БД, если оно было
    rc = MCO_S_OK;

    if (rc)
    {
      LOG(ERROR) << "Unable to clean '"<<m_impl->DatabaseName()<<"' database content";
      m_impl->setError(rtE_RUNTIME_FATAL);
      break;
    }

    if (given_file_name)
    {
      if (strlen(given_file_name) > sizeof(fname))
      {
        LOG(ERROR) << "Given file name for XML snapshot storing is exceed limits ("<<sizeof(fname)<<")";
      }
      strncpy(fname, given_file_name, sizeof(fname));
      fname[sizeof(fname)] = '\0';
    }
    else
    {
      strcpy(fname, m_impl->DatabaseName());
      strcat(fname, ".snap");
    }

    if (NULL == (fbak = fopen(fname, "rb")))
    {
      LOG(ERROR) << "Unable to open snapshot file "<<fname;
      m_impl->setError(rtE_SNAPSHOT_READ);
      break;
    }

    rc = mco_inmem_load((void *)fbak,
                        file_reader,
                        m_impl->DatabaseName(),
                        rtap_db_get_dictionary(),
                        static_cast<mco_device_t*>(m_dev),
                        1,
                        static_cast<mco_db_params_t*>(m_db_params));
    if (rc)
    {
      LOG(ERROR) << "Unable to read data from snapshot file, rc="<<rc;
      m_impl->setError(rtE_SNAPSHOT_READ);
    }
    fclose(fbak);

  } while (false);
#endif

  return m_impl->getLastError();
}


// Создать базу данных с помощью mco_db_open
const Error& RtCoreDatabase::Create()
{
  MCO_RET rc = MCO_S_OK;
  mco_device_t* p_m_dev = static_cast<mco_device_t*>(m_dev);
  mco_db_params_t* p_m_db_params = static_cast<mco_db_params_t*>(m_db_params);
  bool is_loaded_from_snapshot = false;

  m_impl->clearError();

#if EXTREMEDB_VERSION >= 40
  /* setup memory device as a shared named memory region */
  p_m_dev->assignment = MCO_MEMORY_ASSIGN_DATABASE;
  p_m_dev->size       = DatabaseSize;
  p_m_dev->type       = MCO_MEMORY_NAMED; /* DB in shared memory */
  sprintf(p_m_dev->dev.named.name, "%s-db", m_impl->DatabaseName());
  p_m_dev->dev.named.flags = 0;
  p_m_dev->dev.named.hint  = 0;

  mco_db_params_init (p_m_db_params);
  p_m_db_params->db_max_connections = 10;
  /* set page size for in memory part */
  p_m_db_params->mem_page_size      = static_cast<uint2>(MemoryPageSize);
  /* set page size for persistent storage */
  p_m_db_params->disk_page_size     = DbDiskPageSize;
# if USE_EXTREMEDB_HTTP_SERVER
  rc = mco_uda_db_open(m_metadict,
                       0,
                       p_m_dev,
                       1,
                       p_m_db_params,
                       NULL,
                       0);
  if (rc)
    LOG(ERROR) << "Unable to open UDA, rc=" << rc;
# endif

  if (true == m_db_access_flags.test(OF_POS_LOAD_SNAP))
  {
    // Внутри mco_inmem_load вызывается mco_db_open_dev
    if ((LoadSnapshot()).Ok())
    {
      LOG(ERROR) << "Unable to restore content from snapshot";
      is_loaded_from_snapshot = false;
    }
  }

  // База данных не была ранее открыта
  if (false == is_loaded_from_snapshot)
  {
    rc = mco_db_open_dev(m_impl->DatabaseName(),
                       rtap_db_get_dictionary(),
                       p_m_dev,
                       1,
                       p_m_db_params);
  }

#else /* EXTREMEDB_VERSION >= 40 */

   rc = mco_db_open(m_impl->DatabaseName(),
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
       LOG(INFO) << "Database '"<<m_impl->DatabaseName()<<"' already created";
     }
     else
     {
       LOG(ERROR) << "Can't open DB dictionary '" << m_impl->DatabaseName()
                  << "', rc=" << rc;
       m_impl->setError(rtE_RUNTIME_FATAL);
       return m_impl->getLastError();
     }
   }

#ifdef DISK_DATABASE
   LOG(INFO) << "Opening '" << m_dbsFileName << "' disk database";
   rc = mco_disk_open(m_impl->DatabaseName(),
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
     m_impl->setError(rtE_RUNTIME_FATAL);
     return m_impl->getLastError();
   }
#endif
  return m_impl->TransitionToState(DB_STATE_ATTACHED);
}

const Error& RtCoreDatabase::RegisterEvents()
{
  MCO_RET rc;
  mco_trans_h t;

  m_impl->clearError();

  do
  {
    rc = mco_trans_start(static_cast<mco_db_h>(m_db), 
                         MCO_READ_WRITE, 
                         MCO_TRANS_FOREGROUND,
                         &t);
    if (rc)
    {
      LOG(ERROR) << "Starting transaction, rc=" << rc;
      m_impl->setError(rtE_RUNTIME_FATAL);
      break;
    }

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
    if (rc)
    {
      LOG(ERROR) << "Commitment transaction, rc=" << rc;
      m_impl->setError(rtE_RUNTIME_WARNING);
    }

  } while(false);

  if (rc)
    mco_trans_rollback(t);

  return m_impl->getLastError();
}

