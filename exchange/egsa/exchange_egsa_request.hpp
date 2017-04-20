#if !defined EXCHANGE_EGSA_REQUEST_HPP
#define EXCHANGE_EGSA_REQUEST_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <map>
#include <mutex>

// Служебные заголовочные файлы сторонних утилит
#include "exchange_config_egsa.hpp"
#include "exchange_egsa_init.hpp"

// ==============================================================================
class Request
{
  public:
    Request(ech_t_ReqId, ech_t_AcqMode, ega_ega_t_ObjectClass, int);
   ~Request();
    size_t exchange_id() { return m_exchange_id; }
    size_t id() { return m_req_id; }
    ech_t_ReqId req_id() { return m_req_id; }
    ech_t_AcqMode acq_mode() { return m_acq_mode; }
    ega_ega_t_ObjectClass objclass() { return m_objclass; }
    int priority() { return m_prio; }

  private:
    DISALLOW_COPY_AND_ASSIGN(Request);
    std::mutex mtx;
    // Общий счетчик идентификаторов времени выполнения
    static size_t m_sequence;

    // Постоянные атрибуты Запроса
    ech_t_ReqId     m_req_id;
    ech_t_AcqMode   m_acq_mode;
    ega_ega_t_ObjectClass m_objclass;
    int             m_prio;

    // Динамические Атрибуты
    // Идентификатор времени выполнения
    size_t m_exchange_id;
};

// ==============================================================================
class RequestList
{
  public:
    RequestList();
   ~RequestList();

    int add(Request*);
    Request* search(size_t);
    int free(size_t);
    Request* operator[](size_t);
    size_t size() { return m_requests.size(); }

  private:
    DISALLOW_COPY_AND_ASSIGN(RequestList);
    std::map<size_t, Request*> m_requests;
};

#endif

