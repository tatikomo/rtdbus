#ifndef EXCHANGE_CONFIG_REQUEST_HPP
#define EXCHANGE_CONFIG_REQUEST_HPP
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
typedef std::function<int ()> callback_type;
typedef std::chrono::time_point<std::chrono::system_clock> time_type;

#define DUMP_SIZE   150
// ==============================================================================
// Элементарный запрос адрес СС.
// Запросы могут быть простыми|элементарными (basic) и составными (composed).
// Составные состоят из набора базовых Запросов.
// Базовые содержат в себе идентификатор Составного Запроса.
//
class Request
{
  public:
    enum callback_state {
      ONCE      = 0,
      REPEAT    = 1,
      ERROR     = 2
    };

    typedef enum {
      AUTOMATIC = 0,
      CYCLIC    = 1,
      ASYNC     = 2
    } request_class_t;

    typedef enum {
      STATE_UNKNOWN      = 0,
      STATE_INPROGRESS   = 1,
      STATE_ACCEPTED     = 2,
      STATE_SENT         = 3,
      STATE_EXECUTED     = 4,
      STATE_ERROR        = 5,
      STATE_NOTSENT      = 6,
      STATE_WAIT_N       = 7,
      STATE_WAIT_U       = 8,
      STATE_SENT_N       = 9,
      STATE_SENT_U       = 10,
    } request_executing_state_t;

    // Используется для НСИ
    Request(const RequestEntry*);
    // Используется для хранения динамической информации в процессе работы
    Request(const Request*, AcqSiteEntry*, Cycle*);
    Request(const Request&);
    Request(const Request*);
   ~Request();
    Request& operator=(const Request&);

    //
    size_t          exchange_id() const { return m_exchange_id; }
    //
    size_t          generate_exchange_id();
    // Установить/снять признак, последний ли это Запрос в группе
    void            last_in_bundle(bool sign) { m_last_in_bundle = sign; }
    //
    bool            last_in_bundle() const { return m_last_in_bundle; }
    //
    AcqSiteEntry*   site() const { return m_site; }
    //
    Cycle*          cycle() const { return m_cycle; }
    // Вернуть идентификатор Базового (basic) Запроса
    ech_t_ReqId     id() const { return m_config.e_RequestId; }
    // Вернуть идентификатор Составного (composed) запроса
    ech_t_ReqId     composed_id() const { return m_ComposedRequestId; }
    // Установить идентификатор Составного (composed) Запроса
    void            composed_id(ech_t_ReqId _composed_id) { m_ComposedRequestId = _composed_id; }
    //
    ech_t_AcqMode   acq_mode() const { return m_config.e_RequestMode; }
    //
    ega_ega_t_ObjectClass object() const { return m_config.e_RequestObject; }
    // Класс запроса - автоматический, циклический, асинхронный
    request_class_t rclass() const { return m_class; }
    int             priority() const { return m_config.i_RequestPriority; }
    //
    bool            rprocess() const { return m_config.b_Requestprocess; }
    // Получить время начала
    const time_type when() const   { return m_when; }
    // Получить длительность исполнения
    const time_type duration() const { return m_duration; }
    // Установить время начала
    void            arm(const time_type& _when) { m_when = _when; }
    // TODO: разделить события - нормальное завершение или по таймауту
    int             callback() const { return m_trigger_callback(); }
    // Вернуть тег Сайта
    const char*     name() const
      { return (NOT_EXISTENT != m_config.e_RequestId)? m_dict_RequestNames[m_config.e_RequestId] : "error"; }
    static const char* name(ech_t_ReqId _id)
      { return (NOT_EXISTENT != _id)? m_dict_RequestNames[_id] : NULL; }
    static const char* str_state(request_executing_state_t _state) { return m_dict_RequestStates[_state]; }
    const char* str_state() const { return m_dict_RequestStates[m_state]; }
    const int*      included() const { return m_config.r_IncludingRequests; }
    // Проверить, является ли запрос составным
    int             composed() const;
    // Вернуть состояние Сайта
    request_executing_state_t state() const { return m_state; }
    // Установить новое состояние Системы
    void state(request_executing_state_t _state) { m_state = _state; }
    // Событие завершения времени. Предусмотреть флаги - таймаут есть/нет,...
    int             trigger();
    // Строка с характеристиками Запроса
    const char*     dump();
  private:
 //   DISALLOW_COPY_AND_ASSIGN(Request); violate in 'm_request_queue.emplace()
    std::mutex mtx;
    // Тип Запроса - Автоматический, Асинхронный, Циклический
    request_class_t m_class;
    // Общий счетчик идентификаторов времени выполнения
    static size_t m_sequence;
    // Словарь названий Запросов
    static const char* m_dict_RequestNames[];
    // Словарь названий состояний Запросов
    static const char* m_dict_RequestStates[];
    // выделенный буфер для функции dump()
    char m_internal_dump[DUMP_SIZE + 1];

    // Постоянные атрибуты Запроса
    RequestEntry  m_config;
    // Идентификатор composed запроса
    ech_t_ReqId   m_ComposedRequestId;
    // Время инициации запроса
    time_type     m_when;
    // Длительность исполнения
    time_type     m_duration;
    // Функции, вызываемые в указанное время
    callback_type m_trigger_callback;
    // Динамические Атрибуты
    // Идентификатор времени выполнения
    size_t m_exchange_id;
    // Атрибут "Последний запрос в цепочке"
    bool m_last_in_bundle;
    AcqSiteEntry *m_site;
    Cycle        *m_cycle;
    request_executing_state_t m_state;
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
