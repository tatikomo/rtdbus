#include <climits> // USHRT_MAX
#include <assert.h>
#include <bitset>
#include <stdio.h> // snprintf
#include <stdlib.h> // exit

#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"
#include "mcouda.h"  // mco_metadict_header_t
#include "mcohv.h"   // mcohv_p

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
#include "xdb_impl_database.hpp"

using namespace xdb;

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


DatabaseImpl::DatabaseImpl(const char* _name, const Options& _options, mco_dictionary_h _dict) :
  m_snapshot_counter(0),
  m_state(DB_STATE_UNINITIALIZED),
  m_last_error(rtE_NONE),
  m_dict(_dict),
  m_db_access_flags(_options),
  m_save_to_xml_feature(false)
{
  int val;

  m_db_params = new mco_db_params_t;
  m_dev = new mco_device_t;
#if USE_EXTREMEDB_HTTP_SERVER
  m_metadict = new mco_metadict_header_t;
#else
  m_metadict = NULL;
#endif
  strncpy(m_name, _name, DBNAME_MAXLEN);
  m_name[DBNAME_MAXLEN] = '\0';

#ifdef DISK_DATABASE
  /* NB: +5 - для ".dbs" и ".log" с завершающим '\0' */
  m_dbsFileName = new char[strlen(m_name) + 5];
  m_logFileName = new char[strlen(m_name) + 5];

  strcpy(m_dbsFileName, m_name);
  strcat(m_dbsFileName, ".dbs");

  strcpy(m_logFileName, m_name);
  strcat(m_logFileName, ".log");
#endif

  LOG(INFO) << "Constructor database " << m_name;
  // TODO: Проверить флаги открытия базы данных
  // TRUNCATE и LOAD_SNAP не могут быть установленными одновременно
  if (getOption(m_db_access_flags,"OF_CREATE", val) && val)
    LOG(INFO) << "Creates database if it not exist";
  if (getOption(m_db_access_flags,"OF_TRUNCATE", val) && val)
    LOG(INFO) << "Truncate database";
  if (getOption(m_db_access_flags,"OF_RDWR", val) && val)
    LOG(INFO) << "Mount database in read/write mode";
  if (getOption(m_db_access_flags,"OF_READONLY", val) && val)
    LOG(INFO) << "Mount database in read-only mode";
  if (getOption(m_db_access_flags,"OF_LOAD_SNAP", val) && val)
    LOG(INFO) << "Load database' content from snapshot";
  if (getOption(m_db_access_flags,"OF_DATABASE_SIZE", val) && val)
  {
    LOG(INFO) << "Change default database size to " << val << " byte(s)";
    m_DatabaseSize = val;
  }
  else m_DatabaseSize = 1024 * 1024 * 10; // По умолчанию размер базы 10Мб

  if (getOption(m_db_access_flags,"OF_MEMORYPAGE_SIZE", val) && val)
  {
    LOG(INFO) << "Change default page size to " << val << " byte(s)";
    if ((0 < val) && (val < USHRT_MAX))
    {
      m_MemoryPageSize = static_cast<uint2>(val);
    }
    else
    {
      LOG(WARNING) << "MEMORYPAGE_SIZE value (" << val
                   << ") is ignored due to overflow limits [0..65535]";
    }
  }
  else
  {
    m_MemoryPageSize = 256;
  }

  if (getOption(m_db_access_flags,"OF_MAP_ADDRESS", val) && val)
  {
    LOG(INFO) << "Change default map address to " << val;
    m_MapAddress = val;
  }
  else m_MapAddress = 0x20000000;

#ifdef DISK_DATABASE
# ifndef DB_LOG_TYPE
#   define DB_LOG_TYPE REDO_LOG
# endif
#endif
  if (getOption(m_db_access_flags, "OF_DISK_CACHE_SIZE", val) && val)
  {
    m_DbDiskCache = val;
    m_DbDiskPageSize = 1024;
    LOG(INFO) << "Change default disk cache size to " << val;
  }
  else
  {
    m_DbDiskCache = 0;
    m_DbDiskPageSize = 0;
  }
}

unsigned int DatabaseImpl::getSnapshotCounter()
{
  return ++m_snapshot_counter;
}

