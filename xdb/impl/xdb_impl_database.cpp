#include <climits> // USHRT_MAX
#include <assert.h>
#include <bitset>

// kill()
#include <sys/types.h>
#include <signal.h>

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

# if (EXTREMEDB_VERSION <= 40)
mco_size_t file_writer(void*, const void*, mco_size_t);
# else
#include "mcouda.h"  // mco_metadict_header_t
#include "mcohv.h"   // mcohv_p
mco_size_sig_t file_writer(void*, const void*, mco_size_t);
mco_size_sig_t file_reader(void*, void*, mco_size_t);
# endif

#define SETUP_POLICY
#include "mcoxml.h"

void impl_errhandler(MCO_RET);
void extended_impl_errhandler(MCO_RET, const char*, int);

#ifdef __cplusplus
}
#endif

#include "xdb_impl_database.hpp"

using namespace xdb;

int DatabaseImpl::m_count = 0;

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

#if (EXTREMEDB_VERSION <= 40)
mco_size_t 
#else
mco_size_sig_t 
#endif
file_writer( void *stream_handle /* FILE *  */, const void *from, mco_size_t nbytes )
{
  FILE *f = (FILE *)stream_handle;
  mco_size_sig_t nbs;

  nbs = fwrite( from, 1, nbytes, f );
  return nbs;
}

/* Stream reader with prototype mco_stream_read */
#if (EXTREMEDB_VERSION <= 40)
mco_size_t 
#else
mco_size_sig_t 
#endif
file_reader( void *stream_handle /* FILE *  */,  /* OUT */void *to, mco_size_t max_nbytes )
{
  FILE *f = (FILE *)stream_handle;
  mco_size_sig_t nbs;

  nbs = fread( to, 1, max_nbytes, f );
  return nbs;
}

// Проверка статуса процесса по его идентификатору
int task_id_check( pid_t tid ){
    // If sig is 0, then no signal is sent, but error checking is still performed;
    // this can be used to check for the existence of a process ID or process  group ID.
    return kill(tid, 0) == 0 ? 0 : 1;
}

