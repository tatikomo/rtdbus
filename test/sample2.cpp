#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"
#include "mcoxml.h"

#ifdef __cplusplus
}
#endif

#include "dat/xdb_broker.hpp"
#include "sample2.hpp"

/* ================================================================= */
/* Database handler */
/* ================================================================= */
mco_db_h  m_db;
/* ================================================================= */

#if (EXTREMEDB_VERSION<=40)
mco_size_t file_writer(void* stream_handle, const void* from, mco_size_t nbytes)
#else
mco_size_sig_t file_writer(void* stream_handle, const void* from, mco_size_t nbytes)
#endif
{
    FILE* f = (FILE*)stream_handle;
    int nbs = fwrite(from, 1, nbytes, f);
    return nbs;
}

int GetTimerValue(timer_mark_t& timer)
{
  struct timespec res;
  int status = 0;

  if (clock_gettime(CLOCK_REALTIME, &res) != -1)
  {
    timer.tv_sec = res.tv_sec;
    timer.tv_nsec = res.tv_nsec;
    status = 1;
  }
  else
  {
    perror("GetTimerValue");
  }

  return status;
}

void LogError(MCO_RET rc,
            const char *functionName,
            const char *format,
            ...)
{
    const char *pre_format = "E %s [%d]: ";
    char buffer[255];
    char user_msg[255];
    va_list args;

    sprintf(buffer, pre_format, 
            functionName? functionName : empty_str,
            rc);

    va_start (args, format);
    vsprintf (user_msg, format, args);
    strncat(buffer, user_msg, sizeof(buffer)-1);
    fprintf(stderr, "%s\n", buffer);
    fflush(stderr);
    va_end (args);
}

void LogWarn(const char *functionName,
             const char *format,
             ...)
{
    const char *pre_format = "W %s: ";
    char buffer[255];
    char user_msg[255];
    va_list args;

    sprintf(buffer, pre_format, functionName? functionName : empty_str);

    va_start (args, format);
    vsprintf (user_msg, format, args);
    strncat(buffer, user_msg, sizeof(buffer)-1);
    fprintf(stdout, "%s\n", buffer);
    fflush(stdout);
    va_end (args);
}

void LogInfo(const char *format,
             ...)
{
    const char *pre_format = "I: ";
    char buffer[255];
    char user_msg[255];
    va_list args;

    strcpy(buffer, pre_format);

    va_start (args, format);
    vsprintf (user_msg, format, args);
    strncat(buffer, user_msg, sizeof(buffer)-1);
    fprintf(stdout, "%s\n", buffer);
    fflush(stdout);
    va_end (args);
}


/* implement error handler */
void extended_errhandler(MCO_RET errcode, const char* file, int line)
{
  fprintf(stdout, "\neXtremeDB runtime fatal error: %d on line %d of file %s",
          errcode, line, file);
  exit(-1);
}