DatabaseImpl::~DatabaseImpl()
{
  MCO_RET rc;
#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
  int ret;
#endif

  LOG(INFO) << "Current state " << State();
  switch (State())
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
      TransitionToState(DB_STATE_CLOSED);
    break;
  }

  mco_db_kill(m_name);

#ifdef DISK_DATABASE
  delete []m_dbsFileName;
  delete []m_logFileName;
#endif

  delete m_db_params;
  delete m_dev;
  delete m_metadict;

  LOG(INFO) << "Destructor database " << m_name;
}

mco_db_h DatabaseImpl::getDbHandler()
{
  return m_db;
}

/* NB: Сначала Инициализация runtime (Init), потом Подключение (Connect) */
const Error& DatabaseImpl::Init()
{
    MCO_RET rc;
    mco_runtime_info_t info;

    clearError();

    if (DB_STATE_UNINITIALIZED != m_state)
    {
      LOG(WARNING) << "Try to re-init database " << m_name;
      return m_last_error;
    }

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
      setError(rtE_RUNTIME_FATAL);
      return getLastError();
    }

    /* Set the error handler to be called from the eXtremeDB runtime if a fatal error occurs */
    mco_error_set_handler(&impl_errhandler);
//    mco_error_set_handler_ex(&extended_impl_errhandler);
    //LOG(INFO) << "User-defined error handler set";
    
    rc = mco_runtime_start();
    if (rc)
    {
      TransitionToState(DB_STATE_UNINITIALIZED);
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
        rc = mco_metadict_register(m_metadict, m_name, m_dict, NULL);
        if (rc) LOG(INFO) << "mco_metadict_register=" << rc;
      }
#endif

      TransitionToState(DB_STATE_INITIALIZED);
    }

    return getLastError();
}

const Error& DatabaseImpl::Connect()
{
  clearError();

  if (DB_STATE_INITIALIZED != State())
    Init();

  switch (State())
  {
    case DB_STATE_UNINITIALIZED:
      LOG(WARNING)<<"Try to connect to uninitialized database "<<m_name;
    break;

    case DB_STATE_ATTACHED:
    case DB_STATE_CONNECTED:
      LOG(WARNING)<<"Try to re-open database "<<m_name;
    break;

    case DB_STATE_INITIALIZED:
        ConnectToInstance();
        // переход в состояние CONNECTED
    break;

    case DB_STATE_DISCONNECTED:
      LOG(WARNING)<<"Try to connect to disconnected database "<<m_name;
    break;

    default:
      LOG(WARNING) << "Try to open database '" << m_name
         << "' with unknown state " << State();
    break;
  }

  return getLastError();
}

// Подключиться к базе данных.
// NB: Нельзя открывать БД, это выполняется классом RtConnection
//
// Если указан флаг O_TRUNCATE:
//  - попробовать удалить предыдующий экземпляр mco_db_kill()
//  - создать новый экземпляр mco_db_open()
// Если указан флаг O_CREATE
//  - при ошибке подключения к БД попробовать создать новый экземпляр
const Error& DatabaseImpl::ConnectToInstance()
{
  MCO_RET rc = MCO_S_OK;
  int opt_val;

  clearError();

  // Создание нового экземпляра БД (mco_db_open)
  if (!(Create()).Ok())
  {
    LOG(ERROR)<<"Unable attach to database "<<m_name;
    return getLastError();
  }

  // База в состоянии ATTACHED

  LOG(INFO) << "Connecting to instance " << m_name; 
  rc = mco_db_connect(m_name, &m_db);
  if (rc)
  {
    // экземпляр базы не найден, и допускается его создание
    if ((MCO_E_NOINSTANCE == rc)
     && ((true == getOption(m_db_access_flags, "OF_CREATE", opt_val)) && opt_val))
    {
      // Требуется создать экземпляр, если его еще нет
      Create();
    }
    else
    {
      LOG(ERROR) << "Unable connect to " << m_name << ", rc="<<rc;
      TransitionToState(DB_STATE_DISCONNECTED);
    }
  }
  else
  {
     TransitionToState(DB_STATE_CONNECTED);

     // Если требуется удалить старый экземпляр БД
     if (true == getOption(m_db_access_flags, "OF_TRUNCATE", opt_val) && opt_val)
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
         LOG(ERROR) << "Unable truncate database "<<m_name;
       }
       //mco_db_kill(m_name);
    }
 
    if (!rc)
    {
      RegisterEvents();
#if 0
      MCO_TRANS_ISOLATION_LEVEL isolation_level = 
            mco_trans_set_default_isolation_level(m_db, MCO_REPEATABLE_READ);
      LOG(INFO) << "Change default transaction isolation level to MCO_REPEATABLE_READ, "<<isolation_level;
#endif

#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
      m_hv = 0;
      mcohv_start(&m_hv, m_metadict, 0, 0);
#endif
    }
  }
  return getLastError();
}

