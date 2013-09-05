#if !defined XDB_DATABASE_LETTER_HPP
#define XDB_DATABASE_LETTER_HPP
#pragma once

#include <stdint.h>
#include <string>

#include "config.h"
#include "helper.hpp"

class Service;
class Worker;
/*
 */
class Letter
{
  public:
//    static const int NAME_MAXLEN = SERVICE_NAME_MAXLEN;
//    static const int WORKERS_SPOOL_MAXLEN = WORKERS_SPOOL_SIZE;

    enum State
    {
        EMPTY = 0
      , UNASSIGNED = 1
      , ASSIGNED = 2
      , PROCESSING = 3
      , DONE_OK = 4
      , DONE_FAIL = 5
    };

    Letter();
    /* 
     * NB: размер поля данных идентификатора 
     */
    Letter(int64_t, int64_t, int64_t);
    ~Letter();
    void SetID(int64_t);
    void SetSERVICE(Service*);
    void SetSERVICE_ID(int64_t);
    void SetWORKER(Worker*);
    void SetWORKER_ID(int64_t);
    void SetSTATE(State);
    void SetHEAD(const std::string&);
    void SetBODY(const std::string&);
    void SetVALID();
    bool GetVALID();
    int64_t GetID();
    int64_t GetSERVICE_ID();
    int64_t GetWORKER_ID();
    const std::string& GetHEAD();
    const std::string& GetBODY();
    Letter::State GetSTATE();

  private:
    bool     m_modified;
    int64_t  m_id;
    int64_t  m_service_id;
    int64_t  m_worker_id;
    State    m_state;
    timer_mark_t m_expiration;
    std::string  m_head;
    std::string  m_body;
};

#endif