void show_runtime_info(const char * lead_line)
{
  mco_runtime_info_t info;
  
  /* get runtime info */
  mco_get_runtime_info(&info);

  /* Core configuration parameters: */
  if ( *lead_line )
    fprintf( stdout, "%s", lead_line );

  fprintf( stdout, "\n" );
  fprintf( stdout, "\tEvaluation runtime ______ : %s\n", info.mco_evaluation_version   ? "yes":"no" );
  fprintf( stdout, "\tCheck-level _____________ : %d\n", info.mco_checklevel );
  fprintf( stdout, "\tMultithread support _____ : %s\n", info.mco_multithreaded        ? "yes":"no" );
  fprintf( stdout, "\tFixedrec support ________ : %s\n", info.mco_fixedrec_supported   ? "yes":"no" );
  fprintf( stdout, "\tShared memory support ___ : %s\n", info.mco_shm_supported        ? "yes":"no" );
  fprintf( stdout, "\tXML support _____________ : %s\n", info.mco_xml_supported        ? "yes":"no" );
  fprintf( stdout, "\tStatistics support ______ : %s\n", info.mco_stat_supported       ? "yes":"no" );
  fprintf( stdout, "\tEvents support __________ : %s\n", info.mco_events_supported     ? "yes":"no" );
  fprintf( stdout, "\tVersioning support ______ : %s\n", info.mco_versioning_supported ? "yes":"no" );
  fprintf( stdout, "\tSave/Load support _______ : %s\n", info.mco_save_load_supported  ? "yes":"no" );
  fprintf( stdout, "\tRecovery support ________ : %s\n", info.mco_recovery_supported   ? "yes":"no" );
#if (EXTREMEDB_VERSION >=41)
  fprintf( stdout, "\tRTree index support _____ : %s\n", info.mco_rtree_supported      ? "yes":"no" );
#endif
  fprintf( stdout, "\tUnicode support _________ : %s\n", info.mco_unicode_supported    ? "yes":"no" );
  fprintf( stdout, "\tWChar support ___________ : %s\n", info.mco_wchar_supported      ? "yes":"no" );
  fprintf( stdout, "\tC runtime _______________ : %s\n", info.mco_rtl_supported        ? "yes":"no" );
  fprintf( stdout, "\tSQL support _____________ : %s\n", info.mco_sql_supported        ? "yes":"no" );
  fprintf( stdout, "\tPersistent storage support: %s\n", info.mco_disk_supported       ? "yes":"no" );
  fprintf( stdout, "\tDirect pointers mode ____ : %s\n", info.mco_direct_pointers      ? "yes":"no" );  
}

bool Init()
{
    const char *fctName = "Init";
    bool status = false;
    MCO_RET rc;
    mco_runtime_info_t info;

    mco_get_runtime_info(&info);
    if (!info.mco_save_load_supported)
    {
      LogWarn(fctName, "XML import/export doesn't supported by runtime");
      m_save_to_xml_feature = false;
    }
    else
    {
      m_save_to_xml_feature = true;
    }

    if (!info.mco_shm_supported)
    {
      LogWarn(fctName, "Runtime doesn't support shared memory database");
      return false;
    }

    mco_error_set_handler_ex(&extended_errhandler);
    show_runtime_info("");

    rc = mco_runtime_start();

    if (rc) { LogError(rc, fctName, "Can't init eXtremeDB runtime"); }
    else status = true;

    return status;
}

bool Connect()
{
  const char *fctName = "Connect";
  MCO_RET rc;
  bool status = false;
  mco_db_params_t   db_params;
  mco_device_t      dev;

  /* setup memory device as a shared named memory region */
  dev.assignment = MCO_MEMORY_ASSIGN_DATABASE;
  dev.size       = DATABASE_SIZE;
  dev.type       = MCO_MEMORY_NAMED;
  sprintf(dev.dev.named.name, "%s-db", DATABASE_NAME);
  dev.dev.named.flags = 0;
  dev.dev.named.hint  = 0;
  mco_db_params_init (&db_params);

  mco_db_kill(DATABASE_NAME);

  status = Init();
  while (true == status)
  {
    rc = mco_db_open_dev(DATABASE_NAME,
                       xdb_broker_get_dictionary(),
                       &dev,
                       1,
                       &db_params);
    if (rc) { LogError(rc, fctName, "Can't open '%s'", DATABASE_NAME); break; }

    rc = mco_db_connect(DATABASE_NAME, &m_db);
    if (rc) { LogError(rc, fctName, "Can't connect to '%s'", DATABASE_NAME); break; }

    status = true;
    break;
  }

  return status;
}

Service* LoadService(mco_trans_h t,
        autoid_t &aid,
        xdb_broker::XDBService& instance)
{
  const char   *fctName = "LoadService";
  Service      *service = NULL;
  MCO_RET       rc = MCO_S_OK;
  char          name[SERVICENAME_MAXLEN + 1];
  ServiceState  state;

  while (rc == MCO_S_OK)
  {
    rc = instance.name_get(name, (uint2)SERVICENAME_MAXLEN);
    name[SERVICENAME_MAXLEN] = '\0';
    if (rc) { LogError(rc, fctName, "get service's name"); break; }
    rc = instance.state_get(state);
    if (rc) { LogError(rc, fctName, "get state of worker's service"); break; }

    service = new Service;
    service->m_id = aid;
    strncpy(service->m_name, name, SERVICENAME_MAXLEN);
    service->m_name[SERVICENAME_MAXLEN] = '\0';
    service->m_state = state;
    break;
  }

  return service;
}

