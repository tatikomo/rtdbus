#if !defined EXCHANGE_EGSA_REQUEST_HPP
#define EXCHANGE_EGSA_REQUEST_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <map>
#include <chrono>
#include <queue>
#include <mutex>

// Служебные заголовочные файлы сторонних утилит
#include "exchange_config_egsa.hpp"

class Request;

// ==============================================================================
typedef std::function<void (size_t,size_t)> callback_type;
typedef std::chrono::time_point<std::chrono::system_clock> time_type;

// ==============================================================================
// Элементарный запрос адрес СС
class Request
{
  public:
    Request(const ega_ega_odm_t_RequestEntry*);
    Request(const Request&); // для emplace()
    Request(const callback_type&, const time_t&); // emplace
    Request(const callback_type&, const timeval&); // emplace
    Request(const callback_type&, const std::chrono::time_point<std::chrono::system_clock> &); // emplace
   ~Request();
    Request& operator=(const Request&);

    size_t exchange_id() const { return m_exchange_id; }
    ech_t_ReqId id() const { return m_config.e_RequestId; }
    ech_t_AcqMode acq_mode() const { return m_config.e_RequestMode; }
    ega_ega_t_ObjectClass objclass() const { return m_config.e_RequestObject; }
    int priority() const          { return m_config.i_RequestPriority; }
    // Получить время начала
    const time_type when() const  { return m_when; }
    // Установить время начала
    void  begin(const time_type& _when) { m_when = _when; }
    // TODO: разделить события - нормальное завершение или по таймауту
    void operator()() const { m_finish_callback(0, 0); }
    const char* name() const
      { return (ECH_D_NONEXISTANT != m_config.e_RequestId)? m_dict_RequestNames[m_config.e_RequestId] : NULL; }
    static const char* name(ech_t_ReqId _id)
      { return (ECH_D_NONEXISTANT != _id)? m_dict_RequestNames[_id] : NULL; }
    int* included()  { return m_config.r_IncludingRequests; }

     // Событие завершения времени. Предусмотреть флаги - таймаут есть/нет,...
     void on_finish(size_t, size_t);
  private:
 //   DISALLOW_COPY_AND_ASSIGN(Request); violate in 'm_request_queue.emplace(cb, real_when, 0, 0);'
    size_t generate_new_exchange_id();
    std::mutex mtx;
    // Общий счетчик идентификаторов времени выполнения
    static size_t m_sequence;
    static const char* m_dict_RequestNames[];

    // Постоянные атрибуты Запроса
    ega_ega_odm_t_RequestEntry  m_config;
    // Время инициации запроса
    time_type         m_when;
    // Функции, вызываемые в указанное время
    callback_type     m_finish_callback;
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
    void release();
    Request* query_by_id(ech_t_ReqId);

  private:
    DISALLOW_COPY_AND_ASSIGN(RequestDictionary);
    std::map<ech_t_ReqId, Request*> m_requests;
};

// ==============================================================================
// ==============================================================================
// ==============================================================================
// ==============================================================================

struct request_less : public std::less<Request>
{
  bool operator()(const Request &r1, const Request &r2) const
  {
     return (r2.when() < r1.when());
  }
};

template<typename T> class request_priority_queue : public std::priority_queue<T, std::vector<T>, request_less>
{
  public:

      bool remove(const T& value) {
        auto it = std::find(this->c.begin(), this->c.end(), value);
        if (it != this->c.end()) {
            this->c.erase(it);
            std::make_heap(this->c.begin(), this->c.end(), this->comp);
            return true;
       }
       else {
        return false;
       }
  }
};

// ==============================================================================
// Объект "Перечень Запросов во время исполнения"
class RequestRuntimeList
{
  public:
    RequestRuntimeList();
   ~RequestRuntimeList();

    void add(Request*, const time_t&);
    void timer();
    bool remove(const Request&);
    void clear();

  private:
    // Сортированный по времени список Запросов
    request_priority_queue<Request> m_request_queue;

    void add(const callback_type&, const time_t&);
    void add(const callback_type&, const timeval&);
    void add(const callback_type&, const std::chrono::time_point<std::chrono::system_clock>&);
};
#endif

