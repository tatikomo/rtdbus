#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <limits.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_egsa_request.hpp"
#include "exchange_config_egsa.hpp"

size_t Request::m_sequence = 0;

// Вспомогательный словарь для получения соответствия между идентификатором запроса
// (порядковым номером) и символьным описанием (элементом массива)
const char* Request::m_dict_RequestNames[] = {
  EGA_EGA_D_STRGENCONTROL,
  EGA_EGA_D_STRINFOSACQ,
  EGA_EGA_D_STRURGINFOS,
  EGA_EGA_D_STRGAZPROCOP,
  EGA_EGA_D_STREQUIPACQ,
  EGA_EGA_D_STRACQSYSACQ,
  EGA_EGA_D_STRALATHRES,
  EGA_EGA_D_STRTELECMD,
  EGA_EGA_D_STRTELEREGU,
  EGA_EGA_D_STRSERVCMD,
  EGA_EGA_D_STRGLOBDWLOAD,
  EGA_EGA_D_STRPARTDWLOAD,
  EGA_EGA_D_STRGLOBUPLOAD,
  EGA_EGA_D_STRINITCMD,
  EGA_EGA_D_STRGCPRIMARY,
  EGA_EGA_D_STRGCSECOND,
  EGA_EGA_D_STRGCTERTIARY,
  EGA_EGA_D_STRIAPRIMARY,
  EGA_EGA_D_STRIASECOND,
  EGA_EGA_D_STRIATERTIARY,
  EGA_EGA_D_STRINFOSDIFF,
  EGA_EGA_D_STRDELEGATION };

// ==============================================================================
Request::Request(const ega_ega_odm_t_RequestEntry* _config)
{
  const char* name = ((_config)? _config->s_RequestName : "<empty>");
  if (!_config) {
    memcpy(&m_config, '\0', sizeof(ega_ega_odm_t_RequestEntry));
  }
  else {
    memcpy(&m_config, _config, sizeof(ega_ega_odm_t_RequestEntry));
  }
  generate_new_exchange_id();
  m_finish_callback = std::bind(&Request::on_finish, this, 0, 0);
//  LOG(INFO) << "CTOR Request " << name << " ega_ega_odm_t_RequestEntry " << m_exchange_id;
}

Request::Request(const callback_type &cb, const time_t &when)
  : m_finish_callback(cb)
{
  m_when = std::chrono::system_clock::from_time_t(when);
  generate_new_exchange_id();
  m_finish_callback = std::bind(&Request::on_finish, this, 0, 3);
  LOG(INFO) << "CTOR Request time_t " << m_exchange_id;
}

Request::Request(const callback_type &cb, const timeval &when)
  : m_finish_callback(cb)
{
  m_when = std::chrono::system_clock::from_time_t(when.tv_sec) +
                       std::chrono::microseconds(when.tv_usec);
  generate_new_exchange_id();
  m_finish_callback = std::bind(&Request::on_finish, this, 0, 3);
  LOG(INFO) << "CTOR Request timeval " << m_exchange_id;
}

Request::Request(const callback_type& cb, const std::chrono::time_point<std::chrono::system_clock> &when)
  : m_when(when),
    m_finish_callback(cb)
{
  LOG(INFO) << "CTOR Request chrono " << m_exchange_id;
}


Request::Request(const Request& orig)
{
  generate_new_exchange_id();
  m_finish_callback = std::bind(&Request::on_finish, this, 0, 1);
  LOG(INFO) << "CTOR Request Request " << m_exchange_id;
}

Request& Request::operator=(const Request& orig)
{
  generate_new_exchange_id();
  m_finish_callback = std::bind(&Request::on_finish, this, 0, 2);
  LOG(INFO) << "operator= Request " << m_exchange_id;
}

// ==============================================================================
size_t Request::generate_new_exchange_id()
{
  mtx.lock();

  if (m_sequence >= ULONG_MAX - 1)
    m_sequence = 0;

  m_exchange_id = m_sequence++;

  mtx.unlock();
  return m_exchange_id;
}

// ==============================================================================
Request::~Request()
{
  LOG(INFO) << "release request exchange:" << m_exchange_id << " id:" << m_config.e_RequestId;
}

