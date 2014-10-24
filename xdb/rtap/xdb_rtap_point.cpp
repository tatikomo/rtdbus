#include <assert.h>
#include <string>

#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "dat/rtap_db.hxx"
#include "xdb_impl_error.hpp"
#include "xdb_rtap_point.hpp"

using namespace xdb;

static const std::string EMPTY = "<NULL>";

RtPoint::RtPoint(rtap_db::Point& _info) :
  m_info(_info),
  m_last_error(rtE_NONE),
  m_residence(RAM_RESIDENT)
{
}

RtPoint::~RtPoint()
{
}

// Получить тип хранилища данной точки
RtResidence RtPoint::getResidence() const
{
  return m_residence;
}

const std::string& RtPoint::getTag() const
{
  return m_info.tag();
}

// Получить количество атрибутов точки
int RtPoint::getAttibuteCount() const
{
  return m_info.m_attributes.size();
}

// Получить количество атрибутов точки, подходящих под данный шаблон
int RtPoint::getAttibuteCount(const char*) const
{
  return 0;
}

// Получить все атрибуты точки
rtap_db::AttibuteList& RtPoint::getAttributes()
{
  return m_info.m_attributes;
}

// Вернуть все дочерние точки
RtPoint* RtPoint::getChildren()
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return NULL;
}

// Вернуть объект подключения к БД
RtConnection* RtPoint::getDbConnection()
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return NULL;
}

// Получить родительскую точку
RtPoint* RtPoint::getParent()
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return NULL;
}

// Залочить данную точку на указанное количество милисекунд
const Error& RtPoint::lock(long)
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return m_last_error;
}

const Error& RtPoint::unlock()
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return m_last_error;
}

// Перенести блокировку точки указанному процессу
const Error& RtPoint::transferLock(int /*rtdbProcessId*/, long)
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return m_last_error;
}

// Получить список имен точек, в соответствии с заданным критерием
const Error& RtPoint::matchPoints(RtPointFilter::ScopeType,
                                    int,
                                    RtPointFilter*)
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return m_last_error;
}

const Error& RtPoint::matchPoints(RtPointFilter*)
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return m_last_error;
}

