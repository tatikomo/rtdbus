#if !defined EXCHANGE_EGSA_REQUEST_BASE_HPP
#define EXCHANGE_EGSA_REQUEST_BASE_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "exchange_egsa_cycle.hpp"

typedef enum {
  CYCLICREQ = 1,
  AUTOMREQ  = 2,
  ASYNCREQ  = 3
} egsa_request_class_t;

typedef enum {
  INPROGRESS = 1,   // В процессе исполнения
  ACCEPTED   = 2,   // Принят в работу
  DEFERED    = 3,   // Отложен
  REFUSED    = 4,   // Отказ
  COMBINED   = 5,   // Совмещен с другим
} egsa_request_state_t;


class BaseRequest {
  public:
    virtual ~BaseRequest() {};

    // Класс запроса
    egsa_request_class_t cycle_class() { return m_class; };
    // Идентификатор запроса
    int exchange_id() { return m_id; };
    // Тип запроса
    ech_t_ReqId id() { return m_entry.e_RequestId; };
    // Название цикла
    const std::string& cycle_name() { return m_cycle_name; };
    // Состояние запроса
    egsa_request_state_t state() { return m_state; };

  protected:
    BaseRequest(egsa_request_class_t _class, ech_t_ReqId _req_id)
    {
      m_class = _class;
      m_entry.e_RequestId = _req_id;
      m_entry.i_RequestPriority = 80;

    };

    RequestEntry m_entry;
    int  m_id;
    egsa_request_class_t m_class;
    std::string m_cycle_name;
    egsa_request_state_t m_state;

  private:

};

#endif

