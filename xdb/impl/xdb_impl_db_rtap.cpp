#include <new>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h> // exit
#include <stdarg.h>
#include <string.h>

#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "xdb_impl_common.hpp"
#include "xdb_impl_error.hpp"

#include "helper.hpp"
#include "dat/rtap_db.hxx"
#include "xdb_impl_database.hpp"
#include "xdb_impl_db_rtap.hpp"

using namespace xdb;

/* 
 * Включение динамически генерируемых определений 
 * структуры данных для внутренней базы RTAP.
 *
 * Регенерация осуществляется командой mcocomp 
 * на основе содержимого файла rtap_db.mco
 */
#include "dat/rtap_db.hpp"


DatabaseRtapImpl::DatabaseRtapImpl(const char* _name)
{
  Options opt;
  assert(_name);

  // Опции по умолчанию
  setOption(opt, "OF_CREATE",    1);
  setOption(opt, "OF_LOAD_SNAP", 1);
  setOption(opt, "OF_DATABASE_SIZE",   1024 * 1024 * 10);
  setOption(opt, "OF_MEMORYPAGE_SIZE", 1024); // 0..65535
  setOption(opt, "OF_MAP_ADDRESS", 0x25000000);
#if defined USE_EXTREMEDB_HTTP_SERVER
  setOption(opt, "OF_HTTP_PORT", 8082);
#endif

#ifdef DISK_DATABASE
  /* NB: +5 - для ".dbs" и ".log" с завершающим '\0' */
  m_dbsFileName = new char[strlen(m_impl->getName()) + 5];
  m_logFileName = new char[strlen(m_impl->getName()) + 5];

  strcpy(m_dbsFileName, m_impl->getName());
  strcat(m_dbsFileName, ".dbs");

  strcpy(m_logFileName, m_impl->getName());
  strcat(m_logFileName, ".log");

  setOption(opt, "OF_DISK_CACHE_SIZE", 1024 * 1024 * 10);
#else
  setOption(opt, "OF_DISK_CACHE_SIZE", 0);
#endif

  m_impl = new DatabaseImpl(_name, opt, rtap_db_get_dictionary());
}

DatabaseRtapImpl::~DatabaseRtapImpl()
{
  delete m_impl;
}

bool DatabaseRtapImpl::Init()
{
  Error err = m_impl->Open();
  return err.Ok();
}

bool DatabaseRtapImpl::Connect()
{
  Error err = m_impl->Connect();
  return err.Ok();
}

bool DatabaseRtapImpl::Disconnect()
{
  return (m_impl->Disconnect()).Ok();
}

DBState_t DatabaseRtapImpl::State()
{
  return m_impl->State();
}

/*
 * Статический метод, вызываемый из runtime базы данных 
 * при создании нового экземпляра XDBService
 */
MCO_RET DatabaseRtapImpl::new_Point(mco_trans_h /*t*/,
//        XDBService *obj,
        MCO_EVENT_TYPE /*et*/,
        void *p)
{
  DatabaseRtapImpl *self = static_cast<DatabaseRtapImpl*> (p);
  MCO_RET rc;
  autoid_t aid;
  bool status = false;

  assert(self);
//  assert(obj);

  do
  {
  } while (false);

//  LOG(INFO) << "NEW Point "<<obj<<" name '"<<name<<"' self=" << self;

  return MCO_S_OK;
}

/*
 * Статический метод, вызываемый из runtime базы данных 
 * при удалении экземпляра XDBService
 */
MCO_RET DatabaseRtapImpl::del_Point(mco_trans_h /*t*/,
        //XDBService *obj,
        MCO_EVENT_TYPE /*et*/,
        void *p)
{
  DatabaseRtapImpl *self = static_cast<DatabaseRtapImpl*> (p);
  MCO_RET rc;

  assert(self);
//  assert(obj);

  do
  {
  } while (false);

//  LOG(INFO) << "DEL Point "<<obj<<" name '"<<name<<"' self=" << self;

  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::RegisterEvents()
{
  MCO_RET rc;
  mco_trans_h t;

  do
  {
    rc = mco_trans_start(m_impl->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) LOG(ERROR) << "Starting transaction, rc=" << rc;

#if 0
    rc = mco_register_newService_handler(t, 
            &DatabaseRtapImpl::new_Point, 
            static_cast<void*>(this)
#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
            , MCO_AFTER_UPDATE
#endif
            );

    if (rc) LOG(ERROR) << "Registering event on Point creation, rc=" << rc;

    rc = mco_register_delService_handler(t, 
            &DatabaseRtapImpl::del_Point, 
            static_cast<void*>(this));
    if (rc) LOG(ERROR) << "Registering event on Point deletion, rc=" << rc;
#endif

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }
  } while(false);

  if (rc)
   mco_trans_rollback(t);

  return rc;
}

/* Тестовый API сохранения базы */
bool DatabaseRtapImpl::MakeSnapshot(const char* msg)
{
  return (m_impl->SaveAsXML(NULL, msg)).Ok();
}

bool DatabaseRtapImpl::LoadSnapshot(const char* _fname)
{
  bool status = false;

  // Попытаться восстановить восстановить данные в таком порядке:
  // 1. из файла с двоичным дампом
  // 2. из файла в формате стандартного XML-дампа eXtremeDB
  m_impl->LoadSnapshot(_fname);

  if (!getLastError().Ok())
  {
    // TODO Прочитать данные из XML в формате XMLSchema 'rtap_db.xsd'
    // с помощью <Classname>_xml_create() создавать содержимое БД
    LOG(INFO) << "Прочитать данные из XML в формате XMLSchema 'rtap_db.xsd', err "
              << getLastError().code();
  }
  else status = true;

  return status;
}
