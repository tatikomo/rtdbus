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
  EGA_EGA_D_STRDIFFPRIMARY,
  EGA_EGA_D_STRDIFFSECOND,
  EGA_EGA_D_STRDIFFTERTIARY,
  EGA_EGA_D_STRINFOSDIFF,
  EGA_EGA_D_STRDELEGATION };

// ==============================================================================
Request::Request(const ega_ega_odm_t_RequestEntry* _config)
  : m_config(*_config)
{
  generate_new_exchange_id();
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
// Вернуть экземпляр Request соответствующего типа
Request* RequestDictionary::query_by_id(ech_t_ReqId _id)
{
  return m_requests[_id];
}

// ==============================================================================
RequestRuntimeList::RequestRuntimeList()
{
}

// ==============================================================================
RequestRuntimeList::~RequestRuntimeList()
{
}

// ==============================================================================
