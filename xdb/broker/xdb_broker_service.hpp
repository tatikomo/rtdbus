#if !defined XDB_DATABASE_SERVICE_HPP
#define XDB_DATABASE_SERVICE_HPP
#pragma once

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
    static const int NameMaxLen = SERVICE_NAME_MAXLEN;

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
    void SetSTATE(State);
    void SetVALID();
    int64_t GetID();
    const char   *GetNAME();
    State         GetSTATE();
    bool          GetVALID();

  private:
    // Заблокировать конструктор копирования
    DISALLOW_COPY_AND_ASSIGN(Service);
    int64_t  m_id;
    char    *m_name;
    State    m_state;
    bool     m_modified;
};

} //namespace xdb

#endif

