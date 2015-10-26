#pragma once
#ifndef XDB_DATABASE_RTAP_IMPL_HPP
#define XDB_DATABASE_RTAP_IMPL_HPP

#include <string>
#include <map>
#include <vector>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#include "mco.h"
#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
#include "mcohv.h"
#endif
}
#endif

#include "dat/rtap_db.hpp"
#include "dat/rtap_db.hxx"      // Структуры-хранилища данных Point
#include "dat/rtap_db_dict.hxx" // Структуры-хранилища данных НСИ
#include "xdb_common.hpp"

namespace xdb {

class DatabaseRtapImpl;

typedef union {
    void                    *root_pointer;
    rtap_db::TS_passport    *ts;
    rtap_db::TM_passport    *tm;
    rtap_db::TR_passport    *tr;
    rtap_db::TSA_passport   *tsa;
    rtap_db::TSC_passport   *tsc;
    rtap_db::TC_passport    *tc;
    rtap_db::AL_passport    *al;
    rtap_db::ICS_passport   *ics;
    rtap_db::ICM_passport   *icm;
    rtap_db::PIPE_passport  *pipe;
    rtap_db::PIPELINE_passport  *pipeline;
    rtap_db::TL_passport    *tl;
    rtap_db::SC_passport    *sc;
    rtap_db::ATC_passport   *atc;
    rtap_db::GRC_passport   *grc;
    rtap_db::SV_passport    *sv;
    rtap_db::SDG_passport   *sdg;
    rtap_db::RGA_passport   *rga;
    rtap_db::SSDG_passport  *ssdg;
    rtap_db::BRG_passport   *brg;
    rtap_db::SCP_passport   *scp;
    rtap_db::STG_passport   *stg;
    rtap_db::METLINE_passport   *metline;
    rtap_db::ESDG_passport      *esdg;
    rtap_db::SVLINE_passport    *svline;
    rtap_db::SCPLINE_passport   *scpline;
    rtap_db::TLLINE_passport    *tlline;
    rtap_db::DIR_passport       *dir;
    rtap_db::DIPL_passport      *dipl;
    rtap_db::INVT_passport      *invt;
    rtap_db::AUX1_passport      *aux1;
    rtap_db::AUX2_passport      *aux2;
    rtap_db::SA_passport        *sa;
    rtap_db::SITE_passport      *site;
    rtap_db::VA_passport        *valve;
    rtap_db::FIXEDPOINT_passport    *fixed;
} any_passport_t;

// Представление набора данных и объектов XDB
// Используется для унификации функций-создателей атрибутов
class PointInDatabase
{
  public:
    friend class DatabaseRtapImpl; // для доступа к m_passport_instance

    PointInDatabase(rtap_db::Point*);
   ~PointInDatabase();
    objclass_t objclass() { return m_objclass; };
    MCO_RET create(mco_trans_h);
    // Прочитать значения атрибутов Точки из БДРВ во внутреннее представление
    MCO_RET load(mco_trans_h);
    // Установить Дискретную часть
    MCO_RET assign(rtap_db::DiscreteInfoType& di) { return m_point.di_write(di); }; 
    // Установить Аналоговую часть
    MCO_RET assign(rtap_db::AnalogInfoType& ai) { return m_point.ai_write(ai); };
    // Установить Тревожную часть
    MCO_RET assign(rtap_db::AlarmItem& ala) { return m_point.alarm_write(ala); };
    rtap_db::XDBPoint&  xdbpoint() { return m_point; };
    const std::string&  tag() { return m_info->tag(); };
    rtap_db::Attrib&    attribute(unsigned int index) { return m_info->attrib(index); };
    rtap_db::AttributeList& attributes() { return m_info->attributes(); };
    autoid_t id()          { return m_point_aid; };
    autoid_t passport_id() { return m_passport_aid; };
    void setPassportId(autoid_t& id) { m_passport_aid = id; };
    MCO_RET  update_references();
    mco_trans_h current_transaction() { return m_transaction_handler; };
    any_passport_t& passport() { return m_passport; };
    rtap_db::AnalogInfoType& AIT();
    rtap_db::DiscreteInfoType& DIT();
    rtap_db::AlarmItem& alarm();