Worker* LoadWorker(mco_trans_h t,
        autoid_t &srv_aid,
        xdb_broker::XDBWorker& wrk_instance)
{
  const char   *fctName = "LoadWorker";
  Worker       *worker = NULL;
  MCO_RET       rc = MCO_S_OK;
  char          ident[IDENTITY_MAXLEN + 1];
  xdb_broker::timer_mark  xdb_expire_time;
  timer_mark_t  expire_time = {0, 0};
  uint4         timer_value;
  WorkerState   state;

  while (rc == MCO_S_OK)
  {
    rc = wrk_instance.identity_get(ident, (uint2)IDENTITY_MAXLEN);
//    ident[IDENTITY_MAXLEN] = '\0';
    if (rc) { LogError(rc, fctName, "get worker's identity"); break; }
    rc = wrk_instance.expiration_read(xdb_expire_time);
    if (rc) { LogError(rc, fctName, "get worker's expiration mark"); break; }
    rc = wrk_instance.state_get(state);
    if (rc) { LogError(rc, fctName, "get worker's service id"); break; }
    rc = xdb_expire_time.sec_get(timer_value);  expire_time.tv_sec = timer_value;
    rc = xdb_expire_time.nsec_get(timer_value); expire_time.tv_nsec = timer_value;

    worker = new Worker;
    strncpy(worker->m_identity, ident, IDENTITY_MAXLEN);
    worker->m_identity[IDENTITY_MAXLEN] = '\0';
    worker->m_service_id = srv_aid;
    worker->m_state = state;
    memcpy((void*)&worker->m_expiration, 
           (void*)&expire_time, 
           sizeof (worker->m_expiration));

    break;
  }

  return worker;
}

/* NB: to delete return value */
Service *GetServiceById(int64_t _id)
{
  const char   *fctName = "GetServiceById";
  autoid_t      aid = _id;
  mco_trans_h   t;
  MCO_RET       rc;
  Service      *service = NULL;
  xdb_broker::XDBService service_instance;

  rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    rc = xdb_broker::XDBService::autoid::find(t, aid, service_instance);
    if (rc) { LogError(rc, fctName, "locating service with id=%lld", aid); break; }

    service = LoadService(t, aid, service_instance);
    mco_trans_commit(t);
    break;
  }

  if (rc)
    mco_trans_rollback(t);

  return service;
}

uint2 LocatingFirstOccurence(
    xdb_broker::XDBService &service_instance,
    WorkerState            searched_worker_state)
{
  const char   *fctName = "LocatingFirstOccurence";
  MCO_RET      rc = MCO_S_OK;
  xdb_broker::XDBWorker     worker_instance;
  WorkerState  worker_state;
  bool         awaiting_worker_found = false;
  uint2        workers_spool_size = 0;
  uint2        awaiting_worker_idx = -1;

  while (MCO_S_OK == rc)
  {
    rc = service_instance.workers_size(workers_spool_size);
    if (rc) { LogError(rc, fctName, 
            "get size of waiting workers queue"); break; }
    
    for (awaiting_worker_idx = 0;
         awaiting_worker_idx < workers_spool_size;
         awaiting_worker_idx++)
    {
      rc = service_instance.workers_at(awaiting_worker_idx, worker_instance);
      if (MCO_E_EMPTYVECTOREL == rc)
      {
        if (DISARMED == searched_worker_state)
        {
          // When looking DISARMED worker -> empty slot will good too
          awaiting_worker_found = true;
          break;
        }
        continue;
      }

      if (rc)
      {
        awaiting_worker_found = false;
        if (rc) { LogError(rc, fctName, "get service's awaiting worker"); break; }
      }
      else // worker found, check his state
      {
        rc = worker_instance.state_get(worker_state);
        if (rc) { LogError(rc, fctName, "get awaiting worker's state"); break; }

        if (worker_state == searched_worker_state)
        {
          awaiting_worker_found = true;
          break;
        }
      }

    } /* for every slot in spool */

    if (false == awaiting_worker_found)
    {
        /* нет ни одного Обработчика - у всех другое состояние */
        LogWarn(fctName, "no one waiting workers find");
        break;
    }

    break;
  }

  return (false == awaiting_worker_found)? -1 : awaiting_worker_idx;
}

