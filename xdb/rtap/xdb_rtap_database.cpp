#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // exit()

#include "glog/logging.h"
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "dat/rtap_db.hxx"
#include "xdb_impl_common.hpp"
#include "xdb_impl_database.hpp"
#include "xdb_impl_db_rtap.hpp"
#include "xdb_rtap_database.hpp"

using namespace xdb;

RtDatabase::RtDatabase(const char* _name, const ::Options* _options)
 : m_options(_options)
{
  assert (_name);
  // GEV Опции создания БД для RTAP сейчас игнорируются
  m_impl = new DatabaseRtapImpl(_name); //, _options, rtap_db_get_dictionary());
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
    LOG(ERROR) << "Connection бяка";
  }
  return m_impl->getLastError();
}

const Error& RtDatabase::Disconnect()
{
  if (!m_impl->Disconnect())
  {
    LOG(ERROR) << "Disconnect бяка";
  }
  return m_impl->getLastError();
}

const Error&  RtDatabase::Init()
{
  if (!m_impl->Init())
  {
    LOG(ERROR) << "Init бяка";
  }
  return m_impl->getLastError();
}

const Error& RtDatabase::getLastError() const
{
  return m_impl->getLastError();
}

const Error& RtDatabase::Create()
{
  setError(rtE_NOT_IMPLEMENTED);
  return m_impl->getLastError();
}

// NB: входящий указатель на имя файла может быть нулевым
const Error& RtDatabase::LoadSnapshot(const char* _fname)
{
  if (!m_impl->LoadSnapshot(_fname))
  {
    LOG(ERROR) << "LoadSnapshot бяка";
  }
  return m_impl->getLastError();
}

const Error& RtDatabase::MakeSnapshot(const char* _fname)
{
  if (!m_impl->MakeSnapshot(_fname))
  {
    LOG(ERROR) << "MakeSnapshot бяка";
  }
  return m_impl->getLastError();
}

// Функции изменения содержимого БД
// ====================================================
// Создание Точки
const Error& RtDatabase::create(rtap_db::Point&)
{
  return m_impl->getLastError();
}

// Удаление Точки
const Error& RtDatabase::erase(rtap_db::Point&)
{
  return m_impl->getLastError();
}

// Чтение данных Точки
const Error& RtDatabase::read(rtap_db::Point&)
{
  return m_impl->getLastError();
}

// Изменение данных Точки
const Error& RtDatabase::write(rtap_db::Point& info)
{
  return m_impl->write(info);
}

// Блокировка данных Точки от изменения в течение заданного времени
const Error& RtDatabase::lock(rtap_db::Point&, int)
{
  return m_impl->getLastError();
}

const Error& RtDatabase::unlock(rtap_db::Point&)
{
  return m_impl->getLastError();
}

