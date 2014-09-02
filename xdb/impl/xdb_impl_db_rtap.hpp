#pragma once
#ifndef XDB_DATABASE_RTAP_IMPL_HPP
#define XDB_DATABASE_RTAP_IMPL_HPP

#include <string>
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
#include "xdb_impl_common.h"
#include "xdb_impl_common.hpp"

namespace xdb {

class DatabaseImpl;

void errhandler(MCO_RET);
void extended_errhandler(MCO_RET errcode, const char* file, int line);

/* Фактическая реализация функциональности Базы Данных для Брокера */
class DatabaseRtapImpl
{
  friend class DatabaseImpl;

  public:
    DatabaseRtapImpl(const char*);
    ~DatabaseRtapImpl();

    bool Init();
    // Создание экземпляра БД или подключение к уже существующему
    bool Connect();
    bool Disconnect();
    //
    DBState_t State();
    //
    const char* getName() { return m_impl->getName(); };
    // Вернуть последнюю ошибку
    const Error& getLastError() const  { return m_impl->getLastError(); };
    // Вернуть признак отсутствия ошибки
    bool  ifErrorOccured() const { return m_impl->ifErrorOccured(); };
    // Установить новое состояние ошибки
    void  setError(ErrorCode_t code) { m_impl->setError(code); };
    // Сбросить ошибку
    void  clearError();

    /* Загрузка БД из указанного XML-файла */
    bool LoadSnapshot(const char*);
    /* Сохранение БД в указанный XML-файл */
    bool MakeSnapshot(const char*);

  private:
    DISALLOW_COPY_AND_ASSIGN(DatabaseRtapImpl);

    DatabaseImpl            *m_impl;

    /*
     * Зарегистрировать все обработчики событий, заявленные в БД
     */
    MCO_RET RegisterEvents();
    /*
     * Автоматически вызываемые события при работе с экземплярами Point
     */
    static MCO_RET new_Point(mco_trans_h, /*XDBService*,*/ MCO_EVENT_TYPE, void*);
    static MCO_RET del_Point(mco_trans_h, /*XDBService*,*/ MCO_EVENT_TYPE, void*);
    /*
     * Подключиться к БД, а при ее отсутствии - создать
     */
    bool  AttachToInstance();
};

} //namespace xdb
#endif