const Error& DatabaseImpl::SaveAsXML(const char* given_file_name, const char *msg)
{
  MCO_RET rc = MCO_S_OK;
  mco_xml_policy_t op, np;
  mco_trans_h t;
  // TODO: возможно превышение лимита на размер имени сгенерированного файла
  static char calc_file_name[150];
  char *fname;
  FILE* f;

  m_last_error.clear();

  if (false == m_save_to_xml_feature)
  {
    // Отсутствует поддержка сохранения в XML со стороны BD
    m_last_error = rtE_NOT_IMPLEMENTED;
    return m_last_error;
  }

  if (!given_file_name)
  {
    sprintf(calc_file_name, "snap.%09d.%s",
          m_snapshot_counter,
          (NULL == msg)? "xdb" : msg);
  }
  else
  {
    strcpy(calc_file_name, given_file_name);
    if (msg)
    {
      strcat(calc_file_name, ".");
      strncat(calc_file_name, msg, sizeof(calc_file_name)-strlen(calc_file_name)-1);
      calc_file_name[sizeof(calc_file_name) - 1] = '\0';
    }
  }
  fname = calc_file_name;
    
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
      m_last_error.set(rtE_SNAPSHOT_WRITE);
  }

  fclose(f);
  return m_last_error;
} /* ========================================================================= */

const Error& DatabaseImpl::StoreSnapshot(const char* given_file_name)
{
  FILE* fbak;
  MCO_RET rc = MCO_S_OK;
  char fname[40];

  clearError();
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
      snprintf(fname, sizeof(fname), "%s.snap", m_name);
    }
    fname[sizeof(fname)-1] = '\0';

    /* Backup database */
    if (NULL != (fbak = fopen(fname, "wb")))
    {
      rc = mco_inmem_save(static_cast<void*>(fbak), file_writer, m_db);
      if (rc)
      {
        LOG(ERROR) << "Unable to save "<<m_name<<" snapshot into "<<fname;
      }
      fclose(fbak);
    }
    else
    {
      LOG(ERROR) << "Can't open output file for streaming";
      setError(rtE_SNAPSHOT_WRITE);
    }
  } while(false);
#else
#warning "DatabaseImpl::StoreSnapshot is disabled"
#endif

  return getLastError();
}

const Error& DatabaseImpl::Disconnect()
{
  MCO_RET rc = MCO_S_OK;
  DBState_t state = State();

  clearError();

  switch (state)
  {
    case DB_STATE_UNINITIALIZED:
      LOG(INFO) << "Disconnect from uninitialized state";
    break;

    case DB_STATE_DISCONNECTED:
      LOG(INFO) << "Try to disconnect already diconnected database";
    break;

    case DB_STATE_CONNECTED: // база подключена и открыта
      rc = mco_async_event_release_all(m_db);
      if (rc != MCO_S_OK && rc != MCO_S_EVENT_RELEASED)
      {
        LOG(ERROR)<<"Unable to release "<<m_name<<" events, rc="<<rc;
      }

      rc = mco_db_disconnect(m_db);
      if (rc) { LOG(ERROR)<<"Unable to disconnect from "<<m_name<<", rc="<<rc; }

      // NB: break пропущен специально
    case DB_STATE_ATTACHED:  // база подключена
      rc = mco_db_close(m_name);
      if (rc) { LOG(ERROR)<<"Unable to close "<<m_name<<", rc="<<rc; }

      TransitionToState(DB_STATE_DISCONNECTED);
    break;

    default:
      LOG(INFO) << "Disconnect from unknown state:" << state;
  }

  if (rc)
    setError(rtE_DB_NOT_DISCONNECTED);

  return getLastError();
}