DatabaseImpl::DatabaseImpl(const char* _name, const Options* _options, mco_dictionary_h _dict) :
  m_snapshot_counter(0),
  m_DatabaseSize(1024 * 1024 * 10),
  m_MemoryPageSize(256),
  m_MapAddress(0x20000000),
  m_DbDiskCache(0),
  m_DbDiskPageSize(0),
  m_state(DB_STATE_UNINITIALIZED),
  m_last_error(rtE_NONE),
  m_db(NULL),
  m_dict(_dict),
  m_db_access_flags(_options),
  m_save_to_xml_feature(false),
  m_flags(0),
  m_max_connections(DEFAULT_MAX_DB_CONNECTIONS_ALLOWED),
  m_snapshot_loaded(false)
{
  int val;
#if EXTREMEDB_VERSION >= 40 && USE_EXTREMEDB_HTTP_SERVER
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

  if (getOption(const_cast<Options*>(m_db_access_flags),"OF_CREATE", val) && val)
  {
    LOG(INFO) << m_name << ": Creates database if it not exist";
    m_flags.set(OF_POS_CREATE);
  }
  if (getOption(const_cast<Options*>(m_db_access_flags),"OF_TRUNCATE", val) && val)
  {
    LOG(INFO) << m_name << ": Truncate database";
    m_flags.set(OF_POS_TRUNCATE);
  }
  if (getOption(const_cast<Options*>(m_db_access_flags),"OF_RDWR", val) && val)
  {
    LOG(INFO) << m_name << ": Mount database in read/write mode";
    m_flags.set(OF_POS_RDWR);
  }
  if (getOption(const_cast<Options*>(m_db_access_flags),"OF_READONLY", val) && val)
  {
    LOG(INFO) << m_name << ": Mount database in read-only mode";
    m_flags.set(OF_POS_READONLY);
  }
  if (getOption(const_cast<Options*>(m_db_access_flags),"OF_LOAD_SNAP", val) && val)
  {
    LOG(INFO) << m_name << ": Load database content from snapshot";
    m_flags.set(OF_POS_LOAD_SNAP);
  }
  if (getOption(const_cast<Options*>(m_db_access_flags),"OF_SAVE_SNAP", val) && val)
  {
    LOG(INFO) << m_name << ": Store database content before exit";
    m_flags.set(OF_POS_SAVE_SNAP);
  }
  if (getOption(const_cast<Options*>(m_db_access_flags),"OF_MAX_CONNECTIONS", m_max_connections))
  {
    LOG(INFO) << m_name << ": Max allowed database connections is: " << m_max_connections;
  }
  else
  {
    m_max_connections = DEFAULT_MAX_DB_CONNECTIONS_ALLOWED;
  }

  // Проверить флаги открытия базы данных. Не могут быть установленными
  // одновременно:
  // а) TRUNCATE и LOAD_SNAP
  // б) RDWR и READONLY
  if (m_flags[OF_POS_TRUNCATE] && m_flags[OF_POS_LOAD_SNAP])
  {
    LOG(ERROR) << m_name << ": Flags 'Truncate' and 'LoadSnapshot' at same time for '"
               << m_name << "', skip snapshot loading";
    m_flags.reset(OF_POS_LOAD_SNAP);
  }
  if (m_flags[OF_POS_RDWR] && m_flags[OF_POS_READONLY])
  {
    LOG(ERROR) << m_name << ": Flags 'Read-Write' and 'Read-Only' at same time for '"
               << m_name << "', mount will be read-only";
    m_flags.reset(OF_POS_RDWR);
  }

  if (getOption(const_cast<Options*>(m_db_access_flags),"OF_DATABASE_SIZE", val) && val)
  {
    LOG(INFO) << m_name << ": Change default database size to " << val << " byte(s)";
    m_DatabaseSize = val;
  }
  else m_DatabaseSize = 1024 * 1024 * 10; // По умолчанию размер базы 10Мб

  if (getOption(const_cast<Options*>(m_db_access_flags),"OF_MEMORYPAGE_SIZE", val) && val)
  {
    LOG(INFO) << m_name << ": Change default page size to " << val << " byte(s)";
    if ((0 < val) && (val < USHRT_MAX))
    {
      m_MemoryPageSize = static_cast<uint2>(val);
    }
    else
    {
      LOG(WARNING) << m_name << ": MEMORYPAGE_SIZE value ("
                   << val
                   << ") is ignored due to overflow limits [0..65535]";
    }
  }
  else
  {
    m_MemoryPageSize = 256;
  }

  if (getOption(const_cast<Options*>(m_db_access_flags),"OF_MAP_ADDRESS", val) && val)
  {
    LOG(INFO) << m_name << ": Change default map address to " << val;
    m_MapAddress = val;
  }
  else m_MapAddress = 0x20000000;

#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
  m_intf.interface_addr = strdup("0.0.0.0"); // Принимать подключения с любого адреса
  if (getOption(const_cast<Options*>(m_db_access_flags),"OF_HTTP_PORT", val) && val)
  {
    LOG(INFO) << m_name << ": Change default http server port to " << val;
    m_intf.port = val;
  }
  else m_intf.port = 8082; // default port
#endif

#ifdef DISK_DATABASE
# ifndef DB_LOG_TYPE
#   define DB_LOG_TYPE REDO_LOG
# endif
#endif
  if (getOption(const_cast<Options*>(m_db_access_flags), "OF_DISK_CACHE_SIZE", val) && val)
  {
    m_DbDiskCache = val;
    m_DbDiskPageSize = 1024;
    LOG(INFO) << m_name << ": Change default disk cache size to " << val;
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

  LOG(INFO) << m_name << ": Current state " << State();
  switch (State())
  {
    case DB_STATE_CLOSED:
      LOG(WARNING) << "State already CLOSED";
      // NB: break пропущен специально!
    case DB_STATE_UNINITIALIZED:
      // Ничего делать не надо
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
#if 1
      m_count--;
      if (!m_count)  // Удаляется последний экземпляр базы данных
      {
        const char* enable_mco_stop = getenv("MCO_RUNTIME_STOP");
        rc = MCO_S_OK;
        if (enable_mco_stop && strtol(enable_mco_stop,NULL,10) == 1)
        {
          rc = mco_runtime_stop();
          LOG(INFO) << "mco_runtime_stop '" << m_name << "', rc=" << rc;
        }
        if (rc)
        {
          LOG(ERROR) << "Unable to stop database '" << m_name << "' runtime, code=" << rc;
        }
      }
      LOG(INFO) << "mco_runtime_stop '" << m_name << "' " << m_count;
#else
/*
 * Тесты используют последовательное создание и удаление экземпляров БД.
 * Повторный вызов mco_runtime_start|mco_runtime_stop ведет к сбоям в 
 * работе XDB.
 * Необходимо или ограничить вызов mco_runtime_stop, или разбить тесты 
 * на независимые части.
 */
#warning "Временный запрет на вызов mco_runtime_stop"
#endif
      TransitionToState(DB_STATE_CLOSED);
    break;
  }

#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
  free(m_intf.interface_addr);
#endif

#ifdef DISK_DATABASE
  delete []m_dbsFileName;
  delete []m_logFileName;
#endif

  LOG(INFO) << "Destructor database " << m_name;
}

mco_db_h DatabaseImpl::getDbHandler()
{
  if (!m_db)
  {
    LOG(ERROR) << "Get NULL database handler, current state: "<<m_state;
  }
  assert(m_db);
  return m_db;
}

/* NB: Сначала Инициализация runtime (Init), потом Подключение (Connect) */
const Error& DatabaseImpl::Init()
{
    MCO_RET rc = MCO_S_OK;
    mco_runtime_info_t info;

    clearError();

    m_dev_num = 1;
    if (DB_STATE_UNINITIALIZED != m_state)
    {
      LOG(WARNING) << "Try to re-init database " << m_name;
      return m_last_error;
    }

    // Запуск только для первого экземпляра БД
    if (!m_count)
    {
      rc = mco_runtime_start();
    }
    m_count++;

    LOG(INFO) << "mco_runtime_start '" << m_name << "' " << m_count << ", rc=" << rc;
    mco_get_runtime_info(&info);
    if (!info.mco_save_load_supported)
    {
      LOG(WARNING) << "XML import/export doesn't supported by runtime";
      m_save_to_xml_feature = false;
    }
    else
    {
      m_save_to_xml_feature = true;
    }

#if EXTREMEDB_VERSION >= 40
  memset(static_cast<void*>(&m_dev), '\0', sizeof(mco_device_t) * NDEV);
  /* setup memory device as a shared named memory region */
  m_dev[0].assignment = MCO_MEMORY_ASSIGN_DATABASE;
  m_dev[0].size       = m_DatabaseSize;
  m_dev[0].type       = MCO_MEMORY_NAMED; /* DB in shared memory */
  sprintf(m_dev[0].dev.named.name, "%s-db", m_name);
  m_dev[0].dev.named.flags = 0;
  m_dev[0].dev.named.hint  = 0;

  if (info.mco_disk_supported) {
    /* setup memory device for cache */
    m_dev[1].assignment = MCO_MEMORY_ASSIGN_CACHE;      /* assign the device as cache */
    m_dev[1].size       = m_DatabaseSize/2;
    if (info.mco_shm_supported) {
      m_dev[1].type       = MCO_MEMORY_NAMED;           /* set the device as shared named memory device */
      sprintf( m_dev[1].dev.named.name, "%s-cache", m_name ); /* set memory name */
      m_dev[1].dev.named.flags = 0;                     /* zero flags */
      m_dev[1].dev.named.hint  = 0;                     /* set mapping address or null it */
    } else {
      m_dev[1].type       = MCO_MEMORY_CONV;            /* set the device as a conventional memory device */
      m_dev[1].dev.conv.ptr = (void*)malloc( m_DatabaseSize/2 ); /* allocate memory and set device pointer */
    }
    
    /* setup memory device for main database storage */
    m_dev[2].type       = MCO_MEMORY_FILE;                /* set the device as a file device */
    m_dev[2].assignment = MCO_MEMORY_ASSIGN_PERSISTENT;   /* assign the device as a main database persistent storage */
    sprintf( m_dev[2].dev.file.name, "%s.dbs", m_name ); /* name the device */
    m_dev[2].dev.file.flags = MCO_FILE_OPEN_DEFAULT;      /* set the device flags */

    /* setup memory device for transaction log */
    m_dev[3].type       = MCO_MEMORY_FILE;                /* set the device as a file device */
    m_dev[3].assignment = MCO_MEMORY_ASSIGN_LOG;          /* assign the device as a log */
    sprintf( m_dev[3].dev.file.name, "%s.log", m_name ); /* name the device */
    m_dev[3].dev.file.flags = MCO_FILE_OPEN_DEFAULT;      /* set the device flags */

    m_dev_num += 3;
  }

  mco_db_params_init (&m_db_params);
  m_db_params.db_max_connections       = m_max_connections;
  m_db_params.connection_context_size  = sizeof(pid_t);
  /* set page size for in memory part */
  m_db_params.mem_page_size      = m_MemoryPageSize;
  /* set page size for persistent storage */
  m_db_params.disk_page_size     = m_DbDiskPageSize;

#if 0
  m_db_params.mode_mask |= MCO_DB_USE_CRC_CHECK;
  m_db_params.cipher_key = (const char*)"my_secure_key";
#endif

#endif

    if (rc)
    {
      TransitionToState(DB_STATE_UNINITIALIZED);

      // Если требуется создать новый, или обнулить старый экземпляр БД
      if (m_flags[OF_POS_CREATE] || m_flags[OF_POS_TRUNCATE])
      {
        rc = mco_db_kill(m_name);
        LOG(INFO) << "mco_db_kill '" << m_name << "', rc=" << rc;
      }
    }
    else
    {
      if (!info.mco_shm_supported)
      {
          LOG(WARNING) << "This program requires shared memory database runtime";
          setError(rtE_RUNTIME_FATAL);
          return getLastError();
      }

      /* Set the error handler to be called from the eXtremeDB runtime if a fatal error occurs */
      mco_error_set_handler(&impl_errhandler);
      //mco_error_set_handler_ex(&extended_impl_errhandler);
      LOG(INFO) << "User-defined error handler set";
    
#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
      if (1 == m_count) // Первый вызов инициализации
      {
          /* initialize MCOHV */
          m_hv = 0;
          int ret = mcohv_initialize();
          LOG(INFO) << "mcohv_initialize '" << m_name << "', ret=" << ret;

          mco_metadict_size(1, &m_size);
          m_metadict = (mco_metadict_header_t *) malloc(m_size);
          rc = mco_metadict_init (m_metadict, m_size, 0);
          LOG(INFO) << "mco_metadict_init '" << m_name << ", rc=" << rc;
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
            LOG(INFO) << "mco_metadict_register '" << m_name << ", rc=" << rc;
            if (rc) LOG(INFO) << "mco_metadict_register=" << rc;
          }
      }
#endif
      // Рантайм запущен, база не подключена
      TransitionToState(DB_STATE_INITIALIZED);
    }

    return getLastError();
}

