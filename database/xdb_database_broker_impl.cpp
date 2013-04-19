#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "dat/xdb_broker.hpp"
#include "xdb_database_broker.hpp"
#include "xdb_database_broker_impl.hpp"
#include "xdb_database_service.hpp"

const int   SEGSZ = 1024 * 1024 * 1; // 1M
const int   MAP_ADDRESS = 0x20000000;

#ifndef MCO_PLATFORM_X64
    static const int PAGESIZE = 128;
#else 
    static const int PAGESIZE = 256;
#endif 

#ifdef DISK_DATABASE
    #define DB_DISK_CACHE   (4 * 1024 * 1024)
    #define DB_DISK_PAGE_SIZE 4096

    #ifndef DB_LOG_TYPE
        #define DB_LOG_TYPE REDO_LOG
    #endif 
#else 
    #define DB_DISK_CACHE   0
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
    const XDBDatabaseBroker *self, 
    const char *name)
{
  assert(self);
  assert(name);

  m_self = self;
#ifdef DISK_DATABASE
  m_dbsFileName = new char[strlen(name) + 5];
  m_logFileName = new char[strlen(name) + 5];

  strcpy(m_dbsFileName, name);
  strcat(m_dbsFileName, ".dbs");

  strcpy(m_logFileName, name);
  strcat(m_logFileName, ".log");
#endif
  printf("\tXDBDatabaseBrokerImpl(%p, %s)\n", (void*)self, name);
}

XDBDatabaseBrokerImpl::~XDBDatabaseBrokerImpl()
{
  assert(m_self);

#ifdef DISK_DATABASE
  delete m_dbsFileName;
  delete m_logFileName;
#endif

  printf("\t~XDBDatabaseBrokerImpl(%p, %s)\n", 
        (void*)m_self,
        ((XDBDatabase*)m_self)->DatabaseName());
}

void XDBDatabaseBrokerImpl::LogError(MCO_RET rc, 
            const char *functionName, 
            const char *msg)
{
    const char *empty = "";
    printf("E %s: %s [rc=%d]\n", 
        functionName? functionName : empty,
        msg? msg : empty,
        rc);
}

bool XDBDatabaseBrokerImpl::Open()
{
  switch (((XDBDatabase*)m_self)->State())
  {
    case XDBDatabase::OPENED:
      printf("\tTry to reopen database %s\n",
        ((XDBDatabase*)m_self)->DatabaseName());
    break;

    case XDBDatabase::CONNECTED:
    case XDBDatabase::CLOSED:
        AttachToInstance();
    break;

    case XDBDatabase::DISCONNECTED:
      printf("\tTry to reopen disconnected database %s\n",
        ((XDBDatabase*)m_self)->DatabaseName());
    break;

    default:
      printf("\tUnknown database %s state %d\n",
        ((XDBDatabase*)m_self)->DatabaseName(),
        (int)((XDBDatabase*)m_self)->State());
    break;
  }
}

bool XDBDatabaseBrokerImpl::AttachToInstance()
{
    MCO_RET rc;

    /* подключиться к базе данных, предполагая что она создана */
    printf("\nattaching to instance '%s'\n",
           ((XDBDatabase*)m_self)->DatabaseName());
    rc = mco_db_connect(((XDBDatabase*)m_self)->DatabaseName(),
            &db);

    /* ошибка - экземпляр базы не найден, попробуем создать её */
    if (rc == MCO_E_NOINSTANCE)
    {
        printf("'%s' instance not found, create\n",
            ((XDBDatabase*)m_self)->DatabaseName());
        /*
         * TODO: Использование mco_db_open() является запрещенным,
         * начиная с версии 4 и старше
         */
#if EXTREMEDB_VER >= 40
        rc = mco_db_open_dev(((XDBDatabase*)m_self)->DatabaseName(),
                         xdb_broker_get_dictionary(),
                         (void*)MAP_ADDRESS,
                         SEGSZ + DB_DISK_CACHE,
                         PAGESIZE);
#else
        rc = mco_db_open(((XDBDatabase*)m_self)->DatabaseName(),
                         xdb_broker_get_dictionary(),
                         (void*)MAP_ADDRESS,
                         SEGSZ + DB_DISK_CACHE,
                         PAGESIZE);
#endif

#ifdef DISK_DATABASE
        printf("Disk database '%s' opening\n", m_dbsFileName);
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
            printf("\nerror creating disk database");
            return false;
        }
#endif

        /* подключиться к базе данных, т.к. она только что создана */
        printf("connecting to instance '%s'\n", 
            ((XDBDatabase*)m_self)->DatabaseName());
        rc = mco_db_connect(((XDBDatabase*)m_self)->DatabaseName(), &db);
    }

    /* ошибка создания экземпляра - выход из системы */
    if (rc)
    {
        printf("\nCould not create instance: %d\n", rc);
        return false;
    }

    ((XDBDatabase*)m_self)->TransitionToState(XDBDatabase::OPENED);
    return true;
}

