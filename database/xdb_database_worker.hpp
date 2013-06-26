#if !defined XDB_DATABASE_WORKER_HPP
#define XDB_DATABASE_WORKER_HPP
#pragma once

#include <stdint.h>
#include <sys/time.h>

#include "config.h"
#include "helper.hpp"

/*
 * TODO: перенести константу внутрь определения класса
 * 10 секунд между двумя heartbeat
 */
#define WORKER_HEARTBEAT_PERIOD_VALUE 10

/*
 * Содержит
 * 1. Название Обработчика
 * 2. Строковый идентификатор
 * 3. Свое состояние
 */
class Worker
{
  public:
    // NB: создан на основе WorkerState из генерируемого dat/xdb_broker.h
    enum State {
        ARMED       = 0,
        DISARMED    = 1,
        INIT        = 2,
        SHUTDOWN    = 3,
        IN_PROCESS  = 4,
        EXPIRED     = 5
    };

    Worker();
    /* 
     * NB: размер поля данных идентификатора 
     * задан в структуре point_oid файла broker.mco
     */
    Worker(const char *identity, const int64_t service_id);
    ~Worker();
    void SetIDENTITY(const char*);
    void SetEXPIRATION(const timer_mark_t&);
    void SetSTATE(const State);
    const timer_mark_t GetEXPIRATION();
    const State   GetSTATE();
    const int64_t GetSERVICE_ID();
    const char   *GetIDENTITY();
    bool          Expired();

  private:
    int64_t  m_service_id;
    char    *m_identity;
    bool     m_modified;
    State    m_state;
    timer_mark_t m_expiration;

    void     SetSERVICE_ID(int64_t);
};

#endif

