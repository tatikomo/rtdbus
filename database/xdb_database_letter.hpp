#if !defined XDB_DATABASE_LETTER_HPP
#define XDB_DATABASE_LETTER_HPP
#pragma once

#include <iostream>
#include <stdint.h>
#include <string>

#include "config.h"
#include "helper.hpp"
#include "msg_common.h"
#include "msg_message.hpp"

namespace xdb {

class Service;
class Worker;

/*
 */
class Letter
{
  public:
    static const int ExpirationPeriodValue = MESSAGE_EXPIRATION_PERIOD_MSEC;

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
    Letter(int64_t, int64_t, int64_t);
    // Создать экземпляр на основе двух последних фреймов сообщения
    Letter(void*);
    // Создать экземпляр на основе обратного адреса, заголовка и тела сообщения
    Letter(std::string&, msg::Header*, std::string&);
    // Создать экземпляр на основе обратного адреса, заголовка и тела сообщения
    Letter(const char*, const std::string&, const std::string&);

    ~Letter();
    void SetID(int64_t);
    void SetSERVICE(Service*);
    void SetSERVICE_ID(int64_t);
    void SetWORKER(Worker*);
    void SetWORKER_ID(int64_t);
    void SetSTATE(State);
//    void SetREPLY_TO(const std::string&);
    void SetREPLY_TO(const char*);
    void SetHEADER(const std::string&);
    void SetDATA(const std::string&);
    void SetEXPIRATION(const timer_mark_t&);
    void SetVALID();
    bool GetVALID();
    int64_t GetID() const;
    int64_t GetSERVICE_ID() const;
    int64_t GetWORKER_ID() const;
    const char* GetREPLY_TO() const { return m_reply_to; }
    const std::string& GetHEADER();
    const std::string& GetDATA();
    Letter::State GetSTATE();

    friend std::ostream& operator<<(std::ostream&, const Letter&);
    void Dump();

    //
    // Поля Заголовка Сообщения
    int8_t GetPROTOCOL_VERSION() const;
    rtdbExchangeId GetEXCHANGE_ID() const;
    rtdbPid GetSOURCE_PID() const;
    const std::string& GetPROC_DEST() const;
    const std::string& GetPROC_ORIGIN() const;
    rtdbMsgType GetSYS_MSG_TYPE() const;
    rtdbMsgType GetUSR_MSG_TYPE() const;


  private:
    bool     m_modified;
    int64_t  m_id;
    int64_t  m_service_id;
    int64_t  m_worker_id;
    State    m_state;
    timer_mark_t m_expiration;
    msg::Header  m_header;
    char     m_reply_to[WORKER_IDENTITY_MAXLEN + 1];
    std::string  m_frame_data;
    std::string  m_frame_header;
};

}; //namespace xdb

#endif

