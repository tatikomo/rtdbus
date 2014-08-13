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

    // NB: создан на основе WorkerState из генерируемого dat/xdb_broker.h
    enum State {
        DISARMED    = 0,
        ARMED       = 1,
        INIT        = 2,
        SHUTDOWN    = 3,
        OCCUPIED    = 4,
        EXPIRED     = 5
    };

    Worker();
    /* 
     * NB: размер поля данных идентификатора 
     * задан в структуре point_oid файла broker.mco
     */
    Worker(const int64_t, const char*, const int64_t=0);
    ~Worker();
    void SetIDENTITY(const char*);
    void SetEXPIRATION(const timer_mark_t&);
    void SetID(const int64_t);
    /*
     * Текущее состояние Обработчика
     */
    void SetSTATE(const State);
    void SetVALID();
    timer_mark_t GetEXPIRATION() const;
    State   GetSTATE() const;
    int64_t GetSERVICE_ID() const { return m_service_id; }
    int64_t GetID() const        { return m_id; }
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

    int64_t  m_id;
    int64_t  m_service_id;
    char     m_identity[IDENTITY_MAXLEN + 1];
    State    m_state;
    timer_mark_t m_expiration;
    bool     m_modified;
};

} //namespace xdb

#endif

