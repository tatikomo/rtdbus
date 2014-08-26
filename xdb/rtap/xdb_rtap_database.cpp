#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // exit()

#include "glog/logging.h"
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_impl_common.hpp"
#include "xdb_impl_database.hpp"
#include "xdb_rtap_database.hpp"

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "dat/rtap_db.hpp"

using namespace xdb;

RtDatabase::RtDatabase(const char* _name, const Options& _options)
{
  assert (_name);
  m_impl = new DatabaseImpl(_name, _options, rtap_db_get_dictionary());
}

RtDatabase::~RtDatabase()
{
//  m_impl->Disconnect();
  delete m_impl;
}

const char* RtDatabase::getName() const
{
  return m_impl->getName();
}

DBState_t RtDatabase::State() const
{
  return m_impl->State();
}

void RtDatabase::setError(ErrorCode_t _new_error_code)
{
  m_impl->setError(_new_error_code);
}

const Error&  RtDatabase::Connect()
{
  return m_impl->Connect();
}

const Error&  RtDatabase::Disconnect()
{
  return m_impl->Disconnect();
}

const Error&  RtDatabase::Init()
{
  return m_impl->Init();
}

const Error& RtDatabase::getLastError() const
{
  return m_impl->getLastError();
}

const Error& RtDatabase::Create()
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

const Error& RtDatabase::LoadSnapshot(const char*)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

const Error& RtDatabase::StoreSnapshot(const char*)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

