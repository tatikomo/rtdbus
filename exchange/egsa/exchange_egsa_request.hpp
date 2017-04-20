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

// ==============================================================================
// Элементарный запрос адрес СС
class Request
{
  public:
    Request(const ega_ega_odm_t_RequestEntry*);
   ~Request();
    size_t exchange_id()     { return m_exchange_id; }
    ech_t_ReqId id()         { return m_config.e_RequestId; }
    ech_t_AcqMode acq_mode() { return m_config.e_RequestMode; }
    ega_ega_t_ObjectClass objclass() { return m_config.e_RequestObject; }
    int priority()           { return m_config.i_RequestPriority; }
    const char* name()
      { return (ECH_D_NONEXISTANT != m_config.e_RequestId)? m_dict_RequestNames[m_config.e_RequestId] : NULL; }
    static const char* name(ech_t_ReqId _id)
      { return (ECH_D_NONEXISTANT != _id)? m_dict_RequestNames[_id] : NULL; }
    ech_t_ReqId* included()  { return m_config.r_IncludingRequests; }

  private:
    DISALLOW_COPY_AND_ASSIGN(Request);
    size_t generate_new_exchange_id();
    std::mutex mtx;
    // Общий счетчик идентификаторов времени выполнения
    static size_t m_sequence;
    static const char* m_dict_RequestNames[];

    // Постоянные атрибуты Запроса
    ega_ega_odm_t_RequestEntry  m_config;
    //ech_t_ReqId     m_req_id;
    //ech_t_AcqMode   m_acq_mode;
    //ega_ega_t_ObjectClass m_objclass;
    //int             m_prio;

    // Динамические Атрибуты
    // Идентификатор времени выполнения
    size_t m_exchange_id;
};

// ==============================================================================
// НСИ актуального перечня Запросов EGSA
class RequestDictionary
{
  public:
    RequestDictionary();
   ~RequestDictionary();

    size_t size() { return m_requests.size(); }
    int add(Request*);
    Request* query_by_id(ech_t_ReqId);

  private:
    DISALLOW_COPY_AND_ASSIGN(RequestDictionary);
    std::map<ech_t_ReqId, Request*> m_requests;
};

// ==============================================================================
// Объект "Перечень Запросов во время исполнения"
class RequestRuntimeList
{
  public:
    RequestRuntimeList();
   ~RequestRuntimeList();
  private:
    // Сортированный по времени список Запросов
    //std::sorted_queue<>;
};
#endif

