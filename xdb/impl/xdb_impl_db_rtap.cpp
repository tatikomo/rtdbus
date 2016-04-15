/* 
 * ITG
 * author: Eugeniy Gorovoy
 * email: eugeni.gorovoi@gmail.com
 *
 */
#include <new>
#include <map>
//#include <unordered_set>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h> // exit
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>  // kill
#include <signal.h>     // kill

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
#include "xdb_impl_common.h"
#include "xdb_impl_error.hpp"

#include "helper.hpp"
#include "dat/rtap_db.h"
#include "dat/rtap_db.hpp"
#include "dat/rtap_db.hxx"
#include "xdb_impl_database.hpp"
#include "xdb_impl_db_rtap.hpp"
#include "xdb_impl_sbs_rtap.hpp"

using namespace xdb;

/* 
 * Включение динамически генерируемых определений 
 * структуры данных для внутренней базы RTAP.
 *
 * Регенерация осуществляется командой mcocomp 
 * на основе содержимого файла rtap_db.mco
 */
#include "dat/rtap_db.hpp"

// Конверсия между целым числом и значением HistoryType из rtap_db.mco
typedef struct {
  int idx;
  HistoryType htype;
} int_to_his;
static const int_to_his decoder[6] = {
    { 0, PER_NONE },
    { 1, PER_1_MINUTE },
    { 2, PER_5_MINUTES },
    { 3, PER_HOUR },
    { 4, PER_DAY },
    { 5, PER_MONTH }
};

/* helper function to print an XML schema */
# if (EXTREMEDB_VERSION <= 40)
mco_size_t file_writer(void* stream_handle, const void* from, mco_size_t nbytes)
# else
mco_size_sig_t file_writer(void* stream_handle, const void* from, mco_size_t nbytes)
# endif
{
  return (mco_size_t) fwrite(from, 1, nbytes, (FILE*) stream_handle);
}

# if (EXTREMEDB_VERSION > 40)
// --------------------------------------------------------------------------------
// Проверка доступности указанного идентификатора
int os_task_id_check( pid_t tid )
{
  return kill(tid, 0) == 0 ? 0 : 1;
}

// Процедура внутренней проверки имеющихся подключений
MCO_RET sniffer_callback(mco_db_h db, void* context, mco_trans_counter_t trans_no)
{
    pid_t pid = *(pid_t*)context;

    if ( os_task_id_check( pid ) == 0 ) {
      return MCO_S_OK;
    }

    LOG(WARNING) << "Control database: process " << pid << " is crashed";
    return MCO_S_DEAD_CONNECTION;
}
#else
#warning "Connection control is disabled in this XDB version"
#endif

// Класс для получения атрибутов в зависимости от OBJCLASS Точки для формирования
// ответа на событие группы подписки.
static AttributesHolder attribute_holder;

typedef MCO_RET (*schema_f) (mco_trans_h, void*, mco_stream_write);
// ===============================================================================
// Взято из 'xdb/impl/dat/rtap_db.h'
// Результат выполнения "grep _xml_schema rtap_db.h|wc -l" равен 48 (15.01.2016)
const int ALL_TYPES_COUNT = 50;
schema_f ALL_TYPES_LIST[] = {
  XDBPoint_xml_schema,
  DICT_TSC_VAL_LABEL_xml_schema,
  DICT_OBJCLASS_CODE_xml_schema,
  DICT_SIMPLE_TYPE_xml_schema,
  DICT_UNITY_ID_xml_schema,
  SBS_GROUPS_STAT_xml_schema,
  SBS_GROUPS_ITEM_xml_schema,
  XDB_CE_xml_schema,
  ALARM_xml_schema,
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
PointInDatabase::PointInDatabase(rtap_db::Point* info) :
    m_point_aid(0),
    m_passport_aid(0),
    m_CE_aid(0),
    m_SA_aid(0),
    m_rc(MCO_S_OK),
    m_point(),
    m_info(info),
    m_transaction_handler(0),
    m_objclass(static_cast<objclass_t>(m_info->objclass())),
    m_ai(),
    m_di(),
    m_alarm(),
    m_passport(),
    m_is_AI_assigned(false),
    m_is_DI_assigned(false),
    m_is_AL_assigned(false)
{
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
      assert(true == false);
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
      m_passport_aid = m_CE_aid = m_SA_aid = m_point_aid;

      // Успешно создана общая часть паспорта Точки.
      // Приступаем к созданию дополнительных полей,
      // Analog или Discrete, и AlarmInfo, в зависимости от типа класса.
      switch (objclass())
      {
        // Группа точек, имеющих и тревоги, и дискретные атрибуты значения 
        // ===============================================================
        case TS:  /* 0 */
        case AL:  /* 6 */
        case ICS: /* 7 */
        //1 case TSC: /* 4 */
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
        case DIR: /* 30 */
        case DIPL:/* 31 */
        case METLINE:/* 32 */
        case ESDG:   /* 33 */
        case SVLINE: /* 34 */
        case SCPLINE:/* 35 */
        case TLLINE: /* 36 */
        case INVT:/* 37 */
        case AUX1:/* 38 */
        case AUX2:/* 39 */
            m_rc = assign(m_alarm);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign alarm information, rc="<<m_rc; break; }
            m_is_AL_assigned = true;
        // Группа точек, имеющих только дискретные атрибуты значения 
        // =========================================================
        case TSA: /* 3 */ // Вынесен ниже остальных, поскольку не имеет Alarm
            // Только создать дискретную часть Точки, значения заполняются автоматными функциями
            m_rc = assign(m_di);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign discrete information, rc="<<m_rc; break; }
            m_is_DI_assigned = true;
            break;

        // Группа точек, имеющих и тревоги, и аналоговые атрибуты значения
        // ===============================================================
        case TM:  /* 1 */
        case ICM: /* 8 */
            m_rc = assign(m_alarm);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign alarm information, rc="<<m_rc; break; }
            m_is_AL_assigned = true;
        // Группа точек, имеющих только аналоговые атрибуты значения
        // =========================================================
        case TR:  /* 2 */ // Вынесен ниже остальных, поскольку не имеет Alarm
            // Только создать аналоговую часть Точки, значения заполняются автоматными функциями
            m_rc = assign(m_ai);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign analog information, rc="<<m_rc; break; }
            m_is_AI_assigned = true;
            break;

        // Группа точек, не имеющих атрибутов значений VAL|VALACQ
        // ======================================================
#pragma note "Формализовать создание атрибутов VAL для части OBJCLASS в PointInDatabase::create"
        case TC:        /* 05 */
        case PIPE:      /* 11 */
        case PIPELINE:  /* 15 */
        case SA:        /* 50 */
        case SITE:      /* 45 */
        case FIXEDPOINT:/* 79 */
        case HIST:      /* 80 */
        // Хватит паспорта и общей части атрибутов Точки
            break;

        default:
            LOG(ERROR) << "Unsupported objclass " << objclass() << " for point " << tag();
      }
  } while (false);

  return m_rc;
}

// =================================================================================
// Загрузить атрибуты данной Точки
// Перечень читаемых атрибутов:
// TAG
// OBJCLASS
// STATUS
// VALID
// VALID_ACQ
// VAL
// тревоги
// паспортные данные
//
// В начальных данных есть тоько m_point_aid
MCO_RET PointInDatabase::load(mco_trans_h t)
{
  // Учтено увеличение размера строки при хранении русского в UTF-8
  char s_tag[sizeof(wchar_t)*TAG_NAME_MAXLEN + 1];
  uint2 tag_size = sizeof(wchar_t)*TAG_NAME_MAXLEN;
  Validity valid;

  m_rc = MCO_S_OK;
  m_point_aid = m_info->m_tag_id;

  do
  {
    // 1. По заданному идентификатору найти экземпляр XDBpoint
    m_rc = rtap_db::XDBPoint::autoid::find(t, m_point_aid, m_point);
    if (m_rc) { LOG(ERROR) << "Can't locating point with id=" << m_info->tag_id() << ", rc=" << m_rc; break; }

    m_rc = m_point.TAG_get(s_tag, tag_size);
    if (m_rc) { LOG(ERROR) << "Reading tag for point id=" << m_info->tag_id() << ", rc=" << m_rc; break; }
    m_info->m_tag.assign(s_tag);

    m_rc = m_point.OBJCLASS_get(m_objclass);
    if (m_rc) { LOG(ERROR) << "Reading objclass for point '" << m_info->tag() << "', rc=" << m_rc; break; }

//    m_rc = m_point.STATUS_get();
//    if (m_rc) { LOG(ERROR) << "Reading '"<< m_info->tag()<<"."<<RTDB_ATT_STATUS<<"', rc=" << m_rc; break; }

    m_rc = m_point.passport_ref_get(m_passport_aid);
    if (m_rc) { LOG(ERROR) << "Reading passport info '" << m_info->tag() << "', rc=" << m_rc; break; }

    m_rc = m_point.VALIDITY_get(valid);
    if (m_rc) { LOG(ERROR) << "Reading '"<< m_info->tag()<<"."<<RTDB_ATT_VALID<<"', rc=" << m_rc; break; }

//    m_rc = m_point.VALIDACQ_get();
//    if (m_rc) { LOG(ERROR) << "Reading '"<< m_info->tag()<<"."<<RTDB_ATT_VALIDACQ<<"', rc=" << m_rc; break; }

    m_rc = m_point.alarm_read(m_alarm);
    if (m_rc) { LOG(ERROR) << "Reading alarm info '"<< m_info->tag()<<", rc=" << m_rc; break; }

    switch (m_objclass)
    {
        // Группа точек, имеющих и тревоги, и дискретные атрибуты значения 
        // ===============================================================
        case TS:  /* 0 */
        case AL:  /* 6 */
        case ICS: /* 7 */
        //1 case TSC: /* 4 */
        case VA:  /* 19 */
        case ATC: /* 21 */
        case GRC: /* 22 */
        case SDG: /* 24 */
        case RGA: /* 25 */
        case SCP: /* 28 */
        case INVT:/* 37 */
        case AUX1:/* 38 */
        case AUX2:/* 39 */
            m_rc = m_point.alarm_read(m_alarm);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign alarm information, rc="<<m_rc; break; }
            m_is_AL_assigned = true;
        // Группа точек, имеющих только дискретные атрибуты значения 
        // =========================================================
        case TSA: /* 3 */ // Вынесен ниже остальных, поскольку не имеет Alarm
            // Только создать дискретную часть Точки, значения заполняются автоматными функциями
            m_rc = m_point.di_read(m_di);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign discrete information, rc="<<m_rc; break; }
            m_is_DI_assigned = true;
        break;

        // Группа точек, имеющих и тревоги, и аналоговые атрибуты значения
        // ===============================================================
        case TM:  /* 1 */
        case ICM: /* 8 */
            m_rc = m_point.alarm_read(m_alarm);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign alarm information, rc="<<m_rc; break; }
            m_is_AL_assigned = true;
        // Группа точек, имеющих только аналоговые атрибуты значения
        // =========================================================
        case TR:  /* 2 */ // Вынесен ниже остальных, поскольку не имеет Alarm
            // Только создать аналоговую часть Точки, значения заполняются автоматными функциями
            m_rc = m_point.ai_read(m_ai);
            if (m_rc) { LOG(ERROR) << m_info->tag() << " assign analog information, rc="<<m_rc; break; }
            m_is_AI_assigned = true;
        break;

        // Группа точек, не имеющих атрибутов значений VAL|VALACQ
        // ======================================================
        case TC:        /* 05 */
        case PIPE:      /* 11 */
        case PIPELINE:  /* 15 */
        case TL:        /* 16 */
        case SC:        /* 20 */
        case SV:        /* 23 */
        case SSDG:      /* 26 */
        case BRG:       /* 27 */
        case STG:       /* 29 */
        case DIR:       /* 30 */
        case DIPL:      /* 31 */
        case METLINE:   /* 32 */
        case ESDG:      /* 33 */
        case SVLINE:    /* 34 */
        case SCPLINE:   /* 35 */
        case TLLINE:    /* 36 */
        case SA:        /* 50 */
        case SITE:      /* 45 */
        case FIXEDPOINT:/* 79 */
        case HIST:      /* 80 */
        // Хватит паспорта и общей части атрибутов Точки
        break;

        default:
        LOG(ERROR) << "Unsupported objclass " << m_info->objclass() << " for point " << m_info->tag();
    }

  } while (false);

  return m_rc;
}

#if 0
// =================================================================================
any_passport_t& PointInDatabase::passport()
{
  LOG(ERROR) << m_info->tag() << ": NULL passport reference";
//  assert(m_passport.root_pointer);
  return m_passport;
}
#endif

// =================================================================================
rtap_db::AnalogInfoType& PointInDatabase::AIT()
{
  if (!m_is_AI_assigned)
    LOG(ERROR) << m_info->tag() << ": NULL AnalogInformation reference";
  assert(m_is_AI_assigned == true);
  return m_ai;
}

// =================================================================================
rtap_db::DiscreteInfoType& PointInDatabase::DIT()
{
  if (!m_is_DI_assigned)
    LOG(ERROR) << m_info->tag() << ": NULL DiscreteInformation reference";
  assert(m_is_DI_assigned == true);
  return m_di;
}