const Error& DatabaseImpl::Connect()
{
  clearError();

  if (State() != DB_STATE_INITIALIZED)
    Init();

  switch (State())
  {
    case DB_STATE_UNINITIALIZED:
      LOG(WARNING)<<"Try to connect to uninitialized database "<<m_name;
    break;

    case DB_STATE_CONNECTED:
      LOG(WARNING)<<"Try to re-open database "<<m_name;
    break;

    case DB_STATE_INITIALIZED:
      // Open() вызывается далее, в ConnectToInstance()
    case DB_STATE_ATTACHED:
      // Текущее состояние : выполнен mco_db_open()
      // Требуется подключение к БД с помощью mco_db_connect()
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
// NB: Не открывать БД, это выполняется классом RtConnection
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
  char fname[150];

  clearError();

  // Создание нового экземпляра БД (mco_db_open)
  if (!(Open()).Ok())
  {
    LOG(ERROR)<<"Unable attach to database "<<m_name;
    return getLastError();
  }

  // База в состоянии ATTACHED, можно подключаться
  rc = mco_db_connect(m_name, &m_db);
  LOG(INFO) << "mco_db_connect '" << m_name
            << "', state=" << m_state
            << ", rc=" << rc;

  if ((m_flags[OF_POS_LOAD_SNAP]) && (!m_snapshot_loaded))
  {
      // Двоичный снимок не загружен, можем ли попробовать восстановиться с XML-снимка?
      if (m_last_error.code() == rtE_SNAPSHOT_READ)
      {
        snprintf(fname, sizeof(fname)-1, "%s.xml", m_name);

        // Загрузить данные из XML формата eXtremeDB
        LoadFromXML(fname);

        if (!getLastError().Ok())
        {
          LOG(ERROR) << "Unable to read data from XML initial snap";
        }
        else
        {
          m_snapshot_loaded = true;
        }
      }
  }

  // При открытии БД была ошибка. Возможно, еще не существует экземпляра.
  if (rc)
  {
    // Да, экземпляр базы не найден, и допускается его создание
    if ((MCO_E_NOINSTANCE == rc)
     && ((true == getOption(const_cast<Options*>(m_db_access_flags), "OF_CREATE", opt_val)) && opt_val))
    {
      // Повторно попытаться создать экземпляр, поскольку он еще не существовал
      if (!Open().Ok())
      {
        LOG(ERROR) << "Unable to recreating '" << m_name << "', try to kill";
        rc = mco_db_kill(m_name);
        LOG(INFO) << "mco_db_kill '" << m_name << "', rc=" << rc;
        return getLastError();
      }
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
     if (true == getOption(const_cast<Options*>(m_db_access_flags), "OF_TRUNCATE", opt_val) && opt_val)
     {
       /*
        * NB: предварительное удаление экземпляра БД делает бесполезной 
        * последующую попытку подключения к ней.
        * Очистка оставлена в качестве временной меры. В дальнейшем 
        * уже созданный экземпляр БД может использоваться в качестве 
        * persistent-хранилища после аварийного завершения.
        */
       rc = mco_db_clean(m_db);
       LOG(INFO) << "mco_db_clean '" << m_name << "', rc=" << rc;
       if (rc)
       {
         LOG(ERROR) << "Unable truncate database "<<m_name;
       }
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
      mcohv_start(&m_hv, m_metadict, &m_intf, 1);
      LOG(INFO) << "mcohv_start '" << m_name << "', rc=" << rc;
#endif
    }
  }
  return getLastError();
}

/*
 * Сохранение содержимого БД в виде файла XML формата McObject.
 *
 * NB: Внутри функции может упасть xdb runtime в том случае, если 
 * сохраняемая БД имеет хотя бы один безиндексный класс.
 */
const Error& DatabaseImpl::SaveAsXML(const char* given_file_name, const char *msg)
{
  MCO_RET rc = MCO_S_OK;
  mco_xml_policy_t op, np;
  mco_trans_h t;
  static char calc_file_name[150];
  char *fname = &calc_file_name[0];
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
    snprintf(calc_file_name, sizeof(calc_file_name)-1,
          "%09d.%s.%s.snap.xml",
          getSnapshotCounter(),
          m_name,
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
    
  /* setup policy */
  setupPolicy(np);

  LOG(INFO) << "Export DB to " << fname;
  f = fopen(fname, "wb");
  if (!f)
  {
    LOG(ERROR) << "Unable to open XML-snapshot file " << fname;
    m_last_error = rtE_XML_EXPORT;
    return m_last_error;
  }
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

// Установка общих значимых полей для эксорта/импорта данных в XML
void DatabaseImpl::setupPolicy(mco_xml_policy_t& policy)
{
  policy.blob_coding = MCO_TEXT_BASE64; //MCO_TEXT_BINHEX
  // encode line feeds
  policy.encode_lf   = MCO_YES;
  // encode national chars, MCO_YES ПРИВЕДЕТ К ЧИСЛОВОЙ КОДИРОВКЕ РУССКИХ БУКВ
  policy.encode_nat  = MCO_NO;
  policy.encode_spec = MCO_YES;
  policy.float_format= MCO_FLOAT_FIXED;
  policy.ignore_field= MCO_YES;
  policy.indent      = MCO_YES;
  policy.int_base    = MCO_NUM_DEC;
  policy.quad_base   = MCO_NUM_HEX; /* other are invalid */
  policy.text_coding = MCO_TEXT_ASCII;
  policy.truncate_sp = MCO_YES;
  policy.use_xml_attrs  = MCO_NO; //YES;
  policy.ignore_autoid  = MCO_NO;
  policy.ignore_autooid = MCO_NO;
}

const Error& DatabaseImpl::StoreSnapshot(const char* given_file_name)
{
  FILE* fbak = NULL;
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
        LOG(ERROR) << "Given snapshot filename is exceed limits ("<<sizeof(fname)<<")";
      }
      snprintf(fname, sizeof(fname)-1, "%s.snap.bin", given_file_name);
    }
    else
    {
      snprintf(fname, sizeof(fname)-1, "%s.snap.bin", m_name);
    }
    fname[sizeof(fname)-1] = '\0';

    /* Backup database */
    if (NULL != (fbak = fopen(fname, "wb")))
    {
      rc = mco_db_save(static_cast<void*>(fbak), file_writer, m_db);
      if (rc)
      {
        LOG(ERROR) << "Unable to save "<<m_name<<" snapshot into "<<fname<<", rc="<<rc;
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
#warning "Binary snapshot is disabled for this version"
   LOG(WARNING) << "Binary snapshot is disabled for this version";
#endif

#if 1
    // Попытаться сохранить содержимое в XML-формате eXtremeDB
    if (m_flags[OF_POS_SAVE_SNAP]) // Допустим снимок
    {
      snprintf(fname, sizeof(fname)-1, "%s.snap.xml", m_name);

      // Название снимка д.б. в формате "...."
      const Error &status = SaveAsXML(fname, NULL);

      if (!status.Ok())
      {
        LOG(ERROR) << "Can't save content in RTDB's XML format";
      }
    }
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
      LOG(INFO)<<"mco_db_disconnect '" << m_name << "', rc=" << rc;
      if (rc) 
      {
        LOG(ERROR)<<"Unable to disconnect from "<<m_name<<", rc="<<rc;
      }
      else TransitionToState(DB_STATE_DISCONNECTED);

      // NB: break пропущен специально
    case DB_STATE_ATTACHED:  // база подключена
#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
      rc = mco_uda_db_close(m_metadict, 0);
#else      
      rc = mco_db_close(m_name);
#endif
      LOG(INFO)<<"mco_db_close '"<<m_name<<"', rc="<<rc;
      if (rc)
      {
        LOG(ERROR)<<"Unable to close "<<m_name<<", rc="<<rc;
      }
      else TransitionToState(DB_STATE_CLOSED);
    break;

    default:
      LOG(INFO) << "Disconnect from unknown state:" << state;
  }

  if (rc)
    setError(rtE_DB_NOT_DISCONNECTED);

#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
  int ret;
  if (!m_count && m_metadict_initialized == true)
  {
    ret = mcohv_stop(m_hv);
    LOG(INFO) << "mcohv_stop '" << m_name << "', ret=" << ret;

    ret = mcohv_shutdown();
    LOG(INFO) << "mcohv_shutdown '" << m_name << "', ret=" << ret;
    free(m_metadict);
    m_metadict_initialized = false;
  }
#endif

  return getLastError();
}

// NB: нужно выполнить до mco_db_connect (m_db д.б. NULL)
// БД должна быть в состоянии ATTACHED, иначе при попытке 
// восстановления содержимого получим ошибку MCO_E_INSTANCE_DUPLICATE
//
const Error& DatabaseImpl::LoadSnapshot(const char *given_file_name)
{
  MCO_RET rc = MCO_S_OK;
  FILE* fbak;
  Error restoring_state = rtE_SNAPSHOT_READ;
  char fname[150];

  clearError();
  fname[0] = '\0';
  assert(!m_db);

  switch(State())
  {
    case DB_STATE_UNINITIALIZED:
        LOG(INFO) << "Current state: " << State() << ", init runtime first";
        Init();
        // NB: break пропущен специально
    case DB_STATE_INITIALIZED:
    case DB_STATE_ATTACHED:
        LOG(INFO) << "Current state: " << State() << ", load snapshot of " << getName();
    break;

    default:
        LOG(INFO) << "Load snapshot of '" << getName() << "' from state " << State();
  }


  if (given_file_name)
  {
    if (strlen(given_file_name) > sizeof(fname))
    {
      LOG(ERROR) << "Given snapshot filename is exceed limits ("<<sizeof(fname)<<")";
    }
    strncpy(fname, given_file_name, sizeof(fname));
    fname[sizeof(fname)-1] = '\0';
  }
  else
  {
    strcpy(fname, m_name);
    strcat(fname, ".snap.bin");
  }

#if EXTREMEDB_VERSION >= 40
  do
  {
    if (m_snapshot_loaded)
    {
      LOG(ERROR) << "Try to re-open '"<<m_name<<"' database content";
      setError(rtE_SNAPSHOT_READ);
      break;
    }

    if (NULL != (fbak = fopen(fname, "rb")))
    {
        // Файл двоичного снимка успешно открыт
        rc = mco_db_load(static_cast<void*>(fbak),
                            file_reader,
                            m_name,
                            m_dict,
                            m_dev,
                            m_dev_num,
                            &m_db_params);
        if (rc)
        {
          LOG(ERROR) << "Unable to load '" << m_name
                     << "' binary snapshot file '" <<fname
                     << "', rc="<<rc;
          m_snapshot_loaded = false;
          setError(rtE_SNAPSHOT_READ);
        }
        else
        {
          // Успешно прочитали бинарный дамп
          m_snapshot_loaded = true;
          LOG(INFO) << "Binary dump '" << fname << "' succesfully loaded";
        }
        fclose(fbak);
    }
    else
    {
      LOG(ERROR) << "Unable to open binary snapshot file "<<fname;
      setError(rtE_SNAPSHOT_READ);

      // Попробуем прочитать содержимое из XML
      // Есть два вида XML-снимков (в порядке уменьшения приоритета чтения):
      // 1. В формате БДРВ, ранее сделанный с помощью mco_xml_export()
      // 2. В формате RTDBUS, описатель rtap_db.xsd

      strcpy(fname, m_name);
      strcat(fname, ".snap.xml");

      rc = mco_db_open_dev(m_name,
                           m_dict,
                           m_dev,
                           m_dev_num,
                           &m_db_params);
      LOG(INFO) << "mco_db_open_dev '" << m_name << "', rc=" << rc;

      if ( MCO_S_OK == rc )
      {
        // Состояние ATTACHED - подключение m_db не инициализировано
        TransitionToState(DB_STATE_ATTACHED);
        m_snapshot_loaded = false;

        rc = mco_db_connect(m_name, &m_db);
        LOG(INFO) << "mco_db_connect '" << m_name << "', rc=" << rc;
        if ( MCO_S_OK == rc )
        {
          TransitionToState(DB_STATE_CONNECTED);
          restoring_state = LoadFromXML(fname);
          m_snapshot_loaded = restoring_state.Ok();
        }
        else
        {
          LOG(ERROR) << "Can't connecting to freshly opened database "
                     << m_name << " for XML snapshot import";
        }
      }
      else
      {
        LOG(ERROR) << "Failed mco_db_open_dev, rc=" << rc;
      }

      if (!m_snapshot_loaded)
      {
        // TODO Прочитать данные из XML в формате XMLSchema 'rtap_db.xsd'
        // с помощью <Classname>_xml_create() создавать содержимое БД
        LOG(ERROR)<< "Unable to read from XML file (format XMLSchema 'rtap_db.xsd'): "
                  << getLastError().what();

        // Если была ошибка восстановления данных после подключения к пустой БД,
        // то необходимо отключиться.
        if (State() == DB_STATE_CONNECTED)
        {
            rc = mco_db_disconnect(m_db);
            TransitionToState(DB_STATE_DISCONNECTED);
            LOG(INFO) << "Disconnecting from " << m_name << ", rc=" << rc;
        }
        setError(restoring_state);
      }
      
      break;
    }

    // Очистить предыдущее содержимое БД, если оно было
    //rc = mco_db_clean(m_db);
    //LOG(INFO) << "mco_db_clean '" << m_name << "', rc=" << rc;

  } while (false);
#else /* EXTREMEDB_VERSION >= 40 */
  // Не можем читать двоичный дамп, попробуем XML
  // Но для этого необходимо открыть БД с помощью mco_db_open_dev
  setError(rtE_SNAPSHOT_READ);
#endif /* EXTREMEDB_VERSION >= 40 */

  return getLastError();
}

// Попробовать восстановить данные из XML
// 1. прочитать XML-файл НСИ
// 1. загрузки данные из XML-файла с данными
const Error& DatabaseImpl::LoadFromXML(const char* given_file_name)
{
  MCO_RET           rc = MCO_S_OK;
  mco_trans_h       t;
  mco_xml_policy_t  policy;
//  static char       calc_file_name[150];
//  char             *fname = &calc_file_name[0];
  FILE             *f = NULL;

  m_last_error.clear();
  assert(m_db);

  if (false == m_save_to_xml_feature)
  {
    // Отсутствует поддержка сохранения в XML со стороны BD
    m_last_error = rtE_NOT_IMPLEMENTED;
    return m_last_error;
  }

  LOG(INFO) << "Try to restore '" << m_name << "' content from XML";

  if (m_snapshot_loaded)
  {
    LOG(WARNING) << "Database " << m_name << " snapshot is already loaded";
    return m_last_error;
  }

  do
  {
    if (NULL == (f = fopen(given_file_name, "rb")))
    {
      LOG(ERROR) << "Can't open XML snapshot '"
                 << given_file_name
                 << "' for reading (" << strerror(errno) << ")";
      setError(rtE_XML_IMPORT);
      break;
    }

    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc)
    {
      LOG(ERROR) << "Starting transaction, rc=" << rc;
      setError(rtE_RUNTIME_FATAL);
      break;
    }

    // Изменить стандартные правила экспорта/импорта на свои
    mco_xml_get_policy(t, &policy);
    setupPolicy(policy);
    mco_xml_set_policy(t, &policy);

    rc = mco_db_xml_import(t, f, file_reader);

    if (MCO_S_OK == rc)
    {
      rc = mco_trans_commit(t);
      LOG(INFO) << "XML-import '" << m_name << "' is done, rc=" << rc;
      m_snapshot_loaded = true;
    }
    else
    {
      rc = mco_trans_rollback(t);
      LOG(INFO) << "XML-import '" << m_name << "' failure, rc=" << rc;
      m_snapshot_loaded = true;
    }

  } while(false);

  if (f)
    fclose(f);

  if (rc)
  {
    m_last_error.set(rtE_XML_IMPORT);
  }

  return getLastError();
}

// Создать базу данных с помощью mco_db_open
const Error& DatabaseImpl::Open()
{
  MCO_RET rc = MCO_S_OK;

  clearError();

#if EXTREMEDB_VERSION >= 40

#if USE_EXTREMEDB_HTTP_SERVER
  rc = mco_uda_db_open(m_metadict,
                       0,
                       &m_dev,
                       1,
                       &m_db_params,
                       NULL,
                       0);
  LOG(INFO) << "mco_uda_db_open '" << m_name << "', rc=" << rc;
  if (rc)
  {
    LOG(ERROR) << "Unable to open UDA, rc=" << rc;
    m_last_error.set(rtE_RUNTIME_FATAL);
    return m_last_error;
  }
#else /* EXTREMEDB_VERSION > 4 and not USE_EXTREMEDB_HTTP_SERVER */

#if 1
  // NB: Загружать снимок можно только до вызова mco_uda_db_open(),
  // после будет ошибка MCO_E_INSTANCE_DUPLICATE
  //
  // Требуется прочитать ранее сохраненный двоичный снимок?
  // Проверить, не был ли он уже прочитан ранее функцией loadEnvironment()
  if (m_flags[OF_POS_LOAD_SNAP] && !m_snapshot_loaded)
  {
    // Внутри LoadSnapshot: mco_db_load() -> mco_db_open_dev()
    if (!(LoadSnapshot()).Ok())
    {
      LOG(ERROR) << "Unable to restore content from snapshots";
    }
    else
    {
      // Состояние CONNECTED - инициализировано подключение (m_db все еще NULL)
      TransitionToState(DB_STATE_ATTACHED);
    }
  }
#else
  // Состояние CONNECTED - инициализировано подключение (m_db все еще NULL)
  TransitionToState(DB_STATE_ATTACHED);
#endif

  // Если снимок загружен успешно, значит БД уже работает,
  // пропустим повторное ее открытие
  if (!m_snapshot_loaded)
  {
    rc = mco_db_open_dev(m_name,
                         m_dict,
                         m_dev,
                         m_dev_num,
                         &m_db_params);
    LOG(INFO) << "mco_db_open_dev '" << m_name << "', rc=" << rc;
    // Состояние ATTACHED - подключение m_db не инициализировано
    TransitionToState(DB_STATE_ATTACHED);
  }

#endif /* USE_EXTREMEDB_HTTP_SERVER */


#else /* EXTREMEDB_VERSION >= 40 && not USE_EXTREMEDB_HTTP_SERVER */

  // Если снимок загружен успешно, значит БД уже работает,
  // пропустим повторное ее открытие
  if (!m_snapshot_loaded)
  {
   rc = mco_db_open(m_name,
                    m_dict,
                    (void*)m_MapAddress,
                    m_DatabaseSize + m_DbDiskCache,
                    m_MemoryPageSize /*PAGESIZE*/);
    LOG(INFO) << "mco_db_open '" << m_name << "', rc=" << rc;
  }

#endif /* EXTREMEDB_VERSION >= 40 */

   if (rc)
   {
     // Если мы пытаемся открыть уже созданную БД,
     // и при этом флаги открытия не говорят, что нам 
     // нужно было ее создавать - это не ошибка
     if (MCO_E_INSTANCE_DUPLICATE == rc)
     {
       LOG(INFO) << "DatabaseImpl '"<<m_name<<"' is already created";
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
     TransitionToState(DB_STATE_INITIALIZED);
     setError(rtE_RUNTIME_FATAL);
     return getLastError();
   }
#endif
  return getLastError();
}

const Error& DatabaseImpl::RegisterEvents()
{
  MCO_RET rc;
  mco_trans_h t;

  clearError();
#if 0

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

    rc = mco_register_newService_handler(t, 
            &DatabaseBrokerImpl::new_Service, 
            (void*)this
            );

    if (rc) LOG(ERROR) << "Registering event on XDBService creation, rc=" << rc;

    rc = mco_register_delService_handler(t, 
            &DatabaseBrokerImpl::del_Service, 
            (void*)this);
    if (rc) LOG(ERROR) << "Registering event on XDBService deletion, rc=" << rc;

    rc = mco_trans_commit(t);
    if (rc)
    {
      LOG(ERROR) << "Commitment transaction, rc=" << rc;
      setError(rtE_RUNTIME_WARNING);
    }

  } while(false);

  if (rc)
    mco_trans_rollback(t);

#endif
  return getLastError();
}

/*
 * DB_STATE_UNINITIALIZED = 1, // первоначальное состояние
 * DB_STATE_INITIALIZED   = 2, // инициализирован runtime
 * DB_STATE_ATTACHED      = 3, // вызван mco_db_open    - база подключена
 * DB_STATE_CONNECTED     = 4, // вызван mco_db_connect - база подключена и открыта
 * DB_STATE_DISCONNECTED  = 5, // вызван mco_db_disconnect  - база подключена и закрыта
 * DB_STATE_CLOSED        = 6  // вызван mco_db_close       - база отключена
 */
const Error& DatabaseImpl::TransitionToState(DBState_t new_state)
{
  bool transition_correctness = false;

  clearError();
  /* 
   * Проверить допустимость перехода из старого в новое состояние
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
  {
    LOG(INFO) << "Change " << m_name << " state from " << m_state << " to " << new_state;
    m_state = new_state;
  }
  else
    setError(rtE_INCORRECT_DB_STATE);

  return getLastError();
}

// Установить новое состояние ошибки
void DatabaseImpl::setError(ErrorCode_t _new_error_code)
{
  m_last_error.set(_new_error_code);
}

void DatabaseImpl::setError(const Error& _new_error)
{
  m_last_error = _new_error;
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

// Реализация в классе DatabaseRtapImpl
//
// Интерфейс управления БД - Контроль выполнения
const Error& DatabaseImpl::Control(rtDbCq& info)
{
  
  m_last_error = rtE_NOT_IMPLEMENTED;
  return m_last_error;
}

// Интерфейс управления БД - Контроль Точек
const Error& DatabaseImpl::Query(rtDbCq& info)
{
  m_last_error = rtE_NOT_IMPLEMENTED;
  return m_last_error;
}

// Интерфейс управления БД - Контроль выполнения
const Error& DatabaseImpl::Config(rtDbCq& info)
{
  m_last_error = rtE_NOT_IMPLEMENTED;
  return m_last_error;
}
