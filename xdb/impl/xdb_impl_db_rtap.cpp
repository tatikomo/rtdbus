/* 
 * ITG
 * author: Eugeniy Gorovoy
 * email: eugeni.gorovoi@gmail.com
 *
 */
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

#include "xdb_common.hpp"
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

/* helper function to print an XML schema */
# if (EXTREMEDB_VERSION <= 40)
mco_size_t file_writer(void* stream_handle, const void* from, mco_size_t nbytes)
# else
mco_size_sig_t file_writer(void* stream_handle, const void* from, mco_size_t nbytes)
# endif
{
  return (mco_size_t) fwrite(from, 1, nbytes, (FILE*) stream_handle);
}

typedef MCO_RET (*schema_f) (mco_trans_h, void*, mco_stream_write);
// ===============================================================================
// Взято из 'xdb/impl/dat/rtap_db.h'
// Результат выполнения "grep _xml_schema rtap_db.h|wc -l" равен 47 (10.11.2014)
const int ALL_TYPES_COUNT = 47;
schema_f ALL_TYPES_LIST[] = {
  XDBPoint_xml_schema,
  DICT_TSC_VAL_LABEL_xml_schema,
  DICT_OBJCLASS_CODE_xml_schema,
  DICT_SIMPLE_TYPE_xml_schema,
  DICT_UNITY_ID_xml_schema,
  XDB_CE_xml_schema,
  ALARM_xml_schema,
  A_HIST_xml_schema,
  D_HIST_xml_schema,
  LISTACD_xml_schema,
  LISTACT_xml_schema,
  XDBPoint_xml_schema,
  TS_passport_xml_schema,
  TM_passport_xml_schema,
  TR_passport_xml_schema,
  TSA_passport_xml_schema,
  TSC_passport_xml_schema,
  TC_passport_xml_schema,
  AL_passport_xml_schema,
  ICS_passport_xml_schema,
  ICM_passport_xml_schema,
  PIPE_passport_xml_schema,
  PIPELINE_passport_xml_schema,
  TL_passport_xml_schema,
  SC_passport_xml_schema,
  ATC_passport_xml_schema,
  GRC_passport_xml_schema,
  SV_passport_xml_schema,
  SDG_passport_xml_schema,
  RGA_passport_xml_schema,
  SSDG_passport_xml_schema,
  BRG_passport_xml_schema,
  SCP_passport_xml_schema,
  STG_passport_xml_schema,
  METLINE_passport_xml_schema,
  ESDG_passport_xml_schema,
  SVLINE_passport_xml_schema,
  SCPLINE_passport_xml_schema,
  TLLINE_passport_xml_schema,
  DIR_passport_xml_schema,
  DIPL_passport_xml_schema,
  INVT_passport_xml_schema,
  AUX1_passport_xml_schema,
  AUX2_passport_xml_schema,
  SA_passport_xml_schema,
  SITE_passport_xml_schema,
  FIXEDPOINT_passport_xml_schema,
  VA_passport_xml_schema,
  NULL
};

// =================================================================================
PointInDatabase::PointInDatabase(rtap_db::Point* info)
  : m_rc(MCO_S_OK),
    m_info(info)
{
  m_objclass = static_cast<objclass_t>(m_info->objclass());
  m_passport.root_pointer = NULL;
}

// =================================================================================
PointInDatabase::~PointInDatabase()
{
#if 1
  switch (objclass())
  {
    case TS:  /* 00 */ delete passport().ts;     break;
    case TM:  /* 01 */ delete passport().tm;     break;
    case TR:  /* 02 */ delete passport().tr;     break;
    case TSA: /* 03 */ delete passport().tsa;    break;
    case TSC: /* 04 */ delete passport().tsc;    break;
    case TC:  /* 05 */ delete passport().tc;     break;
    case AL:  /* 06 */ delete passport().al;     break;
    case ICS: /* 07 */ delete passport().ics;    break;
    case ICM: /* 08 */ delete passport().icm;    break;
    case PIPE:/* 11 */ delete passport().pipe;   break;
    case PIPELINE:/* 15 */ delete passport().pipeline; break;
    case TL:  /* 16 */ delete passport().tl;     break;
    case VA:  /* 19 */ delete passport().valve;  break;
    case SC:  /* 20 */ delete passport().sc;     break;
    case ATC: /* 21 */ delete passport().atc;    break;
    case GRC: /* 22 */ delete passport().grc;    break;
    case SV:  /* 23 */ delete passport().sv;     break;
    case SDG: /* 24 */ delete passport().sdg;    break;
    case RGA: /* 25 */ delete passport().rga;    break;
    case SSDG:/* 26 */ delete passport().ssdg;   break;
    case BRG: /* 27 */ delete passport().brg;    break;
    case SCP: /* 28 */ delete passport().scp;    break;
    case STG: /* 29 */ delete passport().stg;    break;
    case DIR: /* 30 */ delete passport().dir;    break;
    case DIPL:/* 31 */ delete passport().dipl;   break;
    case METLINE: /* 32 */ delete passport().metline; break;
    case ESDG:/* 33 */ delete passport().esdg;   break;
    case SVLINE:  /* 34 */ delete passport().svline; break;
    case SCPLINE: /* 35 */ delete passport().scpline;break;
    case TLLINE:  /* 36 */ delete passport().tlline; break;
    case INVT:/* 37 */ delete passport().invt;   break;
    case AUX1:/* 38 */ delete passport().aux1;   break;
    case AUX2:/* 39 */ delete passport().aux2;   break;
    case SITE:/* 45 */ delete passport().site;   break;
    case SA:  /* 50 */ delete passport().sa;     break;

    /* Нет паспорта */
    case HIST:      /* 80 */ // NB: break пропущен специально
    case FIXEDPOINT:/* 79 */
        break;

    default:
      LOG(ERROR) << "Memory leak here!";
  }
#else
  // TODO: У этих структур нет общего предка, как можно удалить их проще?
  delete m_passport.root_pointer;
#endif
}

// =================================================================================
// Начальное создание всех объектов XDB, необходимых для последующего 
// создания атрибутов с помощью автоматических функций.
// Экземпляр транзакции запоминается для возможного использования в
// автоматных функциях создания атрибутов.
// Транзакция и начинается, и заканчивается вне данной функции.
// =================================================================================
MCO_RET PointInDatabase::create(mco_trans_h t)
{
  m_transaction_handler = t;

  do
  {
      m_rc = m_point.create(m_transaction_handler);
      if (m_rc) { LOG(ERROR) << m_info->tag() << " creation failure: " << m_rc; break; }

      // Получить его уникальный числовой идентификатор
      m_rc = m_point.autoid_get(m_point_aid);
      if (m_rc) { LOG(ERROR) << "Getting point '" << m_info->tag() << "' id, rc=" << m_rc; break; }

      // Для сохранения ссылочной целостности пока считаем, что ID Паспорта 
      // совпадает с ID Точки. Если в функции создания Паспорта сохранение 
      // данных пройдет успешно, ID Паспорта изменится на уникальное значение. 
      m_passport_aid = m_CE_aid = m_SA_aid = m_hist_aid = m_point_aid;

      // Успешно создана общая часть паспорта Точки.
      // Приступаем к созданию дополнительных полей,
      // Analog или Discrete, и AlarmInfo, в зависимости от типа класса.
      switch (objclass())
      {
        // Группа точек, имеющих и тревоги, и дискретные атрибуты значения 
        // ===============================================================
        case TS:  /* 0 */
        case TSC: /* 4 */
        case AL:  /* 6 */
        case ICS: /* 7 */
            m_rc = assign(m_alarm);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign alarm information, rc="<<m_rc; break; }
        // Группа точек, имеющих только дискретные атрибуты значения 
        // =========================================================
        case TSA: /* 3 */ // Вынесен ниже остальных, поскольку не имеет Alarm
            // Только создать дискретную часть Точки, значения заполняются автоматными функциями
            m_rc = assign(m_di);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign discrete information, rc="<<m_rc; break; }
        break;

        // Группа точек, имеющих и тревоги, и аналоговые атрибуты значения
        // ===============================================================
        case TM:  /* 1 */
        case ICM: /* 8 */
            m_rc = assign(m_alarm);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign alarm information, rc="<<m_rc; break; }
        // Группа точек, имеющих только аналоговые атрибуты значения
        // =========================================================
        case TR:  /* 2 */ // Вынесен ниже остальных, поскольку не имеет Alarm
            // Только создать аналоговую часть Точки, значения заполняются автоматными функциями
            m_rc = assign(m_ai);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign analog information, rc="<<m_rc; break; }

            // Характеристики текущего тревожного сигнала
            m_rc = assign(m_alarm);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign alarm information, rc="<<m_rc; break; }
        break;

        // Группа точек, не имеющих атрибутов значений VAL|VALACQ
        // ======================================================
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
        // Хватит паспорта и общей части атрибутов Точки
        break;

        default:
        LOG(ERROR) << "Unsupported objclass " << objclass() << " for point " << tag();
      }
  } while (false);

  return m_rc;
}

// =================================================================================
MCO_RET PointInDatabase::update_references()
{
  do
  {
    m_rc = m_point.passport_ref_put(m_passport_aid);
    if (m_rc) { LOG(ERROR)<<"Setting passport id("<<m_passport_aid<<")"; break; }

    // Заполнить все ссылки autoid на другие таблицы своим autoid, 
    // чтобы не создавать коллизий одинаковых значений ссылок.
    m_rc = m_point.CE_ref_put(m_CE_aid);
    if (m_rc) { LOG(ERROR)<<"Setting CE link (" << m_CE_aid <<")"; break; }
    m_rc = m_point.SA_ref_put(m_SA_aid);
    if (m_rc) { LOG(ERROR)<<"Setting SA link (" << m_SA_aid << ")"; break; }
    m_rc = m_point.hist_ref_put(m_hist_aid);
    if (m_rc) { LOG(ERROR)<<"Setting HIST link (" << m_hist_aid << ")"; break; }

  } while (false);

  return m_rc;
}

// =================================================================================
// Опции по умолчанию устанавливаются в через setOption() в RtApplication
DatabaseRtapImpl::DatabaseRtapImpl(const char* _name, const Options* _options)
{
  assert(_name);

  mco_dictionary_h rtap_dict = rtap_db_get_dictionary();

  m_impl = new DatabaseImpl(_name, _options, rtap_dict);

  // Инициализация карты функций создания атрибутов
  AttrFuncMapInit();
}

// =================================================================================
DatabaseRtapImpl::~DatabaseRtapImpl()
{
  delete m_impl;
}

// Получить название БДРВ
const char* DatabaseRtapImpl::getName()
{
//  LOG(INFO) << "DatabaseRtapImpl::getName() impl=" << m_impl;
  assert(m_impl);
  return m_impl->getName();
}

// Вернуть последнюю ошибку
const Error& DatabaseRtapImpl::getLastError() const
{
//  LOG(INFO) << "DatabaseRtapImpl::getLastError() impl=" << m_impl;
  assert(m_impl);
  return m_impl->getLastError();
}

// Вернуть признак отсутствия ошибки
bool DatabaseRtapImpl::ifErrorOccured() const
{
//  LOG(INFO) << "DatabaseRtapImpl::ifErrorOccured() impl=" << m_impl;
  assert(m_impl);
  return m_impl->ifErrorOccured();
}

// Установить новое состояние ошибки
void DatabaseRtapImpl::setError(ErrorCode_t code)
{
//  LOG(INFO) << "DatabaseRtapImpl::setError() impl=" << m_impl;
  assert(m_impl);
  m_impl->setError(code);
}

// Сбросить ошибку
void DatabaseRtapImpl::clearError()
{
//  LOG(INFO) << "DatabaseRtapImpl::clearError() impl=" << m_impl;
  assert(m_impl);
  m_impl->clearError();
}

// =================================================================================
bool DatabaseRtapImpl::Connect()
{
  Error err = m_impl->Connect();
  return err.Ok();
}

// =================================================================================
bool DatabaseRtapImpl::Disconnect()
{
  return (m_impl->Disconnect()).Ok();
}

// =================================================================================
DBState_t DatabaseRtapImpl::State()
{
  return m_impl->State();
}

// =================================================================================
// Статический метод, вызываемый из runtime базы данных 
// при создании нового экземпляра XDBService
// =================================================================================
MCO_RET DatabaseRtapImpl::new_Point(mco_trans_h /* t */,
        XDBPoint *obj,
        MCO_EVENT_TYPE /* et */,
        void *p)
{
  DatabaseRtapImpl *self = static_cast<DatabaseRtapImpl*> (p);
//  MCO_RET rc;
//  autoid_t aid;
//  bool status = false;

  assert(self);
  assert(obj);

  do
  {
  } while (false);

//  LOG(INFO) << "NEW Point "<<obj<<" tag '"<<tag<<"' self=" << self;

  return MCO_S_OK;
}

// =================================================================================
// Статический метод, вызываемый из runtime базы данных 
// при удалении экземпляра XDBService
// =================================================================================
MCO_RET DatabaseRtapImpl::del_Point(mco_trans_h t,
        XDBPoint *obj,
        MCO_EVENT_TYPE et,
        void *p)
{
  DatabaseRtapImpl *self = static_cast<DatabaseRtapImpl*> (p);
//  MCO_RET rc;

  assert(self);
  assert(obj);

  do
  {
  } while (false);

//  LOG(INFO) << "DEL Point "<<obj<<" tag '"<<tag<<"' self=" << self;

  return MCO_S_OK;
}

