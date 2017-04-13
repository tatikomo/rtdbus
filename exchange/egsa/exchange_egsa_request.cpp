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

size_t Request::m_sequence = 0;

Request::Request()
{
  mtx.lock();

  if (m_sequence >= ULONG_MAX - 1)
    m_sequence = 0;

  m_id = m_sequence++;

  mtx.unlock();
}

Request::~Request()
{
  LOG(INFO) << "release request " << m_id;
}

size_t Request::id()
{
  return m_id;
}

RequestList::RequestList()
{
  m_requests.clear();
}

RequestList::~RequestList()
{
  m_requests.clear();
}

int RequestList::add(Request* item)
{
  m_requests.insert(std::pair<size_t, Request*>(item->id(), item));
  return NOK;
}

Request* RequestList::search(size_t idx)
{
  LOG(INFO) << "search item " << idx;
  return NULL;
}

int RequestList::free(size_t idx)
{
  LOG(INFO) << "free item " << idx;
  return NOK;
}

Request* RequestList::operator[](size_t)
{
  return NULL;
}


