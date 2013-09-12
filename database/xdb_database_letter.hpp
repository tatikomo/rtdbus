#if !defined XDB_DATABASE_LETTER_HPP
#define XDB_DATABASE_LETTER_HPP
#pragma once

#include <stdint.h>
#include <string>

#include "config.h"
#include "helper.hpp"
#include "msg_common.h"

class Service;
class Worker;
class RTDBUS_MessageHeader;
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
    // Создать экземпляр на основе заголовка и тела сообщения
    Letter(RTDBUS_MessageHeader *h, std::string& b);
    Letter(const std::string& h, const std::string& b);

    ~Letter();
    void SetID(int64_t);
    void SetSERVICE(Service*);
    void SetSERVICE_ID(int64_t);
    void SetWORKER(Worker*);
    void SetWORKER_ID(int64_t);
    void SetSTATE(State);
    void SetHEADER(const std::string&);
    void SetDATA(const std::string&);
    void SetEXPIRATION(const timer_mark_t&);
    void SetVALID();
    bool GetVALID();
    int64_t GetID();
    int64_t GetSERVICE_ID();
    int64_t GetWORKER_ID();
    const std::string& GetHEADER();
    const std::string& GetDATA();
    Letter::State GetSTATE();

    void Dump();

    //
    // Поля Заголовка Сообщения
    int8_t GetPROTOCOL_VERSION();
    rtdbExchangeId GetEXCHANGE_ID();
    rtdbPid GetSOURCE_PID();
    const std::string& GetPROC_DEST();
    const std::string& GetPROC_ORIGIN();
    rtdbMsgType GetSYS_MSG_TYPE();
    rtdbMsgType GetUSR_MSG_TYPE();


  private:
    bool     m_modified;
    int64_t  m_id;
    int64_t  m_service_id;
    int64_t  m_worker_id;
    State    m_state;
    timer_mark_t m_expiration;
    RTDBUS_MessageHeader *m_header;
    std::string  m_frame_data;
    std::string  m_frame_header;
};

#endif