bool XDBDatabaseBrokerImpl::AddService(const char *name)
{
  const char    *fctName = "XDBDatabaseBrokerImpl::AddService";
  bool           status = false;
  xdb_broker::Services instance;
  MCO_RET        rc;
  mco_trans_h    t;

  assert(name);
  rc = mco_trans_start(db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    if (rc) { LogError(rc, fctName, "transaction starting failure"); break; }

    printf("creating...\n");
    rc = instance.create(t);
    if (rc) { LogError(rc, fctName, "instance creating failure"); break; }

    printf("name_put...\n");
    rc = instance.name_put(name, strlen(name));
    if (rc) { LogError(rc, fctName, "name setting failure"); break; }

    printf("checkpoint...\n");
    rc = instance.checkpoint();
    if (rc) { LogError(rc, fctName, "checkpointing failure"); break; }

    printf("commit...\n");
    rc = mco_trans_commit(t);
    if (rc) { LogError(rc, fctName, "transaction commitment failure"); break; }

    status = true;
    break;
  }

  if (rc)
    mco_trans_rollback(t);

  return status;
}

#if 0
  XDBService *service = NULL;
  if (NULL != (service = GetServiceByName(name)))
  {
      oid.id = service->GetID();
      rc = instance.create(t, &oid);
      if (rc) { LogError(rc, fctName, "creating failure"); break; }
      rc = instance.remove();
      if (rc) { LogError(rc, fctName, "removing failure"); break; }
      delete service;
  }
#endif

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
  xdb_broker::Services instance;
  MCO_RET        rc;
  mco_trans_h    t;

  assert(name);
  rc = mco_trans_start(db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    if (rc) { LogError(rc, fctName, "transaction starting failure"); break; }

    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::Services::pair::find(t, name, strlen(name), instance);
    if (MCO_S_NOTFOUND == rc) 
    { 
        LogError(rc, fctName, "removed service doesn't exists"); break;
    }
    if (rc) { LogError(rc, fctName, "searching failure"); break; }
    
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

/*
 * Найти в базе данных запись о Сервисе по его имени
 * @return Новый объект, представляющий Сервис
 * Вызываюшая сторона должна сама удалить объект возврата
 */
XDBService *XDBDatabaseBrokerImpl::GetServiceByName(const char* name)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::GetServiceByName";
  MCO_RET       rc;
  bool          status = false;
  mco_trans_h   t;
  xdb_broker::Services instance;
  XDBService *service = NULL;
  /* NB: 32 - это размер поля service_name_t из broker.mco */
  char     name_from_db[32+1];
  uint2    name_from_db_length = sizeof(name_from_db)-1;
  autoid_t id;

  rc = mco_trans_start(db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  while (rc == MCO_S_OK)
  {
    /* найти запись в таблице сервисов с заданным именем */
    rc = xdb_broker::Services::pair::find(t, name, strlen(name), instance);

    /* Запись не найдена - нет ошибки */
    if (MCO_S_NOTFOUND == rc) break;

    /* Запись не найдена - есть ошибка - сообщить */
    if (rc) { LogError(rc, fctName, "\t\tlocating service failure"); break; }

    /* Запись найдена - сконструировать объект на основе данных из БД */
    rc = instance.name_get(name_from_db, name_from_db_length);
    if (rc) { LogError(rc, fctName, "\t\tget service name failure"); break; }
    rc = instance.autoid_get(id);
    if (rc) { LogError(rc, fctName, "\t\tget service id failure"); break; }

    service = new XDBService(id, name_from_db);
    break;
  }

  if (rc)
    mco_trans_rollback(t);
  else
    mco_trans_commit(t);


  return service;
}

bool XDBDatabaseBrokerImpl::IsServiceExist(const char *name)
{
  const char   *fctName = "XDBDatabaseBrokerImpl::IsServiceExist";
  MCO_RET       rc;
  bool          status = false;
  mco_trans_h   t;
  xdb_broker::Services instance;

  assert(name);
  rc = mco_trans_start(db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
  if (rc)
  {
    LogError(rc, fctName, NULL);
    mco_trans_rollback(t);
    return false;
  }
  rc = xdb_broker::Services::pair::find(t, name, strlen(name), instance);
  mco_trans_commit(t);

  return (MCO_S_NOTFOUND == rc)? false:true;
}