Service* AddService(const char *name)
{
  const char    *fctName = "AddService";
  xdb_broker::XDBService instance;
  MCO_RET        rc;
  mco_trans_h    t;
  autoid_t       aid;
  ServiceState   state;
  Service       *srv = NULL;

  assert(name);
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (MCO_S_OK == rc)
  {
    if (rc) { LogError(rc, fctName, "starting transaction"); break; }

    rc = instance.create(t);
    if (rc) { LogError(rc, fctName, "creating service '%s' instance", name); break; }

    rc = instance.name_put(name, strlen(name));
    if (rc) { LogError(rc, fctName, "name '%s' assign", name); break; }

    rc = instance.workers_alloc(WORKERS_SPOOL_MAXLEN);
    if (rc) { LogError(rc, fctName, "allocating pool"); break; }

    rc = instance.checkpoint();
    if (rc) { LogError(rc, fctName, "checkpointing"); break; }

    rc = instance.state_get(state);
    if (rc) { LogError(rc, fctName, "service's get state"); break; }
    rc = instance.autoid_get(aid);
    if (rc) { LogError(rc, fctName, "service's get id"); break; }

    srv = new Service;
    srv->m_id = aid;
    strncpy(srv->m_name, name, sizeof (srv->m_name));
    srv->m_name[SERVICENAME_MAXLEN] = '\0';
    srv->m_state = state;

    rc = mco_trans_commit(t);
    if (rc) { LogError(rc, fctName, "commitment transaction"); break; }

    break;
  }

  if (rc)
    mco_trans_rollback(t);

  return srv;
}

