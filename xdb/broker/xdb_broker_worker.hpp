#pragma once
#ifndef XDB_DATABASE_WORKER_HPP
#define XDB_DATABASE_WORKER_HPP

#include <stdint.h>
#include <sys/time.h>

#include "config.h"
#include "helper.hpp"

namespace xdb {

/*
 * Содержит
 * 1. Название Обработчика
 * 2. Строковый идентификатор
 * 3. Свое состояние
 */
class Worker
{
  public:

    //  10 секунд между двумя heartbeat
    static const int HeartbeatPeriodValue = HEARTBEAT_PERIOD_MSEC;
    /* NB: должен быть размер поля identity_t из broker.mco */
    static const int IdentityMaxLen = IDENTITY_MAXLEN;

    // NB: изменения синхронизировать с WorkerState из broker_db.mco
    enum State {
        DISARMED    = 0,
        ARMED       = 1,    // Свободен, готов к использованию
        INIT        = 2,    // 
        SHUTDOWN    = 3,    // Завершает свою работу
        OCCUPIED    = 4,    // Занят обслуживанием Запроса
        EXPIRED     = 5     // Превысил таймаут обработки Запроса
    };

    Worker();
    /* 
     * NB: размер поля данных идентификатора 
     * задан в структуре point_oid файла broker.mco
     */
    Worker(const int64_t, const char*, const int64_t=0, const int64_t=0);
    ~Worker();
    void SetIDENTITY(const char*);
    // Установить отсечку времени
    void SetEXPIRATION(const timer_mark_t&);
    void SetID(int64_t);
    void SetLETTER_ID(int64_t);
    /*
     * Текущее состояние Обработчика
     */
    void SetSTATE(const State);
    void SetVALID();
    // Получить отметку времени срока годности
    const timer_mark_t& GetEXPIRATION() const;
    // Сгенерировать новую метку срока годности
    bool CalculateEXPIRATION_TIME(timer_mark_t&);
    State   GetSTATE() const;
    int64_t GetSERVICE_ID() const { return m_service_id; }
    int64_t GetLETTER_ID() const  { return m_letter_id; }
    int64_t GetID() const         { return m_id; }
    const char   *GetIDENTITY() const;
    bool          Expired() const;
    /*
     * Определяет консистентность
     * true:  объект загружен из БД, ручных модификаций не было;
     * false: объект создан вручную, может полностью 
     *        или частично несоответствовать базе;
     */
    bool          GetVALID() const;

  private:
    // Нельзя менять привязку уже существующему серверу
    void SetSERVICE_ID(int64_t);

    int64_t  m_id;          // Собственный идентификатор
    int64_t  m_service_id;  // Идентификатор своей Службы
    int64_t  m_letter_id;   // Идентификатор исполняемого запроса
    char     m_identity[IDENTITY_MAXLEN + 1];
    State    m_state;
    timer_mark_t m_expiration;
    bool     m_modified;
};

} //namespace xdb

#endif

