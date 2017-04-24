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
class AcqSiteEntry;
class Cycle;

// ==============================================================================
typedef std::function<void (size_t,size_t)> callback_type;
typedef std::chrono::time_point<std::chrono::system_clock> time_type;

// ==============================================================================
// Элементарный запрос адрес СС
class Request
{
  public:
    // Используется для НСИ
    Request(const ega_ega_odm_t_RequestEntry*);
    // Используется для хранения динамической информации в процессе работы
    Request(const Request*, AcqSiteEntry*, Cycle*);
    Request(const Request&);
    Request(const Request*);
   ~Request();
    Request& operator=(const Request&);

    size_t exchange_id() const { return m_exchange_id; }
    AcqSiteEntry* site() const { return m_site; }
    Cycle* cycle() const { return m_cycle; }
    ech_t_ReqId id() const { return m_config.e_RequestId; }
    ech_t_AcqMode acq_mode() const { return m_config.e_RequestMode; }
    ega_ega_t_ObjectClass objclass() const { return m_config.e_RequestObject; }
    int priority() const          { return m_config.i_RequestPriority; }
    // Получить время начала
    const time_type when() const  { return m_when; }
    // Установить время начала
    void  begin(const time_type& _when) { m_when = _when; }
    // TODO: разделить события - нормальное завершение или по таймауту
    //1 void operator()() const { m_finish_callback(0, 0); }
    void callback() const { m_finish_callback(m_config.e_RequestId, m_exchange_id); }
    const char* name() const
      { return (ECH_D_NONEXISTANT != m_config.e_RequestId)? m_dict_RequestNames[m_config.e_RequestId] : "error"; }
    static const char* name(ech_t_ReqId _id)
      { return (ECH_D_NONEXISTANT != _id)? m_dict_RequestNames[_id] : NULL; }
    int* included()  { return m_config.r_IncludingRequests; }

     // Событие завершения времени. Предусмотреть флаги - таймаут есть/нет,...
     void on_finish(size_t, size_t);

     // Строка с характеристиками Запроса
     const char* dump();
  private:
 //   DISALLOW_COPY_AND_ASSIGN(Request); violate in 'm_request_queue.emplace(cb, real_when, 0, 0);'
    size_t generate_new_exchange_id();
    std::mutex mtx;
    // Общий счетчик идентификаторов времени выполнения
    static size_t m_sequence;
    static const char* m_dict_RequestNames[];
    // выделенный буфер для функции dump()
    char m_internal_dump[100 + 1];

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
    AcqSiteEntry *m_site;
    Cycle        *m_cycle;
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
// Приоритетность:
// 1) время инициализации
// 2) значение числового атрибута "приоритет" (от 0 до 127 в порядке повышения)
struct request_less : public std::less<Request>
{
  bool operator()(const Request &r1, const Request &r2) const
  {
     return ((r2.when() < r1.when()) || (r2.priority() > r1.priority()));
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

//1    void add(const callback_type&, const time_t&);
//1    void add(const callback_type&, const timeval&);
//1    void add(const callback_type&, const std::chrono::time_point<std::chrono::system_clock>&);
};
#endif

