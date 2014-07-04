#include <assert.h>

#include "config.h"

#include "xdb_core_error.hpp"
#include "xdb_rtap_point.hpp"

using namespace xdb::rtap;

RtPoint::RtPoint() :
  m_last_error(xdb::core::rtE_NONE)
{
}

// Получить тип хранилища данной точки
RtResidence RtPoint::getResidence() const
{
  return m_residence;
}

// Получить алиас точки
RtAttribute* RtPoint::getAlias() const
{
  return NULL;
}

// Получить количество атрибутов точки
int RtPoint::getAttibuteCount()
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  return 0;
}

// Получить количество атрибутов точки, подходящих под данный шаблон
int RtPoint::getAttibuteCount(const char*)
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  return 0;
}

// Получить все атрибуты точки
RtAttribute* RtPoint::getAttributes()
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  return NULL;
}

// Получить все атрибуты точки, подходящие под данный шаблон
RtAttribute* RtPoint::getAttributes(const char*)
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  return NULL;
}

// Вернуть все дочерние точки
RtPoint* RtPoint::getChildren()
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  return NULL;
}

// Вернуть объект подключения к БД
RtConnection* RtPoint::getDbConnection()
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  return NULL;
}

// Получить родительскую точку
RtPoint* RtPoint::getParent()
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  return NULL;
}

// Залочить данную точку на указанное количество милисекунд
const xdb::core::Error& RtPoint::lock(long)
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  return m_last_error;
}

const xdb::core::Error& RtPoint::unlock()
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  return m_last_error;
}

// Перенести блокировку точки указанному процессу
const xdb::core::Error& RtPoint::transferLock(int /*rtdbProcessId*/, long)
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  return m_last_error;
}

// Получить список имен точек, в соответствии с заданным критерием
const xdb::core::Error& RtPoint::matchPoints(RtPointFilter::ScopeType,
                                    int,
                                    RtPointFilter*)
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  return m_last_error;
}

const xdb::core::Error& RtPoint::matchPoints(RtPointFilter*)
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  return m_last_error;
}

// Запись множества значений атрибутов данной точки
const xdb::core::Error& RtPoint::write(std::vector<std::string> attrNames, RtData* data)
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  assert(data);
  assert(!attrNames.empty());
  return m_last_error;
}

// Запись значения заданного атрибута данной точки
const xdb::core::Error& RtPoint::write(std::string& attrName, RtData* data)
{
  m_last_error.set(xdb::core::rtE_NOT_IMPLEMENTED);
  assert(!attrName.empty());
  assert(data);
  return m_last_error;
}

