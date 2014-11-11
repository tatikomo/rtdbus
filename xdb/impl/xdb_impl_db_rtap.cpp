#include <new>
#include <map>
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
#include "dat/rtap_db.h"
#include "dat/rtap_db.hpp"
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

typedef union
{
  uint8 common;
  uint4 part[2];
} datetime_t;

DatabaseRtapImpl::DatabaseRtapImpl(const char* _name)
{
  Options opt;
  assert(_name);

  // Опции по умолчанию
  setOption(opt, "OF_CREATE",    1);
  setOption(opt, "OF_LOAD_SNAP", 1);
  // TODO: исправить поведение опции OF_SAVE_SNAP - происходит падение с ошибкой MCO_ERR_INDEX
//  setOption(opt, "OF_SAVE_SNAP", 1);
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

  // Инициализация карты функций создания атрибутов
  AttrFuncMapInit();
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

// Связать имена атрибутов с создающими их в БДРВ функциями
// NB: Здесь д.б. прописаны только те атрибуты, которые не создаются в паспортах
bool DatabaseRtapImpl::AttrFuncMapInit()
{
// LABEL пока пропустим, возможно вынесем ее хранение во внешнюю БД (поле 'persistent' eXtremeDB?)
//  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("LABEL",      &xdb::DatabaseRtapImpl::createLABEL));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("SHORTLABEL", &xdb::DatabaseRtapImpl::createSHORTLABEL));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("L_SA",       &xdb::DatabaseRtapImpl::createL_SA));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("VALID",      &xdb::DatabaseRtapImpl::createVALID));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("VALIDACQ",   &xdb::DatabaseRtapImpl::createVALIDACQ));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("DATEHOURM",  &xdb::DatabaseRtapImpl::createDATEHOURM));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("DATERTU",    &xdb::DatabaseRtapImpl::createDATERTU));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("STATUS",     &xdb::DatabaseRtapImpl::createSTATUS));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("VALIDCHANGE",&xdb::DatabaseRtapImpl::createVALIDCHANGE));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("UNITY",      &xdb::DatabaseRtapImpl::createUNITY));
#if 0
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("ALDEST",     &xdb::DatabaseRtapImpl::createALDEST));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("INHIB",      &xdb::DatabaseRtapImpl::createINHIB));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("INHIBLOCAL", &xdb::DatabaseRtapImpl::createINHIBLOCAL));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("MINVAL",     &xdb::DatabaseRtapImpl::createMINVAL));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("VALEX",      &xdb::DatabaseRtapImpl::createVALEX));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("MAXVAL",     &xdb::DatabaseRtapImpl::createMAXVAL));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("VAL",        &xdb::DatabaseRtapImpl::createVAL));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("VALACQ",     &xdb::DatabaseRtapImpl::createVALACQ));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("VALMANUAL",  &xdb::DatabaseRtapImpl::createVALMANUAL));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("ALINHIB",    &xdb::DatabaseRtapImpl::createALINHIB));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("DISPP",      &xdb::DatabaseRtapImpl::createDISPP));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("FUNCTION",   &xdb::DatabaseRtapImpl::createFUNCTION));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("CONVERTCOEFF",   &xdb::DatabaseRtapImpl::createCONVERTCOEFF));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("MNVALPHY",   &xdb::DatabaseRtapImpl::createMNVALPHY));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("MXPRESSURE", &xdb::DatabaseRtapImpl::createMXPRESSURE));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("MXVALPHY",   &xdb::DatabaseRtapImpl::createMXVALPHY));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("NMFLOW",     &xdb::DatabaseRtapImpl::createNMFLOW));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("SUPPLIERMODE",   &xdb::DatabaseRtapImpl::createSUPPLIERMODE));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("SUPPLIERSTATE",  &xdb::DatabaseRtapImpl::createSUPPLIERSTATE));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("SYNTHSTATE", &xdb::DatabaseRtapImpl::createSYNTHSTATE));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("DELEGABLE",  &xdb::DatabaseRtapImpl::createDELEGABLE));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("ETGCODE",    &xdb::DatabaseRtapImpl::createETGCODE));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("SITEFLG",    &xdb::DatabaseRtapImpl::createSITEFL));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("FLGREMOTECMD",   &xdb::DatabaseRtapImpl::createFLGREMOTECMD));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("FLGMAINTENANCE", &xdb::DatabaseRtapImpl::createFLGMAINTENANCE));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("NAMEMAINTENANCE",&xdb::DatabaseRtapImpl::createNAMEMAINTENANCE));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("TSSYNTHETICAL",  &xdb::DatabaseRtapImpl::createTSSYNTHETICAL));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("LOCALFLAG",  &xdb::DatabaseRtapImpl::createLOCALFLAG));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("ALARMBEGIN", &xdb::DatabaseRtapImpl::createALARMBEGIN));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("ALARMBEGINACK",  &xdb::DatabaseRtapImpl::createALARMBEGINACK));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("ALARMENDACK",    &xdb::DatabaseRtapImpl::createALARMENDACK));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("ALARMSYNTH", &xdb::DatabaseRtapImpl::createALARMSYNTH));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("L_TYPINFO",  &xdb::DatabaseRtapImpl::createL_TYPINFO));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("L_EQT",      &xdb::DatabaseRtapImpl::createL_EQT));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("REMOTECONTROL",  &xdb::DatabaseRtapImpl::createREMOTECONTROL));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("NONEXE",     &xdb::DatabaseRtapImpl::createNONEXE));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("RESPNEG",    &xdb::DatabaseRtapImpl::createRESPNEG));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("ACTIONTYP",  &xdb::DatabaseRtapImpl::createACTIONTYP));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("VAL_LABEL",  &xdb::DatabaseRtapImpl::createVAL_LABEL));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("LINK_HIST",  &xdb::DatabaseRtapImpl::createLINK_HIST));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("ACQMOD",     &xdb::DatabaseRtapImpl::createACQMOD));
  m_attr_creation_func_map.insert(AttrCreationFuncPair_t("L_DIPL",     &xdb::DatabaseRtapImpl::createL_DIPL));
