#include <assert.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "xdb_database_broker.hpp"
#include "xdb_database_broker_impl.hpp"

const int   SEGSZ = 1024 * 1024 * 10; // 10M
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
        ((XDBDatabaseBroker*)m_self)->DatabaseName());
}

bool XDBDatabaseBrokerImpl::Open()
{
  switch (((XDBDatabaseBroker*)m_self)->State())
  {
    case XDBDatabase::OPENED:
      printf("\tTry to reopen database %s\n",
        ((XDBDatabaseBroker*)m_self)->DatabaseName());
    break;

    case XDBDatabase::CLOSED:
    case XDBDatabase::DISCONNECTED:
        AttachToInstance();
    break;

    default:
      printf("\tUnknown database %s state %d\n",
        ((XDBDatabaseBroker*)m_self)->DatabaseName(),
        (int)((XDBDatabaseBroker*)m_self)->State());
    break;
  }
}

bool XDBDatabaseBrokerImpl::AttachToInstance()
{
    MCO_RET rc;

    /* подключиться к базе данных, предполагая что она создана */
    printf("\nattaching to instance '%s'\n",
           ((XDBDatabaseBroker*)m_self)->DatabaseName());
    rc = mco_db_connect(((XDBDatabaseBroker*)m_self)->DatabaseName(),
            &db);

    /* ошибка - экземпляр базы не найден, попробуем создать её */
    if (rc == MCO_E_NOINSTANCE)
    {
        printf("'%s' instance not found, create\n",
            ((XDBDatabaseBroker*)m_self)->DatabaseName());
        /*
         * TODO: Использование mco_db_open() является запрещенным,
         * начиная с версии 4 и старше
         */
#if EXTREMEDB_VER >= 40
        rc = mco_db_open_dev(((XDBDatabaseBroker*)m_self)->DatabaseName(),
                         xdb_broker_get_dictionary(),
                         (void*)MAP_ADDRESS,
                         SEGSZ + DB_DISK_CACHE,
                         PAGESIZE);
#else
        rc = mco_db_open(((XDBDatabaseBroker*)m_self)->DatabaseName(),
                         xdb_broker_get_dictionary(),
                         (void*)MAP_ADDRESS,
                         SEGSZ + DB_DISK_CACHE,
                         PAGESIZE);
#endif

#ifdef DISK_DATABASE
        printf("Disk database '%s' opening\n", m_dbsFileName);
        rc = mco_disk_open(((XDBDatabaseBroker*)m_self)->DatabaseName(),
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
            ((XDBDatabaseBroker*)m_self)->DatabaseName());
        rc = mco_db_connect(((XDBDatabaseBroker*)m_self)->DatabaseName(), &db);
    }

    /* ошибка создания экземпляра - выход из системы */
    if (rc)
    {
        printf("\nCould not create instance: %d\n", rc);
        return false;
    }

    return true;
}

bool XDBDatabaseBrokerImpl::AddService(const char *name)
{
  bool status = false;

  assert(name);
  printf("\t\tAddService %s\n", name);

  return true;
}

bool XDBDatabaseBrokerImpl::RemoveService(const char *name)
{
  bool status = false;

  assert(name);
  printf("\t\tRemoveService %s\n", name);

  return true;
}

bool XDBDatabaseBrokerImpl::IsServiceExist(const char *name)
{
  bool status = false;

  assert(name);
  printf("\t\tIsServiceExist %s\n", name);

  return true;
}