// NB: нужно выполнить до mco_db_connect
const Error& DatabaseImpl::LoadSnapshot(const char *given_file_name)
{
  MCO_RET rc = MCO_S_OK;
  FILE* fbak;
  char fname[40];

  clearError();

#if EXTREMEDB_VERSION >= 40
  do
  {
    // TODO: Очистить предыдущее содержимое БД, если оно было
    rc = MCO_S_OK;

    if (rc)
    {
      LOG(ERROR) << "Unable to clean '"<<m_name<<"' database content";
      setError(rtE_RUNTIME_FATAL);
      break;
    }

    if (given_file_name)
    {
      if (strlen(given_file_name) > sizeof(fname))
      {
        LOG(ERROR) << "Given file name for XML snapshot storing is exceed limits ("<<sizeof(fname)<<")";
      }
      strncpy(fname, given_file_name, sizeof(fname));
      fname[sizeof(fname)-1] = '\0';
    }
    else
    {
      strcpy(fname, m_name);
      strcat(fname, ".snap");
    }

    if (NULL == (fbak = fopen(fname, "rb")))
    {
      LOG(ERROR) << "Unable to open snapshot file "<<fname;
      setError(rtE_SNAPSHOT_READ);
      break;
    }

    rc = mco_inmem_load(static_cast<void*>(fbak),
                        file_reader,
                        m_name,
                        m_dict,
                        m_dev,
                        1,
                        m_db_params);
    if (rc)
    {
      LOG(ERROR) << "Unable to read data from snapshot file, rc="<<rc;
      setError(rtE_SNAPSHOT_READ);
    }
    fclose(fbak);

  } while (false);
#endif

  return getLastError();
}


// Создать базу данных с помощью mco_db_open
const Error& DatabaseImpl::Create()
{
  MCO_RET rc = MCO_S_OK;
  int opt_val;
  bool is_loaded_from_snapshot = false;

  clearError();

#if EXTREMEDB_VERSION >= 40
  /* setup memory device as a shared named memory region */
  m_dev->assignment = MCO_MEMORY_ASSIGN_DATABASE;
  m_dev->size       = m_DatabaseSize;
  m_dev->type       = MCO_MEMORY_NAMED; /* DB in shared memory */
  sprintf(m_dev->dev.named.name, "%s-db", m_name);
  m_dev->dev.named.flags = 0;
  m_dev->dev.named.hint  = 0;

  mco_db_params_init (m_db_params);
  m_db_params->db_max_connections = 10;
  /* set page size for in memory part */
  m_db_params->mem_page_size      = m_MemoryPageSize;
  /* set page size for persistent storage */
  m_db_params->disk_page_size     = m_DbDiskPageSize;

# if USE_EXTREMEDB_HTTP_SERVER
  rc = mco_uda_db_open(m_metadict,
                       0,
                       m_dev,
                       1,
                       m_db_params,
                       NULL,
                       0);
  if (rc)
    LOG(ERROR) << "Unable to open UDA, rc=" << rc;
# endif

  if ((true == getOption(m_db_access_flags, "OF_LOAD_SNAP", opt_val)) && opt_val) 
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
    rc = mco_db_open_dev(m_name,
                         m_dict,
                         m_dev,
                         1,
                         m_db_params);
  }

#else /* EXTREMEDB_VERSION >= 40 */

   rc = mco_db_open(m_name,
                    m_dict,
                    (void*)m_MapAddress,
                    m_DatabaseSize + m_DbDiskCache,
                    PAGESIZE);

#endif /* EXTREMEDB_VERSION >= 40 */

   if (rc)
   {
     // Если мы пытаемся открыть уже созданную БД,
     // и при этом флаги открытия не говорят, что нам 
     // нужно было ее создавать - это не ошибка
     if (MCO_E_INSTANCE_DUPLICATE == rc)
     {
       LOG(INFO) << "DatabaseImpl '"<<m_name<<"' already created";
     }
     else
     {
       LOG(ERROR) << "Can't open DB dictionary '" << m_name
                  << "', rc=" << rc;
       setError(rtE_RUNTIME_FATAL);
       return getLastError();
     }
   }

#ifdef DISK_DATABASE
   LOG(INFO) << "Opening '" << m_dbsFileName << "' disk database";
   rc = mco_disk_open(m_name,
                      m_dbsFileName,
                      m_logFileName, 
                      0, 
                      m_DbDiskCache, 
                      m_DbDiskPageSize,
                      MCO_INFINITE_DATABASE_SIZE,
                      DB_LOG_TYPE);

   if ((rc != MCO_S_OK)
    && (rc != MCO_ERR_DISK_ALREADY_OPENED)
    && (rc != MCO_E_INSTANCE_DUPLICATE))
   {
     LOG(ERROR) << "Error creating disk database, rc=" << rc;
     setError(rtE_RUNTIME_FATAL);
     return getLastError();
   }