  private:
    DISALLOW_COPY_AND_ASSIGN(PointInDatabase);
    // Ссылки на другие таблицы БДРВ
    autoid_t            m_point_aid;
    autoid_t            m_passport_aid;
    autoid_t            m_CE_aid;
    autoid_t            m_SA_aid;
    autoid_t            m_hist_aid;
    // Код возврата функций XDB
    MCO_RET             m_rc;
    //
    rtap_db::XDBPoint   m_point;
    //
    rtap_db::Point     *m_info;
    //
    mco_trans_h         m_transaction_handler;
    // Закешированное значение класса объекта, оригинал хранится в m_info->objclass()
    objclass_t          m_objclass;
    // Аналоговая часть Точки
    rtap_db::AnalogInfoType    m_ai;
    // Дискретная часть Точки
    rtap_db::DiscreteInfoType  m_di;
    // Тревожная часть :)
    rtap_db::AlarmItem         m_alarm;
    // различные виды паспортов, специализация по objclass
    any_passport_t m_passport;
    // Признаки создания соответствующих необязательных полей XDBPoint
    bool m_is_AI_assigned;  // AnalogInformation
    bool m_is_DI_assigned;  // DiscreteInformation
    bool m_is_AL_assigned;  // AlarmInformation
};

class DatabaseImpl;
class DatabaseRtapImpl;

// Тип функции, создающей известный атрибут
// NB: полный их перечень приведен в словаре 'rtap_db.xsd'
typedef MCO_RET (DatabaseRtapImpl::*AttributeCreatingFunc_t)(PointInDatabase*, rtap_db::Attrib&);
// Тип функции, читающей или изменяющей значение атрибута
// 1. Сначала нужно найти точку, к которой этот атрибут относится - получим экземпляр rtap_db::XDBPoint;
// 2. Для данного экземпляра Точки вызовем соответствующую функцию, обрабатывающую данный атрибут.
typedef MCO_RET (DatabaseRtapImpl::*AttributeProcessingFunc_t)(mco_trans_h&, rtap_db::XDBPoint&, AttributeInfo_t*);

// Таблица соответствия названий всех известных атрибутов (1) и создающих их функций (2)
#ifdef __SUNPRO_CC
// Функции первоначального создания атрибута
typedef std::pair <std::string, AttributeCreatingFunc_t> AttrCreatingFuncPair_t;
typedef std::map  <std::string, AttributeCreatingFunc_t> AttrCreatingFuncMap_t;
// Функции чтения или записи значения атрибута
typedef std::pair <std::string, AttributeProcessingFunc_t> AttrProcessingFuncPair_t;
typedef std::map  <std::string, AttributeProcessingFunc_t> AttrProcessingFuncMap_t;
#else
// Функции первоначального создания атрибута
typedef std::pair <const std::string, AttributeCreatingFunc_t> AttrCreatingFuncPair_t;
typedef std::map  <const std::string, AttributeCreatingFunc_t> AttrCreatingFuncMap_t;
// Функции чтения или записи значения атрибута
typedef std::pair <const std::string, AttributeProcessingFunc_t> AttrProcessingFuncPair_t;
typedef std::map  <const std::string, AttributeProcessingFunc_t> AttrProcessingFuncMap_t;
#endif
typedef AttrCreatingFuncMap_t::iterator AttrCreatingFuncMapIterator_t;
typedef AttrProcessingFuncMap_t::iterator AttrProcessingFuncMapIterator_t;


void errhandler(MCO_RET);
void extended_errhandler(MCO_RET errcode, const char* file, int line);

/* Фактическая реализация функциональности Базы Данных для Брокера */
class DatabaseRtapImpl
{
  public:
    friend class DatabaseImpl;

    DatabaseRtapImpl(const char*, const Options*);
   ~DatabaseRtapImpl();

    // Работа с состоянием БД -  инициализация, подключение, ...
    // ====================================================
    // Создание экземпляра БД или подключение к уже существующему
    bool Connect();
    bool Disconnect();
    // Текущее состояние БД
    DBState_t State();
    // Название БДРВ
    const char* getName();

    // Работа с ошибками
    // ====================================================
    // Вернуть последнюю ошибку
    const Error& getLastError() const;
    // Вернуть признак отсутствия ошибки
    bool  ifErrorOccured() const;
    // Установить новое состояние ошибки
    void  setError(ErrorCode_t code);
    // Сбросить ошибку
    void  clearError();

