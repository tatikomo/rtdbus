#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // exit()

#include "glog/logging.h"
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "dat/rtap_db.hxx"
#include "xdb_impl_database.hpp"
#include "xdb_impl_db_rtap.hpp"
#include "xdb_rtap_database.hpp"

using namespace xdb;

// Класс-API доступа к БДРВ
// Функции изменения содержимого доступны через RtConnection
// ====================================================
RtDatabase::RtDatabase(const char* _name, const ::Options* _options)
 : m_options(_options)
{
  assert (_name);
  // GEV Опции создания БД для RTAP сейчас игнорируются
  m_impl = new DatabaseRtapImpl(_name, _options); //, rtap_db_get_dictionary());
}

RtDatabase::~RtDatabase()
{
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

const Error& RtDatabase::Connect()
{
  if (!m_impl->Connect())
  {
    LOG(ERROR) << "Connection failed";
  }
  return getLastError();
}

const Error& RtDatabase::Disconnect()
{
  if (!m_impl->Disconnect())
  {
    LOG(ERROR) << "Disconnect failed";
  }
  return getLastError();
}

const Error& RtDatabase::Init()
{
  return getLastError();
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

// NB: входящий указатель на имя файла может быть нулевым
const Error& RtDatabase::LoadSnapshot(const char* _fname)
{
  if (!m_impl->LoadSnapshot(_fname))
  {
    LOG(ERROR) << "LoadSnapshot failed";
  }
  return getLastError();
}

const Error& RtDatabase::MakeSnapshot(const char* _fname)
{
  if (!m_impl->MakeSnapshot(_fname))
  {
    LOG(ERROR) << "MakeSnapshot failed";
  }
  return getLastError();
}

// Интерфейс управления БД - Контроль выполнения
const Error& RtDatabase::Control(rtDbCq& info)
{
  return m_impl->Control(info);
}

// Интерфейс управления БД - Запрос информации
const Error& RtDatabase::Query(rtDbCq& info)
{
  return m_impl->Query(info);
}

// Интерфейс управления БД - Конфигуцрирование
const Error& RtDatabase::Config(rtDbCq& info)
{
  return m_impl->Config(info);
}