// =================================================================================
MCO_RET DatabaseRtapImpl::RegisterEvents(mco_db_h& handler)
{
  MCO_RET rc;
  mco_trans_h t;
  mco_new_point_evnt_handler new_handler = DatabaseRtapImpl::new_Point;
  mco_new_point_evnt_handler delete_handler = DatabaseRtapImpl::del_Point;

  do
  {
    rc = mco_trans_start(handler, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
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

// =================================================================================
// Сохранить XML Scheme БДРВ
// =================================================================================
void DatabaseRtapImpl::GenerateXSD()
{
  MCO_RET rc = MCO_S_OK;

  mco_trans_h t;
  FILE *f;
  static char fname[150];

  strcpy(fname, m_impl->getName());
  strcat(fname, ".xsd");

  f = fopen(fname, "w");
  if (f)
  {
    /* print out XML Schema for different classes */
    rc = mco_trans_start(m_impl->getDbHandler(), MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if ( MCO_S_OK == rc )
    {
      size_t i = 0;
      do
      {
        (*ALL_TYPES_LIST[i++])(t, f, file_writer);
      } 
      while (ALL_TYPES_LIST[i]);

      LOG(INFO) << "XSD Schema store successfully";
    }
    mco_trans_rollback(t);
    fclose(f);
  }
  else
  {
    LOG(INFO) << "Unable to save XSD Schema info " << fname;
  }
}

// =================================================================================
/* Тестовый API сохранения базы */
bool DatabaseRtapImpl::MakeSnapshot(const char* msg)
{
  bool stat = false;

  switch(State())
  {
    case DB_STATE_INITIALIZED:
    case DB_STATE_ATTACHED:
    case DB_STATE_CONNECTED:
      // Сохранить структуру БД
      if (NULL != getenv("RTDBUS_GEN_XSD"))
      {
        GenerateXSD();
      }
      else LOG(INFO) << "Do not make XSD, environment variable RTDBUS_GEN_XSD is not set";
      // Сохранить содержимое БД
      stat = (m_impl->SaveAsXML(NULL, msg)).Ok();
      break;

    default:
      LOG(ERROR) << "Try to MakeSnapshot for uninitalized database";
  }
  return stat;
}

// ====================================================
// Восстановление содержимого БДРВ из снимка
bool DatabaseRtapImpl::LoadSnapshot(const char* _fname)
{
  // Попытаться восстановить восстановить данные в таком порядке:
  // 1. из файла с двоичным дампом
  // 2. из файла в формате стандартного XML-дампа eXtremeDB
  m_impl->LoadSnapshot(_fname);

  return getLastError().Ok();
}

// ====================================================
// Удаление Точки
const Error& DatabaseRtapImpl::erase(mco_db_h&, rtap_db::Point&)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// ====================================================
// Чтение данных Точки
const Error& DatabaseRtapImpl::read(mco_db_h&, rtap_db::Point&)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// ====================================================
// Чтение данных Точки
// В теге должно присутствовать явно название атрибута,
// например: /KD4504/GOV020.SHORTLABEL
//
// По названию атрибута вызывается функция его чтения.
// Функции чтения содержатся в хеше DatabaseRtapImpl::m_attr_writing_func_map
const Error& DatabaseRtapImpl::read(mco_db_h& handle, xdb::AttributeInfo_t* info)
{
  rtap_db::XDBPoint instance;
  mco_trans_h t;
  bool func_found;
  AttrProcessingFuncMapIterator_t it;
  MCO_RET rc = MCO_S_OK;

  assert(info);
  setError(rtE_RUNTIME_ERROR);

  info->quality = xdb::ATTR_NOT_FOUND;

  std::string::size_type point_pos = info->name.find(".");
  if (point_pos != std::string::npos)
  {
    // Есть точка в составе тега
    std::string point_name = info->name.substr(0, point_pos);
    std::string attr_name = info->name.substr(point_pos + 1, info->name.size());

//    LOG(INFO) << "Read atribute \"" << attr_name << "\" for point \"" << point_name << "\"";
    do
    {
      // TODO: объединить с аналогичной функцией нахождения точки locate() по имени в xdb_impl_connection.cpp
      rc = mco_trans_start(handle, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
      if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

      rc = rtap_db::XDBPoint::SK_by_tag::find(t, point_name.c_str(), point_name.size(), instance);
      if (rc) { LOG(ERROR) << "Locating point '" << point_name << "', rc=" << rc; setError(rtE_POINT_NOT_FOUND); break; }

      func_found = false;
      it = m_attr_reading_func_map.find(attr_name);
      if (it != m_attr_reading_func_map.end())
      {
          func_found = true;
          // Вызвать функцию чтения атрибута
          rc = (this->*(it->second))(t, instance, info);
          if (rc)
          {
            LOG(ERROR) << "Reading " << point_name << "." << attr_name;
            setError(rtE_ILLEGAL_PARAMETER_VALUE);
            break;
          }
      }

      if (!func_found)
      {
          // Это может быть для атрибутов, создаваемых в паспорте
          // NB: поведение по умолчанию - пропустить атрибут, выдав предупреждение
          LOG(WARNING) << "Function for reading attribute '" << attr_name 
                       << "' doesn't found";
          setError(rtE_ATTR_NOT_FOUND);
      }
      else if (rc)
      {
          // Функция найдена, но вернула ошибку
          LOG(ERROR) << "Function for reading '" << "' failed";
          break;
      }

      rc = mco_trans_commit(t);
      if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

      // Качество атрибута хорошее (без ошибок)
      info->quality = xdb::ATTR_OK;
      clearError();

    } while(false);

    if(rc)
      rc = mco_trans_rollback(t);
  }
  else
  {
    LOG(ERROR) << "Tag doesn't contain attribute: " << info->name;
    setError(rtE_ILLEGAL_TAG_NAME);
  }

  return getLastError();
}

// ====================================================
// Изменение данных Точки
const Error& DatabaseRtapImpl::write(mco_db_h&, rtap_db::Point&)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// ====================================================
// Создание Точки
const Error& DatabaseRtapImpl::create(mco_db_h& handler, rtap_db::Point& info)
{
  rtap_db::XDBPoint instance;
  MCO_RET     rc = MCO_S_OK;
  mco_trans_h t;
  PointInDatabase *point;

  setError(rtE_RUNTIME_ERROR);

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

    point = new PointInDatabase(&info);
    // TODO: Проверить, будем ли мы сохранять эту Точку в БД
    // Если нет -> не начинать транзакцию
    // Если да -> продолжаем
    // 
    // Как проверить? 
    // Сейчас проверка выполняется в createPassport:
    //      если точка игнорируется, функция вернет MCO_E_NOTSUPPORTED
    //
    rc = mco_trans_start(handler, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting '" << info.tag() << "' transaction, rc=" << rc; break; }

    // Создать объект-представление Точки в БД
    rc = point->create(t);
    if (rc) { LOG(ERROR) << "Creating '" << info.tag() << "' instance, rc=" << rc; break; }

    // Вставка данных в таблицу соответствующего паспорта
    rc = createPassport(point);
    if (rc) { LOG(ERROR) << "Creating '" << info.tag() << "' passport, rc=" << rc; break; }

    // Вставка данных в основную таблицу точек
    rc = createPoint(point);
    if (rc) { LOG(ERROR) << "Creating '" << info.tag() << "' point, rc=" << rc; break; }

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment '" << info.tag() << "' transaction, rc=" << rc; }

    clearError();
  }
  while(false);

  if (rc)
    mco_trans_rollback(t);

  delete point;

  return getLastError();
}

// =================================================================================
// Блокировка данных Точки от изменения в течение заданного времени
const Error& DatabaseRtapImpl::lock(mco_db_h&, rtap_db::Point&, int)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// =================================================================================
const Error& DatabaseRtapImpl::unlock(mco_db_h&, rtap_db::Point&)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// =================================================================================
// Группа функций управления
// версия со встроенным идентификатором подключения m_impl->getDbHandler()
const Error& DatabaseRtapImpl::Control(rtDbCq& /* info */)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// =================================================================================
// Группа функций управления
// версия со встроенным идентификатором подключения m_impl->getDbHandler()
const Error& DatabaseRtapImpl::Query(rtDbCq& /* info */)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// =================================================================================
// Группа функций управления
// версия со встроенным идентификатором подключения m_impl->getDbHandler()
const Error& DatabaseRtapImpl::Config(rtDbCq& /* info */)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// =================================================================================
// Группа функций управления
const Error& DatabaseRtapImpl::Control(mco_db_h&, rtDbCq& /* info */)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// =================================================================================
// Группа функций управления
const Error& DatabaseRtapImpl::Query(mco_db_h&, rtDbCq& /* info */)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

// =================================================================================
// Группа функций управления
// =================================================================================
const Error& DatabaseRtapImpl::Config(mco_db_h& handler, rtDbCq& info)
{
  MCO_RET rc;

  clearError();

  switch(info.action.config)
  {
    case rtCONFIG_ADD_TABLE:
        rc = createTable(handler, info);
        if (rc) { LOG(ERROR) << "Table creation facility"; }
        break;

    default:
        setError(rtE_NOT_IMPLEMENTED);
  }

  return getLastError();
}

// =================================================================================
MCO_RET DatabaseRtapImpl::createTable(mco_db_h& handler, rtDbCq& info)
{
  MCO_RET rc = MCO_E_UNSUPPORTED;
  // В перечне тегов только одна запись о названии Таблицы
  assert(info.tags->size() == 1);

  // TODO: при увеличении количества таблиц проводить поиск с помощью хеша
  if (0 == (info.tags->at(0).compare("DICT_TSC_VAL_LABEL")))
  {
    rc = createTableDICT_TSC_VAL_LABEL(handler, static_cast<rtap_db_dict::values_labels_t*>(info.buffer));
  }
  else if (0 == (info.tags->at(0).compare("DICT_UNITY_ID")))
  {
    rc = createTableDICT_UNITY_ID(handler, static_cast<rtap_db_dict::unity_labels_t*>(info.buffer));
  }
  else if (0 == (info.tags->at(0).compare("XDB_CE")))
  {
    rc = createTableXDB_CE(handler, static_cast<rtap_db_dict::macros_def_t*>(info.buffer));
  }
  else
  {
    LOG(ERROR) << "Creating unknown table '" << info.tags->at(0) << "'";
  }
  return rc;
}

// =================================================================================
MCO_RET DatabaseRtapImpl::createTableDICT_TSC_VAL_LABEL(mco_db_h& handler, rtap_db_dict::values_labels_t* dict)
{
  MCO_RET rc = MCO_E_UNSUPPORTED;
  DICT_TSC_VAL_LABEL instance;
  mco_trans_h t;
  
  assert(dict);

  do
  {
    rc = mco_trans_start(handler, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    // Очистить прежнее содержимое, если есть
    rc = DICT_TSC_VAL_LABEL_delete_all(t);
    if (rc) { LOG(ERROR) << "Cleaning VAL_LABEL table, rc=" << rc; break; }
    
    for (unsigned int idx=0; idx < dict->size(); idx++)
    {
      LOG(INFO) << "[" << idx+1 << "/" << dict->size() << "] Insert VAL_LABEL: objclass="
                << dict->at(idx).objclass << " "
                << dict->at(idx).value << " '"
                << dict->at(idx).label << "'";

      rc = DICT_TSC_VAL_LABEL_new(t, &instance);
      if (rc) { LOG(ERROR) << "Creating VAL_LABEL item, rc=" << rc; break; }

      rc = DICT_TSC_VAL_LABEL_classID_put(&instance, static_cast<objclass_t>(dict->at(idx).objclass));
      if (rc) { LOG(ERROR) << "Creating 'objclass', rc=" << rc; break; }

      rc = DICT_TSC_VAL_LABEL_valueID_put(&instance, static_cast<uint1>(dict->at(idx).value));
      if (rc) { LOG(ERROR) << "Creating 'value', rc=" << rc; break; }

      rc = DICT_TSC_VAL_LABEL_label_put(&instance, dict->at(idx).label.c_str(),
                                        static_cast<uint2>(dict->at(idx).label.size()));
      if (rc) { LOG(ERROR) << "Creating 'label' item, rc=" << rc; break; }

      rc = DICT_TSC_VAL_LABEL_checkpoint(&instance);
      if (rc) { LOG(ERROR) << "Checkpointing DICT_TSC_VAL_LABEL, rc=" << rc; break; }
    }

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

  } while (false);

  if (rc)
  {
    mco_trans_rollback(t);
    setError(rtE_RUNTIME_ERROR);
  }

  return rc;
}

// =================================================================================
MCO_RET DatabaseRtapImpl::createTableDICT_UNITY_ID(mco_db_h& handler, rtap_db_dict::unity_labels_t* dict)
{
  MCO_RET rc = MCO_E_UNSUPPORTED;
  DICT_UNITY_ID instance;
  mco_trans_h t;
  
  assert(dict);
  setError(rtE_RUNTIME_ERROR);

  do
  {
    rc = mco_trans_start(handler, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    // Очистить прежнее содержимое, если есть
    rc = DICT_UNITY_ID_delete_all(t);
    if (rc) { LOG(ERROR) << "Cleaning UNITY table, rc=" << rc; break; }
  
    for (unsigned int idx=0; idx < dict->size(); idx++)
    {
      LOG(INFO) << "[" << idx+1 << "/" << dict->size() << "] Insert UNITY: id="
                << dict->at(idx).unity_id << " ("
                << dict->at(idx).dimension_id << ") '"
                << dict->at(idx).unity_entry << "' '"
                << dict->at(idx).dimension_entry << "' '"
                << dict->at(idx).designation_entry<< "'";

      rc = DICT_UNITY_ID_new(t, &instance);
      if (rc) { LOG(ERROR) << "Creating UNITY item, rc=" << rc; break; }

      rc = DICT_UNITY_ID_DimensionID_put(&instance, static_cast<uint2>(dict->at(idx).dimension_id));
      if (rc) { LOG(ERROR) << "Creating 'DimensionID', rc=" << rc; break; }

      rc = DICT_UNITY_ID_unityID_put(&instance, static_cast<uint2>(dict->at(idx).unity_id));
      if (rc) { LOG(ERROR) << "Creating 'DimensionID', rc=" << rc; break; }

      rc = DICT_UNITY_ID_dimension_entry_put(&instance, dict->at(idx).dimension_entry.c_str(),
                                              static_cast<uint2>(dict->at(idx).dimension_entry.size()));
      if (rc) { LOG(ERROR) << "Creating 'DimensionEntry', rc=" << rc; break; }

      rc = DICT_UNITY_ID_UNITY_put(&instance, dict->at(idx).unity_entry.c_str(),
                                              static_cast<uint2>(dict->at(idx).unity_entry.size()));
      if (rc) { LOG(ERROR) << "Creating 'unity_entry item', rc=" << rc; break; }

      rc = DICT_UNITY_ID_designation_entry_put(&instance, dict->at(idx).designation_entry.c_str(),
                                              static_cast<uint2>(dict->at(idx).designation_entry.size()));
      if (rc) { LOG(ERROR) << "Creating 'designation_entry', rc=" << rc; break; }

      rc = DICT_UNITY_ID_checkpoint(&instance);
      if (rc) { LOG(ERROR) << "Checkpointing DICT_UNITY_ID, rc=" << rc; break; }
    }

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

    clearError();

  } while(false);

  if (rc)
  {
    mco_trans_rollback(t);
    setError(rtE_RUNTIME_ERROR);
  }

  return rc;
}

// =================================================================================
MCO_RET DatabaseRtapImpl::createTableXDB_CE(mco_db_h& handler, rtap_db_dict::macros_def_t* dict)
{
  MCO_RET rc = MCO_E_UNSUPPORTED;
  XDB_CE instance;
  mco_trans_h t;
  
  assert(dict);
  setError(rtE_RUNTIME_ERROR);

  do
  {
    rc = mco_trans_start(handler, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    // Очистить прежнее содержимое, если есть
    rc = XDB_CE_delete_all(t);
    if (rc) { LOG(ERROR) << "Cleaning VAL_LABEL table, rc=" << rc; break; }
    
    for (unsigned int idx=0; idx < dict->size(); idx++)
    {
      LOG(INFO) << "[" << idx+1 << "/" << dict->size() << "] Insert XDB_CE: id="
                << dict->at(idx).id << " '"
                << dict->at(idx).definition << "'";

      rc = XDB_CE_new(t, &instance);
      if (rc) { LOG(ERROR) << "Creating CE item, rc=" << rc; break; }

      rc = XDB_CE_macrosId_put(&instance, static_cast<uint2>(dict->at(idx).id));
      if (rc) { LOG(ERROR) << "Creating 'macrosId', rc=" << rc; break; }

      rc = XDB_CE_CE_put(&instance, dict->at(idx).definition.c_str(),
                        static_cast<uint2>(dict->at(idx).definition.size()));
      if (rc) { LOG(ERROR) << "Creating 'definition' item, rc=" << rc; break; }

      rc = XDB_CE_checkpoint(&instance);
      if (rc) { LOG(ERROR) << "Checkpointing CE, rc=" << rc; break; }
    }

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

    clearError();

  } while (false);

  if (rc)
  {
    mco_trans_rollback(t);
    setError(rtE_RUNTIME_ERROR);
  }

  return rc;
}

// =================================================================================
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
// =================================================================================
MCO_RET DatabaseRtapImpl::createPassport(PointInDatabase* point)
{
  MCO_RET   rc = MCO_E_UNSUPPORTED;
  autoid_t  passport_id = point->id();

  switch(point->objclass())
  {
    case TS:  /* 00 */
    point->passport().ts = new rtap_db::TS_passport();
    rc = point->passport().ts->create(point->current_transaction());
    rc = point->passport().ts->autoid_get(passport_id);
    break;

    case TM:  /* 01 */
    point->passport().tm = new rtap_db::TM_passport();
    rc = point->passport().tm->create(point->current_transaction());
    rc = point->passport().tm->autoid_get(passport_id);
    break;

    case TR:  /* 02 */
    point->passport().tr = new rtap_db::TR_passport();
    rc = point->passport().tr->create(point->current_transaction());
    rc = point->passport().tr->autoid_get(passport_id);
    break;

    case TSA: /* 03 */
    point->passport().tsa = new rtap_db::TSA_passport();
    rc = point->passport().tsa->create(point->current_transaction());
    rc = point->passport().tsa->autoid_get(passport_id);
    break;

    case TSC: /* 04 */
    point->passport().tsc = new rtap_db::TSC_passport();
    rc = point->passport().tsc->create(point->current_transaction());
    rc = point->passport().tsc->autoid_get(passport_id);
    break;

    case TC:  /* 05 */
    point->passport().tc = new rtap_db::TC_passport();
    rc = point->passport().tc->create(point->current_transaction());
    rc = point->passport().tc->autoid_get(passport_id);
    break;

    case AL:  /* 06 */
    point->passport().al = new rtap_db::AL_passport();
    rc = point->passport().al->create(point->current_transaction());
    rc = point->passport().al->autoid_get(passport_id);
    break;

    case ICS: /* 07 */
    point->passport().ics = new rtap_db::ICS_passport();
    rc = point->passport().ics->create(point->current_transaction());
    rc = point->passport().ics->autoid_get(passport_id);
    break;

    case ICM: /* 08 */
    point->passport().icm = new rtap_db::ICM_passport();
    rc = point->passport().icm->create(point->current_transaction());
    rc = point->passport().icm->autoid_get(passport_id);
    break;

    case TL:  /* 16 */
    point->passport().tl = new rtap_db::TL_passport();
    rc = point->passport().tl->create(point->current_transaction());
    rc = point->passport().tl->autoid_get(passport_id);
    break;

    case VA:  /* 19 */
    point->passport().valve = new rtap_db::VA_passport();
    rc = point->passport().valve->create(point->current_transaction());
    rc = point->passport().valve->autoid_get(passport_id);
    break;

    case SC:  /* 20 */
    point->passport().sc = new rtap_db::SC_passport();
    rc = point->passport().sc->create(point->current_transaction());
    rc = point->passport().sc->autoid_get(passport_id);
    break;

    case ATC: /* 21 */
    point->passport().atc = new rtap_db::ATC_passport();
    rc = point->passport().atc->create(point->current_transaction());
    rc = point->passport().atc->autoid_get(passport_id);
    break;

    case GRC: /* 22 */
    point->passport().grc = new rtap_db::GRC_passport();
    rc = point->passport().grc->create(point->current_transaction());
    rc = point->passport().grc->autoid_get(passport_id);
    break;

    case SV:  /* 23 */
    point->passport().sv = new rtap_db::SV_passport();
    rc = point->passport().sv->create(point->current_transaction());
    rc = point->passport().sv->autoid_get(passport_id);
    break;

    case SDG: /* 24 */
    point->passport().sdg = new rtap_db::SDG_passport();
    rc = point->passport().sdg->create(point->current_transaction());
    rc = point->passport().sdg->autoid_get(passport_id);
    break;

    case SSDG:/* 26 */
    point->passport().ssdg = new rtap_db::SSDG_passport();
    rc = point->passport().ssdg->create(point->current_transaction());
    rc = point->passport().ssdg->autoid_get(passport_id);
    break;

    case SCP: /* 28 */
    point->passport().scp = new rtap_db::SCP_passport();
    rc = point->passport().scp->create(point->current_transaction());
    rc = point->passport().scp->autoid_get(passport_id);
    break;

    case DIR: /* 30 */
    point->passport().dir = new rtap_db::DIR_passport();
    rc = point->passport().dir->create(point->current_transaction());
    rc = point->passport().dir->autoid_get(passport_id);
    break;

    case DIPL:/* 31 */
    point->passport().dipl = new rtap_db::DIPL_passport();
    rc = point->passport().dipl->create(point->current_transaction());
    rc = point->passport().dipl->autoid_get(passport_id);
    break;

    case METLINE: /* 32 */
    point->passport().metline = new rtap_db::METLINE_passport();
    rc = point->passport().metline->create(point->current_transaction());
    rc = point->passport().metline->autoid_get(passport_id);
    break;

    case ESDG:/* 33 */
    point->passport().esdg = new rtap_db::ESDG_passport();
    rc = point->passport().esdg->create(point->current_transaction());
    rc = point->passport().esdg->autoid_get(passport_id);
    break;

    case SCPLINE: /* 35 */
    point->passport().scpline = new rtap_db::SCPLINE_passport();
    rc = point->passport().scpline->create(point->current_transaction());
    rc = point->passport().scpline->autoid_get(passport_id);
    break;

    case TLLINE:  /* 36 */
    point->passport().tlline = new rtap_db::TLLINE_passport();
    rc = point->passport().tlline->create(point->current_transaction());
    rc = point->passport().tlline->autoid_get(passport_id);
    break;

    case AUX1:/* 38 */
    point->passport().aux1 = new rtap_db::AUX1_passport();
    rc = point->passport().aux1->create(point->current_transaction());
    rc = point->passport().aux1->autoid_get(passport_id);
    break;

    case AUX2:/* 39 */
    point->passport().aux2 = new rtap_db::AUX2_passport();
    rc = point->passport().aux2->create(point->current_transaction());
    rc = point->passport().aux2->autoid_get(passport_id);
    break;

    case SITE:/* 45 */
    point->passport().site = new rtap_db::SITE_passport();
    rc = point->passport().site->create(point->current_transaction());
    rc = point->passport().site->autoid_get(passport_id);
    break;

    case SA:  /* 50 */
    point->passport().sa = new rtap_db::SA_passport();
    rc = point->passport().sa->create(point->current_transaction());
    rc = point->passport().sa->autoid_get(passport_id);
    break;

    case PIPE:/* 11 */
    point->passport().pipe = new rtap_db::PIPE_passport();
    rc = point->passport().pipe->create(point->current_transaction());
    rc = point->passport().pipe->autoid_get(passport_id);
    break;

    case PIPELINE:/* 15 */
    point->passport().pipeline = new rtap_db::PIPELINE_passport();
    rc = point->passport().pipeline->create(point->current_transaction());
    rc = point->passport().pipeline->autoid_get(passport_id);
    break;

    case RGA: /* 25 */
    point->passport().rga = new rtap_db::RGA_passport();
    rc = point->passport().rga->create(point->current_transaction());
    rc = point->passport().rga->autoid_get(passport_id);
    break;

    case BRG: /* 27 */
    point->passport().brg = new rtap_db::BRG_passport();
    rc = point->passport().brg->create(point->current_transaction());
    rc = point->passport().brg->autoid_get(passport_id);
    break;

    case STG: /* 29 */
    point->passport().stg = new rtap_db::STG_passport();
    rc = point->passport().stg->create(point->current_transaction());
    rc = point->passport().stg->autoid_get(passport_id);
    break;

    case SVLINE:  /* 34 */
    point->passport().svline = new rtap_db::SVLINE_passport();
    rc = point->passport().svline->create(point->current_transaction());
    rc = point->passport().svline->autoid_get(passport_id);
    break;

    case INVT:/* 37 */
    point->passport().invt = new rtap_db::INVT_passport();
    rc = point->passport().invt->create(point->current_transaction());
    rc = point->passport().invt->autoid_get(passport_id);
    break;

    // Точки, не имеющие паспорта
    case FIXEDPOINT:  /* 79 */
    case HIST: /* 80 */
       rc = MCO_S_OK;
       break;

    default:
    LOG(ERROR) << "Unknown object class (" << point->objclass() << ") for point";
  }

  point->setPassportId(passport_id);

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

// Функция корректировки переданного пользователем типа данных атрибута
// а) Устанавливает новый тип данных, если пользователь передал тип DB_TYPE_UNDEF
// б) Корректирует полученный тип данных в соответствием со значениями из словаря
// в) Для строковых типов устанавливает размер данных
void DatabaseRtapImpl::check_user_defined_type(int given_type, AttributeInfo_t* attr_info)
{
  // Пользователь указал неверный тип данных?
  if (attr_info->type != AttrTypeDescription[given_type].type)
  {
    // Да
    // Указанный ошибочно тип относится к фиксированной строке?
    if ((DB_TYPE_BYTES4 <= attr_info->type) && (attr_info->type <= DB_TYPE_BYTES256))
    {
      // Да
      // Был ошибочно указан строковый тип другого размера,
      delete [] attr_info->value.dynamic.varchar;
    }
    else if (attr_info->type == DB_TYPE_BYTES) // Ошибочно указан std::string
    {
      delete attr_info->value.dynamic.val_string;
    }
    else
    {
    // Был неверно указан тип целого или вещественного числа, не требующих динамического выделения памяти
    }

    // Правильный тип является строкой фиксированного размера?
    if ((DB_TYPE_BYTES4 <= AttrTypeDescription[given_type].type)
    && (attr_info->type <= AttrTypeDescription[given_type].type))
    {
      attr_info->value.dynamic.size = AttrTypeDescription[given_type].size;
      attr_info->value.dynamic.varchar = new char[attr_info->value.dynamic.size + 1];
    }
    else if (attr_info->type == DB_TYPE_BYTES) // Правильным типом является std::string
    {
      attr_info->value.dynamic.val_string = new std::string();
      //attr_info->value.dynamic.val_string->resize(attr_info->value.dynamic.size + 1);
    }
    else
    {
    // Правильный тип является числом, память не выделяется.
    }
  } // конец действий при неверном указании типа параметра

  attr_info->type = AttrTypeDescription[given_type].type;
}

// =================================================================================
// Найти точку с указанным тегом.
// В имени тега не может быть указан атрибут. 
// Читается весь набор атрибутов заданной точки.
// =================================================================================
rtap_db::Point* DatabaseRtapImpl::locate(mco_db_h& handle, const char* _tag)
{
  mco_trans_h t;
  MCO_RET rc = MCO_S_OK;
  rtap_db::XDBPoint instance;
  uint2 tag_size;
//  uint4 timer_value;
//  timer_mark_t now_time = {0, 0};
//  timer_mark_t prev_time = {0, 0};
  rtap_db::timestamp datehourm;

  assert(_tag);

//  LOG(INFO) << "Locating point " << _tag;
  tag_size = strlen(_tag);
      
  do
  {
    // NB: транзакция сделана на обновление специально с целью измерения производительности
    // TODO: нахождение точки в БД не должно менять значение ее DATEHOURM
//    rc = mco_trans_start(handle, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    rc = mco_trans_start(handle, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    rc = rtap_db::XDBPoint::SK_by_tag::find(t, _tag, tag_size, instance);
    if (rc) { LOG(ERROR) << "Locating point '" << _tag << "', rc=" << rc; break; }

//    rc = instance.DATEHOURM_read(datehourm);
//    if (rc) { LOG(ERROR) << "Reading DATEHOURM, point '" << _tag << "', rc=" << rc; break; }

//    datehourm.sec_get(timer_value);  //prev_time.tv_sec  = timer_value;
//    datehourm.nsec_get(timer_value); //prev_time.tv_nsec = timer_value;

//    GetTimerValue(now_time);

//    LOG(INFO) << "Difference between DATEHOURM : " << now_time.tv_sec - prev_time.tv_sec
//              << "." << (now_time.tv_nsec - prev_time.tv_nsec) / 1000;

//    datehourm.sec_put(now_time.tv_sec);
//    datehourm.nsec_put(now_time.tv_nsec);

//    rc = instance.DATEHOURM_write(datehourm);
//    if (rc) { LOG(ERROR) << "Writing DATEHOURM, point '" << _tag << "', rc=" << rc; break; }

    rc = mco_trans_commit(t);

  } while (false);

  if (rc)
    mco_trans_rollback(t);

  return NULL;
}

// Запись значения атрибута для заданной точки
// =================================================================================
const Error& DatabaseRtapImpl::write(mco_db_h& handle, AttributeInfo_t* info)
{
  rtap_db::XDBPoint instance;
  mco_trans_h t;
  bool func_found;
  AttrProcessingFuncMapIterator_t it;
  MCO_RET rc = MCO_S_OK;

  assert(info);
  setError(rtE_RUNTIME_ERROR);

  std::string::size_type point_pos = info->name.find(".");
  if (point_pos != std::string::npos)
  {
    // Есть точка в составе тега
    std::string point_name = info->name.substr(0, point_pos);
    std::string attr_name = info->name.substr(point_pos + 1, info->name.size());

#if defined VERBOSE
    LOG(INFO) << "Write atribute \"" << attr_name << "\" for point \"" << point_name << "\"";
#endif

    do
    {
      // TODO: объединить с аналогичной функцией нахождения точки locate() по имени в xdb_impl_connection.cpp
      rc = mco_trans_start(handle, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
      if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

      rc = rtap_db::XDBPoint::SK_by_tag::find(t, point_name.c_str(), point_name.size(), instance);
      if (rc) { LOG(ERROR) << "Locating point '" << point_name << "', rc=" << rc; setError(rtE_POINT_NOT_FOUND); break; }

      //LOG(INFO) << "We found point \"" << point_name;
      func_found = false;
      it = m_attr_writing_func_map.find(attr_name);
      if (it != m_attr_writing_func_map.end())
      {
          func_found = true;
          // Вызвать функцию создания атрибута.
          // Атрибуты хранения значений (VALACQ, VALMANUAL, VAL)
          // существуют в двух вариантах, для ТС и ТИ. Для них производится
          // определение типа рабочего объекта (атрибут OBJCLASS).
          rc = (this->*(it->second))(t, instance, info);
          if (rc)
          {
            LOG(ERROR) << "Updating value of " << point_name << "." << attr_name;
            setError(rtE_ILLEGAL_PARAMETER_VALUE);
            break;
          }
      }

      if (!func_found)
      {
          // Это может быть для атрибутов, создаваемых в паспорте
          // NB: поведение по умолчанию - пропустить атрибут, выдав предупреждение
          LOG(WARNING) << "Function for writing attribute '" << attr_name 
                       << "' doesn't found";
          setError(rtE_ATTR_NOT_FOUND);
      }
      else if (rc)
      {
          // Функция найдена, но вернула ошибку
          LOG(ERROR) << "Function for writing '" << "' failed, rc=" << rc;
          setError(rtE_ILLEGAL_PARAMETER_VALUE);
          break;
      }

      rc = mco_trans_commit(t);
      if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

      // Значение успешно занесено в БДРВ
      clearError();

    } while(false);

    if (rc)
      mco_trans_rollback(t);
  }
  else
  {
    LOG(ERROR) << "Tag name doesn't contain attribute: " << info->name;
    setError(rtE_ATTR_NOT_FOUND);
  }

  return getLastError();
}

// создать указанную точку со всеми ее атрибутами
// =================================================================================
MCO_RET DatabaseRtapImpl::createPoint(PointInDatabase* instance)
{
  MCO_RET    rc = MCO_S_OK;
//  rtap_db::Attrib*    attr = NULL;
//  int val;
//  int code;
  // найдена ли у данной точки функция создания хотя бы для одного
  // атрибута из перечня известных?
  bool func_found;
  unsigned int attr_idx;
  AttrCreatingFuncMapIterator_t it;

  setError(rtE_RUNTIME_ERROR);

  do
  {
    // Для всех атрибутов данной точки вызвать соотв. функцию его создания.
    // Вызывается соответствующая функция создания атрибута, определяемая
    // по таблице соответствия всех известных атрибутов и создающих их функций.
    //
    // Функция создания принимает на вход
    //      instance
    //      attribute_name
    //      attribute_value
    // и возвращает свой статус.
    //
    // Если статус ошибочный, откатить транзакцию для всей точки

    rc = createTAG(instance, /* NOT USED */instance->attributes()[0]);
    if (rc) { LOG(ERROR) << "Creating attribute TAG '" << instance->tag() << "'"; break; }

    rc = createOBJCLASS(instance, /* NOT USED */instance->attributes()[0]);
    if (rc) { LOG(ERROR) << "Creating attribute OBJCLASS:" << (int)instance->objclass(); break; }

    // остальные поля необязательные, создавать их по необходимости 
    // ============================================================
    // vvv Начало создания необязательных корневых атрибутов Точки
    // ============================================================
    for(attr_idx=0, func_found=false;
        attr_idx < instance->attributes().size();
        attr_idx++)
    {
      func_found = false;
      it = m_attr_creating_func_map.find(instance->attribute(attr_idx).name());

      if (it != m_attr_creating_func_map.end())
      {
        func_found = true;
        // NB: Часть атрибутов уже прописана в ходе создания паспорта
        //
        // Вызвать функцию создания атрибута
        rc = (this->*(it->second))(instance, instance->attribute(attr_idx));
        if (rc)
        {
          LOG(ERROR) << "Creation attribute '"
                     << instance->attribute(attr_idx).name()
                     << "' for point '" << instance->tag() << "'";
          break;
        }
      }

      if (!func_found)
      {
        // Это может быть для атрибутов, создаваемых в паспорте
        // NB: поведение по умолчанию - пропустить атрибут, выдав предупреждение
        LOG(WARNING) << "Function creating '" << instance->attribute(attr_idx).name() 
                     << "' is not found";
        setError(rtE_ATTR_NOT_FOUND);
      }
      else if (rc)
      {
        // Функция найдена, но вернула ошибку
        LOG(ERROR) << "Function creating '" << "' failed";
        break;
      }    
    }
    // ==========================================================
    // ^^^ Конец создания необязательных корневых атрибутов Точки
    // ==========================================================

    // Была обнаружена ошибка создания одного из атрибутов точки -
    // пропускаем всю точку целиком, откатывая транзакцию
    if (rc)
    {
      LOG(ERROR) << "Skip creating point '" << instance->tag() << "' attibutes";
      break;
    }

    // =================================================
    // Обновление значений служебных атрибутов
    // =================================================
    rc = instance->update_references();

    rc = instance->xdbpoint().checkpoint();
    if (rc) { LOG(ERROR) << "Checkpoint, rc=" << rc; break; }

    clearError();
  }
  while(false);

  return rc;
}

// == Группа общих функций создания указанного атрибута для Точки
// ==============================================================
MCO_RET DatabaseRtapImpl::createTAG(PointInDatabase* instance, rtap_db::Attrib&)
{
  MCO_RET rc = instance->xdbpoint().TAG_put(instance->tag().c_str(),
                                            static_cast<uint2>(instance->tag().size()));
  return rc;
}

// ======================== OBJCLASS ============================
MCO_RET DatabaseRtapImpl::createOBJCLASS(PointInDatabase* instance, rtap_db::Attrib&)
{
  MCO_RET rc = instance->xdbpoint().OBJCLASS_put(instance->objclass());
  return rc;
}

// Прочесть значение штатными средствами
MCO_RET DatabaseRtapImpl::readOBJCLASS(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  objclass_t objclass;
  MCO_RET rc = instance.OBJCLASS_get(objclass);

  if ((GOF_D_BDR_OBJCLASS_TS <= objclass) && (objclass <= GOF_D_BDR_OBJCLASS_LASTUSED))
  {
    // TODO: задать перекодировочный массив rtap_db::objclass <=> константы GOF_D_BDR_OBJCLASS_*
    attr_info->value.fixed.val_int8 = static_cast<int8_t>(objclass);
    attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_OBJCLASS].type;
#if defined VERBOSE
    LOG(INFO) << attr_info->name << " = " << (unsigned int)attr_info->value.fixed.val_int8; //1
#endif
  }
  else
  {
    LOG(ERROR) << "Unable to get " << attr_info->name;
    attr_info->value.fixed.val_int8 = GOF_D_BDR_OBJCLASS_UNUSED;
    attr_info->type = DB_TYPE_UNDEF;
  }

  return rc;
}
// NB: Изменение OBJCLASS уже созданной точки созапрещено

// ======================== STATUS ============================
MCO_RET DatabaseRtapImpl::readSTATUS(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_STATUS;
  MCO_RET rc = MCO_S_OK;
  PointStatus status;

  assert(attr_info);

  rc = instance.STATUS_get(status);

  if (rc)
  {
      LOG(ERROR) << "Reading " << attr_name << " failure (will be assigned to DISABLED), rc=" << rc;
      status = DISABLED;
  }
  attr_info->value.fixed.val_uint8 = static_cast<uint8_t>(status);
  attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_STATUS].type;

#if defined VERBOSE
  LOG(INFO) << attr_info->name << " = " << (unsigned int)attr_info->value.fixed.val_int8; //1
#endif

  return rc;
}

MCO_RET DatabaseRtapImpl::writeSTATUS(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_STATUS;
  MCO_RET rc = MCO_S_OK;
  PointStatus status;

  assert(attr_info);

  switch(attr_info->value.fixed.val_uint8)
  {
      case 0: status = PLANNED;  break;
      case 1: status = WORKED;   break;
      case 2: status = DISABLED; break;

      default:
        LOG(WARNING) << "Unsupported " << attr_name << " value: "
                     << (unsigned int)attr_info->value.fixed.val_uint8;
        rc = MCO_E_ILLEGAL_PARAM; // запретить занесение неверного значения
      break;
  }

  if (MCO_S_OK == rc)
    rc = instance.STATUS_put(status);

  return rc;
}

MCO_RET DatabaseRtapImpl::createSTATUS(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_STATUS;
  MCO_RET rc;
  PointStatus status = WORKED;
  int val = atoi(attr.value().c_str());

  switch(val)
  {
    case 0: status = PLANNED;  break;
    case 1: status = WORKED;   break;
    case 2: status = DISABLED; break;
    default:
      LOG(WARNING) << "Unsupported " << attr_name << " value " << val << ", will use 'DISABLED'";
      status = DISABLED;
  }

  rc = instance->xdbpoint().STATUS_put(status);
  return rc;
}

// ======================== SHORTLABEL ============================
MCO_RET DatabaseRtapImpl::readSHORTLABEL(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc = MCO_S_OK;

  assert(attr_info);

  check_user_defined_type(RTDB_ATT_IDX_SHORTLABEL, attr_info);

  rc = instance.SHORTLABEL_get(attr_info->value.dynamic.varchar, attr_info->value.dynamic.size);

#if defined VERBOSE
  LOG(INFO) << attr_info->name << " = " << attr_info->value.dynamic.varchar;
#endif
  return rc;
}

MCO_RET DatabaseRtapImpl::writeSHORTLABEL(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
//  static const char *attr_name = RTDB_ATT_SHORTLABEL;
  MCO_RET rc = MCO_S_OK;

  assert(attr_info);
  assert(attr_info->value.dynamic.varchar);
//  assert(attr_info->type = DB_TYPE_BYTES32);
//  DbType_t type = AttrTypeDescription[RTDB_ATT_IDX_SHORTLABEL].type;

  rc = instance.SHORTLABEL_put(attr_info->value.dynamic.varchar, AttrTypeDescription[RTDB_ATT_IDX_SHORTLABEL].size /*DbTypeDescription[type].size*/);
  return rc;
}

MCO_RET DatabaseRtapImpl::createSHORTLABEL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  MCO_RET rc = instance->xdbpoint().SHORTLABEL_put(attr.value().c_str(),
                                                   static_cast<uint2>(attr.value().size())); 
  return rc;
}

// ======================== UNITY ============================
// По заданной строке UNITY найти ее идентификатор, и подставить в поле UNITY_ID
MCO_RET DatabaseRtapImpl::createUNITY(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  uint2 unity_id;
  DICT_UNITY_ID instance_unity;
  MCO_RET rc = MCO_S_OK;

  do
  {
    // Найти instance_unity
    rc = DICT_UNITY_ID_unityHash_find(instance->current_transaction(),
        attr.value().c_str(),
        static_cast<uint2>(attr.value().size()),
        &instance_unity);
    if (rc) { LOG(ERROR) << "Locating unity '" << attr.value() << "'"; break; }

    rc = DICT_UNITY_ID_unityID_get(&instance_unity, &unity_id);
    if (rc) { LOG(ERROR) << "Can't get '" << attr.value() << "' unity id"; break; }

    // LOG(INFO) << "rc=" << rc << " " << attr.value() << "->" << unity_id;
    rc = instance->xdbpoint().UNITY_ID_put(unity_id);
    if (rc) { LOG(ERROR) << "Can't put unity id=" << unity_id << " for " << instance->tag(); break; }

  } while (false);

  return rc;
}
//== Конец общей части класса XDBPoint ==============================================

//== Начало индивидуальных атрибутов для различных OBJCLASS таблицы XDBPoint ========
// Создать ссылку на указанную Систему Сбора
// TODO:
// 1. Получить идентификатор данной СС в НСИ
// 2. Занести этот идентификатор в поле SA_ref
MCO_RET DatabaseRtapImpl::createL_SA(PointInDatabase* /* instance */, rtap_db::Attrib& /* attr */)
{
  //autoid_t sa_id;
  MCO_RET rc = MCO_S_OK;
  
  // TODO: реализация
  //rc = instance->xdbpoint().SA_ref_get(sa_id);
  //rc = instance->xdbpoint().SA_ref_put(sa_id);
  return rc;
}

// ======================== VALIDITY ============================
// Соответствует атрибуту БДРВ - VALIDITY
MCO_RET DatabaseRtapImpl::createVALID(PointInDatabase* instance, rtap_db::Attrib& attr)
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
    case 5: /* FAULT */
            // NB: break пропущен специально
    default:
      value = FAULT;
  }

  rc = instance->xdbpoint().VALIDITY_put(value);
  return rc;
}

MCO_RET DatabaseRtapImpl::readVALID(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  Validity value;

  attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_VALID].type;

  rc = instance.VALIDITY_get(value);

  switch(value)
  {
    case INVALID:
    case VALID:
    case MANUAL:
    case DUBIOUS:
    case INHIBITED:
      attr_info->value.fixed.val_uint8 = static_cast<uint8_t>(value);
#if defined VERBOSE
      LOG(INFO) << attr_info->name << " = " << (unsigned int)attr_info->value.fixed.val_uint8; //1
#endif
    break;

    case FAULT: // NB: break пропущен специально
    default:
      attr_info->value.fixed.val_uint8 = static_cast<uint8_t>(FAULT);
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::writeVALID(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  Validity value;

  switch(attr_info->value.fixed.val_uint8)
  {
    case 0 /* INVALID */:
    case 1 /* VALID */:
    case 2 /* MANUAL */:
    case 3 /* DUBIOUS */:
    case 4 /* INHIBITED */:
      value = static_cast<Validity>(attr_info->value.fixed.val_uint8);
    break;

    case 5 /* FAULT */: // NB: break пропущен специально
    default:
      value = static_cast<Validity>(FAULT);
  }

  rc = instance.VALIDITY_put(value);

  return rc;
}

// ======================== VALIDITY_ACQ ============================
// Соответствует атрибуту БДРВ - VALIDITY_ACQ
MCO_RET DatabaseRtapImpl::createVALIDACQ(PointInDatabase* instance, rtap_db::Attrib& attr)
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

  rc = instance->xdbpoint().VALIDITY_ACQ_put(value);
  return rc;
}

MCO_RET DatabaseRtapImpl::readVALIDACQ(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  Validity value;

  attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_VALIDACQ].type;

  rc = instance.VALIDITY_ACQ_get(value);

  switch(value)
  {
    case INVALID:
    case VALID:
    case MANUAL:
    case DUBIOUS:
    case INHIBITED:
      attr_info->value.fixed.val_uint8 = static_cast<uint8_t>(value);
    break;

    case FAULT: // NB: break пропущен специально
    default:
      attr_info->value.fixed.val_uint8 = static_cast<uint8_t>(FAULT);
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::writeVALIDACQ(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  Validity value;

  switch(attr_info->value.fixed.val_uint8)
  {
    case 0 /* INVALID */:
    case 1 /* VALID */:
    case 2 /* MANUAL */:
    case 3 /* DUBIOUS */:
    case 4 /* INHIBITED */:
      value = static_cast<Validity>(attr_info->value.fixed.val_uint8);
    break;

    case 5 /* FAULT */: // NB: break пропущен специально
    default:
      value = static_cast<Validity>(FAULT);
  }

  rc = instance.VALIDITY_ACQ_put(value);

  return rc;
}

// ======================== CURRENT_SHIFT_TIME ============================
MCO_RET DatabaseRtapImpl::readCURRENT_SHIFT_TIME(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_CURRENT_SHIFT_TIME;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::timestamp datehourm;
  datetime_t datetime;
  rtap_db::SITE_passport passport_instance;
  autoid_t passport_id;
  objclass_t objclass;

  do
  {
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    if (objclass != GOF_D_BDR_OBJCLASS_SITE)
    {
      LOG(ERROR) << "Point " <<  attr_info->name << " with objclass "
                 << objclass << " shouldn't contain " << attr_name;
      break;
    }

    rc = instance.passport_ref_get(passport_id);
    if (rc) { LOG(ERROR) << "Can't get passport id for " << attr_info->name; break; }

    rc = rtap_db::SITE_passport::autoid::find(t, passport_id, passport_instance);
    if (rc) { LOG(ERROR) << "Can't find SITE passport for " << attr_info->name; break; }

    rc = passport_instance.CURRENT_SHIFT_TIME_read(datehourm);
    if (rc) { LOG(ERROR) << "Can't read " << attr_name << " for " << attr_info->name; break; }

    rc = datehourm.sec_get(datetime.part[0]);
    rc = datehourm.nsec_get(datetime.part[1]);
    if (rc) { LOG(ERROR) << "Can't read " << attr_name << " value for " << attr_info->name; break; }

    attr_info->value.fixed.val_time.tv_sec = datetime.part[0];
    attr_info->value.fixed.val_time.tv_usec = datetime.part[1];

    attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_CURRENT_SHIFT_TIME].type;

  } while(false);

  return rc;
}

MCO_RET DatabaseRtapImpl::writeCURRENT_SHIFT_TIME(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
//  static const char *attr_name = RTDB_ATT_CURRENT_SHIFT_TIME;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::timestamp datehourm;
  datetime_t datetime;
  rtap_db::SITE_passport passport_instance;
  autoid_t passport_id;

  //assert(attr_info->type = DB_TYPE_ABSTIME);

  do
  {
    rc = instance.passport_ref_get(passport_id);
    if (rc) { LOG(ERROR) << "Can't get passport id for " << attr_info->name; break; }

    rc = rtap_db::SITE_passport::autoid::find(t, passport_id, passport_instance);
    if (rc) { LOG(ERROR) << "Can't find SITE passport for " << attr_info->name; break; }

    rc = passport_instance.CURRENT_SHIFT_TIME_write(datehourm);
    if (rc) { LOG(ERROR) << "Can't write attribute " << attr_info->name; break; }

    datetime.part[0] = attr_info->value.fixed.val_time.tv_sec;
    datetime.part[1] = attr_info->value.fixed.val_time.tv_usec;

    rc = datehourm.sec_put(datetime.part[0]);
    rc = datehourm.nsec_put(datetime.part[1]);

    if (rc) { LOG(ERROR) << "Can't write value for " << attr_info->name; break; }

  } while(false);

  return rc;
}

MCO_RET DatabaseRtapImpl::createCURRENT_SHIFT_TIME(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_CURRENT_SHIFT_TIME;
  rtap_db::timestamp datehourm;
  MCO_RET rc = MCO_S_NOTFOUND;
  datetime_t datetime;

  switch(instance->objclass())
  {
    case SITE:
        rc = instance->passport().site->CURRENT_SHIFT_TIME_write(datehourm);
        if (!rc) 
        {
            // Конвертировать дату из 8 байт XML-файла в формат rtap_db::timestamp
            // (секунды и наносекунды по 4 байта)
            datetime.common = atoll(attr.value().c_str());
            datehourm.sec_put(datetime.part[0]);
            datehourm.nsec_put(datetime.part[1]);
        }
        break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }

  return rc;
}

// ======================== PREV_DISPATCHER ============================
MCO_RET DatabaseRtapImpl::createPREV_DISPATCHER(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_PREV_DISPATCHER;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case SITE:
    rc = instance->passport().site->PREV_DISPATCHER_put(attr.value().c_str(),
                                                        attr.value().size());
    break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }

  return rc;
}

// ======================== PREV_SHIFT_TIME ============================
MCO_RET DatabaseRtapImpl::createPREV_SHIFT_TIME(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_PREV_SHIFT_TIME;
  rtap_db::timestamp datehourm;
  MCO_RET rc = MCO_S_NOTFOUND;
  datetime_t datetime;

  switch(instance->objclass())
  {
    case SITE:
        rc = instance->passport().site->PREV_SHIFT_TIME_write(datehourm);
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
            LOG(ERROR) << "Create " << attr_name << ", rc=" << rc;
        }
        break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }

  return rc;
}

// ======================== DATEAINS ============================
/*
 * Дата введения оборудования в эксплуатацию.
 * Использовать для подсчета наработки времени на отказ и т.п.
 */
MCO_RET DatabaseRtapImpl::createDATEAINS(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_DATEAINS;
  rtap_db::timestamp datehourm;
  MCO_RET rc = MCO_S_NOTFOUND;
  datetime_t datetime;
  struct tm given_time;
  std::string::size_type point_pos;

  switch(instance->objclass())
  {
    case VA:   rc = instance->passport().valve->DATEAINS_write(datehourm);break;
    case GRC:  rc = instance->passport().grc->DATEAINS_write(datehourm);  break;
    case AUX1: rc = instance->passport().aux1->DATEAINS_write(datehourm); break;
    case AUX2: rc = instance->passport().aux2->DATEAINS_write(datehourm); break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }

  // Если экземпляр отметки времени создан, заполним его
  if (!rc) 
  {
    // Конвертировать дату из 8 байт XML-файла в формат rtap_db::timestamp
    // (секунды и наносекунды по 4 байта)
    strptime(attr.value().c_str(), D_DATE_FORMAT_STR, &given_time);
    datetime.part[0] = given_time.tm_sec;
    point_pos = attr.value().find_last_of('.');

    // Если точка найдена, и она не последняя в строке
    if ((point_pos != std::string::npos) && point_pos != attr.value().size())
      datetime.part[1] = atoi(attr.value().substr(point_pos + 1).c_str());
    else
      datetime.part[1] = 0;

    datehourm.sec_put(datetime.part[0]);
    datehourm.nsec_put(datetime.part[1]);
 //   LOG(INFO) << "DATETIME '" << attr.value().c_str() << "' " << datetime.part[0] << ":" << datetime.part[1];
  }

  return rc;
}

// ======================== DATEHOURM ============================
MCO_RET DatabaseRtapImpl::createDATEHOURM(PointInDatabase* instance, rtap_db::Attrib& attr)
{
//  static const char *attr_name = RTDB_ATT_DATEHOURM;
  rtap_db::timestamp datehourm;
  datetime_t datetime;
  struct tm given_time;
  std::string::size_type point_pos;
  MCO_RET rc = instance->xdbpoint().DATEHOURM_write(datehourm);

  if (MCO_S_OK == rc) 
  {
    // Конвертировать дату из 8 байт XML-файла в формат rtap_db::timestamp
    // (секунды и наносекунды по 4 байта)
    strptime(attr.value().c_str(), D_DATE_FORMAT_STR, &given_time);
    datetime.part[0] = given_time.tm_sec;
    point_pos = attr.value().find_last_of('.');

    // Если точка найдена, и она не последняя в строке
    if ((point_pos != std::string::npos) && point_pos != attr.value().size())
      datetime.part[1] = atoi(attr.value().substr(point_pos + 1).c_str());
    else
      datetime.part[1] = 0;

    datehourm.sec_put(datetime.part[0]);
    datehourm.nsec_put(datetime.part[1]);
 //   LOG(INFO) << "DATETIME '" << attr.value().c_str() << "' " << datetime.part[0] << ":" << datetime.part[1];
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::readDATEHOURM(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc = MCO_S_OK;
  datetime_t datetime;
  rtap_db::timestamp ts;

  attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_DATEHOURM].type;

  datetime.part[0] = attr_info->value.fixed.val_time.tv_sec;
  datetime.part[1] = attr_info->value.fixed.val_time.tv_usec;

  rc = instance.DATEHOURM_read(ts);
  if (MCO_S_OK == rc) {
    rc = ts.sec_get(datetime.part[0]);
    rc = ts.nsec_get(datetime.part[1]);
#if defined VERBOSE
    LOG(INFO) << attr_info->name << " = " << datetime.part[0] << ":" << datetime.part[1]; //1
#endif
  }
  else
  {
    attr_info->value.fixed.val_time.tv_sec = 0;
    attr_info->value.fixed.val_time.tv_usec = 0;
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::writeDATEHOURM(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  datetime_t datetime;
  rtap_db::timestamp ts;

  datetime.part[0] = attr_info->value.fixed.val_time.tv_sec;
  datetime.part[1] = attr_info->value.fixed.val_time.tv_usec;

  do {
    rc = ts.sec_put(datetime.part[0]);
    if (rc) { LOG(ERROR) << attr_info->name << ", put " << attr_info->value.fixed.val_time.tv_sec << " seconds"; break; }
    ts.nsec_put(datetime.part[1]);
    if (rc) { LOG(ERROR) << attr_info->name << ", put " << attr_info->value.fixed.val_time.tv_sec << " nseconds"; break; }
    rc = instance.DATEHOURM_write(ts);
  } while (false);

  return rc;
}

// ======================== DATERTU ============================
MCO_RET DatabaseRtapImpl::createDATERTU(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  rtap_db::timestamp datertu;
  datetime_t datetime;
  struct tm given_time;
  std::string::size_type point_pos;
  MCO_RET rc = instance->xdbpoint().DATERTU_write(datertu);

  if (MCO_S_OK == rc)
  {
    // Конвертировать дату из 8 байт XML-файла в формат rtap_db::timestamp
    // (секунды и наносекунды по 4 байта)
    strptime(attr.value().c_str(), D_DATE_FORMAT_STR, &given_time);
    datetime.part[0] = given_time.tm_sec;
    point_pos = attr.value().find_last_of('.');

    // Если точка найдена, и она не последняя в строке
    if ((point_pos != std::string::npos) && point_pos != attr.value().size())
      datetime.part[1] = atoi(attr.value().substr(point_pos + 1).c_str());
    else
      datetime.part[1] = 0;

    datertu.sec_put(datetime.part[0]);
    datertu.nsec_put(datetime.part[1]);
 //   LOG(INFO) << "DATETIME '" << attr.value().c_str() << "' " << datetime.part[0] << ":" << datetime.part[1];
  }

  return rc;
}


// ======================== VALIDCHANGE ============================
MCO_RET DatabaseRtapImpl::createVALIDCHANGE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  uint1 value = atoi(attr.value().c_str());
  ValidChange vc;

  switch (value)
  {
    case 1:  vc = VALIDCHANGE_VALID;     break;
    case 2:  vc = VALIDCHANGE_FORCED;    break;
    case 3:  vc = VALIDCHANGE_INHIB;     break;
    case 4:  vc = VALIDCHANGE_MANUAL;    break;
    case 5:  vc = VALIDCHANGE_END_INHIB; break;
    case 6:  vc = VALIDCHANGE_END_FORCED;break;
    case 7:  vc = VALIDCHANGE_INHIB_GBL; break;
    case 8:  vc = VALIDCHANGE_END_INHIB_GBL; break;
    case 9:  vc = VALIDCHANGE_NULL;      break;
    case 10: vc = VALIDCHANGE_FAULT_GBL; break;
    case 11: vc = VALIDCHANGE_INHIB_SA;  break;
    case 12: vc = VALIDCHANGE_END_INHIB_SA; break;
    default: vc = VALIDCHANGE_NULL;
  }

  MCO_RET rc = instance->xdbpoint().VALIDCHANGE_put(vc);
  return rc;
}

MCO_RET DatabaseRtapImpl::readVALIDCHANGE(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  ValidChange vc;
  MCO_RET rc = instance.VALIDCHANGE_get(vc);

  switch (vc)
  {
    case VALIDCHANGE_VALID:         attr_info->value.fixed.val_int8 = 1;  break;
    case VALIDCHANGE_FORCED:        attr_info->value.fixed.val_int8 = 2;  break;
    case VALIDCHANGE_INHIB:         attr_info->value.fixed.val_int8 = 3;  break;
    case VALIDCHANGE_MANUAL:        attr_info->value.fixed.val_int8 = 4;  break;
    case VALIDCHANGE_END_INHIB:     attr_info->value.fixed.val_int8 = 5;  break;
    case VALIDCHANGE_END_FORCED:    attr_info->value.fixed.val_int8 = 6;  break;
    case VALIDCHANGE_INHIB_GBL:     attr_info->value.fixed.val_int8 = 7;  break;
    case VALIDCHANGE_END_INHIB_GBL: attr_info->value.fixed.val_int8 = 8;  break;
    case VALIDCHANGE_NULL:          attr_info->value.fixed.val_int8 = 9;  break;
    case VALIDCHANGE_FAULT_GBL:     attr_info->value.fixed.val_int8 = 10; break;
    case VALIDCHANGE_INHIB_SA:      attr_info->value.fixed.val_int8 = 11; break;
    case VALIDCHANGE_END_INHIB_SA:  attr_info->value.fixed.val_int8 = 12; break;
    default: attr_info->value.fixed.val_int8 = 9;
  }
  attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_VALIDCHANGE].type;

#if defined VERBOSE
  LOG(INFO) << attr_info->name << " = " << (unsigned int)attr_info->value.fixed.val_int8;
#endif

  return rc;
}

MCO_RET DatabaseRtapImpl::writeVALIDCHANGE(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc = MCO_S_NOTFOUND;
  ValidChange vc;

  switch (attr_info->value.fixed.val_int8)
  {
    case 1:  vc = VALIDCHANGE_VALID;     break;
    case 2:  vc = VALIDCHANGE_FORCED;    break;
    case 3:  vc = VALIDCHANGE_INHIB;     break;
    case 4:  vc = VALIDCHANGE_MANUAL;    break;
    case 5:  vc = VALIDCHANGE_END_INHIB; break;
    case 6:  vc = VALIDCHANGE_END_FORCED;break;
    case 7:  vc = VALIDCHANGE_INHIB_GBL; break;
    case 8:  vc = VALIDCHANGE_END_INHIB_GBL; break;
    case 9:  vc = VALIDCHANGE_NULL;      break;
    case 10: vc = VALIDCHANGE_FAULT_GBL; break;
    case 11: vc = VALIDCHANGE_INHIB_SA;  break;
    case 12: vc = VALIDCHANGE_END_INHIB_SA; break;
    default: vc = VALIDCHANGE_NULL;
  }

  rc = instance.VALIDCHANGE_put(vc);

  return rc;
}

// ======================== LABEL ============================
MCO_RET DatabaseRtapImpl::createLABEL (PointInDatabase* /* instance */, rtap_db::Attrib& /* attr */)
{
//  LOG(INFO) << "CALL createLABEL on " << attr.name();
//  NB: Пока молча пропустим этот атрибут, зато потом...
  return MCO_S_OK;
}

// ======================== TYPE ============================
MCO_RET DatabaseRtapImpl::createTYPE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_TYPE;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 value = atoi(attr.value().c_str());

  switch(instance->objclass())
  {
    case SA: rc = instance->passport().sa->TYPE_put(value); break;
    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::createL_EQTTYP(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_EQTTYP;
  MCO_RET rc = MCO_S_NOTFOUND;
  const std::string value = attr.value();
  uint1 sz = attr.value().size();

  switch(instance->objclass())
  {
    case TL:      rc = instance->passport().tl->L_EQTTYP_put(attr.value().c_str(), sz);      break;
    case VA:      rc = instance->passport().valve->L_EQTTYP_put(attr.value().c_str(), sz);   break;
    case SC:      rc = instance->passport().sc->L_EQTTYP_put(attr.value().c_str(), sz);      break;
    case ATC:     rc = instance->passport().atc->L_EQTTYP_put(attr.value().c_str(), sz);     break;
    case GRC:     rc = instance->passport().grc->L_EQTTYP_put(attr.value().c_str(), sz);     break;
    case SV:      rc = instance->passport().sv->L_EQTTYP_put(attr.value().c_str(), sz);      break;
    case SDG:     rc = instance->passport().sdg->L_EQTTYP_put(attr.value().c_str(), sz);     break;
    case RGA:     rc = instance->passport().rga->L_EQTTYP_put(attr.value().c_str(), sz);     break;
    case SSDG:    rc = instance->passport().ssdg->L_EQTTYP_put(attr.value().c_str(), sz);    break;
    case BRG:     rc = instance->passport().brg->L_EQTTYP_put(attr.value().c_str(), sz);     break;
    case SCP:     rc = instance->passport().scp->L_EQTTYP_put(attr.value().c_str(), sz);     break;
    case STG:     rc = instance->passport().stg->L_EQTTYP_put(attr.value().c_str(), sz);     break;
    case DIPL:    rc = instance->passport().dipl->L_EQTTYP_put(attr.value().c_str(), sz);    break;
    case METLINE: rc = instance->passport().metline->L_EQTTYP_put(attr.value().c_str(), sz); break;
    case ESDG:    rc = instance->passport().esdg->L_EQTTYP_put(attr.value().c_str(), sz);    break;
    case SVLINE:  rc = instance->passport().svline->L_EQTTYP_put(attr.value().c_str(), sz);  break;
    case SCPLINE: rc = instance->passport().scpline->L_EQTTYP_put(attr.value().c_str(), sz); break;
    case TLLINE:  rc = instance->passport().tlline->L_EQTTYP_put(attr.value().c_str(), sz);  break;
    case INVT:    rc = instance->passport().invt->L_EQTTYP_put(attr.value().c_str(), sz);    break;
    case AUX1:    rc = instance->passport().aux1->L_EQTTYP_put(attr.value().c_str(), sz);    break;
    case AUX2:    rc = instance->passport().aux2->L_EQTTYP_put(attr.value().c_str(), sz);    break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }

  return rc;
}


MCO_RET DatabaseRtapImpl::createLOCALFLAG(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_LOCALFLAG;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 value = atoi(attr.value().c_str());
  Responsability resp;

  switch (value)
  {
    case 0: resp = LOCAL;   break;
    case 1: resp = DISTANT; break;
    default:
        LOG(ERROR) <<"Unsupported value=" << value
                   << " for " << attr_name << ", use LOCAL as default";
        resp = LOCAL;
  }

  switch(instance->objclass())
  {
    case PIPE:     rc = instance->passport().pipe->LOCALFLAG_put(resp);break;
    case PIPELINE: rc = instance->passport().pipeline->LOCALFLAG_put(resp);break;
    case TL:       rc = instance->passport().tl->LOCALFLAG_put(resp);  break;
    case VA:       rc = instance->passport().valve->LOCALFLAG_put(resp);   break;
    case SC:       rc = instance->passport().sc->LOCALFLAG_put(resp);  break;
    case ATC:      rc = instance->passport().atc->LOCALFLAG_put(resp); break;
    case GRC:      rc = instance->passport().grc->LOCALFLAG_put(resp); break;
    case SV:       rc = instance->passport().sv->LOCALFLAG_put(resp);  break;
    case SDG:      rc = instance->passport().sdg->LOCALFLAG_put(resp); break;
    case RGA:      rc = instance->passport().rga->LOCALFLAG_put(resp); break;
    case SSDG:     rc = instance->passport().ssdg->LOCALFLAG_put(resp);break;
    case BRG:      rc = instance->passport().brg->LOCALFLAG_put(resp); break;
    case SCP:      rc = instance->passport().scp->LOCALFLAG_put(resp); break;
    case STG:      rc = instance->passport().stg->LOCALFLAG_put(resp); break;
    case DIR:      rc = instance->passport().dir->LOCALFLAG_put(resp); break;
    case DIPL:     rc = instance->passport().dipl->LOCALFLAG_put(resp);break;
    case METLINE:  rc = instance->passport().metline->LOCALFLAG_put(resp); break;
    case ESDG:     rc = instance->passport().esdg->LOCALFLAG_put(resp);    break;
    case SVLINE:   rc = instance->passport().svline->LOCALFLAG_put(resp);  break;
    case SCPLINE:  rc = instance->passport().scpline->LOCALFLAG_put(resp); break;
    case TLLINE:   rc = instance->passport().tlline->LOCALFLAG_put(resp);  break;
    case INVT:     rc = instance->passport().invt->LOCALFLAG_put(resp);break;
    case AUX1:     rc = instance->passport().aux1->LOCALFLAG_put(resp);break;
    case AUX2:     rc = instance->passport().aux2->LOCALFLAG_put(resp);break;
    case SA:       rc = instance->passport().sa->LOCALFLAG_put(resp);  break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createMINVAL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_MINVAL;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case TR: rc = instance->passport().tr->MINVAL_put(value); break;
    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createMAXVAL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_MAXVAL;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case TR: rc = instance->passport().tr->MAXVAL_put(value); break;
    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

// ======================== VAL ============================
// Значение VAL м.б. для аналогового или дискретного инфотипов.
// Соотвествующая своему OBJCLASS структура инфотипа инициируются в create().
MCO_RET DatabaseRtapImpl::createVAL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_VAL;
  MCO_RET rc = MCO_S_NOTFOUND;
  double double_value;
  uint8  integer64_value;

  switch(instance->objclass())
  {
    case TM:
    case TR:
    case ICM:
        double_value = atof(attr.value().c_str());
        rc = instance->AIT().VAL_put(double_value);
        break;

    case TS:
    case TSA:
    case TSC:
    case AL:
    case ICS:
        integer64_value = atoll(attr.value().c_str());
        rc = instance->DIT().VAL_put(integer64_value);
        break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::readVAL(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_VAL;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::DiscreteInfoType di;
  rtap_db::AnalogInfoType   ai;
  objclass_t objclass;

  do
  {
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    switch(objclass)
    {
        case TM:
        case TR:
        case ICM:
            rc = instance.ai_read(ai);
            if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

            rc = ai.VAL_get(attr_info->value.fixed.val_double);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }

            // NB: не использовать таблицу свойств атрибутов AttrTypeDescription, поскольку
            // VAL - это особый случай, он может быть целочисленным или с плав. точкой
            attr_info->type = DB_TYPE_DOUBLE;
#if defined VERBOSE
            LOG(INFO) << attr_info->name << " = " << attr_info->value.fixed.val_double; //1
#endif
            break;

        case TS:
        case TSA:
        case TSC:
        case AL:
        case ICS:
            rc = instance.di_read(di);
            if (rc) { LOG(ERROR) << "Can't read discrete part of " << attr_info->name; break; }

            rc = di.VAL_get(attr_info->value.fixed.val_uint64);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }

            // NB: не использовать таблицу свойств атрибутов AttrTypeDescription, поскольку
            // VAL - это особый случай, он может быть целочисленным или с плав. точкой
            attr_info->type = DB_TYPE_UINT64;
#if defined VERBOSE
            LOG(INFO) << attr_info->name << " = " << attr_info->value.fixed.val_uint64; //1
#endif
            break;

        default:
            LOG(ERROR) << "'" << attr_name
                       << "' for objclass " << objclass
                       << " is not supported, point " << attr_info->name;
            attr_info->type = DB_TYPE_UNDEF;
            break;
    }
    // Если внутри switch(objclass) была ошибка, дальнейшая обработка сразу прекращается
    if (rc) break;

  } while(false);

  return rc;
}

MCO_RET DatabaseRtapImpl::writeVAL(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_VALACQ;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::DiscreteInfoType di;
  rtap_db::AnalogInfoType   ai;
  objclass_t objclass;

  do
  {
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    switch(objclass)
    {
        case TM:
        case TR:
        case ICM:
            rc = instance.ai_read(ai);
            if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

            rc = ai.VAL_put(attr_info->value.fixed.val_double);
            if (rc) { LOG(ERROR) << "Can't write " << attr_info->name; break; }
            break;

        case TS:
        case TSA:
        case TSC:
        case AL:
        case ICS:
            rc = instance.di_read(di);
            if (rc) { LOG(ERROR) << "Can't read discrete part of " << attr_info->name; break; }

            rc = di.VAL_put(attr_info->value.fixed.val_uint64);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }
            break;

        default:
            LOG(ERROR) << "'" << attr_name
                       << "' for objclass " << objclass
                       << " is not supported, point " << attr_info->name;
            break;
    }
    // Если внутри switch(objclass) была ошибка, дальнейшая обработка сразу прекращается
    if (rc) break;

  } while(false);

  return rc;
}

// ======================== VALACQ ============================
MCO_RET DatabaseRtapImpl::createVALACQ(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_VALACQ;
  MCO_RET rc = MCO_S_NOTFOUND;
  double double_value;
  uint8  integer64_value;

  switch(instance->objclass())
  {
    case TM:
        double_value = atof(attr.value().c_str());
        rc = instance->AIT().VALACQ_put(double_value);
        break;

    case TS:
    case TSA:
    case TSC:
    case AL:
        integer64_value = atoll(attr.value().c_str());
        rc = instance->DIT().VALACQ_put(integer64_value);
        break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::readVALACQ(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_VALACQ;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::DiscreteInfoType di;
  rtap_db::AnalogInfoType   ai;
  objclass_t objclass;

  do
  {
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    switch(objclass)
    {
        case TM:
        case TR:
        case ICM:
            rc = instance.ai_read(ai);
            if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

            rc = ai.VALACQ_get(attr_info->value.fixed.val_double);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }

            // NB: не использовать таблицу свойств атрибутов AttrTypeDescription, поскольку
            // VAL - это особый случай, он может быть целочисленным или с плав. точкой
            attr_info->type = DB_TYPE_DOUBLE;
            break;

        case TS:
        case TSA:
        case TSC:
        case AL:
        case ICS:
            rc = instance.di_read(di);
            if (rc) { LOG(ERROR) << "Can't read discrete part of " << attr_info->name; break; }

            rc = di.VALACQ_get(attr_info->value.fixed.val_uint64);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }

            // NB: не использовать таблицу свойств атрибутов AttrTypeDescription, поскольку
            // VAL - это особый случай, он может быть целочисленным или с плав. точкой
            attr_info->type = DB_TYPE_UINT64;
            break;

        default:
            LOG(ERROR) << "'" << attr_name
                       << "' for objclass " << objclass
                       << " is not supported, point " << attr_info->name;
            attr_info->type = DB_TYPE_UNDEF;
            break;
    }
    // Если внутри switch(objclass) была ошибка, дальнейшая обработка сразу прекращается
    if (rc) break;

  } while(false);

  return rc;
}

MCO_RET DatabaseRtapImpl::writeVALACQ(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_VALACQ;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::DiscreteInfoType di;
  rtap_db::AnalogInfoType   ai;
  objclass_t objclass;

  do
  {
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    switch(objclass)
    {
        case TM:
        case TR:
        case ICM:
            rc = instance.ai_read(ai);
            if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

            rc = ai.VALACQ_put(attr_info->value.fixed.val_double);
            if (rc) { LOG(ERROR) << "Can't write " << attr_info->name; break; }
            break;

        case TS:
        case TSA:
        case TSC:
        case AL:
        case ICS:
            rc = instance.di_read(di);
            if (rc) { LOG(ERROR) << "Can't read discrete part of " << attr_info->name; break; }

            rc = di.VALACQ_put(attr_info->value.fixed.val_uint64);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }
            break;

        default:
            LOG(ERROR) << "'" << attr_name
                       << "' for objclass " << objclass
                       << " is not supported, point " << attr_info->name;
            break;
    }
    // Если внутри switch(objclass) была ошибка, дальнейшая обработка сразу прекращается
    if (rc) break;

  } while(false);

  return rc;
}

// ======================== VALMANUAL ============================
MCO_RET DatabaseRtapImpl::createVALMANUAL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_VALMANUAL;
  MCO_RET rc = MCO_S_NOTFOUND;
  double double_value;
  uint8  integer64_value;

  switch(instance->objclass())
  {
    case TM:
        double_value = atof(attr.value().c_str());
        rc = instance->AIT().VALMANUAL_put(double_value);
        break;

    case TS:
    case TSA:
    case TSC:
        integer64_value = atoll(attr.value().c_str());
        rc = instance->DIT().VALMANUAL_put(integer64_value);
        break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::readVALMANUAL(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_VALMANUAL;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::DiscreteInfoType di;
  rtap_db::AnalogInfoType   ai;
  objclass_t objclass;

  do
  {
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    switch(objclass)
    {
        case TM:
        case TR:
        case ICM:
            rc = instance.ai_read(ai);
            if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

            rc = ai.VALMANUAL_get(attr_info->value.fixed.val_double);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }

            // NB: не использовать таблицу свойств атрибутов AttrTypeDescription, поскольку
            // VAL - это особый случай, он может быть целочисленным или с плав. точкой
            attr_info->type = DB_TYPE_DOUBLE;
            break;

        case TS:
        case TSA:
        case TSC:
        case AL:
        case ICS:
            rc = instance.di_read(di);
            if (rc) { LOG(ERROR) << "Can't read discrete part of " << attr_info->name; break; }

            rc = di.VALMANUAL_get(attr_info->value.fixed.val_uint64);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }

            // NB: не использовать таблицу свойств атрибутов AttrTypeDescription, поскольку
            // VAL - это особый случай, он может быть целочисленным или с плав. точкой
            attr_info->type = DB_TYPE_UINT64;
            break;

        default:
            LOG(ERROR) << "'" << attr_name
                       << "' for objclass " << objclass
                       << " is not supported, point " << attr_info->name;
            attr_info->type = DB_TYPE_UNDEF;
            break;
    }
    // Если внутри switch(objclass) была ошибка, дальнейшая обработка сразу прекращается
    if (rc) break;

  } while(false);

  return rc;
}

MCO_RET DatabaseRtapImpl::writeVALMANUAL(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_VALMANUAL;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::DiscreteInfoType di;
  rtap_db::AnalogInfoType   ai;
  objclass_t objclass;

  do
  {
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    switch(objclass)
    {
        case TM:
        case TR:
        case ICM:
            rc = instance.ai_read(ai);
            if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

            rc = ai.VALMANUAL_put(attr_info->value.fixed.val_double);
            if (rc) { LOG(ERROR) << "Can't write " << attr_info->name; break; }
            break;

        case TS:
        case TSA:
        case TSC:
        case AL:
        case ICS:
            rc = instance.di_read(di);
            if (rc) { LOG(ERROR) << "Can't read discrete part of " << attr_info->name; break; }

            rc = di.VALMANUAL_put(attr_info->value.fixed.val_uint64);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }
            break;

        default:
            LOG(ERROR) << "'" << attr_name
                       << "' for objclass " << objclass
                       << " is not supported, point " << attr_info->name;
            attr_info->type = DB_TYPE_UNDEF;
            break;
    }
    // Если внутри switch(objclass) была ошибка, дальнейшая обработка сразу прекращается
    if (rc) break;

  } while(false);

  return rc;
}

// ======================== VALEX ============================
MCO_RET DatabaseRtapImpl::createVALEX(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_VALEX;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case TR: rc = instance->passport().tr->VALEX_put(value); break;
    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createALINHIB(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_ALINHIB;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 value = atoi(attr.value().c_str());
  Boolean result;

  switch (value)
  {
    case 0: result = FALSE; break;
    case 1:
    default: result = TRUE;
  }

  switch(instance->objclass())
  {
    case PIPE:    rc = instance->passport().pipe->ALINHIB_put(result);     break;
    case PIPELINE:rc = instance->passport().pipeline->ALINHIB_put(result); break;
    case TL:      rc = instance->passport().tl->ALINHIB_put(result);       break;
    case VA:      rc = instance->passport().valve->ALINHIB_put(result);    break;
    case SC:      rc = instance->passport().sc->ALINHIB_put(result);       break;
    case ATC:     rc = instance->passport().atc->ALINHIB_put(result);      break;
    case GRC:     rc = instance->passport().grc->ALINHIB_put(result);      break;
    case SV:      rc = instance->passport().sv->ALINHIB_put(result);       break;
    case SDG:     rc = instance->passport().sdg->ALINHIB_put(result);      break;
    case RGA:     rc = instance->passport().rga->ALINHIB_put(result);      break;
    case SSDG:    rc = instance->passport().ssdg->ALINHIB_put(result);     break;
    case BRG:     rc = instance->passport().brg->ALINHIB_put(result);      break;
    case SCP:     rc = instance->passport().scp->ALINHIB_put(result);      break;
    case STG:     rc = instance->passport().stg->ALINHIB_put(result);      break;
    case DIPL:    rc = instance->passport().dipl->ALINHIB_put(result);     break;
    case METLINE: rc = instance->passport().metline->ALINHIB_put(result);  break;
    case ESDG:    rc = instance->passport().esdg->ALINHIB_put(result);     break;
    case SVLINE:  rc = instance->passport().svline->ALINHIB_put(result);   break;
    case SCPLINE: rc = instance->passport().scpline->ALINHIB_put(result);  break;
    case TLLINE:  rc = instance->passport().tlline->ALINHIB_put(result);   break;
    case INVT:    rc = instance->passport().invt->ALINHIB_put(result);     break;
    case AUX1:    rc = instance->passport().aux1->ALINHIB_put(result);     break;
    case AUX2:    rc = instance->passport().aux2->ALINHIB_put(result);     break;
    case SA:      rc = instance->passport().sa->ALINHIB_put(result);       break;
    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createMNVALPHY(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_MNVALPHY;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case TM: rc = instance->passport().tm->MNVALPHY_put(value); break;
    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createMXVALPHY(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_MXVALPHY;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case TM: rc = instance->passport().tm->MXVALPHY_put(value); break;
    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createDISPP(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_DISPP;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case DIPL:
        rc = instance->passport().dipl->DISPP_put(attr.value().c_str(),
                                                  static_cast<uint2>(attr.value().size()));
        break;

    case SITE:
        rc = instance->passport().site->DISPP_put(attr.value().c_str(),
                                                  static_cast<uint2>(attr.value().size()));
        break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createINHIB(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_INHIB;
  MCO_RET rc = MCO_S_NOTFOUND;
  Boolean result;
  uint1   value = atoi(attr.value().c_str());

  switch (value)
  {
    case 0: result = FALSE; break;
    case 1:
    default: result = TRUE;
  }

  switch(instance->objclass())
  {
    case PIPE:    rc = instance->passport().pipe->INHIB_put(result);     break;
    case PIPELINE:rc = instance->passport().pipeline->INHIB_put(result); break;
    case TL:      rc = instance->passport().tl->INHIB_put(result);       break;
    case VA:      rc = instance->passport().valve->INHIB_put(result);    break;
    case SC:      rc = instance->passport().sc->INHIB_put(result);       break;
    case ATC:     rc = instance->passport().atc->INHIB_put(result);      break;
    case GRC:     rc = instance->passport().grc->INHIB_put(result);      break;
    case SV:      rc = instance->passport().sv->INHIB_put(result);       break;
    case SDG:     rc = instance->passport().sdg->INHIB_put(result);      break;
    case RGA:     rc = instance->passport().rga->INHIB_put(result);      break;
    case SSDG:    rc = instance->passport().ssdg->INHIB_put(result);     break;
    case BRG:     rc = instance->passport().brg->INHIB_put(result);      break;
    case SCP:     rc = instance->passport().scp->INHIB_put(result);      break;
    case STG:     rc = instance->passport().stg->INHIB_put(result);      break;
    case DIPL:    rc = instance->passport().dipl->INHIB_put(result);     break;
    case METLINE: rc = instance->passport().metline->INHIB_put(result);  break;
    case ESDG:    rc = instance->passport().esdg->INHIB_put(result);     break;
    case SVLINE:  rc = instance->passport().svline->INHIB_put(result);   break;
    case SCPLINE: rc = instance->passport().scpline->INHIB_put(result);  break;
    case TLLINE:  rc = instance->passport().tlline->INHIB_put(result);   break;
    case INVT:    rc = instance->passport().invt->INHIB_put(result);     break;
    case AUX1:    rc = instance->passport().aux1->INHIB_put(result);     break;
    case AUX2:    rc = instance->passport().aux2->INHIB_put(result);     break;
    case SA:      rc = instance->passport().sa->INHIB_put(result);       break;
    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createINHIBLOCAL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_INHIBLOCAL;
  MCO_RET rc = MCO_S_NOTFOUND;
  Boolean result;
  uint1   value = atoi(attr.value().c_str());

  switch (value)
  {
    case 0: result = FALSE; break;
    case 1:
    default: result = TRUE;
  }

  switch(instance->objclass())
  {
    case TS:      rc = instance->passport().ts->INHIBLOCAL_put(result);     break;
    case TM:      rc = instance->passport().tm->INHIBLOCAL_put(result);     break;
    case TSC:     rc = instance->passport().tsc->INHIBLOCAL_put(result);     break;
    case AL:      rc = instance->passport().al->INHIBLOCAL_put(result);     break;
    case PIPE:    rc = instance->passport().pipe->INHIBLOCAL_put(result);     break;
    case PIPELINE:rc = instance->passport().pipeline->INHIBLOCAL_put(result); break;
    case TL:      rc = instance->passport().tl->INHIBLOCAL_put(result);       break;
    case VA:      rc = instance->passport().valve->INHIBLOCAL_put(result);    break;
    case SC:      rc = instance->passport().sc->INHIBLOCAL_put(result);       break;
    case ATC:     rc = instance->passport().atc->INHIBLOCAL_put(result);      break;
    case GRC:     rc = instance->passport().grc->INHIBLOCAL_put(result);      break;
    case SV:      rc = instance->passport().sv->INHIBLOCAL_put(result);       break;
    case SDG:     rc = instance->passport().sdg->INHIBLOCAL_put(result);      break;
    case RGA:     rc = instance->passport().rga->INHIBLOCAL_put(result);      break;
    case SSDG:    rc = instance->passport().ssdg->INHIBLOCAL_put(result);     break;
    case BRG:     rc = instance->passport().brg->INHIBLOCAL_put(result);      break;
    case SCP:     rc = instance->passport().scp->INHIBLOCAL_put(result);      break;
    case STG:     rc = instance->passport().stg->INHIBLOCAL_put(result);      break;
    case DIPL:    rc = instance->passport().dipl->INHIBLOCAL_put(result);     break;
    case METLINE: rc = instance->passport().metline->INHIBLOCAL_put(result);  break;
    case ESDG:    rc = instance->passport().esdg->INHIBLOCAL_put(result);     break;
    case SVLINE:  rc = instance->passport().svline->INHIBLOCAL_put(result);   break;
    case SCPLINE: rc = instance->passport().scpline->INHIBLOCAL_put(result);  break;
    case TLLINE:  rc = instance->passport().tlline->INHIBLOCAL_put(result);   break;
    case INVT:    rc = instance->passport().invt->INHIBLOCAL_put(result);     break;
    case AUX1:    rc = instance->passport().aux1->INHIBLOCAL_put(result);     break;
    case AUX2:    rc = instance->passport().aux2->INHIBLOCAL_put(result);     break;
    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createL_TL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_TL;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case PIPE:
    rc = instance->passport().pipe->L_TL_put(attr.value().c_str(),
                                             static_cast<uint2>(attr.value().size()));
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createL_NETTYPE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_NETTYPE;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case PIPE:
    rc = instance->passport().pipe->L_NETTYPE_put(attr.value().c_str(),
                                                  static_cast<uint2>(attr.value().size()));
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createL_NET(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_NET;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case PIPE:
    rc = instance->passport().pipe->L_NET_put(attr.value().c_str(),
                                              static_cast<uint2>(attr.value().size()));
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createL_EQTORBORUPS(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_EQTORBORUPS;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case PIPE:
    rc = instance->passport().pipe->L_EQTORBORUPS_put(attr.value().c_str(),
                                                      static_cast<uint2>(attr.value().size()));
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createL_EQTORBORDWN(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_EQTORBORDWN;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case PIPE:
    rc = instance->passport().pipe->L_EQTORBORDWN_put(attr.value().c_str(),
                                                      static_cast<uint2>(attr.value().size()));
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createNMPRESSURE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_NMPRESSURE;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case PIPE:
    rc = instance->passport().pipe->NMPRESSURE_put(value);
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createKMREFUPS(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_KMREFUPS;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case PIPE:
    rc = instance->passport().pipe->KMREFUPS_put(value);
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createKMREFDWN(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_KMREFDWN;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case PIPE:
    rc = instance->passport().pipe->KMREFDWN_put(value);
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createSUPPLIER(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_SUPPLIER;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case AUX1:
    case AUX2:
    rc = instance->passport().aux2->SUPPLIER_put(attr.value().c_str(),
            static_cast<uint2>(attr.value().size()));
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}


MCO_RET DatabaseRtapImpl::createSUPPLIERMODE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_SUPPLIERMODE;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case SA:
    rc = instance->passport().sa->SUPPLIERMODE_put(attr.value().c_str(),
            static_cast<uint2>(attr.value().size()));
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createSUPPLIERSTATE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_SUPPLIERSTATE;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case SA:
    rc = instance->passport().sa->SUPPLIERSTATE_put(attr.value().c_str(),
            static_cast<uint2>(attr.value().size()));
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createL_CONSUMER(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_CONSUMER;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case SSDG:
    rc = instance->passport().ssdg->L_CONSUMER_put(attr.value().c_str(),
            static_cast<uint2>(attr.value().size()));
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

/* ================================================================================
 * Тип атрибута: string
 * ================================================================================*/
MCO_RET DatabaseRtapImpl::createL_CORRIDOR(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_CORRIDOR;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case PIPELINE:
    rc = instance->passport().pipeline->L_CORRIDOR_put(attr.value().c_str(),
            static_cast<uint2>(attr.value().size()));
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createPLANFLOW(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_PLANFLOW;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case SSDG:
    rc = instance->passport().ssdg->PLANFLOW_put(value);
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createPLANPRESSURE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_PLANPRESSURE;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case SSDG:
    rc = instance->passport().ssdg->PLANPRESSURE_put(value);
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}


// ======================== FUNCTION ============================
MCO_RET DatabaseRtapImpl::createFUNCTION(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_FUNCTION;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case PIPELINE:
    rc = instance->passport().pipeline->FUNCTION_put(attr.value().c_str(),
            static_cast<uint2>(attr.value().size()));
    break;

    case VA:
    rc = instance->passport().valve->FUNCTION_put(attr.value().c_str(),
            static_cast<uint2>(attr.value().size()));
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::readFUNCTION(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_FUNCTION;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::PIPELINE_passport pipeline_pass;
  rtap_db::VA_passport va_pass;
  objclass_t objclass;
  autoid_t aid;

  check_user_defined_type(RTDB_ATT_IDX_FUNCTION, attr_info);

  // Выделить память, если этого не было сделано ранее
  if (!attr_info->value.dynamic.varchar)
    attr_info->value.dynamic.varchar = new char[attr_info->value.dynamic.size + 1];

  do
  {
    attr_info->value.dynamic.size = var_size[attr_info->type];

    rc = instance.passport_ref_get(aid);
    if (rc) { LOG(ERROR) << "Can't get passport id for " << attr_info->name; break; }

    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    switch(objclass)
    {
      case PIPELINE:
        rc = rtap_db::PIPELINE_passport::autoid::find(t, aid, pipeline_pass);
        if (rc) { LOG(ERROR) << "Can't get passport for " << attr_info->name << ", rc=" << rc; break; }

        rc = pipeline_pass.FUNCTION_get(attr_info->value.dynamic.varchar, attr_info->value.dynamic.size);
      break;

      case VA:
        rc = rtap_db::VA_passport::autoid::find(t, aid, va_pass);
        if (rc) { LOG(ERROR) << "Can't get passport for " << attr_info->name << ", rc=" << rc; break; }

        rc = va_pass.FUNCTION_get(attr_info->value.dynamic.varchar, attr_info->value.dynamic.size);

      LOG(INFO) << attr_info->name
                 << " objclass:" << objclass
                 << " pass_id:" << aid
                 << " size:" << attr_info->value.dynamic.size
//                 << " data:" << attr_info->value.dynamic.varchar
                 << " type:" << attr_info->type
                 << ", rc=" << rc;
      break;

      default:
        LOG(ERROR) << "'" << attr_name << "' for objclass " << objclass << " is not supported";
    }

    // Ошибки чтения паспорта
    if (rc)
    {
      LOG(ERROR) << attr_info->name
                 << " objclass:" << objclass
                 << " pass_id:" << aid
                 << " size:" << attr_info->value.dynamic.size
                 << " data:" << attr_info->value.dynamic.varchar
                 << " type:" << attr_info->type
                 << ", rc=" << rc;
      break;
    }

    attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_FUNCTION].type;
#if defined VERBOSE
    LOG(INFO) << attr_info->name << " = "<< attr_info->value.dynamic.varchar; //1
#endif

  } while (false);

  return rc;
}


MCO_RET DatabaseRtapImpl::writeFUNCTION(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_FUNCTION;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::PIPELINE_passport pipeline_pass;
  rtap_db::VA_passport va_pass;
  objclass_t objclass;
  autoid_t aid;
  uint2 size;

  do
  {
    attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_FUNCTION].type;

    // Проверим, не выходит ли данный размер поля за допустимые границы
    if (attr_info->value.dynamic.size > var_size[attr_info->type])
      size = var_size[attr_info->type];
    else
      size = attr_info->value.dynamic.size;

    rc = instance.autoid_get(aid);
    if (rc) { LOG(ERROR) << "Can't get autoid for " << attr_info->name; break; }

    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    switch(objclass)
    {
      case PIPELINE:
        rc = rtap_db::PIPELINE_passport::autoid::find(t, aid, pipeline_pass);
        if (rc) { LOG(ERROR) << "Can't get passport for " << attr_info->name; break; }

        rc = pipeline_pass.FUNCTION_put(attr_info->value.dynamic.varchar, size);
      break;

      case VA:
        rc = rtap_db::VA_passport::autoid::find(t, aid, va_pass);
        if (rc) { LOG(ERROR) << "Can't get passport for " << attr_info->name; break; }

        rc = va_pass.FUNCTION_put(attr_info->value.dynamic.varchar, size);
      break;

      default:
        LOG(ERROR) << "'" << attr_name << "' for objclass " << objclass << " is not supported";
    }

    // Ошибки чтения паспорта
    if (rc)
      break;

  } while (false);

  return rc;
}

// ======================== CONVERTCOEFF ============================
MCO_RET DatabaseRtapImpl::createCONVERTCOEFF(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_CONVERTCOEFF;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case TM: rc = instance->passport().tm->CONVERTCOEFF_put(value); break;

    default:
      LOG(ERROR) << "'" << attr_name
                 << "' for objclass " << instance->objclass()
                 << " is not supported";
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::createGRADLEVEL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_GRADLEVEL;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case TM:  rc = instance->passport().tm->GRADLEVEL_put(value);  break;
    case ICM: rc = instance->passport().icm->GRADLEVEL_put(value); break;

    default:
      LOG(ERROR) << "'" << attr_name
                 << "' for objclass " << instance->objclass()
                 << " is not supported";
  }

  return rc;
}


MCO_RET DatabaseRtapImpl::createMXPRESSURE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_MXPRESSURE;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case PIPELINE: rc = instance->passport().pipeline->MXPRESSURE_put(value); break;
    case SSDG:     rc = instance->passport().ssdg->MXPRESSURE_put(value);     break;
    case ESDG:     rc = instance->passport().esdg->MXPRESSURE_put(value);     break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createNMFLOW(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_NMFLOW;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case PIPELINE: rc = instance->passport().pipeline->NMFLOW_put(value); break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createMXFLOW(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_MXFLOW;
  MCO_RET rc = MCO_S_NOTFOUND;
  double value = atof(attr.value().c_str());

  switch(instance->objclass())
  {
    case SDG:      rc = instance->passport().sdg->MXFLOW_put(value);      break;
    //case PIPELINE: rc = instance->passport().pipeline->MXFLOW_put(value); break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}


MCO_RET DatabaseRtapImpl::createSYNTHSTATE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_SYNTHSTATE;
  MCO_RET rc = MCO_S_NOTFOUND;
  SystemState state;
  uint1 value;

  switch(instance->objclass())
  {
    case SA:
      value = atoi(attr.value().c_str());
      switch (value)
      {
        case 0: state = UNREACH;  break;
        case 1: state = OPER;     break;
        case 2: state = PRE_OPER; break;
        default:
            LOG(WARNING) << "Unknown '" << attr_name << "' value (" << value << ")";
            state = UNREACH;
      }
      rc = instance->passport().sa->SYNTHSTATE_put(state);
      break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createDELEGABLE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_DELEGABLE;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 value = atoi(attr.value().c_str());
  Boolean result;

  switch (value)
  {
    case 0: result = FALSE; break;
    case 1:
    default: result = TRUE;
  }

  switch(instance->objclass())
  {
    case SA: rc = instance->passport().sa->DELEGABLE_put(result); break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createFLGREMOTECMD(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_FLGREMOTECMD;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 value = atoi(attr.value().c_str());
  Boolean result;

  switch (value)
  {
    case 0: result = FALSE; break;
    case 1:
    default: result = TRUE;
  }

  switch(instance->objclass())
  {
    case TR:   rc = instance->passport().tr->FLGREMOTECMD_put(result);   break;
    case VA:   rc = instance->passport().valve->FLGREMOTECMD_put(result);break;
    case SC:   rc = instance->passport().sc->FLGREMOTECMD_put(result);   break;
    case ATC:  rc = instance->passport().atc->FLGREMOTECMD_put(result);  break;
    case GRC:  rc = instance->passport().grc->FLGREMOTECMD_put(result);  break;
    case AUX1: rc = instance->passport().aux1->FLGREMOTECMD_put(result); break;
    case AUX2: rc = instance->passport().aux2->FLGREMOTECMD_put(result); break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createFLGMAINTENANCE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_FLGMAINTENANCE;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 value = atoi(attr.value().c_str());
  Boolean result;

  switch (value)
  {
    case 0: result = FALSE; break;
    case 1:
    default: result = TRUE;
  }

  switch(instance->objclass())
  {
    case VA: rc = instance->passport().valve->FLGMAINTENANCE_put(result); break;
    default:
            LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createNAMEMAINTENANCE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_NAMEMAINTENANCE;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 sz = attr.value().size();

  switch(instance->objclass())
  {
    case VA:  rc = instance->passport().valve->NAMEMAINTENANCE_put(attr.value().c_str(), sz);  break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

// TODO: Убрать дублирующий атрибут в общей части Точки для избранных OBJCLASS ('alarm')
MCO_RET DatabaseRtapImpl::createTSSYNTHETICAL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_TSSYNTHETICAL;
  MCO_RET rc = MCO_S_NOTFOUND;
  AlarmState state = NO_ALARM;
  uint1 value = atoi(attr.value().c_str());

  switch (value)
  {
    case 0: state = NO_ALARM;     break;
    case 1: state = ALARM_NOTACK; break;
    case 2: state = ALARM_ACK;    break;

    default:
      LOG(WARNING) << "Unknown '" << attr_name << "' value (" << value << ")";
      state = NO_ALARM;
  }

  switch(instance->objclass())
  {
    case VA:   rc = instance->passport().valve->TSSYNTHETICAL_put(state);break;
    case ATC:  rc = instance->passport().atc->TSSYNTHETICAL_put(state);  break;
    case GRC:  rc = instance->passport().grc->TSSYNTHETICAL_put(state);  break;
    case SDG:  rc = instance->passport().sdg->TSSYNTHETICAL_put(state);  break;
    case RGA:  rc = instance->passport().rga->TSSYNTHETICAL_put(state);  break;
    case SCP:  rc = instance->passport().scp->TSSYNTHETICAL_put(state);  break;
    case INVT: rc = instance->passport().invt->TSSYNTHETICAL_put(state); break;
    case AUX1: rc = instance->passport().aux1->TSSYNTHETICAL_put(state); break;
    case AUX2: rc = instance->passport().aux2->TSSYNTHETICAL_put(state); break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createALARMBEGIN(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_ALARMBEGIN;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint4 value = atoi(attr.value().c_str());

  switch(instance->objclass())
  {
    case TS:
    case TM:
    case TSC:
    case AL:
    case ICS:
    case ICM:
        rc = instance->alarm().ALARMBEGIN_put(value);
        break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createALARMBEGINACK(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_ALARMBEGINACK;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint4 value = atoi(attr.value().c_str());

  switch(instance->objclass())
  {
    case TS:
    case TM:
    case TSC:
    case AL:
    case ICS:
    case ICM:
        rc = instance->alarm().ALARMBEGINACK_put(value);
        break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createALARMENDACK(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_ALARMENDACK;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint4 value = atoi(attr.value().c_str());

  switch(instance->objclass())
  {
    case TS:
    case TM:
    case TSC:
    case AL:
    case ICS:
    case ICM:
        rc = instance->alarm().ALARMENDACK_put(value);
        break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createALARMSYNTH(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_ALARMSYNTH;
  AlarmState state = NO_ALARM;
  MCO_RET rc = MCO_S_NOTFOUND;
  int value = atoi(attr.value().c_str());

  switch (value)
  {
    case 0: state = NO_ALARM;     break;
    case 1: state = ALARM_NOTACK; break;
    case 2: state = ALARM_ACK;    break;

    default:
      LOG(WARNING) << "Unknown '" << attr_name << "' value ("
                   << value << "), set default to NO_ALARM";
      state = NO_ALARM;
  }

  switch(instance->objclass())
  {
    case TS:
    case TM:
    case TSC:
    case AL:
    case ICS:
    case ICM:
        rc = instance->alarm().ALARMSYNTH_put(state);
        break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createL_TYPINFO(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_TYPINFO;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 sz = attr.value().size();

  switch(instance->objclass())
  {
    case TS:  rc = instance->passport().ts->L_TYPINFO_put(attr.value().c_str(), sz);  break;
    case TM:  rc = instance->passport().tm->L_TYPINFO_put(attr.value().c_str(), sz);  break;
    case TR:  rc = instance->passport().tr->L_TYPINFO_put(attr.value().c_str(), sz);  break;
    case TSA: rc = instance->passport().tsa->L_TYPINFO_put(attr.value().c_str(), sz); break;
    case TSC: rc = instance->passport().tsc->L_TYPINFO_put(attr.value().c_str(), sz); break;
    case TC:  rc = instance->passport().tc->L_TYPINFO_put(attr.value().c_str(), sz);  break;
    case AL:  rc = instance->passport().al->L_TYPINFO_put(attr.value().c_str(), sz);  break;
    case ICS: rc = instance->passport().ics->L_TYPINFO_put(attr.value().c_str(), sz); break;
    case ICM: rc = instance->passport().icm->L_TYPINFO_put(attr.value().c_str(), sz); break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createL_EQT(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_EQT;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 sz = attr.value().size();

  switch(instance->objclass())
  {
    case TS:      rc = instance->passport().ts->L_EQT_put(attr.value().c_str(), sz);      break;
    case TM:      rc = instance->passport().tm->L_EQT_put(attr.value().c_str(), sz);      break;
    case TR:      rc = instance->passport().tr->L_EQT_put(attr.value().c_str(), sz);      break;
    case TSA:     rc = instance->passport().tsa->L_EQT_put(attr.value().c_str(), sz);     break;
    case TSC:     rc = instance->passport().tsc->L_EQT_put(attr.value().c_str(), sz);     break;
    case TC:      rc = instance->passport().tc->L_EQT_put(attr.value().c_str(), sz);      break;
    case AL:      rc = instance->passport().al->L_EQT_put(attr.value().c_str(), sz);      break;
    case ICS:     rc = instance->passport().ics->L_EQT_put(attr.value().c_str(), sz);     break;
    case ICM:     rc = instance->passport().icm->L_EQT_put(attr.value().c_str(), sz);     break;
    case RGA:     rc = instance->passport().rga->L_EQT_put(attr.value().c_str(), sz);     break;
    case METLINE: rc = instance->passport().metline->L_EQT_put(attr.value().c_str(), sz); break;
    case SVLINE:  rc = instance->passport().svline->L_EQT_put(attr.value().c_str(), sz);  break;
    case SCPLINE: rc = instance->passport().scpline->L_EQT_put(attr.value().c_str(), sz); break;
    case TLLINE:  rc = instance->passport().tlline->L_EQT_put(attr.value().c_str(), sz);  break;
    case INVT:    rc = instance->passport().invt->L_EQT_put(attr.value().c_str(), sz);    break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createREMOTECONTROL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_REMOTECONTROL;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 value = atoi(attr.value().c_str());
  Responsability resp;

  switch (value)
  {
    case 0: resp = LOCAL;   break;
    case 1: resp = DISTANT; break;
    default:
        LOG(ERROR) <<"Unsupported value="<<value<<" for REMOTECONTROL, use LOCAL as default";
        resp = LOCAL;
  }

  switch(instance->objclass())
  {
    case TR:   rc = instance->passport().tr->REMOTECONTROL_put(resp);   break;
    case TC:   rc = instance->passport().tc->REMOTECONTROL_put(resp);   break;
    case VA:   rc = instance->passport().valve->REMOTECONTROL_put(resp);break;
    case SC:   rc = instance->passport().sc->REMOTECONTROL_put(resp);   break;
    case ATC:  rc = instance->passport().atc->REMOTECONTROL_put(resp);  break;
    case GRC:  rc = instance->passport().grc->REMOTECONTROL_put(resp);  break;
    case SV:   rc = instance->passport().sv->REMOTECONTROL_put(resp);   break;
    case SDG:  rc = instance->passport().sdg->REMOTECONTROL_put(resp);  break;
    case RGA:  rc = instance->passport().rga->REMOTECONTROL_put(resp);  break;
    case SSDG: rc = instance->passport().ssdg->REMOTECONTROL_put(resp); break;
    case BRG:  rc = instance->passport().brg->REMOTECONTROL_put(resp);  break;
    case SCP:  rc = instance->passport().scp->REMOTECONTROL_put(resp);  break;
    case STG:  rc = instance->passport().stg->REMOTECONTROL_put(resp);  break;
    case ESDG: rc = instance->passport().esdg->REMOTECONTROL_put(resp); break;
    case AUX1: rc = instance->passport().aux1->REMOTECONTROL_put(resp); break;
    case AUX2: rc = instance->passport().aux2->REMOTECONTROL_put(resp); break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createACTIONTYP(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_ACTIONTYP;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 value = atoi(attr.value().c_str());
  Action action;

  switch (value)
  {
    case 1: action = TC_ON;     break;
    case 2: action = TC_OFF;    break;
    // NB: Пока есть дубликаты с одинаковыми номерами различных команд разного оборудования
    // TODO: Предусмотреть изменение кодов команд по сравнению с принятыми в ГОФО
	case 3: action = TC_SC_START_MAIN_LINE; break; /* as TC_ATC_STOP_URG and TC_GRC_STOP_NORMAL */
	case 4: action = TC_SC_STOP_MAIN_LINE;  break;
	case 5: action = TC_ATC_STOP_NORMAL;    break;
	case 6: action = TC_ATC_START_LOOPING;  break;
	case 7: action = TC_ATC_START_MAIN_LINE;break;
	case 8: action = TC_ATC_STOP_MAIN_LINE; break;
	case 9: action = TC_ATC_SWITCH_COLD_REGUL1;     break;
	case 10: action = TC_ATC_SWITCH_COLD_REGUL2;    break;
	case 11: action = TC_GRC_STOP_URG_NO_EVAC_GAZ;  break; /* as TC_ATC_STOP_URG_NO_EVAC_GAZ */
	case 12: action = TC_ATC_STOP_URG_WITH_EVAC_GAZ;break; /* as TC_GRC_STOP_URG_WITH_EVAC_GAZ */
	case 13: action = TC_GRC_START_LOOPING;         break;
	case 14: action = TC_GRC_START_MAIN_LINE;       break;
	case 15: action = TC_AUX_FLOW_REGUL;            break;
	case 16: action = TC_AUX_PRESSURE_REGUL;        break;
	case 17: action = TC_AUX_START_PUMP;         break;
	case 18: action = TC_AUX_STOP_PUMP;          break;
	case 19: action = TC_AUX_START_COMPRESSOR;   break;
	case 20: action = TC_AUX_STOP_COMPRESSOR;    break;
	case 21: action = TC_AUX_STOP_URG_AIRCOOLING;break;
	case 22: action = TC_AUX_START_VENTILATOR;   break;
	case 23: action = TC_AUX_STOP_VENTILATOR;    break;

    case 0:
    default:
        LOG(ERROR) <<"Unsupported value="<<value<<" for ACTIONTYP, use NONE as default";
        action = TC_NONE;
  }

  switch(instance->objclass())
  {
    case TC: rc = instance->passport().tc->ACTIONTYP_put(action);   break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createVAL_LABEL  (PointInDatabase* /* instance */, rtap_db::Attrib& /* attr */)
{
  // NB: Пропустим этот атрибут
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createLINK_HIST(PointInDatabase* /* instance */, rtap_db::Attrib& /* attr */)
{
  // NB: Пропустим этот атрибут
  return MCO_S_OK;
}

MCO_RET DatabaseRtapImpl::createL_DIPL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_DIPL;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case TS: rc = instance->passport().ts->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case TM: rc = instance->passport().tm->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case TSA: rc = instance->passport().tsa->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case TSC: rc = instance->passport().tsc->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case AL: rc = instance->passport().al->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case ICS: rc = instance->passport().ics->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case ICM: rc = instance->passport().icm->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case SSDG: rc = instance->passport().ssdg->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createALDEST(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_ALDEST;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 value = atoi(attr.value().c_str());
  Boolean result;

  switch (value)
  {
    case 0: result = FALSE; break;
    case 1:
    default: result = TRUE;
  }

  switch(instance->objclass())
  {
    case TS: rc = instance->passport().ts->ALDEST_put(result);  break;
    case TM: rc = instance->passport().tm->ALDEST_put(result);  break;
    case TSC:rc = instance->passport().tsc->ALDEST_put(result); break;
    case AL: rc = instance->passport().al->ALDEST_put(result);  break;
    case ICM:rc = instance->passport().icm->ALDEST_put(result); break;
    case SA: rc = instance->passport().sa->ALDEST_put(result);  break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createL_PIPE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_PIPE;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case SC:
    rc = instance->passport().sc->L_PIPE_put(attr.value().c_str(), attr.value().size());
    break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createL_PIPELINE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_PIPELINE;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case VA:
    rc = instance->passport().valve->L_PIPELINE_put(attr.value().c_str(), attr.value().size());
    break;
    case TLLINE:
    rc = instance->passport().tlline->L_PIPELINE_put(attr.value().c_str(), attr.value().size());
    break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::createCONFREMOTECMD(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_CONFREMOTECMD;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint1 value = atoi(attr.value().c_str());
  Boolean result;

  switch (value)
  {
    case 0: result = FALSE; break;
    case 1:
    default: result = TRUE;
  }


  switch(instance->objclass())
  {
    case TR:   rc = instance->passport().tr->CONFREMOTECMD_put(result);   break;
    case TL:   rc = instance->passport().tl->CONFREMOTECMD_put(result);   break;
    case VA:   rc = instance->passport().valve->CONFREMOTECMD_put(result);break;
    case SC:   rc = instance->passport().sc->CONFREMOTECMD_put(result);   break;
    case ATC:  rc = instance->passport().atc->CONFREMOTECMD_put(result);  break;
    case GRC:  rc = instance->passport().grc->CONFREMOTECMD_put(result);  break;
    case SV:   rc = instance->passport().sv->CONFREMOTECMD_put(result);   break;
    case SDG:  rc = instance->passport().sdg->CONFREMOTECMD_put(result);  break;
    case RGA:  rc = instance->passport().rga->CONFREMOTECMD_put(result);  break;
    case SSDG: rc = instance->passport().ssdg->CONFREMOTECMD_put(result); break;
    case BRG:  rc = instance->passport().brg->CONFREMOTECMD_put(result);  break;
    case SCP:  rc = instance->passport().scp->CONFREMOTECMD_put(result);  break;
    case STG:  rc = instance->passport().stg->CONFREMOTECMD_put(result);  break;
    case DIPL: rc = instance->passport().dipl->CONFREMOTECMD_put(result); break;
    case ESDG: rc = instance->passport().esdg->CONFREMOTECMD_put(result); break;
    case AUX1: rc = instance->passport().aux1->CONFREMOTECMD_put(result); break;
    case AUX2: rc = instance->passport().aux2->CONFREMOTECMD_put(result); break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

// Связать имена атрибутов с создающими их в БДРВ функциями
// NB: Здесь д.б. прописаны только те атрибуты, которые не создаются в паспортах
bool DatabaseRtapImpl::AttrFuncMapInit()
{
  // Функции записи
  // ======================================================================================================
  // LABEL пока пропустим, возможно вынесем ее хранение во внешнюю БД (поле 'persistent' eXtremeDB?)
  //
  // Два атрибута, TAG и OBJCLASS, обрабатываются специальным образом, поэтому исключены из общей обработки.
  //
  //m_attr_creating_func_map.insert(AttrCreatingFuncPair_t("TAG",        &xdb::DatabaseRtapImpl::createTAG));
  //m_attr_creating_func_map.insert(AttrCreatingFuncPair_t("OBJCLASS",   &xdb::DatabaseRtapImpl::createOBJCLASS));
  //
#include "dat/impl_attr_creating_map.gen"
  // Функции чтения
#include "dat/impl_attr_reading_map.gen"
  m_attr_reading_func_map.insert(AttrProcessingFuncPair_t("OBJCLASS", &xdb::DatabaseRtapImpl::readOBJCLASS));
  // Функции записи
#include "dat/impl_attr_writing_map.gen"

#if 0
  for (AttrProcessingFuncMapIterator_t it = m_attr_creating_func_map.begin();
       it != m_attr_creating_func_map.end();
       it++)
  {
    LOG(INFO) << "GEV: attr='" << it->first << "' func=" << it->second;
  }
#endif
  return true;
}

#include "dat/impl_func_read.gen"

#include "dat/impl_func_write.gen"

