#if !defined XDB_DATABASE_SERVICE_HPP
#define XDB_DATABASE_SERVICE_HPP
#pragma once

#include <stdint.h>
#include "config.h"

/*
 * Содержит
 * 1. Название сервиса
 * 1. список Обработчиков (Worker-ов)
 * 2. список Команд 
 */
class Service
{
  public:
    static const int NAME_MAXLEN = SERVICE_NAME_MAXLEN;
    static const int WORKERS_SPOOL_MAXLEN = WORKERS_SPOOL_SIZE;

    enum State
    {
        UNKNOWN    = 0,
        REGISTERED = 1,
        ACTIVATED  = 2,
        DISABLED   = 3
    };

    Service();
    /* 
     * NB: размер поля данных идентификатора 
     * задан в структуре point_oid файла broker.mco
     */
    Service(const int64_t, const char*);
    ~Service();
    void SetID(const int64_t);
    void SetNAME(const char*);
    void  SetSTATE(State);
    const int64_t GetID();
    const char   *GetNAME();
    State         GetSTATE();
    const char   *GetSERVICE_NAME();

  private:
    int64_t  m_id;
    char    *m_name;
    State    m_state;
    char    *m_service_name;
    bool     m_modified;
};

#endif