    // Импорт/экспорт содержимого БД во внешние форматы (XML)
    // ====================================================
    // Загрузка БД из указанного XML-файла
    bool LoadSnapshot(const char*);
    // Сохранение БД в указанный XML-файл
    bool MakeSnapshot(const char*);

    // Функции изменения содержимого БД
    // NB: Для всех подобных функций, которые могут вызываться
    // из разных тредов, необходим дополнительный параметр с
    // идентификатором подключения.
    // ====================================================
    const Error& Control(mco_db_h&, rtDbCq&);
    const Error& Query(mco_db_h&, rtDbCq&);
    const Error& Config(mco_db_h&, rtDbCq&);
    // Создание Точки
    const Error& create(mco_db_h&, rtap_db::Point&);
    // Удаление Точки
    const Error& erase(mco_db_h&, rtap_db::Point&);
    // Чтение данных Точки
    const Error& read(mco_db_h&, rtap_db::Point&);
    const Error& read(mco_db_h&, AttributeInfo_t*);
    // Чтение данных группы Точек
    const Error& read(mco_db_h&, std::string&, int*, xdb::SubscriptionPoints_t*);
    // Изменение данных Точки
    const Error& write(mco_db_h&, rtap_db::Point&);
    // Запись значения атрибута для заданной точки
    const Error& write(mco_db_h&, AttributeInfo_t*);
    // Найти и вернуть значения атрибутов указанной точки
    rtap_db::Point* locate(mco_db_h&, const char* _tag);
    // Блокировка данных Точки от изменения в течение заданного времени
    const Error& lock(mco_db_h&, rtap_db::Point&, int);
    const Error& unlock(mco_db_h&, rtap_db::Point&);
    
    // Версии сервисных функций без необходимого идентификатора подключения.
    // В этом случае берется встроенный идентификатор, полученный внутри экземпляра
    // DatabaseImpl при внутреннем подключении в DatabaseImpl::ConnectToInstance().
    const Error& Control(rtDbCq&);
    const Error& Query(rtDbCq&);
    const Error& Config(rtDbCq&);

  private:
    DISALLOW_COPY_AND_ASSIGN(DatabaseRtapImpl);

    // Ссылка на объект-реализацию функциональности БД
    DatabaseImpl *m_impl;

    // Зарегистрировать все обработчики событий, заявленные в БД
    MCO_RET RegisterEvents();
    bool m_register_events;

    // Создать общую часть Точки в БД, вернуть идентификатор
    MCO_RET createPoint(PointInDatabase*);

    // Создать паспорт Точки в БД, вернуть идентификатор
    MCO_RET createPassport(PointInDatabase*);

    // Автоматически вызываемые события при работе с экземплярами Point
    static MCO_RET new_Point(mco_trans_h, XDBPoint*, MCO_EVENT_TYPE, void*);
    static MCO_RET del_Point(mco_trans_h, XDBPoint*, MCO_EVENT_TYPE, void*);
    // Событие изменения VALIDCHANGE
    static MCO_RET on_update_VALIDCHANGE(mco_trans_h, XDBPoint*, MCO_EVENT_TYPE, void*);

    // По заданному идентификатору прочитать из БДРВ значения атрибутов точки XDBPoint
    MCO_RET LoadPointInfo(mco_db_h&, mco_trans_h, autoid_t, xdb::PointDescription_t*);

    // Подключиться к БД, а при ее отсутствии - создать
    bool AttachToInstance();

    // Актуализация типа параметра в соответствии с внутреннем словарем
    // и коррекция выделенного ранее буфера для содержимого параметра,
    // поскольку функция выделения буфера не знала на тот момент о
    // фактическом типе параметра.
    void check_user_defined_type(int, AttributeInfo_t*);
    // ------------------------------------------------------------
    // Сохранить XML Scheme БДРВ
    void GenerateXSD();

    // Код СЕ для VALIDCHANGE аналоговых телеизмерений
    MCO_RET GOFVALTI(mco_trans_h,
        XDBPoint*,
        AnalogInfoType&,
        const char*,
        objclass_t,
        ValidChange,
        double&,
        timestamp&,
        double&,
        Validity,
        double&,
        Validity,
        SystemState,
        SystemState,
        bool,
        bool);
    // Код СЕ для VALIDCHANGE Тревог
    MCO_RET GOFVALAL(mco_trans_h,
        XDBPoint*,
        DiscreteInfoType&,
        const char*,
        objclass_t,
        ValidChange,
        uint64_t&,
        timestamp&,
        uint64_t&,
        Validity,
        uint64_t&,
        Validity,
        SystemState,
        SystemState);
    // Код СЕ для VALIDCHANGE Оборудования
    MCO_RET GOFVALTSC(mco_trans_h,
        XDBPoint*,
        DiscreteInfoType&,
        const char*,
        objclass_t,
        ValidChange,
        uint64_t&,
        timestamp&,
        uint64_t&,
        Validity,
        uint64_t&,
        Validity,
        SystemState,
        SystemState);

