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

// ==============================================================================
Request::Request(ech_t_ReqId _req_id, ech_t_AcqMode _acq_mode, ega_ega_t_ObjectClass _objclass, int _prio)
  : m_req_id(_req_id),
    m_acq_mode(_acq_mode),
    m_objclass(_objclass),
    m_prio(_prio)
{
  mtx.lock();

  if (m_sequence >= ULONG_MAX - 1)
    m_sequence = 0;

  m_exchange_id = m_sequence++;

  mtx.unlock();
}

// ==============================================================================
Request::~Request()
{
  LOG(INFO) << "release request exchange:" << m_exchange_id << " id:" << m_req_id;
}

// ==============================================================================
RequestList::RequestList()
{
  m_requests.clear();
}

// ==============================================================================
RequestList::~RequestList()
{
  for(std::map<size_t, Request*>::const_iterator it = m_requests.begin();
      it != m_requests.end();
      ++it)
  {
    delete (*it).second;
  }

  m_requests.clear();
}

// ==============================================================================
int RequestList::add(Request* item)
{
  m_requests.insert(std::pair<size_t, Request*>(item->id(), item));
  return NOK;
}

// ==============================================================================
Request* RequestList::search(size_t idx)
{
  LOG(INFO) << "search item " << idx;
  return NULL;
}

// ==============================================================================
int RequestList::free(size_t idx)
{
  LOG(INFO) << "free item " << idx;
  return NOK;
}

// ==============================================================================
Request* RequestList::operator[](size_t id)
{
  return m_requests[id];
}

// ==============================================================================

