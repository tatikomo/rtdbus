#pragma once
#ifndef XDB_DATABASE_SERVICE_HPP
#define XDB_DATABASE_SERVICE_HPP

#include <stdint.h>
#include "config.h"

namespace xdb {

/*
 * Содержит
 * 1. Название сервиса
 * 1. список Обработчиков (Worker-ов)
 * 2. список Команд 
 */
class Service
{
  public:
    static const short NameMaxLen = SERVICE_NAME_MAXLEN;
    static const short EndpointMaxLen = ENDPOINT_MAXLEN;

    // NB: изменения синхронизировать с ServiceState из broker_db.mco
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
    void SetENDPOINT(const char*);
    void SetSTATE(State);
    void SetVALID();
    int64_t GetID();
    const char   *GetNAME() const;
    const char   *GetENDPOINT() const;
    State         GetSTATE() const;
    bool          GetVALID() const;

  private:
    // Заблокировать конструктор копирования
    DISALLOW_COPY_AND_ASSIGN(Service);
    int64_t  m_id;
    char    *m_name;
    char    *m_endpoint;
    State    m_state;
    bool     m_modified;
};

} //namespace xdb

#endif