    // Связь между названием атрибута и функцией его создания
    AttrCreatingFuncMap_t     m_attr_creating_func_map;
    // Связь между названием атрибута и функцией его записи
    AttrProcessingFuncMap_t   m_attr_writing_func_map;
    // Связь между названием атрибута и функцией его чтения
    AttrProcessingFuncMap_t   m_attr_reading_func_map;

    bool AttrFuncMapInit();
    // Функции создания таблиц
    MCO_RET createTable(mco_db_h&, rtDbCq& info);
    MCO_RET createTableDICT_TSC_VAL_LABEL(mco_db_h&, rtap_db_dict::values_labels_t*);
    MCO_RET createTableDICT_UNITY_ID(mco_db_h&, rtap_db_dict::unity_labels_t*);
    MCO_RET createTableXDB_CE(mco_db_h&, rtap_db_dict::macros_def_t*);

    // ====================================================
    // Работа с группами подписки через интерфейс Config()
    // ====================================================
    // Активировать подписку
    // NB: пока (2015-10-01) нет механизма контроля за списком подписчиков на конкретную группу,
    // поэтому Клиентам нельзя управлять включением/выключением групп.
    MCO_RET enableGroup(mco_db_h&, rtDbCq&);
    // Приостановить подписку
    MCO_RET suspendGroup(mco_db_h&, rtDbCq&);
    // Создать группу атрибутов под заданным именем
    MCO_RET createGroup(mco_db_h&, rtDbCq&);
    // Удалить группу атрибутов с заданным именем
    MCO_RET deleteGroup(mco_db_h&, rtDbCq&);
    // ====================================================
    // Работа с группами подписки через интерфейс Query()
    // ====================================================
    // Получить список групп подписки с активными (модифицированными элементами)
    MCO_RET querySbsArmedGroup(mco_db_h&, rtDbCq&);
    // Сбросить флаг модифицикации атрибутов всех групп из указанного списка
    MCO_RET querySbsDisarmSelectedPoints(mco_db_h&, rtDbCq&);
    // Многофункциональная процедура, тип работы зависит от значения последнего аргумента
    // rtQUERY_SBS_POINTS_ARMED: Получить перечень активированных точек для указанной группы 
    // rtQUERY_SBS_POINTS_DISARM: Для указанной группы Точек сбросить признак модификации
    // rtQUERY_SBS_READ_POINTS_ARMED: Прочитать значения всех атрибутов указанной группы
    MCO_RET querySbsPoints(mco_db_h&, rtDbCq&, TypeOfQuery);
     
    // Главный источник переченя атрибутов - rtap_db.xsd
    // Но поскольку скриптом мне проще обрабатывать плоский текстовый
    // файл вместо XSD-файла, то названия прототипов берутся из файла attr_list.txt
    // Прототипы функций чтения/записи значений атрибутов генерируются автоматически
#include "dat/proto_attr_reading.gen"
    // TAG не является полноценным атрибутом, потому обрабатывается спец. образом
    MCO_RET readTAG(mco_trans_h&, rtap_db::XDBPoint&, AttributeInfo_t*);
    // OBJCLASS не является полноценным атрибутом, потому обрабатывается спец. образом
    MCO_RET readOBJCLASS(mco_trans_h&, rtap_db::XDBPoint&, AttributeInfo_t*);
#include "dat/proto_attr_writing.gen"
    // Функции записи значений атрибутов
    // Два этих атрибута создаются особым образом, т.к. фактически не являются атрибутами
    MCO_RET createTAG        (PointInDatabase*, rtap_db::Attrib&);
    MCO_RET createOBJCLASS   (PointInDatabase*, rtap_db::Attrib&);

    // Определение прототипов собирается автоматически
#include "dat/proto_attr_creating.gen"

};

} //namespace xdb
#endif
