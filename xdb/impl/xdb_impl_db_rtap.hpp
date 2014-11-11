#pragma once
#ifndef XDB_DATABASE_RTAP_IMPL_HPP
#define XDB_DATABASE_RTAP_IMPL_HPP

#include <string>
#include <map>
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
#include "dat/rtap_db.hxx" // Структуры-хранилища данных Point
#include "xdb_impl_common.h"
#include "xdb_impl_common.hpp"

#include "dat/rtap_db.h" // для функций-создателей атрибутов нужен доступ к MCO_Hf

namespace xdb {

class DatabaseImpl;
class DatabaseRtapImpl;

// Тип функции, создающей известный атрибут
// NB: полный их перечень приведен в словаре 'rtap_db.xsd'
typedef MCO_RET (DatabaseRtapImpl::*AttributeCreationFunc_t)(rtap_db::XDBPoint&, rtap_db::Attrib&);

// Таблица соответствия всех известных атрибутов и создающих их функций
#ifdef __SUNPRO_CC
typedef std::pair <std::string, AttributeCreationFunc_t> AttrCreationFuncPair_t;
typedef std::map  <std::string, AttributeCreationFunc_t> AttrCreationFuncMap_t;
#else
typedef std::pair <const std::string, AttributeCreationFunc_t> AttrCreationFuncPair_t;
typedef std::map  <const std::string, AttributeCreationFunc_t> AttrCreationFuncMap_t;
#endif
typedef AttrCreationFuncMap_t::iterator AttrCreationFuncMapIterator_t;


void errhandler(MCO_RET);
void extended_errhandler(MCO_RET errcode, const char* file, int line);
// Функция инициализации карты соответствий между именами атрибутов и функций, их создающих
bool AttrFuncMapInit();

/* Фактическая реализация функциональности Базы Данных для Брокера */
class DatabaseRtapImpl
{
  friend class DatabaseImpl;

  public:
    DatabaseRtapImpl(const char*);
    ~DatabaseRtapImpl();

    // Работа с состоянием БД -  инициализация, подключение, ...
    // ====================================================
    bool Init();
    // Создание экземпляра БД или подключение к уже существующему
    bool Connect();
    bool Disconnect();
    // Текущее состояние БД
    DBState_t State();

    // Работа с ошибками
    // ====================================================
    const char* getName() { return m_impl->getName(); };
    // Вернуть последнюю ошибку
    const Error& getLastError() const  { return m_impl->getLastError(); };
    // Вернуть признак отсутствия ошибки
    bool  ifErrorOccured() const { return m_impl->ifErrorOccured(); };
    // Установить новое состояние ошибки
    void  setError(ErrorCode_t code) { m_impl->setError(code); };
    // Сбросить ошибку
    void  clearError();

    // Импорт/экспорт содержимого БД во внешние форматы (XML)
    // ====================================================
    // Загрузка БД из указанного XML-файла
    bool LoadSnapshot(const char*);
    // Сохранение БД в указанный XML-файл
    bool MakeSnapshot(const char*);

    // Функции изменения содержимого БД
    // ====================================================
    const Error& Control(rtDbCq&);
    const Error& Query(rtDbCq&);
    const Error& Config(rtDbCq&);
    // Создание Точки
    const Error& create(rtap_db::Point&);
    // Удаление Точки
    const Error& erase(rtap_db::Point&);
    // Чтение данных Точки
    const Error& read(rtap_db::Point&);
    // Изменение данных Точки
    const Error& write(rtap_db::Point&);
    // Блокировка данных Точки от изменения в течение заданного времени
    const Error& lock(rtap_db::Point&, int);
    const Error& unlock(rtap_db::Point&);

  private:
    DISALLOW_COPY_AND_ASSIGN(DatabaseRtapImpl);

    // Ссылка на объект-реализацию функциональности БД
    DatabaseImpl *m_impl;

    // Зарегистрировать все обработчики событий, заявленные в БД
    MCO_RET RegisterEvents();

    // Создать общую часть Точки в БД, вернуть идентификатор
    MCO_RET createPoint(mco_trans_h,
                        rtap_db::XDBPoint&,
                        rtap_db::Point&,
                        const std::string&,
                        autoid_t&,
                        autoid_t&);

    // Создать паспорт Точки в БД, вернуть идентификатор
    MCO_RET createPassport(mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);

    // Создать TM паспорт Точки в БД, вернуть идентификатор
    MCO_RET createPassportTS    (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportTM    (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportTR    (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportTSA   (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportTSC   (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportTC    (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportAL    (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportICS   (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportICM   (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportTL    (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportVA    (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportATC   (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportGRC   (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportSV    (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportSDG   (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportSSDG  (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportSCP   (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportDIR   (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportDIPL  (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportMETLINE(mco_trans_h,rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportESDG  (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportSCPLINE(mco_trans_h,rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportTLLINE(mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportAUX1  (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportAUX2  (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportSITE  (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);
    MCO_RET createPassportSA    (mco_trans_h, rtap_db::XDBPoint&, rtap_db::Point&, autoid_t&, autoid_t&);

    // Автоматически вызываемые события при работе с экземплярами Point
    static MCO_RET new_Point(mco_trans_h, XDBPoint*, MCO_EVENT_TYPE, void*);
    static MCO_RET del_Point(mco_trans_h, XDBPoint*, MCO_EVENT_TYPE, void*);

    // Подключиться к БД, а при ее отсутствии - создать
    bool  AttachToInstance();

    // Связь между названием атрибута и функцией его создания
    AttrCreationFuncMap_t   m_attr_creation_func_map;
    bool AttrFuncMapInit();
    // Взято перечень атрибутов из rtap_db.xsd
    MCO_RET createSHORTLABEL(rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createL_SA      (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createVALID     (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createVALIDACQ  (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createDATEHOURM (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createDATERTU   (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createSTATUS    (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createVALIDCHANGE(rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createUNITY      (rtap_db::XDBPoint&, rtap_db::Attrib&);

#if 0
    MCO_RET createMINVAL    (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createVALEX     (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createMAXVAL    (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createVAL       (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createVALACQ    (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createVALMANUAL (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createALINHIB   (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createDISPP     (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createFUNCTION  (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createCONVERTCOEFF  (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createLABEL     (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createINHIB     (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createMNVALPHY  (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createMXPRESSURE(rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createMXVALPHY  (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createNMFLOW    (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createSUPPLIERMODE  (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createSUPPLIERSTATE (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createSYNTHSTATE    (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createDELEGABLE (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createETGCODE   (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createSITEFL    (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createFLGREMOTECMD   (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createFLGMAINTENANCE (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createNAMEMAINTENANCE(rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createTSSYNTHETICAL  (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createLOCALFLAG      (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createALARMBEGIN     (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createALARMBEGINACK  (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createALARMENDACK    (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createALARMSYNTH     (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createL_TYPINFO  (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createL_EQT      (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createREMOTECONTROL (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createNONEXE     (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createRESPNEG    (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createACTIONTYP  (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createVAL_LABEL  (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createLINK_HIST  (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createACQMOD     (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createL_DIPL     (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createALDEST     (rtap_db::XDBPoint&, rtap_db::Attrib&);
    MCO_RET createINHIBLOCAL (rtap_db::XDBPoint&, rtap_db::Attrib&);
#endif
};

} //namespace xdb
#endif