bool PushWorker(Worker *wrk)
{
  const char   *fctName = "PushWorker";
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
  char wrk_identity[IDENTITY_MAXLEN + 1];

  assert(wrk);
  strncpy(wrk_identity, wrk->m_identity, IDENTITY_MAXLEN);
  wrk_identity[IDENTITY_MAXLEN] = '\0';

  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    if (NULL != (srv = GetServiceById(wrk->m_service_id)))
    {
      // Service located, try to find worker's slot number in spool
      rc = xdb_broker::XDBService::SK_ident::find(t,
                wrk_identity,
                strlen(wrk_identity),
                /*OUT*/ service_instance,
                /*OUT*/ worker_idx);

      // service_instance is invalid if worker not found
      if (MCO_S_NOTFOUND == rc)
      {
        // 
        // NB: Сервис есть (srv), но у него нет указанного Обработчика.
        // Может быть это первая регистрация Обработчика данного Сервиса.
        rc = xdb_broker::XDBService::autoid::find(t, 
                            srv->m_id, 
                            service_instance);
        if (rc) 
        { 
            LogError(rc, fctName, "locating service by id=%lld", srv->m_id); break; 
        }

        // Индекс не корректен - данный Обработчик регистрируется впервые
        // Найти в спуле для него свободное место
        if ((uint2)-1 == (worker_idx = LocatingFirstOccurence(service_instance, DISARMED)))
        {
            // Не удалось найти свободного места в спуле
            LogError(rc, fctName, "free space in pool of service '%s' [id=%lld] is over", 
                srv->m_name, srv->m_id); break;
        }
      }
      else
      {
        // Обработчик уже есть в спуле, его индекс найден (worker_idx)
        if (rc) 
        { 
            LogError(rc, fctName, "locating worker '%s' in service's spool", 
                wrk_identity); break; 
        }
      }

      rc = service_instance.workers_size(workers_spool_size);
      if (rc) { LogError(rc, fctName, "get worker's spool size"); break; }
      if (!workers_spool_size)
      {
        // требуется первоначальное размещение пула заданного размера
        rc = service_instance.workers_alloc(WORKERS_SPOOL_MAXLEN);
        if (rc) { LogError(rc, fctName, "set waiting workers quantity failure"); break; }
        workers_spool_size = WORKERS_SPOOL_MAXLEN;
      }

      // Номер искомого Обработчика в спуле есть worker_idx
      rc = service_instance.workers_at(worker_idx, worker_instance);
      // Если Обработчика в состоянии DISARMED по данному индексу не окажется,
      // его следует создать.
      if (MCO_E_EMPTYVECTOREL == rc)
      {
        rc = service_instance.workers_put(worker_idx, worker_instance);
        if (rc) { LogError(rc, fctName, "put new worker to spool failure"); break; }
        rc = service_instance.checkpoint();
      }
      if (rc) { LogError(rc, fctName, "get worker from spool failure"); break; }

      rc = worker_instance.identity_put(wrk_identity, strlen(wrk_identity));
      if (rc) { LogError(rc, fctName, "worker identity put failure"); break; }

      // пометим как "ЗАНЯТ"
      rc = worker_instance.state_put(ARMED);
      if (rc) { LogError(rc, fctName, "worker's inprocess set failure"); break; }

      /* Установить новое значение expiration */
      if (GetTimerValue(now_time))
      {
        next_heartbeat_time.tv_nsec = now_time.tv_nsec;
        next_heartbeat_time.tv_sec = now_time.tv_sec + HEARTBEAT_PERIOD_VALUE;
      }
      else { LogError(rc, fctName, "calculation expiration time"); break; }

      rc = worker_instance.expiration_write(xdb_next_heartbeat_time);
      if (rc) { LogError(rc, fctName, "set worker's expiration time"); break; }
      rc = xdb_next_heartbeat_time.sec_put(next_heartbeat_time.tv_sec);
      if (rc) { LogError(rc, fctName, "set seconds of expiration time"); break; }
      rc = xdb_next_heartbeat_time.nsec_put(next_heartbeat_time.tv_nsec);
      if (rc) { LogError(rc, fctName, "set nanoseconds of expiration time"); break; }
      rc = worker_instance.expiration_write(xdb_next_heartbeat_time);
      if (rc) { LogError(rc, fctName, "set worker's expiration"); break; }

      status = true;
    }
    else // Service was not found
    { 
      LogError(rc, fctName, "workers's owner not found");
      status = false;
    }

    break;
  }

  // Service was allocated in GetServiceById()
  delete srv;

  if (true == status)
    mco_trans_commit(t);
  else
    mco_trans_rollback(t);

  return status;
}


/* ########################################################### */
/* ADD this Worker for specific Service                        */
/* ########################################################### */
bool AddWorkerForService(Worker* worker,
        const char* worker_identity,
        int64_t service_id)
{
  const char *fctName = "AddWorkerForService";
  bool status;

  worker = new Worker;
  worker->m_service_id = service_id;
  strncpy(worker->m_identity, worker_identity, sizeof(worker->m_identity));
  worker->m_identity[IDENTITY_MAXLEN] = '\0';
  worker->m_state = DISARMED;
  memset((void*)&worker->m_expiration, '\0', sizeof(worker->m_expiration));

  if (true == (status = PushWorker(worker)))
  {
    LogInfo("Push worker '%s' into service with id=%lld is OK", 
            worker_identity, service_id);
    status = true;
    delete worker;
  }
  else
  {
    LogWarn(fctName, "Can't adding worker '%s' for service id=%lld",
            worker_identity, service_id);
    status = false;
  }

  return status;
}