#endif

  for (AttrCreationFuncMapIterator_t it = m_attr_creation_func_map.begin();
       it != m_attr_creation_func_map.end();
       it++)
  {
    LOG(INFO) << "GEV: attr='" << it->first << "' func=" << it->second;
  }
  return true;
}

/*
 * Статический метод, вызываемый из runtime базы данных 
 * при создании нового экземпляра XDBService
 */
MCO_RET DatabaseRtapImpl::new_Point(mco_trans_h t,
        XDBPoint *obj,
        MCO_EVENT_TYPE et,
        void *p)
{
  DatabaseRtapImpl *self = static_cast<DatabaseRtapImpl*> (p);
  MCO_RET rc;
  autoid_t aid;
  bool status = false;

  assert(self);
  assert(obj);

  do
  {
  } while (false);

//  LOG(INFO) << "NEW Point "<<obj<<" tag '"<<tag<<"' self=" << self;

  return MCO_S_OK;
}

/*
 * Статический метод, вызываемый из runtime базы данных 
 * при удалении экземпляра XDBService
 */
MCO_RET DatabaseRtapImpl::del_Point(mco_trans_h t,
        XDBPoint *obj,
        MCO_EVENT_TYPE et,
        void *p)
{
  DatabaseRtapImpl *self = static_cast<DatabaseRtapImpl*> (p);
  MCO_RET rc;

  assert(self);
  assert(obj);

  do
  {
  } while (false);

//  LOG(INFO) << "DEL Point "<<obj<<" tag '"<<tag<<"' self=" << self;

  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::RegisterEvents()
{
  MCO_RET rc;
  mco_trans_h t;
  mco_new_point_evnt_handler new_handler = DatabaseRtapImpl::new_Point;
  mco_new_point_evnt_handler delete_handler = DatabaseRtapImpl::del_Point;

  do
  {
    rc = mco_trans_start(m_impl->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) LOG(ERROR) << "Starting transaction, rc=" << rc;

#if EXTREMEDB_VERSION <= 50
    rc = mco_register_new_point_evnt_handler(t, 
            new_handler,
            static_cast<void*>(this)
            );

    if (rc) LOG(ERROR) << "Registering event on Point creation, rc=" << rc;

    rc = mco_register_delete_point_evnt_handler(t, 
            delete_handler,
            static_cast<void*>(this));
    if (rc) LOG(ERROR) << "Registering event on Point deletion, rc=" << rc;
#else
#warning "Registering events in eXtremeDB 4.1 differ than 5.0"
#endif

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; }
  }
  while(false);

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

// ====================================================
// Создание Точки
const Error& DatabaseRtapImpl::create(rtap_db::Point&)
{
  return m_impl->getLastError();
}

// Удаление Точки
const Error& DatabaseRtapImpl::erase(rtap_db::Point&)
{
  return m_impl->getLastError();
}

// Чтение данных Точки
const Error& DatabaseRtapImpl::read(rtap_db::Point&)
{
  return m_impl->getLastError();
}

// Изменение данных Точки
const Error& DatabaseRtapImpl::write(rtap_db::Point& info)
{
  rtap_db::XDBPoint instance;
  autoid_t    passport_aid;
  autoid_t    point_aid;
  MCO_RET     rc = MCO_S_OK;
  mco_trans_h t;

  do
  {
    if (DB_STATE_CONNECTED != State())
    {
        LOG(ERROR) << "Database '" << m_impl->getName()
                   << "' is not connected (state=" << State() << ")";
        setError(rtE_INCORRECT_DB_STATE);
        break;
    }

    if ((info.tag()).empty())
    {
        // Пустое универсальное имя - пропустить эту Точку
        LOG(ERROR) << "Empty point's tag, skipping";
        setError(rtE_STRING_IS_EMPTY);
        break;
    }

    // TODO: Проверить, будем ли мы сохранять эту Точку в БД
    // Если нет -> не начинать транзакцию
    // Если да -> продолжаем
    // 
    // Как проверить? 
    // Сейчас проверка выполняется в createPassport:
    //      если точка игнорируется, функция вернет MCO_E_NOTSUPPORTED
    //
    rc = mco_trans_start(m_impl->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting '" << info.tag() << "' transaction, rc=" << rc; break; }

    // Создать объект-представление Точки в БД
    rc = instance.create(t);
    if (rc) { LOG(ERROR) << "Creating '" << info.tag() << "' instance, rc=" << rc; break; }

    // Получить его уникальный числовой идентификатор
    rc = instance.autoid_get(point_aid);
    if (rc) { LOG(ERROR) << "Getting point '" << info.tag() << "' id, rc=" << rc; break; }

    // Вставка данных в таблицу соответствующего паспорта
    // Возвращается идентификатор паспорта
    rc = createPassport(t, instance, info, passport_aid, point_aid);
    if (rc) { LOG(ERROR) << "Creating '" << info.tag() << "' passport, rc=" << rc; break; }

    // Вставка данных в основную таблицу точек
    rc = createPoint(t, instance, info, info.tag(), passport_aid, point_aid);
    if (rc) { LOG(ERROR) << "Creating '" << info.tag() << "' point, rc=" << rc; break; }

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment '" << info.tag() << "' transaction, rc=" << rc; }
  }
  while(false);

  if (rc)
    mco_trans_rollback(t);

  return m_impl->getLastError();
}

// Блокировка данных Точки от изменения в течение заданного времени
const Error& DatabaseRtapImpl::lock(rtap_db::Point&, int)
{
  return m_impl->getLastError();
}

const Error& DatabaseRtapImpl::unlock(rtap_db::Point&)
{
  return m_impl->getLastError();
}

// Группа функций управления
const Error& DatabaseRtapImpl::Control(rtDbCq& info)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// Группа функций управления
const Error& DatabaseRtapImpl::Query(rtDbCq& info)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// Группа функций управления
const Error& DatabaseRtapImpl::Config(rtDbCq& info)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// Создать паспорт для Точки типа TS
MCO_RET DatabaseRtapImpl::createPassportTS(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  uint1     byte1;
  rtap_db::Attrib* attr = NULL;

  do
  {
    rc = passport_instance.create(t);
    if (rc) { LOG(ERROR) << "Creating TS passport"; break; }

    // 1 INHIBLOCAL
    if (NULL != (attr = info.attrib(RTDB_ATT_INHIBLOCAL)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.INHIBLOCAL_put(byte1);
    if (rc) { LOG(ERROR) << "Set TS passport " << info.tag() <<" INHIBLOCAL, rc=" << rc; break; }

    // 2 ALDEST
    if (NULL != (attr = info.attrib(RTDB_ATT_ALDEST)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.ALDEST_put(byte1);
    if (rc) { LOG(ERROR) << "Set TS passport " << info.tag() << " ALDEST, rc=" << rc; break; }

    rc = passport_instance.autoid_get(passport_aid);
    if (rc) { LOG(ERROR) << "Getting TS passport " << info.tag() << " id, rc=" << rc; break; }
  }
  while (false);

  if (rc)
  {
    // Была ошибки при создании паспорта.
    // Необходимо удалить его остатки для сохранения когерентности
    passport_aid = point_aid; // для сохранения ссылочной целостности
    passport_instance.remove();
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportTM(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TM_passport passport_instance;
  rtap_db::Attrib* attr = NULL;
  double analog_val;

  do
  {
    rc = passport_instance.create(t);
    if (rc) { LOG(ERROR) << "Creating TM passport"; break; }

    // 1 MNVALPHY
    if (NULL != (attr = info.attrib(RTDB_ATT_MNVALPHY)))
      analog_val = atof(attr->value().c_str());
    else analog_val = 0.0;
    rc = passport_instance.MNVALPHY_put(analog_val);
    if (rc) { LOG(ERROR) << "Set TM passport " << info.tag() <<" MNVALPHY, rc=" << rc; break; }

    // 2 MXVALPHY
    if (NULL != (attr = info.attrib(RTDB_ATT_MXVALPHY)))
      analog_val = atof(attr->value().c_str());
    else analog_val = 1.0;
    rc = passport_instance.MXVALPHY_put(analog_val);
    if (rc) { LOG(ERROR) << "Set TM passport " << info.tag() <<" MXVALPHY, rc=" << rc; break; }

    // 3 CONVERTCOEFF
    if (NULL != (attr = info.attrib(RTDB_ATT_CONVERTCOEFF)))
      analog_val = atof(attr->value().c_str());
    else analog_val = 1.0;
    rc = passport_instance.CONVERTCOEFF_put(analog_val);
    if (rc) { LOG(ERROR) << "Set TM passport " << info.tag() <<" CONVERTCOEFF, rc=" << rc; break; }

    //rc = passport_instance.LINK_HIST_put();

    rc = passport_instance.autoid_get(passport_aid);
    if (rc) { LOG(ERROR) << "Getting TM passport " << info.tag() <<" id, rc=" << rc; break; }
  }
  while (false);

  if (rc)
  {
    passport_aid = point_aid; // для сохранения ссылочной целостности
    passport_instance.remove();
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportTR(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TR_passport passport_instance;
  passport_aid = point_aid;
  rtap_db::Attrib* attr = NULL;
  double analog_val;
  uint1 byte1;

  do
  {
    rc = passport_instance.create(t);
    if (rc) { LOG(ERROR) << "Creating TR passport"; break; }

    // 1 MINVAL
    if (NULL != (attr = info.attrib(RTDB_ATT_MINVAL)))
      analog_val = atof(attr->value().c_str());
    else analog_val = 0.0;
    rc = passport_instance.MINVAL_put(analog_val);
    if (rc) { LOG(ERROR) << "Set TR passport " << info.tag() <<" MAXVAL, rc=" << rc; break; }

    // 2 MAXVAL
    if (NULL != (attr = info.attrib(RTDB_ATT_MAXVAL)))
      analog_val = atof(attr->value().c_str());
    else analog_val = 0.0;
    rc = passport_instance.MAXVAL_put(analog_val);
    if (rc) { LOG(ERROR) << "Set TR passport " << info.tag() <<" MINVAL, rc=" << rc; break; }

    // 3 VALEX
    if (NULL != (attr = info.attrib(RTDB_ATT_VALEX)))
      analog_val = atof(attr->value().c_str());
    else analog_val = 0.0;
    rc = passport_instance.VALEX_put(analog_val);
    if (rc) { LOG(ERROR) << "Set TR passport " << info.tag() <<" VALEX, rc=" << rc; break; }

    // 4 CONFREMOTECMD
    if (NULL != (attr = info.attrib(RTDB_ATT_CONFREMOTECMD)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.CONFREMOTECMD_put(byte1);
    if (rc) { LOG(ERROR) << "Set TR passport " << info.tag() <<" CONFREMOTECMD, rc=" << rc; break; }

    // 5 FLGREMOTECMD
    if (NULL != (attr = info.attrib(RTDB_ATT_FLGREMOTECMD)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.FLGREMOTECMD_put(byte1);
    if (rc) { LOG(ERROR) << "Set TR passport " << info.tag() <<" FLGREMOTECMD, rc=" << rc; break; }

    // 6 REMOTECONTROL
    if (NULL != (attr = info.attrib(RTDB_ATT_REMOTECONTROL)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.REMOTECONTROL_put(byte1);
    if (rc) { LOG(ERROR) << "Set TR passport " << info.tag() <<" REMOTECONTROL, rc=" << rc; break; }

//    rc = passport_instance.autoid_get(passport_aid);
//    if (rc) { LOG(ERROR) << "Getting TR passport " << info.tag() <<" id, rc=" << rc; break; }
  }
  while (false);

  if (rc)
  {
    passport_instance.remove();
    passport_aid = point_aid; // для сохранения ссылочной целостности
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportTSA(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TSA_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportTSA";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportTSC(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportTSC";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportTC(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportTC";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportAL(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportAL";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportICM(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportICM";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportICS(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportICS";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportTL(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TL_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportICM";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportVA(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::VA_passport passport_instance;
  rtap_db::Attrib* attr = NULL;
  uint1 byte1;
  std::string literal_val;

  do
  {
    rc = passport_instance.create(t);
    if (rc) { LOG(ERROR) << "Creating VA passport"; break; }

    // 1 LOCALFLAG
    if (NULL != (attr = info.attrib(RTDB_ATT_LOCALFLAG)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.LOCALFLAG_put(byte1);
    if (rc) { LOG(ERROR) << "Set VA passport " << info.tag() <<" LOCALFLAG, rc=" << rc; break; }

    // 2 INHIB
    if (NULL != (attr = info.attrib(RTDB_ATT_INHIB)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.INHIB_put(byte1);
    if (rc) { LOG(ERROR) << "Set VA passport " << info.tag() <<" INHIB, rc=" << rc; break; }

    // 3 INHIBLOCAL
    if (NULL != (attr = info.attrib(RTDB_ATT_INHIBLOCAL)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.INHIBLOCAL_put(byte1);
    if (rc) { LOG(ERROR) << "Set VA passport " << info.tag() <<" INHIBLOCAL, rc=" << rc; break; }

    // 4 ALINHIB
    if (NULL != (attr = info.attrib(RTDB_ATT_ALINHIB)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.ALINHIB_put(byte1);
    if (rc) { LOG(ERROR) << "Set VA passport " << info.tag() <<" ALINHIB, rc=" << rc; break; }

    // 5 CONFREMOTECMD
    if (NULL != (attr = info.attrib(RTDB_ATT_CONFREMOTECMD)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.CONFREMOTECMD_put(byte1);
    if (rc) { LOG(ERROR) << "Set VA passport " << info.tag() <<" CONFREMOTECMD, rc=" << rc; break; }

    // 6 FLGREMOTECMD
    if (NULL != (attr = info.attrib(RTDB_ATT_FLGREMOTECMD)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.FLGREMOTECMD_put(byte1);
    if (rc) { LOG(ERROR) << "Set VA passport " << info.tag() <<" FLGREMOTECMD, rc=" << rc; break; }

    // 7 REMOTECONTROL
    if (NULL != (attr = info.attrib(RTDB_ATT_REMOTECONTROL)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.REMOTECONTROL_put(byte1);
    if (rc) { LOG(ERROR) << "Set VA passport " << info.tag() <<" REMOTECONTROL, rc=" << rc; break; }

    // 8 FLGMAINTENANCE
    if (NULL != (attr = info.attrib(RTDB_ATT_FLGMAINTENANCE)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.FLGMAINTENANCE_put(byte1);
    if (rc) { LOG(ERROR) << "Set VA passport " << info.tag() <<" FLGMAINTENANCE, rc=" << rc; break; }

    // 9 TSSYNTHETICAL
    if (NULL != (attr = info.attrib(RTDB_ATT_TSSYNTHETICAL)))
      byte1 = atoi(attr->value().c_str());
    else byte1 = 0;
    rc = passport_instance.TSSYNTHETICAL_put(byte1);
    if (rc) { LOG(ERROR) << "Set VA passport " << info.tag() <<" TSSYNTHETICAL, rc=" << rc; break; }

    // 10 NAMEMAINTENANCE;
    if (NULL != (attr = info.attrib(RTDB_ATT_NAMEMAINTENANCE)))
      literal_val.assign(attr->value());
    else literal_val.clear();
    rc = passport_instance.NAMEMAINTENANCE_put(literal_val.c_str(), static_cast<uint2>(literal_val.size()));
    if (rc) { LOG(ERROR) << "Set VA passport " << info.tag() <<" NAMEMAINTENANCE, rc=" << rc; break; }

    rc = passport_instance.autoid_get(passport_aid);
    if (rc) { LOG(ERROR) << "Getting VA passport " << info.tag() <<" id, rc=" << rc; break; }
  }
  while (false);

  if (rc)
  {
    passport_aid = point_aid; // для сохранения ссылочной целостности
    passport_instance.remove();
  }

  return rc;
}


MCO_RET DatabaseRtapImpl::createPassportATC(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportATC";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportGRC(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportGRC";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportSV(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportSV";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportSDG(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportSDG";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportSSDG(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportSSDG";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportSCP(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportSCP";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportDIR(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportDIR";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportDIPL(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportDIPL";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportMETLINE(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportMETLINE";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportESDG(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportESDG";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportSCPLINE(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportSCPLINE";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportTLLINE(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportTLLINE";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportAUX1(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportAUX1";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportAUX2(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportAUX2";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportSITE(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportSITE";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportSA(
    mco_trans_h         t,
    rtap_db::XDBPoint&  instance,
    rtap_db::Point&     info,
    autoid_t&           passport_aid,
    autoid_t&           point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportSA";
  return rc;
}

//
// Создать паспорт заданной Точки
//
// Процедура выполнения
// Проверка:
//   Создается ли Паспорт для данного класса Точки?
//      Если да:
//          вызывается функция создания соответствующего класса
//      Если нет:
//          идентификатор паспорта присваивается равным 
//          идентификатору экземпляра Точки
//      Если класс точки неизвестен:
//          Возвращается код MCO_E_UNSUPPORTED
//
// Создается соответствующий класс
//
MCO_RET DatabaseRtapImpl::createPassport(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_id,
    autoid_t& point_id)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_E_UNSUPPORTED;

  // для сохранения ссылочной целостности пока считаем, что ID Паспорта 
  // совпадает с ID Точки. Если в функции создания Паспорта сохранение 
  // данных пройдет успешно, ID Паспорта изменится на уникальное значение. 
  passport_id = point_id;

  switch(objclass)
  {
    case TS:  /* 00 */
    rc = createPassportTS(t, instance, info, passport_id, point_id);
    break;

    case TM:  /* 01 */
    rc = createPassportTM(t, instance, info, passport_id, point_id);
    break;

    case TR:  /* 02 */
    rc = createPassportTR(t, instance, info, passport_id, point_id);
    break;

    case TSA: /* 03 */
    rc = createPassportTSA(t, instance, info, passport_id, point_id);
    break;

    case TSC: /* 04 */
    rc = createPassportTSC(t, instance, info, passport_id, point_id);
    break;

    case TC:  /* 05 */
    rc = createPassportTC(t, instance, info, passport_id, point_id);
    break;

    case AL:  /* 06 */
    rc = createPassportAL(t, instance, info, passport_id, point_id);
    break;

    case ICS: /* 07 */
    rc = createPassportICS(t, instance, info, passport_id, point_id);
    break;

    case ICM: /* 08 */
    rc = createPassportICM(t, instance, info, passport_id, point_id);
    break;

    case TL:  /* 16 */
    rc = createPassportTL(t, instance, info, passport_id, point_id);
    break;

    case VA:  /* 19 */
    rc = createPassportVA(t, instance, info, passport_id, point_id);
    break;

    case ATC: /* 21 */
    rc = createPassportATC(t, instance, info, passport_id, point_id);
    break;

    case GRC: /* 22 */
    rc = createPassportGRC(t, instance, info, passport_id, point_id);
    break;

    case SV:  /* 23 */
    rc = createPassportSV(t, instance, info, passport_id, point_id);
    break;

    case SDG: /* 24 */
    rc = createPassportSDG(t, instance, info, passport_id, point_id);
    break;

    case SSDG:/* 26 */
    rc = createPassportSSDG(t, instance, info, passport_id, point_id);
    break;

    case SCP: /* 28 */
    rc = createPassportSCP(t, instance, info, passport_id, point_id);
    break;

    case DIR: /* 30 */
    rc = createPassportDIR(t, instance, info, passport_id, point_id);
    break;

    case DIPL:/* 31 */
    rc = createPassportDIPL(t, instance, info, passport_id, point_id);
    break;

    case METLINE: /* 32 */
    rc = createPassportMETLINE(t, instance, info, passport_id, point_id);
    break;

    case ESDG:/* 33 */
    rc = createPassportESDG(t, instance, info, passport_id, point_id);
    break;

    case SCPLINE: /* 35 */
    rc = createPassportSCPLINE(t, instance, info, passport_id, point_id);
    break;

    case TLLINE:  /* 36 */
    rc = createPassportTLLINE(t, instance, info, passport_id, point_id);
    break;

    case AUX1:/* 38 */
    rc = createPassportAUX1(t, instance, info, passport_id, point_id);
    break;

    case AUX2:/* 39 */
    rc = createPassportAUX2(t, instance, info, passport_id, point_id);
    break;

    case SITE:/* 45 */
    rc = createPassportSITE(t, instance, info, passport_id, point_id);
    break;

    case SA:  /* 50 */
    rc = createPassportSA(t, instance, info, passport_id, point_id);
    break;


    // Точки, не имеющие паспорта
    case PIPE:/* 11 */
    case PIPELINE:/* 15 */
    case SC:  /* 20 */
    case RGA: /* 25 */
    case BRG: /* 27 */
    case STG: /* 29 */
    case SVLINE:  /* 34 */
    case INVT:/* 37 */
    case FIXEDPOINT:  /* 79 */
    case HIST: /* 80 */

       passport_id = point_id;
       rc = MCO_S_OK;
    break;

    default:
    LOG(ERROR) << "Unknown object class (" << objclass << ") for point";
  }

#if 0
  // Если экземпляр Паспорта был создан, проверить ссылочную целостность
  if (passport_id != point_id)
  {
    rc = passport_instance.checkpoint();
    if (rc)
    { 
      LOG(ERROR) << "Checkpoint " << info.tag() <<", rc=" << rc;
    }
    // TODO: что в этом случае делать с уже созданным паспортом?
  }
#endif

  return rc;
}

MCO_RET DatabaseRtapImpl::createPoint(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    const std::string& tag,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.objclass());
  MCO_RET    rc = MCO_S_OK;
  // Аналоговая часть Точки
  rtap_db::AnalogInfoType    ai;
  double ai_val = 0.0;
  // Дискретная часть Точки
  rtap_db::DiscreteInfoType  di;
  uint8  di_val = 0;
  rtap_db::AlarmItem  alarm;
  rtap_db::Attrib*    attr = NULL;
  int val;
  int code;
  // найдена ли у данной точки функция создания хотя бы для одного
  // атрибута из перечня известных?
  bool func_found;
  unsigned int attr_idx;
  AttrCreationFuncMapIterator_t it;

  do
  {
    // Для всех атрибутов данной точки вызвать соотв. функцию его создания.
    // Только два атрибута задаются явно, OBJCLASS и TAG, поскольку не имеют
    // отдельного соответствия в 'rtap_db.xsd'.
    //
    // Для этого вызывается соответствующая функция создания атрибута, определяемая
    // по таблице соответствия всех известных атрибутов и создающих их функций.
    //
    // Функция создания принимает на вход
    //      instance
    //      attribute_name
    //      attribute_value
    // и возвращает свой статус.
    //
    // Если статус ошибочный, откатить транзакцию для всей точки

    // 1. TAG - обязательное поле
    rc = instance.TAG_put(tag.c_str(), static_cast<uint2>(tag.size()));
    if (rc) { LOG(ERROR) << "Setting tag"; break; }

    // 2. OBJCLASS - обязательное поле
    rc = instance.OBJCLASS_put(objclass);
    if (rc) { LOG(ERROR) << "Setting object class"; break; }

    // остальные поля необязательные, создавать их по необходимости 
    // ============================================================
    // vvv Начало создания необязательных корневых атрибутов Точки
    // ============================================================
    for(attr_idx=0, func_found=false;
        attr_idx < info.attributes().size();
        attr_idx++)
    {
      func_found = false;
      it = m_attr_creation_func_map.find(info.attributes()[attr_idx].name());

      if (it != m_attr_creation_func_map.end())
      {
        func_found = true;
        // NB: Часть атрибутов уже прописана в ходе создания паспорта
        //
        // Вызвать функцию создания атрибута
        rc = (this->*(it->second))(instance, info.attributes()[attr_idx]);
        if (rc)
        {
          LOG(ERROR) << "Creation attribute '"
                     << info.attributes()[attr_idx].name()
                     << "' for point '" << tag << "'";
          break;
        }
      }

      if (!func_found)
      {
        // Это может быть для атрибутов, создаваемых в паспорте
        LOG(WARNING) << "Function creating '" << info.attributes()[attr_idx].name() 
                     << "' is not found";
        // NB: поведение по умолчанию - пропустить данный атрибут,
        // выдав предупреждение
      }
      else if (rc)
      {
        // Функция найдена, но вернула ошибку
        LOG(ERROR) << "Function creating '" << "' failed";
        break;
      }    
    }
    // ======================================================
    // ^^^ Конец создания необязательных корневых атрибутов Точки
    // ======================================================

    // Была обнаружена ошибка создания одного из атрибутов точки -
    // пропускаем всю точку целиком, откатывая транзакцию
    if (rc)
    {
      LOG(ERROR) << "Skip creating point '" << tag << "' attibutes";
      break;
    }

    // Успешно создана общая часть паспорта Точки.
    // Приступаем к созданию дополнительных полей, Analog или Discrete,
    // в зависимости от типа класса.
    switch (objclass)
    {
      // Группа точек, имеющих дискретные атрибуты значения
      // ==================================================
      case TS:  /* 0 */
      case TSA: /* 3 */
      case TSC: /* 4 */
      case AL:  /* 6 */
      case ICS: /* 7 */

        // Создать дискретную часть Точки
        code = rc = instance.di_write(di);
        // Заполнить дискретную часть Точки
        // Конкретное значение кода ошибки не анализируем,
        // важен только факт неудачи
        code += rc = di.VAL_put(di_val);
        code += rc = di.VALACQ_put(di_val);
        code += rc = di.VALMANUAL_put(di_val);

        if (code) { LOG(ERROR) << "Setting discrete information"; break; }

      break;

      // Группа точек, имеющих аналоговые атрибуты значения
      // ==================================================
      case TM:  /* 1 */
      case TR:  /* 2 */
      case ICM: /* 8 */

        // Создать дискретную часть Точки
        code = rc = instance.ai_write(ai);
        // Заполнить дискретную часть Точки
        code += rc = ai.VAL_put(ai_val);
        code += rc = ai.VALACQ_put(ai_val);
        code += rc = ai.VALMANUAL_put(ai_val);

        if (code) { LOG(ERROR) << "Setting analog information"; break; }

        // Характеристики текущего тревожного сигнала
        rc = instance.alarm_write(alarm);

      break;

      // Группа точек, не имеющих атрибутов значений VAL|VALACQ
      // ==================================================
      case TC:  /* 5 */
      case PIPE:/* 11 */
      case PIPELINE:/* 15 */
      case TL:  /* 16 */
      case VA:  /* 19 */
      case SC:  /* 20 */
      case ATC: /* 21 */
      case GRC: /* 22 */
      case SV:  /* 23 */
      case SDG: /* 24 */
      case RGA: /* 25 */
      case SSDG:/* 26 */
      case BRG: /* 27 */
      case SCP: /* 28 */
      case STG: /* 29 */
      case METLINE: /* 32 */
      case ESDG:/* 33 */
      case SVLINE:  /* 34 */
      case SCPLINE: /* 35 */
      case TLLINE:  /* 36 */
      case DIR: /* 30 */
      case DIPL:/* 31 */
      case INVT:/* 37 */
      case AUX1:/* 38 */
      case AUX2:/* 39 */
      case SA:  /* 50 */
      case SITE:/* 45 */
      case FIXEDPOINT:  /* 79 */
      case HIST: /* 80 */
        // Ничего создавать не нужно, хватит общей части атрибутов Точки
      break;

      default:
        LOG(ERROR) << "Unknown objclass (" << objclass << ")";
    }

    rc = instance.passport_ref_put(passport_aid);
    if (rc) { LOG(ERROR)<<"Setting passport id("<<passport_aid<<")"; break; }

    // Заполнить все ссылки autoid на другие таблицы своим autoid, 
    // чтобы не создавать коллизий одинаковых значений ссылок.
    code = rc = instance.CE_ref_put(point_aid);
    code += rc = instance.SA_ref_put(point_aid);
//    rc |= instance.alarm_ref_put(point_aid);
    code += rc = instance.hist_ref_put(point_aid);
    if (code) { LOG(ERROR)<<"Setting external links"; break; }

    rc = instance.checkpoint();
    if (rc) { LOG(ERROR) << "Checkpoint, rc=" << rc; break; }
  }
  while(false);

  return rc;
}

// Группа функций создания указанного атрибута для Точки
MCO_RET DatabaseRtapImpl::createSHORTLABEL(rtap_db::XDBPoint& instance, rtap_db::Attrib& attr)
{
  MCO_RET rc = instance.SHORTLABEL_put(attr.value().c_str(), static_cast<uint2>(attr.value().size()));
  return rc;
}

// Создать ссылку на указанную Систему Сбора
// TODO:
// 1. Получить идентификатор данной СС в НСИ
// 2. Занести этот идентификатор в поле SA_ref
MCO_RET DatabaseRtapImpl::createL_SA(rtap_db::XDBPoint& instance, rtap_db::Attrib& attr)
{
  autoid_t sa_id;
  MCO_RET rc = instance.SA_ref_get(sa_id);

  // TODO: реализация
  rc = instance.SA_ref_put(sa_id);
  return rc;
}

// Соответствует атрибуту БДРВ - VALIDITY
MCO_RET DatabaseRtapImpl::createVALID(rtap_db::XDBPoint& instance, rtap_db::Attrib& attr)
{
  Validity value;
  int raw_val = atoi(attr.value().c_str());
  MCO_RET rc;

  switch(raw_val)
  {
    case 0: value = INVALID;    break;
    case 1: value = VALID;      break;
    case 2: value = MANUAL;     break;
    case 3: value = DUBIOUS;    break;
    case 4: value = INHIBITED;  break;
    case 5: // NB: break пропущен специально
    default:
      value = FAULT;
  }

  rc = instance.VALIDITY_put(value);
  return rc;
}

// Соответствует атрибуту БДРВ - VALIDITY_ACQ
MCO_RET DatabaseRtapImpl::createVALIDACQ  (rtap_db::XDBPoint& instance, rtap_db::Attrib& attr)
{
  Validity value;
  int raw_val = atoi(attr.value().c_str());
  MCO_RET rc;

  switch(raw_val)
  {
    case 0: value = INVALID;    break;
    case 1: value = VALID;      break;
    case 2: value = MANUAL;     break;
    case 3: value = DUBIOUS;    break;
    case 4: value = INHIBITED;  break;
    case 5: // NB: break пропущен специально
    default:
      value = FAULT;
  }

  rc = instance.VALIDITY_ACQ_put(value);
  return rc;
}

MCO_RET DatabaseRtapImpl::createDATEHOURM(rtap_db::XDBPoint& instance, rtap_db::Attrib& attr)
{
  rtap_db::timestamp datehourm;
  datetime_t datetime;
  MCO_RET rc = instance.DATEHOURM_write(datehourm);

  if (!rc) 
  {
    // Конвертировать дату из 8 байт XML-файла в формат rtap_db::timestamp
    // (секунды и наносекунды по 4 байта)
    datetime.common = atoll(attr.value().c_str());
    datehourm.sec_put(datetime.part[0]);
    datehourm.nsec_put(datetime.part[1]);
  }
  else
  {
    LOG(ERROR) << "Create DATEHOURM";
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::createDATERTU(rtap_db::XDBPoint& instance, rtap_db::Attrib& attr)
{
  rtap_db::timestamp datertu;
  datetime_t datetime;
  MCO_RET rc = instance.DATERTU_write(datertu);

  if (!rc) 
  {
    // Конвертировать дату из 8 байт XML-файла в формат rtap_db::timestamp
    // (секунды и наносекунды по 4 байта)
    datetime.common = atoll(attr.value().c_str());
    datertu.sec_put(datetime.part[0]);
    datertu.nsec_put(datetime.part[1]);
  }
  else
  {
    LOG(ERROR) << "Create DATERTU";
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::createSTATUS(rtap_db::XDBPoint& instance, rtap_db::Attrib& attr)
{
  MCO_RET rc;
  PointStatus status = WORKED;
  int val = atoi(attr.value().c_str());

  switch(val)
  {
    case 0:
      status = PLANNED;
      break;

    case 1:
      status = WORKED;
      break;

    case 2:
      status = DISABLED;
      break;

    default:
      LOG(WARNING) << "Point doesn't have STATUS attribute, use default 'WORKED'";
      status = WORKED;
  }

  rc = instance.STATUS_put(status);
  if (rc) { LOG(ERROR) << "Setting status"; }

  return rc;
}

MCO_RET DatabaseRtapImpl::createVALIDCHANGE(rtap_db::XDBPoint& instance, rtap_db::Attrib& attr)
{
  uint1 value = atoi(attr.value().c_str());
  MCO_RET rc = instance.VALIDCHANGE_put(value);
  return rc;
}

// По заданной строке UNITY найти ее иденификатор и подставить в поле UNITY_ID
MCO_RET DatabaseRtapImpl::createUNITY(rtap_db::XDBPoint& instance, rtap_db::Attrib& attr)
{
  uint2 unity_id;
  MCO_RET rc;

  rc = instance.UNITY_ID_get(unity_id);
  rc = instance.UNITY_ID_put(unity_id);

  return rc;
}

#if 0
MCO_RET DatabaseRtapImpl::createLABEL (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createLABEL on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createMINVAL    (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createMINVAL on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createVALEX     (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createVALEX on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createMAXVAL    (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createMAXVAL on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createVAL       (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createVAL on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createVALACQ    (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createVALACQ on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createVALMANUAL (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createVALMANUAL on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createALINHIB   (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createALINHIB on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createDISPP     (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createDISPP on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createFUNCTION  (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createFUNCTION on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createCONVERTCOEFF  (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createCONVERTCOEFF on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createINHIB     (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createINHIB on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createMNVALPHY  (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createMNVALPHY on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createMXPRESSURE(rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createMXPRESSURE on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createMXVALPHY  (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createMXVALPHY on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createNMFLOW    (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createNMFLOW on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createSUPPLIERMODE  (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createSUPPLIERMODE on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createSUPPLIERSTATE (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createSUPPLIERSTATE on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createSYNTHSTATE    (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createSYNTHSTATE on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createDELEGABLE (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createDELEGABLE on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createETGCODE   (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createETGCODE on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createSITEFL    (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createSITEFL on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createFLGREMOTECMD   (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createFLGREMOTECMD on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createFLGMAINTENANCE (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createFLGMAINTENANCE on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createNAMEMAINTENANCE(rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createNAMEMAINTENANCE on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createTSSYNTHETICAL  (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createTSSYNTHETICAL on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createLOCALFLAG      (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createLOCALFLAG on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createALARMBEGIN     (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createALARMBEGIN on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createALARMBEGINACK  (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createALARMBEGINACK on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createALARMENDACK    (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createALARMENDACK on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createALARMSYNTH     (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createALARMSYNTH on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createL_TYPINFO  (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createL_TYPINFO on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createL_EQT      (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createL_EQT on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createREMOTECONTROL (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createREMOTECONTROL on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createNONEXE     (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createNONEXE on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createRESPNEG    (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createRESPNEG on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createACTIONTYP  (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createACTIONTYP on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createVAL_LABEL  (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createVAL_LABEL on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createLINK_HIST  (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createLINK_HIST on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createACQMOD     (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createACQMOD on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createL_DIPL     (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createL_DIPL on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createALDEST     (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createALDEST on " << attr.name();
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createINHIBLOCAL (rtap_db::XDBPoint&, rtap_db::Attrib& attr)
{
  LOG(INFO) << "CALL createINHIBLOCAL on " << attr.name();
  return MCO_S_OK;
}

#endif
