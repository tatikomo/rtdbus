#if !defined XDB_DATABASE_WORKER_HPP
#define XDB_DATABASE_WORKER_HPP
#pragma once

#include <stdint.h>
#include <sys/time.h>

#include "config.h"
#include "helper.hpp"

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
    static const int HEARTBEAT_PERIOD_VALUE = HEARTBEAT_PERIOD;
    /* NB: должен быть размер поля identity_t из broker.mco */
    static const int IDENTITY_MAXLEN = WORKER_IDENTITY_MAXLEN;

    // NB: создан на основе WorkerState из генерируемого dat/xdb_broker.h
    enum State {
        DISARMED    = 0,
        ARMED       = 1,
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
    Worker(const char*, const int64_t, const char* = NULL);
    ~Worker();
    void SetIDENTITY(const char*);
    void SetSERVICE_NAME(const char*);
    void SetEXPIRATION(const timer_mark_t&);
    /*
     * Текущее состояние Обработчика
     */
    void SetSTATE(const State);
    /*
     * Номер в спуле своего Сервиса
     */
    void SetINDEX(const uint16_t&);
    void SetVALID();
    const timer_mark_t GetEXPIRATION();
    const State   GetSTATE();
    const int64_t GetSERVICE_ID();
    const char   *GetIDENTITY();
    const char   *GetSERVICE_NAME();
    const uint16_t GetINDEX();
    bool          Expired();
    /*
     * Определяет консистентность
     * true:  объект загружен из БД, ручных модификаций не было;
     * false: объект создан вручную, может полностью 
     *        или частично несоответствовать базе;
     */
    bool          GetVALID();

  private:
    int64_t  m_service_id;
    char     m_identity[WORKER_IDENTITY_MAXLEN + 1];
    uint16_t m_index_in_spool;
    bool     m_modified;
    State    m_state;
    timer_mark_t m_expiration;
    char     m_service_name[SERVICE_NAME_MAXLEN + 1];

    void     SetSERVICE_ID(int64_t);
};

#endif