void MakeSnapshot(const char* msg)
{

  if (false == m_initialized)
  {
    m_snapshot_counter = 0;
    strcpy(m_snapshot_file_prefix, "snap");
    m_initialized = true;
  }

  sprintf(snapshot_file_name, "%s.%s.%03d",
          m_snapshot_file_prefix,
          (NULL == msg)? "xdb" : msg,
          ++m_snapshot_counter);

  if (true == m_save_to_xml_feature) 
    SaveDbToFile(snapshot_file_name);
}

/* --------------------------------- */
/* Save database content as XML file */
/* --------------------------------- */
MCO_RET SaveDbToFile(const char* fname)
{
  const char *fctName = "SaveDbToFile";
  MCO_RET rc = MCO_S_OK;
  mco_xml_policy_t op, np;
  mco_trans_h t;
  FILE* f;

  /* setup policy */
  np.blob_coding = MCO_TEXT_BASE64;
  np.encode_lf = MCO_YES;
  np.encode_nat = MCO_NO;
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

  f = fopen(fname, "wb");

  /* export content of the database to a file */
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_HIGH, &t);

  if (rc == MCO_S_OK)
  {
       /* setup XML subsystem*/
      mco_xml_get_policy(t, &op);
      rc = mco_xml_set_policy(t, &np);
      if (MCO_S_OK != rc)
      {
        LogError(rc, fctName, "Unable to set xml policy");
      }
      rc = mco_db_xml_export(t, f, file_writer);

      /* revert xml policy */
      mco_xml_set_policy(t, &op);

      if (rc != MCO_S_OK)
      {
         LogError(rc, fctName, "Exporting to XML");
         mco_trans_rollback(t);
      }
      else
      {
         LogInfo("Exporting database to %s", fname);
         mco_trans_commit(t);
      }
  }
  else
  {
    LogError(rc, fctName, "Opening a transaction");
  }

  fclose(f);
  return rc;
}

/* ---------------------------- */
/* Change Worker state to ARMED */
/* ---------------------------- */
Worker* PopWorker(Service *service)
{
  const char   *fctName = "PopWorker";
  MCO_RET       rc;
  mco_trans_h   t;
  xdb_broker::XDBService    service_instance;
  xdb_broker::XDBWorker     worker_instance;
  uint2         workers_spool_size = 0;
  autoid_t      aid = 0;
  Worker       *worker = NULL;
  uint2         awaiting_worker_idx = 0;

  assert(service);
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    rc = xdb_broker::XDBService::autoid::find(t, service->m_id, service_instance);
    if (rc) { LogError(rc, fctName, "locating service id=%lld failure", service->m_id); break; }

    rc = service_instance.workers_size(workers_spool_size);
    if (rc) { LogError(rc, fctName, 
            "get service's waiting workers queue size failure"); break; }
    
    if (!workers_spool_size)
    {
       LogWarn(fctName, "waiting workers spool is empty");
       break;
    }

    if ((uint2)-1 == (awaiting_worker_idx = LocatingFirstOccurence(service_instance, ARMED)))
    {
       LogWarn(fctName, "no one waiting workers in spool");
       break;
    }

    rc = service_instance.workers_at(awaiting_worker_idx, worker_instance);
    if (rc) { LogError(rc, fctName, 
            "get service's first awaiting worker failure"); break; }
    aid = service->m_id;

    rc = worker_instance.state_put(IN_PROCESS);
    if (rc) { LogError(rc, fctName, "change worker state failure"); break; }

    worker = LoadWorker(t, aid, worker_instance);
    mco_trans_commit(t);
    break;
  }

  if (rc)
    mco_trans_rollback(t);

  return worker;
}

/* ------------------------------------- */
/* Detach from database and stop runtime */
/* ------------------------------------- */
void Disconnect()
{
  const char *fctName = "Disconnect";
  MCO_RET rc;

  rc = mco_db_disconnect(m_db);
  if (rc) LogError(rc, fctName, "Can't disconnection");

  rc = mco_db_close(DATABASE_NAME);
  if (rc) LogError(rc, fctName, "Can't close database '%s'", DATABASE_NAME);

  rc = mco_runtime_stop();
  if (rc) LogError(rc, fctName, "Can't stop eXtrmeneDB runtime");
}