// =================================================================================
rtap_db::AlarmItem& PointInDatabase::alarm()
{
  if (!m_is_AL_assigned)
    LOG(ERROR) << m_info->tag() << ": NULL AlartInformation reference";
  assert(m_is_AL_assigned == true);
  return m_alarm;
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

  } while (false);

  return m_rc;
}

// =================================================================================
// Опции по умолчанию устанавливаются в через setOption() в RtApplication
DatabaseRtapImpl::DatabaseRtapImpl(const char* _name, const Options* _options) :
  m_impl(NULL),
  m_register_events(false),
  m_attr_creating_func_map(),
  m_attr_writing_func_map(),
  m_attr_reading_func_map()
{
  int val;

  LOG(INFO) << "constructor DatabaseRtapImpl";
  assert(_name);

  mco_dictionary_h rtap_dict = rtap_db_get_dictionary();

  m_impl = new DatabaseImpl(_name, _options, rtap_dict);

  // Инициализация карты функций создания атрибутов
  AttrFuncMapInit();

  if (getOption(const_cast<Options*>(_options),"OF_REGISTER_EVENT", val) && val)
  {
    LOG(INFO) << "Database events will be enabled";
    m_register_events = true;
  }
  else
  {
    LOG(INFO) << "Database events will be suppressed";
    m_register_events = false;
  }
}