// ==============================================================================
void Request::on_finish(size_t one, size_t two)
{
  LOG(INFO) << "REQUEST TRIGGER: " << one << ", " << two;
}

// ==============================================================================
RequestDictionary::RequestDictionary()
{
  m_requests.clear();
}

// ==============================================================================
RequestDictionary::~RequestDictionary()
{
  for(std::map<ech_t_ReqId, Request*>::const_iterator it = m_requests.begin();
      it != m_requests.end();
      ++it)
  {
    delete (*it).second;
  }

  m_requests.clear();
}

// ==============================================================================
int RequestDictionary::add(Request* item)
{
  m_requests.insert(std::pair<ech_t_ReqId, Request*>(item->id(), item));
  return NOK;
}

// ==============================================================================
void RequestDictionary::release()
{
  for(std::map<ech_t_ReqId, Request*>::iterator it = m_requests.begin();
      it != m_requests.end();
      ++it)
  {
    LOG(INFO) << "Free dictionary req " << (*it).second->name();
    delete (*it).second;
  }
}

// ==============================================================================
// Вернуть экземпляр Request соответствующего типа
Request* RequestDictionary::query_by_id(ech_t_ReqId _id)
{
  std::map<ech_t_ReqId, Request*>::iterator it;
  Request* item = NULL;

  if (m_requests.end() != (it = m_requests.find(_id))) {
    item = (*it).second;
    assert(item);
    LOG(INFO) << "query_by_id(" << _id << ")=" << item->name();
  }
  else LOG(ERROR) << "Can't locate request " << _id << " from dictionary";

  return item;
}

// ==============================================================================
RequestRuntimeList::RequestRuntimeList()
{
  LOG(INFO) << "CTOR RequestRuntimeList";
}

// ==============================================================================
RequestRuntimeList::~RequestRuntimeList()
{
  LOG(INFO) << "DTOR RequestRuntimeList";
   while (!m_request_queue.empty())
   {
      LOG(INFO) << "REQ QUEUE TOP: " << m_request_queue.top().name();
      m_request_queue.pop();

     /* if (!queue.empty())
      {
        LOG(INFO) << ", ";
      }*/
   }

}

// ==============================================================================
void RequestRuntimeList::add(Request* req, const time_t &delta)
{
  const time_t when = time(0) + delta;
  LOG(INFO) << "ARM " << req->name() << " trigger for " << delta << " sec"; 
  add(std::bind(&Request::on_finish, req, 1, 2), when);
}

// ==============================================================================
void RequestRuntimeList::add(const callback_type &cb, const time_t &when)
{
  auto real_when = std::chrono::system_clock::from_time_t(when);

  m_request_queue.emplace(cb, real_when);
}

// ==============================================================================
void RequestRuntimeList::add(const callback_type &cb, const timeval &when)
{
  auto real_when = std::chrono::system_clock::from_time_t(when.tv_sec) +
                   std::chrono::microseconds(when.tv_usec);

  m_request_queue.emplace(cb, real_when);
}

// ==============================================================================
void RequestRuntimeList::add(const callback_type &cb,
         const std::chrono::time_point<std::chrono::system_clock> &when)
{
  m_request_queue.emplace(cb, when);
}

// ==============================================================================
void RequestRuntimeList::clear()
{
  while (!m_request_queue.empty())
  {
    m_request_queue.pop();
  }
}

// ==============================================================================
bool RequestRuntimeList::remove(const Request& re) { /*
    auto it = std::find(m_request_queue.begin(), m_request_queue.end(), ev);
    if (it != m_request_queue.end()) {
          m_request_queue.erase(it);
          std::make_heap(m_request_queue.begin(), m_request_queue.end(), event_less);
          return true;
    }
    else */{
      return false;
    }
}

// ==============================================================================
void RequestRuntimeList::timer()
{
      time_type now = std::chrono::system_clock::now();

      while (!m_request_queue.empty() &&
             (m_request_queue.top().when() < now))
      {
          m_request_queue.top()();
          m_request_queue.pop();
      }
}

// ==============================================================================