/* -------------------------------------- */
/* Change Worker state to IN_PROCESS|BUSY */
/* -------------------------------------- */
bool RemoveWorker(Worker *wrk)
{
  const char   *fctName = "RemoveWorker";
  bool          status = false;
  MCO_RET       rc;
  mco_trans_h   t;
  xdb_broker::XDBService service_instance;
  xdb_broker::XDBWorker  worker_instance;
  uint2         worker_index;
  char          identity[IDENTITY_MAXLEN+1];

  assert(wrk);
  strncpy(identity, wrk->m_identity, IDENTITY_MAXLEN);
  identity[IDENTITY_MAXLEN] = '\0';

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
            "Disabling worker '%s' of non-existense service id=%lld",
            identity, wrk->m_service_id);
        break;
      }
      if (rc) { LogError(rc, fctName, "Worker '%s' location", identity); break; }

      // номер искомого Обработчика в спуле есть worker_index
      rc = service_instance.workers_at(worker_index, worker_instance);
      if (rc) 
      { 
        LogError(rc, fctName, "Get worker '%s' at pool's index=%d",
            identity, worker_index); break; 
      }

      // Не удалять, пометить как "неактивен"
      rc = worker_instance.state_put(DISARMED);
      if (rc) 
      { 
        LogError(rc, fctName, "Disarming worker '%s'", identity); break;
      }

      mco_trans_commit(t);
      status = true;
      break;
  }

  if (rc)
    mco_trans_rollback(t);

  return status;
}

/* -----------------------------*/
/* Remove Service from database */
/* -----------------------------*/
bool RemoveService(const char *name)
{
  const char *fctName = "RemoveService";
  bool        status = false;
  xdb_broker::XDBService instance;
  MCO_RET        rc;
  mco_trans_h    t;

  assert(name);
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    if (rc) { LogError(rc, fctName, "Starting transaction"); break; }

    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::XDBService::PK_name::find(t, name, strlen(name), instance);
    if (MCO_S_NOTFOUND == rc) 
    { 
        LogError(rc, fctName, "Removed service '%s' doesn't exists", name); break;
    }
    if (rc) { LogError(rc, fctName, "Searching service '%s'", name); break; }
    
    rc = instance.remove();
    if (rc) { LogError(rc, fctName, "Removing service"); break; }

    status = true;
    mco_trans_commit(t);
    break;
  }

  if (rc)
    mco_trans_rollback(t);

  return status;
}

/* ############################################################ */

