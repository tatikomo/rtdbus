#if !defined EXCHANGE_EGSA_REQUEST_HPP
#define EXCHANGE_EGSA_REQUEST_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <map>
#include <mutex>

// ==============================================================================
class Request
{
  public:
    Request();
   ~Request();
    size_t id();

  private:
    std::mutex mtx;
    size_t m_id;
    static size_t m_sequence;
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

  private:
    std::map<size_t, Request*> m_requests;
};

#endif

