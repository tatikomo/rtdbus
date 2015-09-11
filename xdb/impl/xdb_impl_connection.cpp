#include <assert.h>
#include <cstddef> // ptrdiff_t

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "glog/logging.h"

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "helper.hpp"
#include "timer.hpp"

#include "xdb_impl_error.hpp"
#include "xdb_impl_connection.hpp"
#include "xdb_impl_database.hpp"
#include "xdb_impl_db_rtap.hpp"

#include "dat/rtap_db.h"
#include "dat/rtap_db.hpp"
#include "dat/rtap_db.hxx"

using namespace xdb;

// TODO: Оценить необходимость наличия данного класса, все его функции могут быть переданы в DatabaseRtapImpl
// Вероятно он необходим для многопоточной реализации, т.к. каждый поток должен явно вызывать mco_db_connect().
// В таком случае, все вызовы из ConnectionImpl нужно дополнить локальным идентификатором m_database_handle.
//
// Следовательно, ConnectionImpl должен иметь доступ к DatabaseRtapImpl.
// Нужно передать ему указатель в конструкторе.
//
// Это приведет к явной невозможности применения ConnectionImpl с любой другой реализацией БДРВ (потомком
// DatabaseImpl), кроме DatabaseRtapImpl.
// Время жизни m_rtap_db_impl превышает таковое у ConnectionImpl, последний не аллоцирует и не освобождает его.
ConnectionImpl::ConnectionImpl(DatabaseRtapImpl* rtap_db/*EnvironmentImpl *env*/)
 : /* m_env_impl(env),*/
   m_rtap_db_impl(rtap_db),
   m_database_handle(NULL),
   m_state(CONNECTION_INVALID),
   m_last_error(rtE_CONNECTION_INVALID)
{
  MCO_RET rc;

  // Каждая нить должна вызвать mco_db_connect
  rc = mco_db_connect(m_rtap_db_impl->getName(), &m_database_handle);
  if (rc)
  {
    LOG(ERROR) << "Couldn't get new database '"<<m_rtap_db_impl->getName()<<"' connection, rc="<<rc;
  }
  else {
    LOG(INFO) << "Get new database '"<<m_rtap_db_impl->getName()<<"' connection "<< m_database_handle;
    m_last_error.clear();
    m_state = CONNECTION_VALID;
  }
}

ConnectionImpl::~ConnectionImpl()
{
  MCO_RET rc;

  LOG(INFO) << "Closing database '"<<m_rtap_db_impl->getName()<<"' connection "<< m_database_handle;
  if (m_database_handle)
  {
    rc = mco_db_disconnect(m_database_handle);
    if (rc) {
        LOG(ERROR) << "Closing database '" << m_rtap_db_impl->getName() 
        << "' connection failure, rc=" << rc; }
  }
}

// Найти точку с указанным тегом.
// В имени тега не может быть указан атрибут. 
// Читается весь набор атрибутов заданной точки.
rtap_db::Point* ConnectionImpl::locate(const char* _tag)
{
  if (CONNECTION_VALID == m_state)
    return m_rtap_db_impl->locate(m_database_handle, _tag);
  else
    return NULL;
}

// Прочитать значение атрибута по указанному тегу.
// Реализация находится в DatabaseRtapImpl (xdb_impl_db_rtap.cpp)
const Error& ConnectionImpl::read(AttributeInfo_t* info)
{
  if (CONNECTION_VALID == m_state)
    m_last_error = m_rtap_db_impl->read(m_database_handle, info);

  return m_last_error;
}

// Прочитать значение атрибутов указанной группы точек.
// Реализация находится в DatabaseRtapImpl (xdb_impl_db_rtap.cpp)
const Error& ConnectionImpl::read(std::string& _group_name, int* _size, SubscriptionPoints_t* _points)
{
  if (CONNECTION_VALID == m_state)
    m_last_error = m_rtap_db_impl->read(m_database_handle, _group_name, _size, _points);

  return m_last_error;
}


// Записать значение указанного атрибута точки.
// Реализация находится в DatabaseRtapImpl (xdb_impl_db_rtap.cpp)
const Error& ConnectionImpl::write(const AttributeInfo_t* info)
{
  if (CONNECTION_VALID == m_state)
    m_last_error = m_rtap_db_impl->write(m_database_handle, const_cast<AttributeInfo_t*>(info));

  return m_last_error;
}

// Изменить значения атрибутов указанной точки
const Error& ConnectionImpl::write(rtap_db::Point& point)
{
  if (CONNECTION_VALID == m_state)
    m_last_error = m_rtap_db_impl->write(m_database_handle, point);

  return m_last_error;
}

const Error& ConnectionImpl::Control(rtDbCq& info)
{
  if (CONNECTION_VALID == m_state)
    m_last_error = m_rtap_db_impl->Control(m_database_handle, info);

  return m_last_error;
}

const Error& ConnectionImpl::Query(rtDbCq& info)
{
  if (CONNECTION_VALID == m_state)
    m_last_error = m_rtap_db_impl->Query(m_database_handle, info);

  return m_last_error;
}

const Error& ConnectionImpl::Config(rtDbCq& info)
{
  if (CONNECTION_VALID == m_state)
    m_last_error = m_rtap_db_impl->Config(m_database_handle, info);

  return m_last_error;
}

// Создание Точки
const Error& ConnectionImpl::create(rtap_db::Point* point)
{
  if (CONNECTION_VALID == m_state)
    m_last_error = m_rtap_db_impl->create(m_database_handle, *point);

  return m_last_error;
}

// Удаление Точки
const Error& ConnectionImpl::erase(rtap_db::Point* point)
{
  if (CONNECTION_VALID == m_state)
    m_last_error = m_rtap_db_impl->erase(m_database_handle, *point);

  return m_last_error;
}

