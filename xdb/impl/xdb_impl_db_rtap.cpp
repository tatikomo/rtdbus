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
MCO_RET DatabaseRtapImpl::new_Point(mco_trans_h t,
        rtap_db::XDBPoint *obj,
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

//  LOG(INFO) << "NEW Point "<<obj<<" name '"<<name<<"' self=" << self;

  return MCO_S_OK;
}

/*
 * Статический метод, вызываемый из runtime базы данных 
 * при удалении экземпляра XDBService
 */
MCO_RET DatabaseRtapImpl::del_Point(mco_trans_h t,
        rtap_db::XDBPoint *obj,
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

#if EXTREMEDB_VERSION <= 50
    rc = mco_register_new_point_evnt_handler(t, 
            &DatabaseRtapImpl::new_Point, 
            static_cast<void*>(this)
            );

    if (rc) LOG(ERROR) << "Registering event on Point creation, rc=" << rc;

    rc = mco_register_delete_point_evnt_handler(t, 
            &DatabaseRtapImpl::del_Point, 
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
  rtap_db::Attrib *m_univname_attr = info.attrib("UNIVNAME");
  std::string uni;

  do
  {
    if (DB_STATE_CONNECTED != State())
    {
        LOG(ERROR) << "Database '" << m_impl->getName()
                   << "' is not connected (state=" << State() << ")";
        setError(rtE_INCORRECT_DB_STATE);
        break;
    }

    if (!m_univname_attr)
    {
        // Не найден атрибут с универсальным именем - пропустить эту Точку
        LOG(ERROR) << "Empty point's univname, skip";
        setError(rtE_STRING_IS_EMPTY);
        break;
    }

    uni.assign(m_univname_attr->value());

    // TODO: Проверить, будем ли мы сохранять эту Точку в БД
    // Если нет -> не начинать транзакцию
    // Если да -> продолжаем
    // 
    // Как проверить? 
    // Сейчас проверка выполняется в createPassport:
    //      если точка игнорируется, функция вернет MCO_E_NOTSUPPORTED
    //
    rc = mco_trans_start(m_impl->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting '" << uni << "' transaction, rc=" << rc; break; }

    // Создать объект-представление Точки в БД
    rc = instance.create(t);
    if (rc) { LOG(ERROR) << "Creating '" << uni << "' instance, rc=" << rc; break; }

    // Получить его уникальный числовой идентификатор
    rc = instance.autoid_get(point_aid);
    if (rc) { LOG(ERROR) << "Getting point '" << uni << "' id, rc=" << rc; break; }

    // Вставка данных в таблицу соответствующего паспорта
    // Возвращается идентификатор паспорта
    rc = createPassport(t, instance, info, passport_aid, point_aid);
    if (rc) { LOG(ERROR) << "Creating '" << uni << "' passport, rc=" << rc; break; }

    // Вставка данных в основную таблицу точек
    rc = createPoint(t, instance, info, uni, passport_aid, point_aid);
    if (rc) { LOG(ERROR) << "Creating '" << uni << "' point, rc=" << rc; break; }

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment '" << uni << "' transaction, rc=" << rc; }
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

MCO_RET DatabaseRtapImpl::createPassportTS(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.code());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  uint1     inhiblocal, aldest;
  rtap_db::Attrib* attr = NULL;

  do
  {
    rc = passport_instance.create(t);
    if (rc) { LOG(ERROR) << "Creating TS passport"; break; }

    // 1 INHIBLOCAL
    if (NULL != (attr = info.attrib(RTDB_ATT_INHIBLOCAL)))
      inhiblocal = atoi(attr->value().c_str());
    else inhiblocal = 0;
    rc = passport_instance.INHIBLOCAL_put(inhiblocal);
    if (rc) { LOG(ERROR) << "Set TS passport " << info.name() <<" INHIBLOCAL, rc=" << rc; break; }

    // 2 ALDEST
    if (NULL != (attr = info.attrib(RTDB_ATT_ALDEST)))
      aldest = atoi(attr->value().c_str());
    else aldest = 0;
    rc = passport_instance.ALDEST_put(aldest);
    if (rc) { LOG(ERROR) << "Set TS passport " << info.name() << " ALDEST, rc=" << rc; break; }

    rc = passport_instance.autoid_get(passport_aid);
    if (rc) { LOG(ERROR) << "Getting TS passport " << info.name() << " id, rc=" << rc; break; }
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
  objclass_t objclass = static_cast<objclass_t>(info.code());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TM_passport passport_instance;
  rtap_db::Attrib* attr = NULL;
  double mnvalphy, mxvalphy, convertcoeff;

  do
  {
    rc = passport_instance.create(t);
    if (rc) { LOG(ERROR) << "Creating TM passport"; break; }

    // 1 MNVALPHY
    if (NULL != (attr = info.attrib(RTDB_ATT_MNVALPHY)))
      mnvalphy = atof(attr->value().c_str());
    else mnvalphy = 0.0;
    rc = passport_instance.MNVALPHY_put(mnvalphy);
    if (rc) { LOG(ERROR) << "Set TM passport " << info.name() <<" MNVALPHY, rc=" << rc; break; }

    // 2 MXVALPHY
    if (NULL != (attr = info.attrib(RTDB_ATT_MXVALPHY)))
      mxvalphy = atof(attr->value().c_str());
    else mxvalphy = 1.0;
    rc = passport_instance.MXVALPHY_put(mxvalphy);
    if (rc) { LOG(ERROR) << "Set TM passport " << info.name() <<" MXVALPHY, rc=" << rc; break; }

    // 3 CONVERTCOEFF
    if (NULL != (attr = info.attrib(RTDB_ATT_CONVERTCOEFF)))
      convertcoeff = atof(attr->value().c_str());
    else convertcoeff = 1.0;
    rc = passport_instance.CONVERTCOEFF_put(convertcoeff);
    if (rc) { LOG(ERROR) << "Set TM passport " << info.name() <<" CONVERTCOEFF, rc=" << rc; break; }

    //rc = passport_instance.LINK_HIST_put();

    rc = passport_instance.autoid_get(passport_aid);
    if (rc) { LOG(ERROR) << "Getting TM passport " << info.name() <<" id, rc=" << rc; break; }
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
  objclass_t objclass = static_cast<objclass_t>(info.code());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TR_passport passport_instance;
  passport_aid = point_aid;
  rtap_db::Attrib* attr = NULL;
  double minval, maxval, valex;
  uint1 confremotecmd, flgremotecmd, remotecontrol;

  do
  {
    rc = passport_instance.create(t);
    if (rc) { LOG(ERROR) << "Creating TR passport"; break; }

    // 1 MINVAL
    if (NULL != (attr = info.attrib(RTDB_ATT_MINVAL)))
      minval = atof(attr->value().c_str());
    else minval = 0.0;
    rc = passport_instance.MINVAL_put(minval);
    if (rc) { LOG(ERROR) << "Set TR passport " << info.name() <<" MAXVAL, rc=" << rc; break; }

    // 2 MAXVAL
    if (NULL != (attr = info.attrib(RTDB_ATT_MAXVAL)))
      maxval = atof(attr->value().c_str());
    else maxval = 0.0;
    rc = passport_instance.MAXVAL_put(maxval);
    if (rc) { LOG(ERROR) << "Set TR passport " << info.name() <<" MINVAL, rc=" << rc; break; }

    // 3 VALEX
    if (NULL != (attr = info.attrib(RTDB_ATT_VALEX)))
      valex = atof(attr->value().c_str());
    else valex = 0.0;
    rc = passport_instance.VALEX_put(valex);
    if (rc) { LOG(ERROR) << "Set TR passport " << info.name() <<" VALEX, rc=" << rc; break; }

    // 4 CONFREMOTECMD
    if (NULL != (attr = info.attrib(RTDB_ATT_CONFREMOTECMD)))
      confremotecmd = atoi(attr->value().c_str());
    else confremotecmd = 0;
    rc = passport_instance.CONFREMOTECMD_put(confremotecmd);
    if (rc) { LOG(ERROR) << "Set TR passport " << info.name() <<" CONFREMOTECMD, rc=" << rc; break; }

    // 5 FLGREMOTECMD
    if (NULL != (attr = info.attrib(RTDB_ATT_FLGREMOTECMD)))
      flgremotecmd = atoi(attr->value().c_str());
    else flgremotecmd = 0;
    rc = passport_instance.FLGREMOTECMD_put(flgremotecmd);
    if (rc) { LOG(ERROR) << "Set TR passport " << info.name() <<" FLGREMOTECMD, rc=" << rc; break; }

    // 6 REMOTECONTROL
    if (NULL != (attr = info.attrib(RTDB_ATT_REMOTECONTROL)))
      remotecontrol = atoi(attr->value().c_str());
    else remotecontrol = 0;
    rc = passport_instance.REMOTECONTROL_put(remotecontrol);
    if (rc) { LOG(ERROR) << "Set TR passport " << info.name() <<" REMOTECONTROL, rc=" << rc; break; }

//    rc = passport_instance.autoid_get(passport_aid);
//    if (rc) { LOG(ERROR) << "Getting TR passport " << info.name() <<" id, rc=" << rc; break; }
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
  objclass_t objclass = static_cast<objclass_t>(info.code());
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
  objclass_t objclass = static_cast<objclass_t>(info.code());
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
  objclass_t objclass = static_cast<objclass_t>(info.code());
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
  objclass_t objclass = static_cast<objclass_t>(info.code());
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
  objclass_t objclass = static_cast<objclass_t>(info.code());
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
  objclass_t objclass = static_cast<objclass_t>(info.code());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportICS";
  return rc;
}

MCO_RET DatabaseRtapImpl::createPassportVA(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.code());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::TS_passport passport_instance;
  passport_aid = point_aid; // GEV времено - для сохранения ссылочной целостности
  LOG(INFO) << "createPassportVA";
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
  objclass_t objclass = static_cast<objclass_t>(info.code());
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

    case VA:  /* 19 */
    rc = createPassportVA(t, instance, info, passport_id, point_id);
    break;

    // Точки, не имеющие паспорта
    case PIPE:/* 11 */
    case PIPELINE:/* 15 */
    case TL:  /* 16 */
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
      LOG(ERROR) << "Checkpoint " << info.name() <<", rc=" << rc;
    }
    // TODO: что в этом случае делать с уже созданным паспортом?
  }
#endif

  return rc;
}

MCO_RET DatabaseRtapImpl::createPoint(mco_trans_h t,
    rtap_db::XDBPoint& instance,
    rtap_db::Point& info,
    std::string& univname,
    autoid_t& passport_aid,
    autoid_t& point_aid)
{
  objclass_t objclass = static_cast<objclass_t>(info.code());
  MCO_RET    rc = MCO_S_OK;
  rtap_db::timestamp datehourm;
  // Аналоговая часть Точки
  rtap_db::AnalogInfoType    ai;
  double ai_val = 0.0;
  // Дискретная часть Точки
  rtap_db::DiscreteInfoType  di;
  uint8  di_val = 0;
  static const char* di_val_label = "test";
  rtap_db::AlarmItem  alarm;
  rtap_db::Attrib*    attr = NULL;
  int val;
  PointStatus status = WORKED;
  datetime_t datetime;

  do
  {
    // 1. UNIVNAME
    rc = instance.UNIVNAME_put(univname.c_str(), static_cast<uint2>(univname.size()));
    if (rc) { LOG(ERROR) << "Setting name"; break; }

    // 2. OBJCLASS
    rc = instance.OBJCLASS_put(objclass);
    if (rc) { LOG(ERROR) << "Setting object class"; break; }

    // 3. DATEHOURM
    rc = instance.DATEHOURM_write(datehourm);
    if (rc) { LOG(ERROR) << "Create DATEHOURM"; break; }
    if (NULL != (attr = info.attrib(RTDB_ATT_DATEHOURM)))
    {
      // Конвертировать дату из 8 байт XML-файла в формат rtap_db::timestamp
      // (секунды и наносекунды по 4 байта)
      datetime.common = atoll(attr->value().c_str());
      datehourm.sec_put(datetime.part[0]);
      datehourm.nsec_put(datetime.part[1]);
    }
    else
    {
      datehourm.sec_put(0);
      datehourm.nsec_put(0);
    }

    // 4. STATUS
    if (NULL != (attr = info.attrib(RTDB_ATT_STATUS)))
    {
      int val = atoi(attr->value().c_str());
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
          LOG(WARNING) << "Point " << univname
                       << " doesn't have STATUS attribute, use default 'WORKED'";
          status = WORKED;
      }
    }
    rc = instance.STATUS_put(status);
    if (rc) { LOG(ERROR) << "Setting status"; break; }

    // Создание дополнительных полей, Analog или Discrete, в зависимости от типа класса
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
        rc = instance.di_write(di);
        // Заполнить дискретную часть Точки
        rc |= di.VAL_put(di_val);
        rc |= di.VALACQ_put(di_val);
        rc |= di.VALMANUAL_put(di_val);

        if (rc) { LOG(ERROR) << "Setting discrete information"; break; }

      break;

      // Группа точек, имеющих аналоговые атрибуты значения
      // ==================================================
      case TM:  /* 1 */
      case TR:  /* 2 */
      case ICM: /* 8 */

        // Создать дискретную часть Точки
        rc = instance.ai_write(ai);
        // Заполнить дискретную часть Точки
        rc |= ai.VAL_put(ai_val);
        rc |= ai.VALACQ_put(ai_val);
        rc |= ai.VALMANUAL_put(ai_val);

        if (rc) { LOG(ERROR) << "Setting analog information"; break; }

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
    rc = instance.CE_ref_put(point_aid);
    rc |= instance.SA_ref_put(point_aid);
//    rc |= instance.alarm_ref_put(point_aid);
    rc |= instance.hist_ref_put(point_aid);
    if (rc) { LOG(ERROR)<<"Setting external links"; break; }

    rc = instance.checkpoint();
    if (rc) { LOG(ERROR) << "Checkpoint, rc=" << rc; break; }
  }
  while(false);

  return rc;
}