int main(int argc, char **argv)
{
  const char *fctName = "SAMPLE2";
  bool status = false;

  /* Init global variables for snapshot making */
  m_initialized = false;
  m_snapshot_counter = 0;

  /* Start eXtremeDB runtime */
  status = Connect();
  while (true == status)
  {
    LogInfo("Connection is OK");

/* ########################################################### */
/* ADD service1                                                */
/* ########################################################### */
    if (NULL == (service1 = AddService(service_name_1)))
    {
      LogWarn(fctName, "Can't add service '%s'", service_name_1);
      break;
    }
/* ======> service1 is added */
    service1_id = service1->m_id;
    LogInfo("Adding service '%s' (id=%lld) is OK",
            service_name_1, service1_id);;

/* ########################################################### */
/* ADD service2                                                */
/* ########################################################### */
    if (NULL == (service2 = AddService(service_name_2)))
    {
      LogWarn(fctName, "Can't add service '%s'", service_name_2);
      break;
    }
/* ======> service2 is added */
    service2_id = service2->m_id;
    LogInfo("Adding service '%s' (id=%lld) is OK", 
            service_name_2, service2_id);
    
    MakeSnapshot("ONLY_SERVICES");
/* ########################################################### */
/* ADD Workers into specific Services                          */
/* ########################################################### */
    AddWorkerForService(worker, worker_identity_1, service1_id);
    MakeSnapshot("ADD_WRK_1");
    AddWorkerForService(worker, worker_identity_2, service1_id);
    MakeSnapshot("ADD_WRK_2");
    AddWorkerForService(worker, worker_identity_3, service2_id);
    MakeSnapshot("ADD_WRK_3");

/* ########################################################### */
/* Remove Workers from Service1                                */
/* ########################################################### */
    if (NULL == (worker = PopWorker(service1)))
    {
      LogWarn(fctName, "Can't get worker from service '%s'", service_name_1);
      break;
    }
    LogInfo("Pop worker '%s' from '%s' is OK",
            worker->m_identity, service1->m_name);
    MakeSnapshot("AFTER_POP_WRK_1");

#if 1
    if (false == (status = RemoveWorker(worker)))
    {
      LogWarn(fctName, "Can't remove worker '%s' from '%s'",
            worker->m_identity, service1->m_name);
    }
    else
    {
      LogInfo("Remove worker '%s' from '%s' is OK",
              worker->m_identity, service1->m_name);
    }
    MakeSnapshot("AFTER_DIS_WRK_1");
#else
    LogInfo("SKIP DISABLING WORKER '%s'", worker->m_identity);
#endif
    delete worker;

    if (NULL == (worker = PopWorker(service1)))
    {
      fprintf(stderr, "Can't get worker from service '%s'", service_name_1);
      break;
    }
    LogInfo("Pop worker '%s' from '%s' is OK", 
        worker->m_identity, service1->m_name);
    MakeSnapshot("AFTER_POP_WRK_2");

#if 1
    if (false == (status = RemoveWorker(worker)))
    {
      LogWarn(fctName, "Can't remove worker '%s' from '%s'", 
            worker->m_identity, service1->m_name);
    }
    else
    {
      LogInfo("Removing worker '%s' from '%s' is OK", 
              worker->m_identity, service1->m_name);
    }
    MakeSnapshot("AFTER_DIS_WRK_2");
#else
    LogInfo("SKIP DISABLING WORKER '%s'", worker->m_identity);
#endif
    delete worker;
    

/* ########################################################### */
/* REMOVE SERVICE1                                             */
/* ########################################################### */
#if 1
    if (false == (status = RemoveService(service_name_1)))
    {
      LogWarn(fctName, "Can't remove service '%s'", service_name_1);
      break;
    }
    LogInfo("Removing service '%s' is OK", service_name_1);
#else
    LogInfo("SKIP REMOVING SERVICE '%s'", service_name_1);
#endif
    delete service1;

/* ########################################################### */
/* REMOVE WORKERS OF SERVICE2                                  */
/* ########################################################### */
    worker = PopWorker(service2);
    if (NULL == (worker = PopWorker(service2)))
    {
      LogWarn(fctName, "Can't pop the worker from service '%s'", service_name_2);
      break;
    }
    LogInfo("Pop worker '%s' from '%s' is OK", 
        worker->m_identity, service_name_2);
#if 1
    if (false == (status = RemoveWorker(worker)))
    {
      LogWarn(fctName, "Can't removing worker '%s' from '%s'", 
            worker->m_identity, service2->m_name);
    }
    else
    {
      LogInfo("Removing worker '%s' from '%s' is OK", 
              worker->m_identity, service2->m_name);
    }
#else 
    LogInfo("SKIP POPING WORKER FROM SERVICE '%s'", service_name_2);
#endif
    delete worker;

/* ########################################################### */
/* REMOVE SERVICE2                                             */
/* ########################################################### */
#if 1
    if (false == (status = RemoveService(service_name_2)))
    {
      LogWarn(fctName, "Can't remove service '%s'", service_name_2);
      break;
    }
    LogInfo("Removing service '%s' is OK", service_name_2);
#else
    LogInfo("SKIP REMOVING SERVICE '%s'", service_name_2);
#endif
    delete service2;

    LogInfo("ALL DONE");
    status = true;
    break;
  }

  /* Stop eXtremeDB runtime and close database */
  Disconnect();

  return (status == true)? 0 : -1;
}