// =================================================================================
DatabaseRtapImpl::~DatabaseRtapImpl()
{
  LOG(INFO) << "Start destructor DatabaseRtapImpl";
  delete m_impl;
  LOG(INFO) << "Finish destructor DatabaseRtapImpl";
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

  if (true == m_register_events)
    RegisterEvents();

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
// Статический метод, вызываемый из runtime БДРВ при обновлении атрибута VALIDCHANGE
// Атрибут VALIDCHANGE должен изменяться последним, чтобы обеспечить коррекцию
// других рассчетных атрибутов данной Точки.
//
// Для аналогов, дискретов и алармов используются свои, соответствующие реализации:
//  GOFVALTSC
//  GOFVALTI
//  GOFVALAL
//
// Изменения атрибутов VAL и VALID накапливаются с помощью службы HIST.
// Хранится история как аналоговых, так и для дискретных параметров.
//
// TODO: включить в список аргументов атрибуты .INHIBLOCAL и .INHIB,
//  переместив их из паспортов в XDBPoint.
// =================================================================================
MCO_RET DatabaseRtapImpl::on_update_VALIDCHANGE(mco_trans_h t,
        XDBPoint *obj,
        MCO_EVENT_TYPE et,
        void *p)
{
  static const char *fname = "ON_UPDATE_VALIDCHANGE";
  DatabaseRtapImpl *self = static_cast<DatabaseRtapImpl*> (p);
  MCO_RET rc = MCO_S_OK;
  char tag[TAG_NAME_MAXLEN*sizeof(wchar_t) + 1];
  uint2 tag_size = TAG_NAME_MAXLEN*sizeof(wchar_t);
  autoid_t aid;
  autoid_t passport_aid;
  SA_passport sa_passport_instance;
  XDBPoint sa_instance;
  objclass_t objclass;
  Validity current_validity;      // Current validity  : VALID                  
  Validity acquired_validity;     // Acquired validity : VALIDACQ
 // Validity last_acquired_validity;//
  timestamp last_update_time;     // Acquired date     : DATEHOURM
  AnalogInfoType ai;
  DiscreteInfoType di;
  double acquired_val_f,    // FLOAT TM Acquired value : VALACQ
         manual_val_f,      // FLOAT TM Forced value   : VALMANUAL
         current_val_f;     // FLOAT Current value     : VAL
  uint64_t acquired_val_i,  // INT TS Acquired value   : VALACQ
         manual_val_i,      // INT TS Forced value     : VALMANUAL
         current_val_i;     // INT Current value       : VAL
  ValidChange validchange_val; // Acquired validity    : VALIDCHANGE
//  DbType_t type;
  SystemState sa_state;     // Состояние СС            : SASTATE
  SystemState old_sa_state; // Предыдущее состояние СС : OLDSASTATE
  Boolean inhiblocal;       // Локальный запрет        : INHIBLOCAL
  Boolean inhib;            // Запрет                  : INHIB

  assert(self);
  assert(obj);

  LOG(INFO) << "VALIDCHANGE update start "<<obj<<" self=" << self;
  do
  {
    rc = XDBPoint_OBJCLASS_get(obj, &objclass);
    if (rc) { LOG(ERROR) << fname << ": read "<<RTDB_ATT_OBJCLASS<<", rc=" << rc; break; }

    rc = XDBPoint_VALIDCHANGE_get(obj, &validchange_val);
    if (rc) { LOG(ERROR) << fname << ": read "<<RTDB_ATT_VALIDCHANGE<<", rc=" << rc; break; }

    rc = XDBPoint_TAG_get(obj, tag, tag_size);
    if (rc) { LOG(ERROR) << fname << ": read "<<RTDB_ATT_TAG<<", rc=" << rc; break; }

    rc = XDBPoint_VALIDITY_get(obj, &current_validity);
    if (rc) { LOG(ERROR) << fname << ": read "<<RTDB_ATT_VALID<<", rc=" << rc; break; }

    rc = XDBPoint_VALIDITY_ACQ_get(obj, &acquired_validity);
    if (rc) { LOG(ERROR) << fname << ": read "<<RTDB_ATT_VALIDACQ<<", rc=" << rc; break; }

    rc = XDBPoint_DATEHOURM_read_handle(obj, &last_update_time);
    if (rc) { LOG(ERROR) << fname << ": read "<<RTDB_ATT_DATEHOURM<<", rc=" << rc; break; }

    rc = XDBPoint_INHIBLOCAL_get(obj, &inhiblocal);
    if (rc) { LOG(ERROR) << fname << ": read "<<RTDB_ATT_INHIBLOCAL<<", rc=" << rc; break; }

    rc = XDBPoint_INHIB_get(obj, &inhib);
    if (rc) { LOG(ERROR) << fname << ": read "<<RTDB_ATT_INHIB<<", rc=" << rc; break; }

    // Прочитать состояние системы сбора данной Точки
    rc = XDBPoint_SA_ref_get(obj, &aid); // aid - идентификатор Точки тип SA
    if (rc) { LOG(ERROR) << fname << ": read '"<<tag<<"' SA reference, rc="<<rc; break; }

    // Получили экземпляр Точки SA в sa_instance
    rc = XDBPoint_autoid_find(t, aid, &sa_instance);
    if (rc) { LOG(ERROR) << fname << ": locate '"<<tag<<"' SA instance, rc="<<rc; break; }

    // Прочитали иденификатор паспорта SA в passport_aid
    rc = XDBPoint_passport_ref_get(&sa_instance, &passport_aid);
    if (rc) { LOG(ERROR) << "Get '"<<tag<<"' SA reference, rc="<<rc; break; }

    // Прочитали экземпляр паспорта SA
    rc = SA_passport_autoid_find(t, passport_aid, &sa_passport_instance);
    // Проверить возможную ситуацию с отсутствием информации по СС
    if (rc)
    {
      // К данному параметру нет привязанной СС
      if (rc == MCO_S_NOTFOUND)
      {
        LOG(WARNING) << "Point '"<<tag<< "' has no linked SA";
        sa_state = old_sa_state = SS_WORK;
        rc = MCO_S_OK;
      }
      else // Привязка к СС есть, но её прочесть не удалось по каким-либо причинам
      {
        LOG(ERROR) << "Get '"<<tag<<"' SA passport with id="<<passport_aid<<", rc="<<rc;
        break;
      }
    }
    else
    {
      rc = SA_passport_sa_state_get(&sa_passport_instance, &sa_state);
      if (rc) { LOG(ERROR) << "Get '"<<tag<<"' SA state, rc="<<rc; break; }

      rc = SA_passport_old_sa_state_get(&sa_passport_instance, &old_sa_state);
      if (rc) { LOG(ERROR) << "Get '"<<tag<<"' SA old state, rc="<<rc; break; }
    }
 
    // TODO: набор применимых СЕ зависит в первую очередь от типа объекта управления, ГТП или ЛПУ
    //
    // В зависимости от типа Точки вызывается соответствующая фунция GOFVAL{TI|AL|TSC}
    switch(objclass)
    {
      case TM:
      case TR:
      case ICM:

        //type = ClassDescriptionTable[objclass].val_type;

        rc = XDBPoint_ai_read_handle(obj, &ai);
        if (rc) { LOG(ERROR) << "Read analog part of '"<<tag<<"', rc="<<rc; break; }

        rc = AnalogInfoType_VALACQ_get(&ai, &acquired_val_f);
        if (rc) { LOG(ERROR) << "Read '"<<tag<<"."<<RTDB_ATT_VALACQ<<"', rc=" << rc; break; }

        rc = AnalogInfoType_VALMANUAL_get(&ai, &manual_val_f);
        if (rc) { LOG(ERROR) << "Read '"<<tag<<"."<<RTDB_ATT_VALMANUAL<<"', rc=" << rc; break; }

        rc = AnalogInfoType_VAL_get(&ai, &current_val_f);
        if (rc) { LOG(ERROR) << "Read '"<<tag<<"."<<RTDB_ATT_VAL<<"', rc=" << rc; break; }

        rc = self->GOFVALTI(t,              // Текущая транзакция
                      obj,                  // Ссылка на экземпляр Точки
                      ai,                   // Аналоговая часть Точки
                      tag,                  // Тег Точки
                      objclass,             // Класс Точки
                      validchange_val,      // VALIDCHANGE
                      acquired_val_f,       // VALACQ
                      last_update_time,     // DATEHOURM
                      manual_val_f,         // VALMANUAL
                      acquired_validity,    // VALIDACQ
                      current_val_f,        // VAL
                      current_validity,     // VALID
                      sa_state,             // SASTATE
                      old_sa_state,         // OLDSASTATE
                      (inhiblocal == TRUE), // INHIBLOCAL
                      (inhib == TRUE)       // INHIB
                      );
        if (rc) { LOG(ERROR) << "Call GOFVALTI on '"<<tag<<"."<<RTDB_ATT_VALIDCHANGE<<"', rc=" << rc; break; }
      break;

      case TS:
      case TSA:
      case VA:
      case ATC:
      case AUX1:
      case AUX2:
      case ICS:

//        type = ClassDescriptionTable[objclass].val_type;

        rc = XDBPoint_di_read_handle(obj, &di);
        if (rc) { LOG(ERROR) << "Read discrete part of '"<<tag<<"', rc="<<rc; break; }

        rc = DiscreteInfoType_VALACQ_get(&di, &acquired_val_i);
        if (rc) { LOG(ERROR) << "Read '"<<tag<< "."<<RTDB_ATT_VALACQ<<"', rc="<<rc; break; }

        rc = DiscreteInfoType_VALMANUAL_get(&di, &manual_val_i);
        if (rc) { LOG(ERROR) << "Read '"<<tag<<"."<<RTDB_ATT_VALMANUAL<<"', rc=" << rc; break; }

        rc = self->GOFVALTSC(t,             // Текущая транзакция
                      obj,                  // Ссылка на экземпляр Точки
                      di,                   // Дискретная часть Точки
                      tag,                  // Тег Точки
                      objclass,             // Класс Точки
                      validchange_val,      // VALIDCHANGE
                      acquired_val_i,       // VALACQ
                      last_update_time,     // DATEHOURM
                      manual_val_i,         // VALMANUAL
                      acquired_validity,    // VALIDACQ
                      current_val_i,        // VAL
                      current_validity,     // VALID
                      sa_state,             // SASTATE
                      old_sa_state          // OLDSASTATE
                      );
        if (rc) { LOG(ERROR) << "Call GOFVALTSC on '"<<tag<<"."<<RTDB_ATT_VALIDCHANGE<<"', rc=" << rc; break; }
      break;

      case AL:
        rc = self->GOFVALAL(t,              // Текущая транзакция
                      obj,                  // Ссылка на экземпляр Точки
                      di,                   // Дискретная часть Точки
                      tag,                  // Тег Точки
                      objclass,             // Класс Точки
                      validchange_val,      // VALIDCHANGE
                      acquired_val_i,       // VALACQ
                      last_update_time,     // DATEHOURM
                      manual_val_i,         // VALMANUAL
                      acquired_validity,    // VALIDACQ
                      current_val_i,        // VAL
                      current_validity,     // VALID
                      sa_state,             // SASTATE
                      old_sa_state          // OLDSASTATE
                      );
        if (rc) { LOG(ERROR) << "Call GOFVALAL on '"<<tag<<"."<<RTDB_ATT_VALIDCHANGE<<"', rc=" << rc; break; }
      break;

      default:
        LOG(ERROR) << "Call '" << fname << "' on point '"<< tag
                   <<"' with unsupported objclass: " << objclass;
        rc = MCO_S_NOTFOUND;
    }
 
  } while (false);

  LOG(INFO) << "VALIDCHANGE update end "<<obj<<" tag '"<<tag<<"' ["<<tag_size<<"] self=" << self << ", rc="<<rc;

  return rc;
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
MCO_RET DatabaseRtapImpl::RegisterEvents()
{
  MCO_RET rc;
  mco_trans_h t;
  mco_new_point_evnt_handler new_handler = DatabaseRtapImpl::new_Point;
  mco_new_point_evnt_handler delete_handler = DatabaseRtapImpl::del_Point;
  mco_new_point_evnt_handler validchange_handler = DatabaseRtapImpl::on_update_VALIDCHANGE;

  assert(m_impl);

  LOG(INFO) << "This thread is arm VALIDCHANGE";
  do
  {
    rc = mco_trans_start(m_impl->getDbHandler(), MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) LOG(ERROR) << "Starting transaction, rc=" << rc;

    rc = mco_register_update_validchange_point_evnt_handler(t,
            validchange_handler,
            static_cast<void*>(this)
//#if EXTREMEDB_VERSION >= 50
            ,MCO_AFTER_UPDATE
//#endif
            );
    if (rc) LOG(ERROR) << "Registering event on VALIDCHANGE updates, rc=" << rc;
    else LOG(INFO) << "Register event on VALIDCHANGE update is OK";

    rc = mco_register_new_point_evnt_handler(t, 
            new_handler,
            static_cast<void*>(this)
            );
    if (rc) LOG(ERROR) << "Registering event on Point creation, rc=" << rc;

    rc = mco_register_delete_point_evnt_handler(t, 
            delete_handler,
            static_cast<void*>(this)
            );
    if (rc) LOG(ERROR) << "Registering event on Point deletion, rc=" << rc;

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
// Чтение данных набора Точек из заданной группы
// Для определения количества точек в группе вызвать функцию с size=0
const Error& DatabaseRtapImpl::read(mco_db_h& handle, std::string& sbs_name, int* p_size, xdb::SubscriptionPoints_t* points_list)
{
  mco_trans_h t;
  MCO_RET rc;
  // Элементарная запись о связи идентификатора группы и точки
  rtap_db::SBS_GROUPS_ITEM sbs_item_instance;
  SBS_GROUPS_ITEM sbs_item_handle;
  // Элементарная запись о состоянии данной группы подписки
  rtap_db::SBS_GROUPS_STAT sbs_stat_instance;
  autoid_t  point_aid;
  autoid_t  sbs_aid;
  int       compare_result = 0;
  int       num_points = 0;
  // Набор атрибутов для одного элемента группы подписки
  PointDescription_t* point_info = NULL;
  mco_cursor_t csr;

  assert(p_size);

#if defined VERBOSE
  LOG(INFO) << "Read values of SBS GROUP '" << sbs_name << "'";
#endif

  setError(rtE_RUNTIME_ERROR);

  do
  {
    rc = mco_trans_start(handle, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    // Проверить, существует ли уже группа с таким названием?
    rc = rtap_db::SBS_GROUPS_STAT::PK_by_name::find(t, sbs_name.c_str(), sbs_name.size(), sbs_stat_instance);
    if (rc && rc != MCO_S_NOTFOUND)
       { LOG(ERROR) << "Locating SBS " << sbs_name << ", rc="<<rc; break; }

    if (rc != MCO_S_NOTFOUND)
    {
      // Группа найдена, получить ее идентификатор
      rc = sbs_stat_instance.autoid_get(sbs_aid);
      if (rc) { LOG(ERROR) << "Get autoid for SBS " << sbs_name << ", rc="<<rc; break; }

      // 1. Получить список элементов данной группы из SBS_GROUPS_ITEM
      rc = SBS_GROUPS_ITEM_PK_by_group_id_index_cursor(t, &csr);
      if (MCO_S_OK == rc)
      {
        rc = SBS_GROUPS_ITEM_PK_by_group_id_search(t, &csr, MCO_EQ, sbs_aid);
        if (rc && (rc != MCO_S_NOTFOUND)) { LOG(ERROR) << "Can't find items in SBS_GROUPS_ITEM with id="<<sbs_aid<<", rc="<<rc; break; }

        if (rc != MCO_S_NOTFOUND)
        {
            // Прочитать очередной экземпляр SBS_GROUPS_ITEM
            rc = mco_cursor_first(t, &csr);
            if (rc) { LOG(ERROR) << "Unable to get first item from SBS_GROUPS_ITEM with id="<<sbs_aid<<", rc=" << rc; break; }

            while (MCO_S_OK == rc)
            {
              rc = SBS_GROUPS_ITEM_PK_by_group_id_compare(t, &csr, sbs_aid, &compare_result);
              if (rc) { LOG(ERROR) << "Unable to compare current item from SBS_GROUPS_ITEM with id="<<sbs_aid<<", rc=" << rc; break; }

              if (0 == compare_result) // Найден элемент, принадлежащий нужной группе
              {
                // Если был передан нулевой размер выходного массива
                if (!*p_size)
                {
                  // То считаем количество элементов
                  num_points++;
                }
                else // Иначе читаем фактические данные
                {
                  rc = SBS_GROUPS_ITEM_from_cursor(t, &csr, &sbs_item_handle);
                  if (rc) { LOG(ERROR) << "Unable to get current item from SBS_GROUPS_ITEM with id="<<sbs_aid<<", rc=" << rc; break; }

                  rc = SBS_GROUPS_ITEM_tag_id_get(&sbs_item_handle, &point_aid);
                  if (rc) { LOG(ERROR) << "Unable to get point id in SBS_GROUPS_ITEM, rc=" << rc; break; }

                  point_info = new xdb::PointDescription_t;
                  // По заданному id Точки прочитать значения её значимых для группы подписки атрибутов
                  rc = LoadPointInfo(handle, t, point_aid, point_info);
                  if (rc)
                  {
                    LOG(ERROR) << "Unable to load attributes for point id="<<point_aid<<", rc=" << rc;
                    delete point_info;
                  }
                  else // В случае единичной ошибки чтения продолжить работу
                  {
                    points_list->push_back(point_info);
                    // С этим атрибутом закончили, переходим к следующему
                    num_points++;
                    //1 LOG(INFO) << "GEV: points_list #"<<num_points<<" add point id=" << point_aid;//1
                  }
                }

              } // конец проверки, принадлежит ли нужной группе очередной элемент курсора

              rc = mco_cursor_next(t, &csr);

            } // конец цикла проверки всех элементов в SBS_GROUPS_ITEM
        } // конец проверки на существование группы с заданным именем

#if (EXTREMEDB_VERSION >=40)
        rc = mco_cursor_close(t, &csr);
        if (rc) { LOG(ERROR) << "Unable to close SBS_GROUPS_ITEM cursor, rc=" << rc; break; }
#endif
      }

      // Если был запрос на получение количества элементов в группе, передадим подсчитанную цифру
      if (!*p_size)
        *p_size = num_points;

      // 2. SBS_GROUPS_STAT
      rc = rtap_db::SBS_GROUPS_STAT::autoid::find(t, sbs_aid, sbs_stat_instance);
      if (rc) { LOG(ERROR) << "Locating SBS_STAT " << sbs_name << ", rc="<<rc; break; }

      rc = mco_trans_commit(t);
      if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

      // Все ОК
      clearError();
    }
    else
    {
      LOG(WARNING) << "Try to read unexistant subscription group: " << sbs_name;
    }

  } while (false);

  if (rc)
    mco_trans_rollback(t);

  return getLastError();
}

// По заданной своим id XDBPoint создать и заполнить структуру AttributeInfo_t
MCO_RET DatabaseRtapImpl::LoadPointInfo(mco_db_h& handle,
                                        mco_trans_h t,
                                        autoid_t point_aid,
                                        xdb::PointDescription_t* point_info)
{
  // Учтено увеличение размера строки при хранении русского в UTF-8
  char s_tag[sizeof(wchar_t)*TAG_NAME_MAXLEN + 1];
  uint2 tag_size = sizeof(wchar_t)*TAG_NAME_MAXLEN;
  rtap_db::XDBPoint point;
  // Аналоговая часть Точки
  rtap_db::AnalogInfoType   ai;
  // Дискретная часть Точки
  rtap_db::DiscreteInfoType di;
  // Тревожная часть :)
  rtap_db::AlarmItem        alarm;
  bool read_func_found;
  objclass_t objclass;
  AttributeInfo_t attr_info;
  AttrProcessingFuncMapIterator_t it;
  MCO_RET rc;

  rc = MCO_S_OK;

  LOG(INFO) << "LoadPointInfo: point id=" << point_aid;//1
  do
  {
    // 1. По заданному идентификатору найти экземпляр XDBpoint
    rc = rtap_db::XDBPoint::autoid::find(t, point_aid, point);
    if (rc) { LOG(ERROR) << "Can't locating point with id=" << point_aid << ", rc=" << rc; break; }

    // 2. Прочитать тег и класс точки
    rc = point.TAG_get(s_tag, tag_size);
    if (rc) { LOG(ERROR) << "Reading tag for point id=" << point_aid << ", rc=" << rc; break; }
    point_info->tag.assign(s_tag);

    rc = point.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Reading OBJCLASS for point '" << s_tag << "', rc=" << rc; break; }
    point_info->objclass = objclass;

    // 3. Прочитать все участвующие в подписке атрибуты.
    // NB: В зависимости от objclass следует читать разные наборы атрибутов
    const read_attribs_on_sbs_event_t info = attribute_holder.info(objclass);

    for(int idx = 0; idx < info.quantity; idx++)
    {
      read_func_found = false;
      // info.list[idx] это индекс атрибута
      // AttrTypeDescription[индекс].name это название атрибута
      it = m_attr_reading_func_map.find(AttrTypeDescription[/*RTDB_ATT_IDX_...*/info.list[idx]].name);
      //attributes_for_subscription_group[idx]);
      if (it != m_attr_reading_func_map.end())
      {
          read_func_found = true;
          // Вызвать функцию чтения атрибута
          rc = (this->*(it->second))(t, point, &attr_info);
          if (rc)
          {
            LOG(ERROR) << "Reading " << s_tag << "." << AttrTypeDescription[info.list[idx]].name;
            setError(rtE_ILLEGAL_PARAMETER_VALUE);
            break;
          }
          else
          {
            point_info->attributes.insert(
                    AttributeMapPair_t(AttrTypeDescription[info.list[idx]].name, attr_info)
                                         );
          }
      }
      else
      {
        // Сейчас возможна ситуация, что для данного типа Точки некоторых атрибутов
        // из таблицы m_attr_reading_func_map не будет. Следует использовать свою таблицу
        // для каждого OBJCLASS
        LOG(ERROR) << "Point '"<<s_tag<<"' doesn't have necessary attribute "
                   << AttrTypeDescription[info.list[idx]].name;
      }

      if (!read_func_found)
        LOG(ERROR) << "Find reading function for '" << s_tag << "'";
    }
  } while(false);

  return rc;
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
const Error& DatabaseRtapImpl::Control(mco_db_h& handler, rtDbCq& info)
{
  MCO_RET rc;

  setError(rtE_NOT_IMPLEMENTED);

  switch(info.action.control)
  {
    case rtCONTROL_SAVE_XSD:
      break;

    case rtCONTROL_CHECK_CONN:
      clearError();
      rc = controlConnections(handler, info);
      if (rc)
      {
        LOG(ERROR) << "Connection check, rc=" << rc;
        setError(rtE_RUNTIME_ERROR); // TODO: уточни в зависимости от rc
      }
      break;

    default:
      setError(rtE_NOT_IMPLEMENTED);
  }

  return getLastError();
}

// =================================================================================
// Проверка состояния подключений
MCO_RET DatabaseRtapImpl::controlConnections (mco_db_h& handler, rtDbCq& info)
{
  MCO_RET rc = MCO_S_OK;

#if (EXTREMEDB_VERSION > 40)
  // Проверка статуса активных соединений.
  // Если процесс недоступен, но информация о соединении сохранилась, это подключение
  // считается аварийно завершенным.
  // NB: данная проверка возможна, если подключение происходит функцией mco_db_connect_ctx()
  rc = mco_db_sniffer(handler, sniffer_callback, MCO_SNIFFER_INSPECT_ACTIVE_CONNECTIONS);
  LOG(INFO) << "Control database connections: " << mco_ret_string(rc, NULL);
#else
  LOG(INFO) << "Control database connections not supported in this XDB version";
#endif

  return rc;
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
        if (rc) { LOG(ERROR) << "Subscription group creation facility"; }
        break;

    case rtCONFIG_ADD_GROUP_SBS:
        rc = createGroup(handler, info);
        if (rc) { LOG(ERROR) << "Subscription group creation facility"; }
        break;

    case rtCONFIG_ENABLE_GROUP_SBS:
        rc = enableGroup(handler, info);
        if (rc)
        {
          LOG(ERROR) << "Enable subscription group: "
                     << *static_cast<std::string*>(info.buffer)
                     << ", rc=" << rc;
          setError(rtE_RUNTIME_WARNING);
        }
        break;

    case rtCONFIG_SUSPEND_GROUP_SBS:
        rc = suspendGroup(handler, info);
        if (rc)
        {
          LOG(ERROR) << "Suspend subscription group: "
                     <<*static_cast<std::string*>(info.buffer)
                     << ", rc=" << rc;
          setError(rtE_RUNTIME_WARNING);
        }
        break;

    case rtCONFIG_DEL_GROUP_SBS:
        rc = deleteGroup(handler, info);
        if (rc) { LOG(ERROR) << "Subscription group deletion facility"; }
        break;

    default:
        setError(rtE_NOT_IMPLEMENTED);
  }

  return getLastError();
}

// =================================================================================
// Интерфейс активации группы атрибутов под заданным именем
MCO_RET DatabaseRtapImpl::enableGroup(mco_db_h& handler, rtDbCq& info)
{
  MCO_RET rc = MCO_S_OK;
  return rc;
}

// =================================================================================
// Интерфейс деактивации группы атрибутов под заданным именем
MCO_RET DatabaseRtapImpl::suspendGroup(mco_db_h& handler, rtDbCq& info)
{
  MCO_RET rc = MCO_S_NOTFOUND;
  return rc;
}

// =================================================================================
// Интерфейс создания группы атрибутов под заданным именем
MCO_RET DatabaseRtapImpl::createGroup(mco_db_h& handler, rtDbCq& info)
{
  MCO_RET rc = MCO_S_NOTFOUND;
  // Экземпляр точки, попавшей в данную группу
  rtap_db::XDBPoint point_instance;
  // Элементарная запись о связи идентификатора группы и точки
  rtap_db::SBS_GROUPS_ITEM sbs_item_instance;
  // Элементарная запись о состоянии данной группы подписки
  rtap_db::SBS_GROUPS_STAT sbs_stat_instance;
  std::string* sbs_name = static_cast<std::string*>(info.buffer);
  mco_trans_h t;
  int idx = 0;
  InternalState state = ENABLE;
  autoid_t  point_aid = 0;
  autoid_t  sbs_aid = 0;

  assert(info.action.config == rtCONFIG_ADD_GROUP_SBS);
//  assert(info.addrCnt);
  assert(info.tags);

  LOG(INFO) << "Create SBS GROUP '"<<*sbs_name<<"' with "<<info.tags->size()<<" elements";

  setError(rtE_RUNTIME_ERROR);
  do
  {
    rc = mco_trans_start(handler, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    // Проверить, существует ли уже группа с таким названием?
    rc = rtap_db::SBS_GROUPS_STAT::PK_by_name::find(t, sbs_name->c_str(), sbs_name->size(), sbs_stat_instance);
    if (rc && rc != MCO_S_NOTFOUND)
       { LOG(ERROR) << "Locating SBS '" << *sbs_name << "', rc="<<rc; break; }
    if (rc == MCO_S_NOTFOUND)
    {
        // Не было группы с заданным именем
        // 1. Создать запись в SBS_GROUPS_STAT
        rc = sbs_stat_instance.create(t);
        if (rc) { LOG(ERROR) << "Creating SBS_STAT '" << *sbs_name << "', rc="<<rc; break; }
        rc = sbs_stat_instance.name_put(sbs_name->c_str(), (uint2)sbs_name->size());
        if (rc) { LOG(ERROR) << "Set name '" << *sbs_name << "' in SBS_STAT, rc="<<rc; break; }
        rc = sbs_stat_instance.state_put(state);
        if (rc) { LOG(ERROR) << "Set name '" << *sbs_name << "' in SBS_STAT, rc="<<rc; break; }
        // Проверить возможность создания - страховка от обнаружения ошибки на более поздней стадии
        rc = sbs_stat_instance.checkpoint();
        if (rc) { LOG(ERROR) << "Checkout of SBS_STAT '" << *sbs_name << "', rc="<<rc; break; }

        // Получить ее идентификатор
        rc = sbs_stat_instance.autoid_get(sbs_aid);
        if (rc) { LOG(ERROR) << "Get autoid for SBS '" << *sbs_name << "', rc="<<rc; break; }

        LOG(INFO) << "Insert " <<info.tags->size()<< " items with group_id=" <<sbs_aid
                  << " in SBS_GROUPS_STAT as '" << *sbs_name << "'";

        // Найти и запомнить идентификаторы тегов элементов группы
        for (std::vector<std::string>::iterator it = info.tags->begin();
             it < info.tags->end();
             it++)
        {
          // 2. Создать запись в таблице SBS_GROUPS_ITEM
          rc = sbs_item_instance.create(t);
          if (rc) { LOG(ERROR) << "Creating SBS_INFO for '" << *sbs_name << "', rc="<<rc; break; }

          // Заполнить идентификатор подписки
          rc = sbs_item_instance.group_id_put(sbs_aid);
          if (rc) { LOG(ERROR) << "Set group id for SBS '" << *sbs_name << "', rc="<<rc; break; }

          // Найти точку по заданному тегу
          LOG(INFO) << ++idx << ": " << *it;

          rc = rtap_db::XDBPoint::SK_by_tag::find(t, (*it).c_str(), (*it).size(), point_instance);
          if (rc) { LOG(ERROR) << "Locating point '" << *it << "', rc=" << rc; setError(rtE_POINT_NOT_FOUND); break; }

          rc = point_instance.autoid_get(point_aid);
          if (rc) { LOG(ERROR) << "Can't get autoid for " << (*it); break; }

          rc = sbs_item_instance.tag_id_put(point_aid);
          if (rc) { LOG(ERROR) << "Set tag id for SBS '" << *sbs_name << "', rc="<<rc; break; }

          // Проверить возможность создания записи
          rc = sbs_item_instance.checkpoint();
          if (rc) { LOG(ERROR) << "Checkout of SBS_ITEM '" << *sbs_name << "', rc="<<rc; break; }
          LOG(INFO) << "Insert item with point_id=" <<point_aid << " in SBS_GROUPS_ITEM with group_id="<<sbs_aid;
        }
    }
    else
    {
        // Группа с заданным именем уже содержится в БДРВ
        // Получить ее идентификатор
        rc = sbs_stat_instance.autoid_get(sbs_aid);
        if (rc) { LOG(ERROR) << "Get autoid for SBS '" << *sbs_name << "', rc="<<rc; break; }

        // Запрещено создавать группы подписки с одинаковыми именами
        LOG(ERROR) << "Creating clone of existing SBS '" << *sbs_name << "' with id=" << sbs_aid;
        break;
    }

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

    // Все ОК
    clearError();

  } while(false);

  if (rc)
    mco_trans_rollback(t);

  return rc;
}

// =================================================================================
// Интерфейс удаления группы атрибутов под заданным именем
MCO_RET DatabaseRtapImpl::deleteGroup(mco_db_h& handler, rtDbCq& info)
{
  mco_trans_h t;
  MCO_RET rc, rc_del;
  // Элементарная запись о связи идентификатора группы и точки
  rtap_db::SBS_GROUPS_ITEM sbs_item_instance;
  SBS_GROUPS_ITEM sbs_item_handle;
  // Элементарная запись о состоянии данной группы подписки
  rtap_db::SBS_GROUPS_STAT sbs_stat_instance;
  autoid_t  point_aid = 0;
  autoid_t  sbs_aid = 0;
  int       compare_result = 0;
  mco_cursor_t csr;
  std::string* sbs_name = static_cast<std::string*>(info.buffer);

  assert(info.action.config == rtCONFIG_DEL_GROUP_SBS);

  LOG(INFO) << "Delete SBS GROUP '" << *sbs_name << "'";

  setError(rtE_RUNTIME_ERROR);
  do
  {
    rc = mco_trans_start(handler, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    // Проверить, существует ли уже группа с таким названием?
    rc = rtap_db::SBS_GROUPS_STAT::PK_by_name::find(t, sbs_name->c_str(), sbs_name->size(), sbs_stat_instance);
    if (rc && rc != MCO_S_NOTFOUND)
       { LOG(ERROR) << "Locating SBS " << *sbs_name << ", rc="<<rc; break; }

    if (rc != MCO_S_NOTFOUND)
    {
      // Группа найдена, получить ее идентификатор
      rc = sbs_stat_instance.autoid_get(sbs_aid);
      if (rc) { LOG(ERROR) << "Get autoid for SBS " << *sbs_name << ", rc="<<rc; break; }

      // 1. Удалить из SBS_GROUPS_ITEM
      rc = SBS_GROUPS_ITEM_PK_by_group_id_index_cursor(t, &csr);
      if (MCO_S_OK == rc)
      {
        rc = SBS_GROUPS_ITEM_PK_by_group_id_search(t, &csr, MCO_EQ, sbs_aid);
        if (rc && (rc != MCO_S_NOTFOUND)) { LOG(ERROR) << "Can't find items in SBS_GROUPS_ITEM with id="<<sbs_aid<<", rc="<<rc; break; }

        // После сбоев озможна ситуация, когда таблицы SBS_GROUPS_STAT и SBS_GROUPS_ITEM рассинхронизированы.
        // В этом случае в SBS_GROUPS_ITEM ссылок на SBS_GROUPS_STAT может не быть, но удалить запись в
        // SBS_GROUPS_STAT необходимо.
        if (rc != MCO_S_NOTFOUND)
        {
            // Прочитать очередной экземпляр SBS_GROUPS_ITEM перед его удалением
            rc = mco_cursor_first(t, &csr);
            if (rc) { LOG(ERROR) << "Unable to get first item from SBS_GROUPS_ITEM with id="<<sbs_aid<<", rc=" << rc; break; }

            while (MCO_S_OK == rc)
            {
              rc = SBS_GROUPS_ITEM_PK_by_group_id_compare(t, &csr, sbs_aid, &compare_result);
              if (rc) { LOG(ERROR) << "Unable to compare current item from SBS_GROUPS_ITEM with id="<<sbs_aid<<", rc=" << rc; break; }

              if (0 == compare_result) // Найден элемент с удаляемым id
              {
                  rc = SBS_GROUPS_ITEM_from_cursor(t, &csr, &sbs_item_handle);
                  if (rc) { LOG(ERROR) << "Unable to get current item from SBS_GROUPS_ITEM with id="<<sbs_aid<<", rc=" << rc; break; }

                  rc = SBS_GROUPS_ITEM_tag_id_get(&sbs_item_handle, &point_aid);
                  if (rc) { LOG(ERROR) << "Unable to get point id in SBS_GROUPS_ITEM, rc=" << rc; break; }

#if 0
                  LOG(INFO) << "Delete item with point_id=" <<point_aid << " in SBS_GROUPS_ITEM with group_id="<<sbs_aid;
                  // Перед удалением текущего экземпляра передвинуть курсор на следующую запись.
                  // Необходимо для того, чтобы после удаления не разрушился курсор.
                  rc = mco_cursor_next(t, &csr);
#endif

                  rc_del = SBS_GROUPS_ITEM_delete(&sbs_item_handle);
                  if (rc_del)
                  {
                      LOG(ERROR) << "Unable to delete SBS_GROUPS_ITEM with sbs_aid="<<sbs_aid
                                 <<" point_aid=" << point_aid
                                 << ", rc=" << rc_del;
                      break;
                  } // если удаление прошло успешно
#if 1
                  rc = mco_cursor_next(t, &csr);
#endif
              } // конец проверки, является ли очередной элемент курсора удаляемым
              else
              {
                // Пропустим этот элемент
                rc = mco_cursor_next(t, &csr);
              }
            } // конец цикла удаления всех элементов в SBS_GROUPS_ITEM
        }

#if (EXTREMEDB_VERSION >=40)
        rc = mco_cursor_close(t, &csr);
        if (rc) { LOG(ERROR) << "Unable to close SBS_GROUPS_ITEM cursor, rc=" << rc; break; }
#endif
      }

      // 2. Удалить из SBS_GROUPS_STAT
      rc = rtap_db::SBS_GROUPS_STAT::autoid::find(t, sbs_aid, sbs_stat_instance);
      if (rc) { LOG(ERROR) << "Locating SBS_STAT " << *sbs_name << ", rc="<<rc; break; }

      rc = sbs_stat_instance.remove();
      if (rc)
      {
        LOG(ERROR) << "Unable to delete group '"<<*sbs_name
                   << "' in SBS_GROUPS_STAT with sbs_id=" <<sbs_aid<< ", rc=" << rc;
        break;
      }
      LOG(INFO) << "Delete group '"<<*sbs_name<< "' in SBS_GROUPS_STAT with id="<<sbs_aid;

      rc = mco_trans_commit(t);
      if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

      // Все ОК
      clearError();
    }
    else
    {
      LOG(WARNING) << "Try to delete unexistant subscription group: " << *sbs_name;
    }

  } while (false);

  if (rc)
    mco_trans_rollback(t);

  return rc;
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
    point->passport().ts = new rtap_db::TS_passport;
    rc = point->passport().ts->create(point->current_transaction());
    rc = point->passport().ts->autoid_get(passport_id);
    break;

    case TM:  /* 01 */
    point->passport().tm = new rtap_db::TM_passport;
    rc = point->passport().tm->create(point->current_transaction());
    rc = point->passport().tm->autoid_get(passport_id);
    break;

    case TR:  /* 02 */
    point->passport().tr = new rtap_db::TR_passport;
    rc = point->passport().tr->create(point->current_transaction());
    rc = point->passport().tr->autoid_get(passport_id);
    break;

    case TSA: /* 03 */
    point->passport().tsa = new rtap_db::TSA_passport;
    rc = point->passport().tsa->create(point->current_transaction());
    rc = point->passport().tsa->autoid_get(passport_id);
    break;

    case TSC: /* 04 */
    point->passport().tsc = new rtap_db::TSC_passport;
    rc = point->passport().tsc->create(point->current_transaction());
    rc = point->passport().tsc->autoid_get(passport_id);
    break;

    case TC:  /* 05 */
    point->passport().tc = new rtap_db::TC_passport;
    rc = point->passport().tc->create(point->current_transaction());
    rc = point->passport().tc->autoid_get(passport_id);
    break;

    case AL:  /* 06 */
    point->passport().al = new rtap_db::AL_passport;
    rc = point->passport().al->create(point->current_transaction());
    rc = point->passport().al->autoid_get(passport_id);
    break;

    case ICS: /* 07 */
    point->passport().ics = new rtap_db::ICS_passport;
    rc = point->passport().ics->create(point->current_transaction());
    rc = point->passport().ics->autoid_get(passport_id);
    break;

    case ICM: /* 08 */
    point->passport().icm = new rtap_db::ICM_passport;
    rc = point->passport().icm->create(point->current_transaction());
    rc = point->passport().icm->autoid_get(passport_id);
    break;

    case TL:  /* 16 */
    point->passport().tl = new rtap_db::TL_passport;
    rc = point->passport().tl->create(point->current_transaction());
    rc = point->passport().tl->autoid_get(passport_id);
    break;

    case VA:  /* 19 */
    point->passport().valve = new rtap_db::VA_passport;
    rc = point->passport().valve->create(point->current_transaction());
    rc = point->passport().valve->autoid_get(passport_id);
    break;

    case SC:  /* 20 */
    point->passport().sc = new rtap_db::SC_passport;
    rc = point->passport().sc->create(point->current_transaction());
    rc = point->passport().sc->autoid_get(passport_id);
    break;

    case ATC: /* 21 */
    point->passport().atc = new rtap_db::ATC_passport;
    rc = point->passport().atc->create(point->current_transaction());
    rc = point->passport().atc->autoid_get(passport_id);
    break;

    case GRC: /* 22 */
    point->passport().grc = new rtap_db::GRC_passport;
    rc = point->passport().grc->create(point->current_transaction());
    rc = point->passport().grc->autoid_get(passport_id);
    break;

    case SV:  /* 23 */
    point->passport().sv = new rtap_db::SV_passport;
    rc = point->passport().sv->create(point->current_transaction());
    rc = point->passport().sv->autoid_get(passport_id);
    break;

    case SDG: /* 24 */
    point->passport().sdg = new rtap_db::SDG_passport;
    rc = point->passport().sdg->create(point->current_transaction());
    rc = point->passport().sdg->autoid_get(passport_id);
    break;

    case SSDG:/* 26 */
    point->passport().ssdg = new rtap_db::SSDG_passport;
    rc = point->passport().ssdg->create(point->current_transaction());
    rc = point->passport().ssdg->autoid_get(passport_id);
    break;

    case SCP: /* 28 */
    point->passport().scp = new rtap_db::SCP_passport;
    rc = point->passport().scp->create(point->current_transaction());
    rc = point->passport().scp->autoid_get(passport_id);
    break;

    case DIR: /* 30 */
    point->passport().dir = new rtap_db::DIR_passport;
    rc = point->passport().dir->create(point->current_transaction());
    rc = point->passport().dir->autoid_get(passport_id);
    break;

    case DIPL:/* 31 */
    point->passport().dipl = new rtap_db::DIPL_passport;
    rc = point->passport().dipl->create(point->current_transaction());
    rc = point->passport().dipl->autoid_get(passport_id);
    break;

    case METLINE: /* 32 */
    point->passport().metline = new rtap_db::METLINE_passport;
    rc = point->passport().metline->create(point->current_transaction());
    rc = point->passport().metline->autoid_get(passport_id);
    break;

    case ESDG:/* 33 */
    point->passport().esdg = new rtap_db::ESDG_passport;
    rc = point->passport().esdg->create(point->current_transaction());
    rc = point->passport().esdg->autoid_get(passport_id);
    break;

    case SCPLINE: /* 35 */
    point->passport().scpline = new rtap_db::SCPLINE_passport;
    rc = point->passport().scpline->create(point->current_transaction());
    rc = point->passport().scpline->autoid_get(passport_id);
    break;

    case TLLINE:  /* 36 */
    point->passport().tlline = new rtap_db::TLLINE_passport;
    rc = point->passport().tlline->create(point->current_transaction());
    rc = point->passport().tlline->autoid_get(passport_id);
    break;

    case AUX1:/* 38 */
    point->passport().aux1 = new rtap_db::AUX1_passport;
    rc = point->passport().aux1->create(point->current_transaction());
    rc = point->passport().aux1->autoid_get(passport_id);
    break;

    case AUX2:/* 39 */
    point->passport().aux2 = new rtap_db::AUX2_passport;
    rc = point->passport().aux2->create(point->current_transaction());
    rc = point->passport().aux2->autoid_get(passport_id);
    break;

    case SITE:/* 45 */
    point->passport().site = new rtap_db::SITE_passport;
    rc = point->passport().site->create(point->current_transaction());
    rc = point->passport().site->autoid_get(passport_id);
    break;

    case SA:  /* 50 */
    point->passport().sa = new rtap_db::SA_passport;
    rc = point->passport().sa->create(point->current_transaction());
    rc = point->passport().sa->autoid_get(passport_id);
    break;

    case PIPE:/* 11 */
    point->passport().pipe = new rtap_db::PIPE_passport;
    rc = point->passport().pipe->create(point->current_transaction());
    rc = point->passport().pipe->autoid_get(passport_id);
    break;

    case PIPELINE:/* 15 */
    point->passport().pipeline = new rtap_db::PIPELINE_passport;
    rc = point->passport().pipeline->create(point->current_transaction());
    rc = point->passport().pipeline->autoid_get(passport_id);
    break;

    case RGA: /* 25 */
    point->passport().rga = new rtap_db::RGA_passport;
    rc = point->passport().rga->create(point->current_transaction());
    rc = point->passport().rga->autoid_get(passport_id);
    break;

    case BRG: /* 27 */
    point->passport().brg = new rtap_db::BRG_passport;
    rc = point->passport().brg->create(point->current_transaction());
    rc = point->passport().brg->autoid_get(passport_id);
    break;

    case STG: /* 29 */
    point->passport().stg = new rtap_db::STG_passport;
    rc = point->passport().stg->create(point->current_transaction());
    rc = point->passport().stg->autoid_get(passport_id);
    break;

    case SVLINE:  /* 34 */
    point->passport().svline = new rtap_db::SVLINE_passport;
    rc = point->passport().svline->create(point->current_transaction());
    rc = point->passport().svline->autoid_get(passport_id);
    break;

    case INVT:/* 37 */
    point->passport().invt = new rtap_db::INVT_passport;
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
          if (!rc) // Если запись прошла успешно
          {
            // Взведем признак модификации атрибутов для последующей проверки группой подписки
            rc = instance.is_modified_put(TRUE);
            if (rc) { LOG(ERROR) << "Set '"<<point_name<<"' modified flag, rc="<<rc; break; }
          }
          if (rc) // Или запись атрибута прошла неудачно, или ошибка взведения флага модификации
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
      LOG(ERROR) << "Skip creating attributes of point '" << instance->tag() << "'";
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

// Прочесть значение штатными средствами
MCO_RET DatabaseRtapImpl::readTAG(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc;

  assert(attr_info);

  check_user_defined_type(RTDB_ATT_IDX_UNIVNAME, attr_info);

  rc = instance.TAG_get(attr_info->value.dynamic.varchar, attr_info->value.dynamic.size);
#if defined VERBOSE
  LOG(INFO) << attr_info->name << " = " << attr_info->value.dynamic.varchar;
#endif

  return rc;
}
// NB: Изменение TAG уже созданной точки запрещено

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
// NB: Изменение OBJCLASS уже созданной точки запрещено

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
  assert(attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_SHORTLABEL].type);

  rc = instance.SHORTLABEL_put(attr_info->value.dynamic.varchar,
                               AttrTypeDescription[RTDB_ATT_IDX_SHORTLABEL].size);
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

MCO_RET DatabaseRtapImpl::readUNITY(mco_trans_h& t,
                                    rtap_db::XDBPoint& instance,
                                    AttributeInfo_t* attr_info)
{
  MCO_RET rc = MCO_S_OK;
  uint2 unity_id;
  rtap_db::DICT_UNITY_ID instance_unity;

  assert(attr_info);
  check_user_defined_type(RTDB_ATT_IDX_UNITY, attr_info);

  do
  {
    // Прочитать идентификатор UNITY в заданном экземпляре Точки
    rc = instance.UNITY_ID_get(unity_id);
    if (rc) { LOG(ERROR) << "Can't get unity id, rc="<<rc; break; }

    // Найти соответствующий прочитанному идентификатору экземпляр
    // в таблице Размерностей (DICT_UNITY_ID)
    rc = rtap_db::DICT_UNITY_ID::PK_by_id::find(t, unity_id, instance_unity);
    if (rc)
    {
      LOG(ERROR) << "Can't find unity with id="<<unity_id<<", rc="<<rc;
      break;
    }

    rc = instance_unity.UNITY_get(attr_info->value.dynamic.varchar, attr_info->value.dynamic.size);
    if (rc) { LOG(ERROR) << "Can't get unity label for id="<<unity_id<<", rc="<<rc; break; }

#if defined VERBOSE
    LOG(INFO) << attr_info->name << " = " << attr_info->value.dynamic.varchar;
#endif

  } while (false);

  return rc;
}

MCO_RET DatabaseRtapImpl::writeUNITY(mco_trans_h& t,
                                     rtap_db::XDBPoint& instance,
                                     AttributeInfo_t* attr_info)
{
  MCO_RET rc = MCO_S_OK;
  uint2 unity_id;
  rtap_db::DICT_UNITY_ID instance_unity;

  assert(attr_info);
  assert(attr_info->value.dynamic.varchar);
  assert(attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_UNITY].type);

  do
  {
    // Найти соответствующий полученному названию экземпляр
    // в таблице Размерностей (DICT_UNITY_ID)
    rc = rtap_db::DICT_UNITY_ID::unityHash::find(t,
                                                 attr_info->value.dynamic.varchar,
                                                 attr_info->value.dynamic.size,
                                                 instance_unity);
    if (rc)
    {
      LOG(ERROR) << "Can't find unity '"<<attr_info->value.dynamic.varchar<<"', rc="<<rc;
      break;
    }

    // Найти идентификатор данного экземпляра Размерности
    rc = instance_unity.unityID_get(unity_id);
    if (rc)
    {
      LOG(ERROR) << "Can't get unity id for '"
                 <<attr_info->value.dynamic.varchar<<"', rc="<<rc;
      break;
    }
    
    // И присвоить его значение полю UNITY_ID Точки
    rc = instance.UNITY_ID_put(unity_id);
    if (rc)
    {
      LOG(ERROR) << "Can't put unity id="<<unity_id
                 << " '"<<attr_info->value.dynamic.varchar
                 << "' for '" << attr_info->name
                 << "', rc="<<rc; break; }

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

MCO_RET DatabaseRtapImpl::readL_SA(mco_trans_h& t,
                                   rtap_db::XDBPoint& instance,
                                   AttributeInfo_t* attr_info)
{
  autoid_t sa_aid;
  rtap_db::XDBPoint point_instance;
  MCO_RET rc = MCO_S_OK;

  check_user_defined_type(RTDB_ATT_IDX_L_SA, attr_info);

  do
  {
    // Получить хендл экземпляра SA
    rc = instance.SA_ref_get(sa_aid);
    if (rc) { LOG(ERROR) << "Read '"<<attr_info->name<<"' reference, rc="<<rc; break; }

    rc = rtap_db::XDBPoint::autoid::find(t, sa_aid, point_instance);
    if (rc) { LOG(ERROR) << "Locate '"<<attr_info->name<<"' instance, rc="<<rc; break; }

    rc = point_instance.TAG_get(attr_info->value.dynamic.varchar, attr_info->value.dynamic.size);
    if (rc) { LOG(ERROR) << "Get '"<<attr_info->name<<"' tag, rc="<<rc; break; }

#if defined VERBOSE
    LOG(INFO) << attr_info->name << " = " << attr_info->value.dynamic.varchar;
#endif

  } while (false);

  return rc;
}

// Создать ссылку на уже имеющуюся в БДРВ Систему Сбора
MCO_RET DatabaseRtapImpl::writeL_SA(mco_trans_h& t,
                                    rtap_db::XDBPoint& instance,
                                    AttributeInfo_t* attr_info)
{
  autoid_t sa_aid;
  rtap_db::XDBPoint point_instance;
  MCO_RET rc = MCO_S_OK;

  assert(attr_info);
  assert(attr_info->value.dynamic.varchar);
  assert(attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_L_SA].type);

  do
  {
    // 1. Найти экземпляр SA по названию
    rc = rtap_db::XDBPoint::SK_by_tag::find(t,
            attr_info->value.dynamic.varchar,
            attr_info->value.dynamic.size,
            point_instance);
    if (rc) { LOG(ERROR) << "Locating point '"<<attr_info->name<<"', rc=" << rc; setError(rtE_POINT_NOT_FOUND); break; }

    // 2. Получить идентификатор экземпляра SA
    rc = point_instance.autoid_get(sa_aid);
    if (rc) { LOG(ERROR) << "Read '"<<attr_info->name<<"' reference, rc="<<rc; break; }

    // 3. Занести в поле SA_ref данной точки идентификатор SA
    rc = instance.SA_ref_put(sa_aid);
    if (rc) { LOG(ERROR) << "Locate '"<<attr_info->name<<"' instance, rc="<<rc; break; }

#if defined VERBOSE
    LOG(INFO) << attr_info->name << " = " << attr_info->value.dynamic.varchar;
#endif

  } while (false);

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
    case 0: value = VALIDITY_FAULT;       break;
    case 1: value = VALIDITY_VALID;       break;
    case 2: value = VALIDITY_MANUAL;      break;
    case 3: value = VALIDITY_INHIBITION;  break;
    case 4: value = VALIDITY_FAULT_INHIB; break;
    case 5: value = VALIDITY_FAULT_FORCED;break;
    case 6: value = VALIDITY_INQUIRED;    break;
    case 7: value = VALIDITY_NO_INSTRUM;  break;
    case 8: value = VALIDITY_GLOBAL_FAULT;break;
    default:value = VALIDITY_FAULT;
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
    case VALIDITY_FAULT:
    case VALIDITY_VALID:
    case VALIDITY_MANUAL:
    case VALIDITY_INHIBITION:
    case VALIDITY_FAULT_INHIB:
    case VALIDITY_FAULT_FORCED:
    case VALIDITY_INQUIRED:
    case VALIDITY_NO_INSTRUM:
    case VALIDITY_GLOBAL_FAULT:
      attr_info->value.fixed.val_uint8 = static_cast<uint8_t>(value);
#if defined VERBOSE
      LOG(INFO) << attr_info->name << " = " << (unsigned int)attr_info->value.fixed.val_uint8; //1
#endif
    break;
    default:
      attr_info->value.fixed.val_uint8 = static_cast<uint8_t>(VALIDITY_FAULT);
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::writeVALID(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  Validity value;

  switch(attr_info->value.fixed.val_uint8)
  {
    case 0: /* VALIDITY_FAULT */
    case 1: /* VALIDITY_VALID */
    case 2: /* VALIDITY_MANUAL */
    case 3: /* VALIDITY_INHIBITION */
    case 4: /* VALIDITY_FAULT_INHIB */
    case 5: /* VALIDITY_FAULT_FORCED */
    case 6: /* VALIDITY_INQUIRED */
    case 7: /* VALIDITY_NO_INSTRUM */
    case 8: /* VALIDITY_GLOBAL_FAULT */
      value = static_cast<Validity>(attr_info->value.fixed.val_uint8);
    break;
    default:
      value = static_cast<Validity>(VALIDITY_FAULT);
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
    case 0: value = VALIDITY_FAULT;       break;
    case 1: value = VALIDITY_VALID;       break;
    case 2: value = VALIDITY_MANUAL;      break;
    case 3: value = VALIDITY_INHIBITION;  break;
    case 4: value = VALIDITY_FAULT_INHIB; break;
    case 5: value = VALIDITY_FAULT_FORCED;break;
    case 6: value = VALIDITY_INQUIRED;    break;
    case 7: value = VALIDITY_NO_INSTRUM;  break;
    case 8: value = VALIDITY_GLOBAL_FAULT;break;
    default:value = VALIDITY_FAULT;
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
    case VALIDITY_FAULT:
    case VALIDITY_VALID:
    case VALIDITY_MANUAL:
    case VALIDITY_INHIBITION:
    case VALIDITY_FAULT_INHIB:
    case VALIDITY_FAULT_FORCED:
    case VALIDITY_INQUIRED:
    case VALIDITY_NO_INSTRUM:
    case VALIDITY_GLOBAL_FAULT:
      attr_info->value.fixed.val_uint8 = static_cast<uint8_t>(value);
    break;
    default:
      attr_info->value.fixed.val_uint8 = static_cast<uint8_t>(VALIDITY_FAULT);
  }

  return rc;
}

MCO_RET DatabaseRtapImpl::writeVALIDACQ(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  Validity value;

  switch(attr_info->value.fixed.val_uint8)
  {
    case 0: /* VALIDITY_FAULT */
    case 1: /* VALIDITY_VALID */
    case 2: /* VALIDITY_MANUAL */
    case 3: /* VALIDITY_INHIBITION */
    case 4: /* VALIDITY_FAULT_INHIB */
    case 5: /* VALIDITY_FAULT_FORCED */
    case 6: /* VALIDITY_INQUIRED */
    case 7: /* VALIDITY_NO_INSTRUM */
    case 8: /* VALIDITY_GLOBAL_FAULT */
      value = static_cast<Validity>(attr_info->value.fixed.val_uint8);
    break;
    default:
      value = static_cast<Validity>(VALIDITY_FAULT);
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

MCO_RET DatabaseRtapImpl::readPREV_DISPATCHER(mco_trans_h& t,
            rtap_db::XDBPoint& instance,
            AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  objclass_t objclass;
  autoid_t passport_aid;
  rtap_db::SITE_passport passport_instance;

  assert(attr_info);
  check_user_defined_type(RTDB_ATT_IDX_PREV_DISPATCHER, attr_info);

  do
  {
    // NB: Атрибут допустим только для Точки SITE
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for '"<<attr_info->name<<"', rc="<<rc; break; }

    switch(objclass)
    {
      case SITE:
        // 1. Найти ссылку на паспорт SITE_passport
        rc = instance.passport_ref_get(passport_aid);
        if (rc) { LOG(ERROR) << "Get passport reference for '"<<attr_info->name<<"', rc"<<rc; break; }

        // 2. Найти по ранее полученной ссылке SITE_passport
        rc = rtap_db::SITE_passport::autoid::find(t, passport_aid, passport_instance);
        if (rc) { LOG(ERROR) << "Locate passport for '"<<attr_info->name<<"', rc="<<rc; break; }

        // 3. Прочитать у полученного экземпляра паспорта нужный атрибут
        rc = passport_instance.PREV_DISPATCHER_get(attr_info->value.dynamic.varchar, attr_info->value.dynamic.size);
        if (rc) { LOG(ERROR) << "Read '"<<attr_info->name<<"', rc="<<rc; break; }
#if defined VERBOSE
        LOG(INFO) << attr_info->name << " = " << attr_info->value.dynamic.varchar;
#endif
      break;

      default:
        LOG(ERROR) << "'" << attr_info->name
                   << "' for objclass " << objclass
                   << " is not supported";
    }

  } while (false);

  return rc;
}

MCO_RET DatabaseRtapImpl::writePREV_DISPATCHER(mco_trans_h& t,
            rtap_db::XDBPoint& instance,
            AttributeInfo_t* attr_info)
{
  objclass_t objclass;
  autoid_t passport_aid;
  rtap_db::SITE_passport passport_instance;
  MCO_RET rc;

  assert(attr_info);
  assert(attr_info->value.dynamic.varchar);
  assert(attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_PREV_DISPATCHER].type);
  check_user_defined_type(RTDB_ATT_IDX_PREV_DISPATCHER, attr_info);

  do
  {
    // NB: Атрибут допустим только для Точки SITE
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for '"<<attr_info->name<<"', rc="<<rc; break; }

    switch(objclass)
    {
      case SITE:
        // 1. Найти ссылку на паспорт SITE_passport
        rc = instance.passport_ref_get(passport_aid);
        if (rc) { LOG(ERROR) << "Get passport reference for '"<<attr_info->name<<"', rc"<<rc; break; }

        // 2. Найти по ранее полученной ссылке SITE_passport
        rc = rtap_db::SITE_passport::autoid::find(t, passport_aid, passport_instance);
        if (rc) { LOG(ERROR) << "Locate passport for '"<<attr_info->name<<"', rc="<<rc; break; }

        // 3. Поменять у полученного экземпляра паспорта нужный атрибут
        rc = passport_instance.PREV_DISPATCHER_put(attr_info->value.dynamic.varchar, attr_info->value.dynamic.size);
        if (rc) { LOG(ERROR) << "Read '"<<attr_info->name<<"', rc="<<rc; break; }
#if defined VERBOSE
        LOG(INFO) << attr_info->name << " = " << attr_info->value.dynamic.varchar;
#endif
      break;

      default:
        LOG(ERROR) << "'" << attr_info->name
                   << "' for objclass " << objclass
                   << " is not supported";
    }

  } while (false);

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

MCO_RET DatabaseRtapImpl::readPREV_SHIFT_TIME(mco_trans_h& t,
            rtap_db::XDBPoint& instance,
            AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  objclass_t objclass;
  autoid_t passport_aid;
  rtap_db::timestamp datehourm;
  datetime_t datetime;
  rtap_db::SITE_passport passport_instance;

  assert(attr_info);

  do
  {
    // NB: Атрибут допустим только для Точки SITE
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for '"<<attr_info->name<<"', rc="<<rc; break; }

    switch(objclass)
    {
      case SITE:
        // 1. Найти ссылку на паспорт SITE_passport
        rc = instance.passport_ref_get(passport_aid);
        if (rc) { LOG(ERROR) << "Get passport reference for '"<<attr_info->name<<"', rc"<<rc; break; }

        // 2. Найти по ранее полученной ссылке SITE_passport
        rc = rtap_db::SITE_passport::autoid::find(t, passport_aid, passport_instance);
        if (rc) { LOG(ERROR) << "Locate passport for '"<<attr_info->name<<"', rc="<<rc; break; }

        // 3. Прочитать у полученного экземпляра паспорта нужный атрибут
        rc = passport_instance.PREV_SHIFT_TIME_read(datehourm);
        if (rc) { LOG(ERROR) << "Read '"<<attr_info->name<<"', rc="<<rc; break; }

        rc = datehourm.sec_get(datetime.part[0]);
        rc = datehourm.nsec_get(datetime.part[1]);
        if (rc) { LOG(ERROR) << "Read value of '" << attr_info->name << "' , rc="<< rc; break; }

        attr_info->value.fixed.val_time.tv_sec = datetime.part[0];
        attr_info->value.fixed.val_time.tv_usec = datetime.part[1];
        attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_PREV_SHIFT_TIME].type;

#if defined VERBOSE
        LOG(INFO) << attr_info->name << " = "
                  << attr_info->value.fixed.val_time.tv_sec
                  << "."
                  << attr_info->value.fixed.val_time.tv_usec;
#endif
      break;

      default:
        LOG(ERROR) << "'" << attr_info->name
                   << "' for objclass " << objclass
                   << " is not supported";
    }

  } while (false);

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
    case 1:  vc = VALIDCHANGE_VALID;        break;
    case 2:  vc = VALIDCHANGE_FORCED;       break;
    case 3:  vc = VALIDCHANGE_INHIB;        break;
    case 4:  vc = VALIDCHANGE_MANUAL;       break;
    case 5:  vc = VALIDCHANGE_END_INHIB;    break;
    case 6:  vc = VALIDCHANGE_END_FORCED;   break;
    case 7:  vc = VALIDCHANGE_INHIB_GBL;    break;
    case 8:  vc = VALIDCHANGE_END_INHIB_GBL;break;
    case 9:  vc = VALIDCHANGE_NULL;         break;
    case 10: vc = VALIDCHANGE_FAULT_GBL;    break;
    case 11: vc = VALIDCHANGE_INHIB_SA;     break;
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
    case 1:  vc = VALIDCHANGE_VALID;        break;
    case 2:  vc = VALIDCHANGE_FORCED;       break;
    case 3:  vc = VALIDCHANGE_INHIB;        break;
    case 4:  vc = VALIDCHANGE_MANUAL;       break;
    case 5:  vc = VALIDCHANGE_END_INHIB;    break;
    case 6:  vc = VALIDCHANGE_END_FORCED;   break;
    case 7:  vc = VALIDCHANGE_INHIB_GBL;    break;
    case 8:  vc = VALIDCHANGE_END_INHIB_GBL;break;
    case 9:  vc = VALIDCHANGE_NULL;         break;
    case 10: vc = VALIDCHANGE_FAULT_GBL;    break;
    case 11: vc = VALIDCHANGE_INHIB_SA;     break;
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

// ======================== MINVAL ============================
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

// ======================== MAXVAL ============================
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
// TODO: Перенос атрибутов из TSC в классы с дискр. состоянием непосредственно
    //1 case TSC:
    case PIPE:  // 11
    case TL:    // 16
    case VA:    // 19
    case SC:    // 20
    case ATC:   // 21
    case GRC:   // 22
    case SV:    // 23
    case SDG:   // 24
    case RGA:   // 25
    case SSDG:  // 26
    case BRG:   // 27
    case SCP:   // 28
    case STG:   // 29
    case DIR:   // 30
    case DIPL:  // 31
    case METLINE:  // 32
    case ESDG:     // 33
    case SVLINE:   // 34
    case SCPLINE:  // 35
    case TLLINE:   // 36
    case AUX1:     // 38
    case AUX2:     // 39

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
        case TM:    // 01
        case TR:    // 02
        case ICM:   // 08
            rc = instance.ai_read(ai);
            if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

            rc = ai.VAL_get(attr_info->value.fixed.val_double);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }

            // NB: не использовать таблицу свойств атрибутов AttrTypeDescription, поскольку
            // VAL - это особый случай, он может быть целочисленным или с плав. точкой
            attr_info->type = ClassDescriptionTable[objclass].val_type;
#if defined VERBOSE
            LOG(INFO) << attr_info->name << " = " << attr_info->value.fixed.val_double; //1
#endif
            break;

        case TS:    // 00
        case TSA:   // 03
        case AL:    // 06
        case ICS:   // 07
        case PIPE:  // 11
        case TL:    // 16
        case VA:    // 19
        case SC:    // 20
        case ATC:   // 21
        case GRC:   // 22
        case SV:    // 23
        case SDG:   // 24
        case RGA:   // 25
        case SSDG:  // 26
        case BRG:   // 27
        case SCP:   // 28
        case STG:   // 29
        case DIR:   // 30
        case DIPL:  // 31
        case METLINE:  // 32
        case ESDG:     // 33
        case SVLINE:   // 34
        case SCPLINE:  // 35
        case TLLINE:   // 36
        case AUX1:     // 38
        case AUX2:     // 39
            rc = instance.di_read(di);
            if (rc) { LOG(ERROR) << "Can't read discrete part of " << attr_info->name; break; }

            rc = di.VAL_get(attr_info->value.fixed.val_uint64);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }

            // NB: не использовать таблицу свойств атрибутов AttrTypeDescription, поскольку
            // VAL - это особый случай, он может быть целочисленным или с плав. точкой
            attr_info->type = ClassDescriptionTable[objclass].val_type;
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

        case TS:    // 00
        case TSA:   // 03 
        case AL:    // 06
        case ICS:   // 07
        case PIPE:  // 11
        case TL:    // 16
        case VA:    // 19
        case SC:    // 20
        case ATC:   // 21
        case GRC:   // 22
        case SV:    // 23
        case SDG:   // 24
        case RGA:   // 25
        case SSDG:  // 26
        case BRG:   // 27
        case SCP:   // 28
        case STG:   // 29
        case DIR:   // 30
        case DIPL:  // 31
        case METLINE:  // 32
        case ESDG:     // 33
        case SVLINE:   // 34
        case SCPLINE:  // 35
        case TLLINE:   // 36
        case AUX1:     // 38
        case AUX2:     // 39
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

    case TS:    // 00
    case TSA:   // 03
    case AL:    // 06
    case ICS:   // 07
    case PIPE:  // 11
    case TL:    // 16
    case VA:    // 19
    case SC:    // 20
    case ATC:   // 21
    case GRC:   // 22
    case SV:    // 23
    case SDG:   // 24
    case RGA:   // 25
    case SSDG:  // 26
    case BRG:   // 27
    case SCP:   // 28
    case STG:   // 29
    case METLINE:  // 32
    case ESDG:     // 33
    case SVLINE:   // 34
    case SCPLINE:  // 35
    case TLLINE:   // 36
    case AUX1:     // 38
    case AUX2:     // 39
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
        case TM:    // 01
        case TR:    // 02
        case ICM:   // 08
            rc = instance.ai_read(ai);
            if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

            rc = ai.VALACQ_get(attr_info->value.fixed.val_double);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }

            // NB: не использовать таблицу свойств атрибутов AttrTypeDescription, поскольку
            // VALACQ - это особый случай, он может быть целочисленным или с плав. точкой
            attr_info->type = DB_TYPE_DOUBLE;
            break;

        case TS:    // 00
        case TSA:   // 03
        case AL:    // 06
        case ICS:   // 07
        case PIPE:  // 11
        case TL:    // 16
        case VA:    // 19
        case SC:    // 20
        case ATC:   // 21
        case GRC:   // 22
        case SV:    // 23
        case SDG:   // 24
        case RGA:   // 25
        case SSDG:  // 26
        case BRG:   // 27
        case SCP:   // 28
        case STG:   // 29
        case METLINE:  // 32
        case ESDG:     // 33
        case SVLINE:   // 34
        case SCPLINE:  // 35
        case TLLINE:   // 36
        case AUX1:     // 38
        case AUX2:     // 39
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
        case TM:    // 01
        case TR:    // 02
        case ICM:   // 08
            rc = instance.ai_read(ai);
            if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

            rc = ai.VALACQ_put(attr_info->value.fixed.val_double);
            if (rc) { LOG(ERROR) << "Can't write " << attr_info->name; break; }
            break;

        case TS:    // 00
        case TSA:   // 03
        case AL:    // 06
        case ICS:   // 07
        case PIPE:  // 11
        case TL:    // 16
        case VA:    // 19
        case SC:    // 20
        case ATC:   // 21
        case GRC:   // 22
        case SV:    // 23
        case SDG:   // 24
        case RGA:   // 25
        case SSDG:  // 26
        case BRG:   // 27
        case SCP:   // 28
        case STG:   // 29
        case METLINE:  // 32
        case ESDG:     // 33
        case SVLINE:   // 34
        case SCPLINE:  // 35
        case TLLINE:   // 36
        case AUX1:     // 38
        case AUX2:     // 39
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
    case TM:    // 01
        double_value = atof(attr.value().c_str());
        rc = instance->AIT().VALMANUAL_put(double_value);
        break;

    case TS:    // 00
    case TSA:   // 03
    case AL:    // 06
    case ICS:   // 07
    case PIPE:  // 11
    case TL:    // 16
    case VA:    // 19
    case SC:    // 20
    case ATC:   // 21
    case GRC:   // 22
    case SV:    // 23
    case SDG:   // 24
    case RGA:   // 25
    case SSDG:  // 26
    case BRG:   // 27
    case SCP:   // 28
    case STG:   // 29
    case METLINE:  // 32
    case ESDG:     // 33
    case SVLINE:   // 34
    case SCPLINE:  // 35
    case TLLINE:   // 36
    case AUX1:     // 38
    case AUX2:     // 39
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

        case TS:    // 00
        case TSA:   // 03
        case AL:    // 06
        case ICS:   // 07
        case PIPE:  // 11
        case TL:    // 16
        case VA:    // 19
        case SC:    // 20
        case ATC:   // 21
        case GRC:   // 22
        case SV:    // 23
        case SDG:   // 24
        case RGA:   // 25
        case SSDG:  // 26
        case BRG:   // 27
        case SCP:   // 28
        case STG:   // 29
        case METLINE:  // 32
        case ESDG:     // 33
        case SVLINE:   // 34
        case SCPLINE:  // 35
        case TLLINE:   // 36
        case AUX1:     // 38
        case AUX2:     // 39
            rc = instance.di_read(di);
            if (rc) { LOG(ERROR) << "Can't read discrete part of " << attr_info->name; break; }

            rc = di.VALMANUAL_get(attr_info->value.fixed.val_uint64);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }

            // NB: не использовать таблицу свойств атрибутов AttrTypeDescription, поскольку
            // VALMANUAL - это особый случай, он может быть целочисленным или с плав. точкой
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

        case TS:    // 00
        case TSA:   // 03
        case AL:    // 06
        case ICS:   // 07
        case PIPE:  // 11
        case TL:    // 16
        case VA:    // 19
        case SC:    // 20
        case ATC:   // 21
        case GRC:   // 22
        case SV:    // 23
        case SDG:   // 24
        case RGA:   // 25
        case SSDG:  // 26
        case BRG:   // 27
        case SCP:   // 28
        case STG:   // 29
        case METLINE:  // 32
        case ESDG:     // 33
        case SVLINE:   // 34
        case SCPLINE:  // 35
        case TLLINE:   // 36
        case AUX1:     // 38
        case AUX2:     // 39
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

// ======================== HISTOTYPE ============================
MCO_RET DatabaseRtapImpl::createHISTOTYPE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_HISTOTYPE;
  MCO_RET rc = MCO_S_NOTFOUND;
  HistoryType histotype;
  int given_type;

  switch(instance->objclass())
  {
    case TM:    // 01
    case ICS:   // 07
    case ICM:   // 08
        // NB: Значение HISTOTYPE в RTAP м.б. { -1, 1, 2, 3, ...}
        given_type = atoi(attr.value().c_str());
        if ((PER_NONE >= given_type) && (given_type <= PER_MONTH)) {
          histotype = decoder[given_type].htype;
        }
        else {
          histotype = PER_NONE;
#ifdef VERBOSE
          LOG(WARNING) << "Unsupported value=" << given_type
                       << " for " << instance->tag() << "." << attr_name;
#endif
        }
        rc = instance->AIT().HISTOTYPE_put(histotype);
        if (rc) { LOG(ERROR) << "Can't write " << instance->tag(); break; }

        break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::readHISTOTYPE(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_HISTOTYPE;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::AnalogInfoType   ai;
  HistoryType histotype;
  objclass_t objclass;

  do
  {
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    switch(objclass)
    {
      case TM:    // 01
      case ICS:   // 07
      case ICM:   // 08
        attr_info->value.fixed.val_uint16 = PER_NONE;

        rc = instance.ai_read(ai);
        if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

        rc = ai.HISTOTYPE_get(histotype);
        if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }
        attr_info->value.fixed.val_uint16 = static_cast<uint16_t>(histotype);
        attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_HISTOTYPE].type;
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

MCO_RET DatabaseRtapImpl::writeHISTOTYPE(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_HISTOTYPE;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::AnalogInfoType   ai;
  HistoryType histotype = PER_NONE;
  objclass_t objclass;

  assert(attr_info->type == AttrTypeDescription[RTDB_ATT_IDX_HISTOTYPE].type);
  do
  {
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    switch(objclass)
    {
      case TM:    // 01
      case ICS:   // 07
      case ICM:   // 08
        rc = instance.ai_read(ai);
        if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

        if ((PER_NONE >= attr_info->value.fixed.val_uint16)
        && (attr_info->value.fixed.val_uint16 <= PER_MONTH)) {
          histotype = decoder[attr_info->value.fixed.val_uint16].htype;
          rc = ai.HISTOTYPE_put(histotype);
          if (rc) { LOG(ERROR) << "Can't write " << attr_info->name; break; }
        }
        else {
          LOG(ERROR) << "Unsupported value=" << attr_info->value.fixed.val_uint16
                     << " for " << attr_info->name << "." << attr_name;
        }
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
// Предыдущее значение ТИ
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

// ======================== ALINHIB ============================
// Запрет генерации Алармов по данной Точке
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

// ======================== MNVALPHY ============================
// Минимальный физический диапазон
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

// ======================== MXVALPHY ============================
// Максимальный физический диапазон
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

// ======================== DISPP ============================
// Текущий диспетчер
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

MCO_RET DatabaseRtapImpl::readDISPP(mco_trans_h& t,
            rtap_db::XDBPoint& instance,
            AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  objclass_t objclass;
  autoid_t passport_aid;
  rtap_db::SITE_passport passport_instance;

  assert(attr_info);
  check_user_defined_type(RTDB_ATT_IDX_DISPP, attr_info);

  do
  {
    // NB: Атрибут допустим только для Точки SITE
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for '"<<attr_info->name<<"', rc="<<rc; break; }

    switch(objclass)
    {
      case SITE:
        // 1. Найти ссылку на паспорт SITE_passport
        rc = instance.passport_ref_get(passport_aid);
        if (rc) { LOG(ERROR) << "Get passport reference for '"<<attr_info->name<<"', rc"<<rc; break; }

        // 2. Найти по ранее полученной ссылке SITE_passport
        rc = rtap_db::SITE_passport::autoid::find(t, passport_aid, passport_instance);
        if (rc) { LOG(ERROR) << "Locate passport for '"<<attr_info->name<<"', rc="<<rc; break; }

        // 3. Прочитать у полученного экземпляра паспорта нужный атрибут
        rc = passport_instance.DISPP_get(attr_info->value.dynamic.varchar, attr_info->value.dynamic.size);
        if (rc) { LOG(ERROR) << "Read '"<<attr_info->name<<"', rc="<<rc; break; }
#if defined VERBOSE
        LOG(INFO) << attr_info->name << " = " << attr_info->value.dynamic.varchar;
#endif
      break;

      default:
        LOG(ERROR) << "'" << attr_info->name
                   << "' for objclass " << objclass
                   << " is not supported";
    }

  } while (false);

  return rc;
}

// ======================== INHIB ============================
// Признак нахождения оборудования в запрете использования или сбора данных (?)
MCO_RET DatabaseRtapImpl::createINHIB(PointInDatabase* instance, rtap_db::Attrib& attr)
{
//  static const char *attr_name = RTDB_ATT_INHIB;
  MCO_RET rc = MCO_S_NOTFOUND;
  Boolean result;
  uint1   value = atoi(attr.value().c_str());

  switch (value)
  {
    case 0: result = FALSE; break;
    case 1:
    default: result = TRUE;
  }

  rc = instance->xdbpoint().INHIB_put(result);
  return rc;
}

MCO_RET DatabaseRtapImpl::readINHIB(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  Boolean result = FALSE;

  rc = instance.INHIB_get(result);

  switch (result)
  {
    case FALSE: attr_info->value.fixed.val_bool = false; break;
    case TRUE:  attr_info->value.fixed.val_bool = true;  break;
  }

  attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_INHIB].type;

#if defined VERBOSE
  LOG(INFO) << attr_info->name << " = " << (unsigned int)attr_info->value.fixed.val_bool;
#endif

  return rc;
}

MCO_RET DatabaseRtapImpl::writeINHIB(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  Boolean result = FALSE;

  assert(attr_info->type == AttrTypeDescription[RTDB_ATT_IDX_INHIB].type);

  switch (attr_info->value.fixed.val_bool)
  {
    case false: result = FALSE; break;
    case true:  result = TRUE;  break;
  }

  rc = instance.INHIB_put(result);

  return rc;
}

// ======================== INHIBLOCAL ============================
// Признак нахождения оборудования в запрете использования или сбора данных (?)
MCO_RET DatabaseRtapImpl::createINHIBLOCAL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
//  static const char *attr_name = RTDB_ATT_INHIBLOCAL;
  MCO_RET rc = MCO_S_NOTFOUND;
  Boolean result;
  uint1   value = atoi(attr.value().c_str());

  switch (value)
  {
    case 0: result = FALSE; break;
    case 1:
    default: result = TRUE;
  }

  rc = instance->xdbpoint().INHIBLOCAL_put(result);
  return rc;
}

MCO_RET DatabaseRtapImpl::readINHIBLOCAL(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  Boolean result = FALSE;

  rc = instance.INHIBLOCAL_get(result);

  switch (result)
  {
    case FALSE: attr_info->value.fixed.val_bool = false; break;
    case TRUE:  attr_info->value.fixed.val_bool = true;  break;
  }

  attr_info->type = AttrTypeDescription[RTDB_ATT_IDX_INHIBLOCAL].type;

#if defined VERBOSE
  LOG(INFO) << attr_info->name << " = " << (unsigned int)attr_info->value.fixed.val_bool;
#endif

  return rc;
}

MCO_RET DatabaseRtapImpl::writeINHIBLOCAL(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  MCO_RET rc;
  Boolean result = FALSE;

  assert(attr_info->type == AttrTypeDescription[RTDB_ATT_IDX_INHIBLOCAL].type);

  switch (attr_info->value.fixed.val_bool)
  {
    case false: result = FALSE; break;
    case true:  result = TRUE;  break;
  }

  rc = instance.INHIBLOCAL_put(result);

  return rc;
}


// ======================== L_TL ============================
// Ссылка на объект "Линия Передачи"
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

// ======================== L_TM ============================
// Ссылка на объект "Телеизмерение"
MCO_RET DatabaseRtapImpl::createL_TM(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_TM;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case TR:
    rc = instance->passport().tr->L_TM_put(attr.value().c_str(),
                                             static_cast<uint2>(attr.value().size()));
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

// ======================== L_NETTYPE ============================
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

// ======================== L_NET ============================
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

// ======================== L_EQTORBORUPS ============================
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

// ======================== L_EQTORBORDWN ============================
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

// ======================== NMPRESSURE ============================
// Номинальное давление (nominal pressure)
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

// ======================== KMREFUPS ============================
// Расстояние до следующего технологического объекта
// (Left technological object kilometer reference)
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

// ======================== KMREFDWN ============================
// Расстояние до предыдущего технологического объекта
// (Right technological object kilometer reference)
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

// ======================== SUPPLIER ============================
// Поставщик
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


// ======================== SUPPLIERMODE ============================
// Полученный режим работы Поставщика
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

// ======================== SUPPLIERSTATE ============================
// Полученное состояние работы Поставщика
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

// ======================== EXPMODE ============================
// Числовое состояние работы Поставщика - техобслуживание (0) или работа (1)
MCO_RET DatabaseRtapImpl::createEXPMODE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_EXPMODE;
  MCO_RET rc = MCO_S_NOTFOUND;
  Boolean result;
  uint1   value = atoi(attr.value().c_str());

  switch(instance->objclass())
  {
    case SA:
    rc = instance->passport().sa->EXPMODE_put((value == 0)? 0 : 1);
    break;

    default:
    LOG(ERROR) << "'" << attr_name
               << "' for objclass " << instance->objclass()
               << " is not supported";
  }
  return rc;
}

// ======================== L_CONSUMER ============================
// Ссылка на Потребителя
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

// ======================== PLANFLOW ============================
// Плановая величина Расхода
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

// ======================== PLANPRESSURE ============================
// Плановая величина Давления
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
// Описание назначения (функции) объекта "Газпровод" или "Кран"
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
// Коэффициент конвертации значения
// TODO: Для чего используется CONVERTCOEFF?
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

// ======================== GRADLEVEL ============================
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


// ======================== MXPRESSURE ============================
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

// ======================== NMFLOW ============================
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

// ======================== MXFLOW ============================
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


// ======================== SYNTHSTATE ============================
MCO_RET DatabaseRtapImpl::createSYNTHSTATE(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_SYNTHSTATE;
  MCO_RET rc = MCO_S_NOTFOUND;
  SynthState state;
  uint1 value;

  switch(instance->objclass())
  {
    case SA:
      value = atoi(attr.value().c_str());
      switch (value)
      {
        case 0: state = SS_UNREACH;  break;
        case 1: state = SS_OPER;     break;
        case 2: state = SS_PRE_OPER; break;
        default:
            LOG(WARNING) << "Unknown '" << attr_name << "' value (" << value << ")";
            state = SS_UNREACH;
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

// ======================== DELEGABLE ============================
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

// ======================== FLGREMOTECMD ============================
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

// ======================== FLGMAINTENANCE ============================
// Признак нахождения оборудования на Техобслуживании
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

// ======================== NAMEMAINTENANCE ============================
// Фамилия ответственного за перевод оборудования в Техобслуживание
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

// ======================== ALARMBEGIN ============================
MCO_RET DatabaseRtapImpl::createALARMBEGIN(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_ALARMBEGIN;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint4 value = atoi(attr.value().c_str());

  switch(instance->objclass())
  {
    case TS:
    case TM:
    //1 case TSC:
    case VA:
    case ATC:
    case AUX1:
    case AUX2:
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

// ======================== ALARMBEGINACK ============================
MCO_RET DatabaseRtapImpl::createALARMBEGINACK(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_ALARMBEGINACK;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint4 value = atoi(attr.value().c_str());

  switch(instance->objclass())
  {
    case TS:
    case TM:
    //1 case TSC:
        case VA:
        case ATC:
        case AUX1:
        case AUX2:
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

// ======================== ALARMENDACK ============================
MCO_RET DatabaseRtapImpl::createALARMENDACK(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_ALARMENDACK;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint4 value = atoi(attr.value().c_str());

  switch(instance->objclass())
  {
    case TS:
    case TM:
    //1 case TSC:
        case VA:
        case ATC:
        case AUX1:
        case AUX2:
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

// ======================== ALARMSYNTH ============================
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
    //1 case TSC:
        case VA:
        case ATC:
        case AUX1:
        case AUX2:
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

// ======================== L_TYPINFO ============================
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
//1    case TSC: rc = instance->passport().tsc->L_TYPINFO_put(attr.value().c_str(), sz); break;
    case VA:  rc = instance->passport().valve->L_TYPINFO_put(attr.value().c_str(),  sz); break;
    case ATC: rc = instance->passport().atc->L_TYPINFO_put(attr.value().c_str(), sz); break;
    case AUX1:rc = instance->passport().aux1->L_TYPINFO_put(attr.value().c_str(),sz); break;
    case AUX2:rc = instance->passport().aux2->L_TYPINFO_put(attr.value().c_str(),sz); break;
    case TC:  rc = instance->passport().tc->L_TYPINFO_put(attr.value().c_str(),  sz); break;
    case AL:  rc = instance->passport().al->L_TYPINFO_put(attr.value().c_str(),  sz); break;
    case ICS: rc = instance->passport().ics->L_TYPINFO_put(attr.value().c_str(), sz); break;
    case ICM: rc = instance->passport().icm->L_TYPINFO_put(attr.value().c_str(), sz); break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

// ======================== L_EQT ============================
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
    //1 case TSC:     rc = instance->passport().tsc->L_EQT_put(attr.value().c_str(), sz);     break;
//    case VA:  rc = instance->passport().valve->L_EQT_put(attr.value().c_str(), sz); break;
//    case ATC: rc = instance->passport().atc->L_EQT_put(attr.value().c_str(), sz); break;
//    case AUX1:rc = instance->passport().aux1->L_EQT_put(attr.value().c_str(),sz); break;
//    case AUX2:rc = instance->passport().aux2->L_EQT_put(attr.value().c_str(),sz); break;
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

// ======================== REMOTECONTROL ============================
// Признак текущего уровня управления оборудованием (0 - местный / 1 - дистанционный)
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

MCO_RET DatabaseRtapImpl::createVAL_LABEL(PointInDatabase* /* instance */, rtap_db::Attrib& /* attr */)
{
  // NB: Пропустим этот атрибут
  return MCO_S_OK;
}

#if 0
// ======================== LINK_HIST ============================
MCO_RET DatabaseRtapImpl::createLINK_HIST(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_LINK_HIST;
  MCO_RET rc = MCO_S_NOTFOUND;
  uint2 history_index = atoi(attr.value().c_str());

  switch(instance->objclass())
  {
    case TM:
    case ICS:
    case ICM:
        rc = instance->AIT().LINK_HIST_put(history_index);
        break;

    default:
        LOG(ERROR) << "'" << attr_name
                   << "' for objclass " << instance->objclass()
                   << " is not supported";
  }
  return rc;
}

MCO_RET DatabaseRtapImpl::readLINK_HIST(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_LINK_HIST;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::AnalogInfoType   ai;
  objclass_t objclass;
  int2 index;

  // Инициализация индекса, на случай аварийного возврата до чтения
  attr_info->value.fixed.val_uint16 = 0;

  do
  {
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    switch(objclass)
    {
        case TM:    // 01
        case ICS:   // 07
        case ICM:   // 08
            rc = instance.ai_read(ai);
            if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

            rc = ai.LINK_HIST_get(index);
            if (rc) { LOG(ERROR) << "Can't read " << attr_info->name; break; }
            attr_info->value.fixed.val_uint16 = index;
            attr_info->type = DB_TYPE_UINT16;
#if defined VERBOSE
            LOG(INFO) << attr_info->name << " = " << attr_info->value.fixed.val_uint16; //1
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

MCO_RET DatabaseRtapImpl::writeLINK_HIST(mco_trans_h& t, rtap_db::XDBPoint& instance, AttributeInfo_t* attr_info)
{
  static const char *attr_name = RTDB_ATT_LINK_HIST;
  MCO_RET rc = MCO_S_NOTFOUND;
  rtap_db::AnalogInfoType   ai;
  objclass_t objclass;

  do
  {
    rc = instance.OBJCLASS_get(objclass);
    if (rc) { LOG(ERROR) << "Can't get objclass for " << attr_info->name; break; }

    switch(objclass)
    {
        case TM:    // 01
        case ICS:   // 07
        case ICM:   // 08
            rc = instance.ai_read(ai);
            if (rc) { LOG(ERROR) << "Can't read analog part of " << attr_info->name; break; }

            rc = ai.LINK_HIST_put(attr_info->value.fixed.val_uint16);
            if (rc) { LOG(ERROR) << "Can't write " << attr_info->name; break; }
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
#endif

// ======================== L_DIPL ============================
MCO_RET DatabaseRtapImpl::createL_DIPL(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  static const char *attr_name = RTDB_ATT_L_DIPL;
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(instance->objclass())
  {
    case TS:  rc = instance->passport().ts->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case TM:  rc = instance->passport().tm->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case TSA: rc = instance->passport().tsa->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    /*
    //1
    case TSC: rc = instance->passport().tsc->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    */
    case VA:  rc = instance->passport().valve->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case ATC: rc = instance->passport().atc->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case AUX1:rc = instance->passport().aux1->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case AUX2:rc = instance->passport().aux2->L_DIPL_put(attr.value().c_str(),
                                                      attr.value().size());
    break;
    case AL:  rc = instance->passport().al->L_DIPL_put(attr.value().c_str(),
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
    //1 case TSC:rc = instance->passport().tsc->ALDEST_put(result); break;
    case VA: rc = instance->passport().valve->ALDEST_put(result);  break;
    case ATC:rc = instance->passport().atc->ALDEST_put(result); break;
    case AUX1: rc= instance->passport().aux1->ALDEST_put(result); break;
    case AUX2: rc= instance->passport().aux2->ALDEST_put(result); break;
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

// ======================== UNITYCATEG ============================
// По заданной строке UNITY найти идентификатор категории, и подставить в поле UNITYCATEG
MCO_RET DatabaseRtapImpl::createUNITYCATEG(PointInDatabase* instance, rtap_db::Attrib& attr)
{
  MCO_RET rc = MCO_S_OK;
  // Это поле является фиктивным, чтение и запись фактически осуществляется
  // из таблицы НСИ DICT_UNITY_ID, поле DimensionID 
  //
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