#endif
  return TransitionToState(DB_STATE_ATTACHED);
}

const Error& DatabaseImpl::RegisterEvents()
{
  MCO_RET rc;
  mco_trans_h t;

  clearError();

  do
  {
    rc = mco_trans_start(m_db, 
                         MCO_READ_WRITE, 
                         MCO_TRANS_FOREGROUND,
                         &t);
    if (rc)
    {
      LOG(ERROR) << "Starting transaction, rc=" << rc;
      setError(rtE_RUNTIME_FATAL);
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
      setError(rtE_RUNTIME_WARNING);
    }

  } while(false);

  if (rc)
    mco_trans_rollback(t);

  return getLastError();
}

/*
 * DB_STATE_UNINITIALIZED = 1, // первоначальное состояние
 * DB_STATE_INITIALIZED   = 2, // инициализирован runtime
 * DB_STATE_ATTACHED      = 3, // вызван mco_db_open
 * DB_STATE_CONNECTED     = 4, // вызван mco_db_connect
 * DB_STATE_DISCONNECTED  = 5, // вызван mco_db_disconnect
 * DB_STATE_CLOSED        = 6  // вызван mco_db_close
 */
const Error& DatabaseImpl::TransitionToState(DBState_t new_state)
{
  bool transition_correctness = false;

  clearError();
  /* 
   * TODO проверить допустимость перехода из 
   * старого в новое состояние
   */
  switch (m_state)
  {
    // Состояние "ОТКЛЮЧЕНО" может перейти в "ПОДКЛЮЧЕНО" или "ЗАКРЫТО"
    case DB_STATE_DISCONNECTED:
    {
      switch (new_state)
      {
        case DB_STATE_CONNECTED:
        case DB_STATE_CLOSED:
        case DB_STATE_INITIALIZED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }
    }
    break;

    // Состояние "ПОДКЛЮЧЕНО" может перейти в состояние "ОТКЛЮЧЕНО"
    case DB_STATE_CONNECTED:
    {
      switch (new_state)
      {
        case DB_STATE_DISCONNECTED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }
    }
    break;

    // Состояние "ПРИСОЕДИНЕНО" может перейти в состояние "ПОДКЛЮЧЕНО"\"ОТКЛЮЧЕНО"
    case DB_STATE_ATTACHED:
    {
      switch (new_state)
      {
        case DB_STATE_CONNECTED:
        case DB_STATE_DISCONNECTED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }
    }
    break;

    // Состояние "ЗАКРЫТО" не может перейти ни в какое другое состояние
    case DB_STATE_CLOSED:
    {
       transition_correctness = false;
    }
    break;

    case DB_STATE_INITIALIZED:
    {
      switch (new_state)
      {
        case DB_STATE_ATTACHED:
        case DB_STATE_CONNECTED:
        case DB_STATE_DISCONNECTED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }

    }
    break;

    // Неинициализированное состояние БД может перейти в "ОТКЛЮЧЕНО" или "ЗАКРЫТО"
    case DB_STATE_UNINITIALIZED:
    {
      switch (new_state)
      {
        case DB_STATE_DISCONNECTED:
        case DB_STATE_INITIALIZED:
        case DB_STATE_CLOSED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }
    }
    break;
  }

  if (transition_correctness)
    m_state = new_state;
  else
    setError(rtE_INCORRECT_DB_TRANSITION_STATE);

  return getLastError();
}

// Установить новое состояние ошибки
void DatabaseImpl::setError(ErrorCode_t _new_error_code)
{
  m_last_error.set(_new_error_code);
}

DBState_t DatabaseImpl::State() const
{
  return m_state;
}

// Вернуть последнюю ошибку
const Error& DatabaseImpl::getLastError() const
{
  return m_last_error;
}

// Вернуть признак отсутствия ошибки
bool DatabaseImpl::ifErrorOccured() const
{
 return (m_last_error.getCode() != rtE_NONE);
}

// Сбросить ошибку
void DatabaseImpl::clearError()
{
  m_last_error.set(rtE_NONE);
}

