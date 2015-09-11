#include <assert.h>

#include "glog/logging.h"
#if defined HAVE_CONFIG_H
#include "config.h"
#endif

// common typedef's
#include "xdb_common.hpp"
// Interface headers
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"
#include "xdb_rtap_database.hpp"
#include "xdb_rtap_point.hpp"
// Implementation headers
#include "xdb_impl_connection.hpp"

using namespace xdb;

// Все операции доступа к БДРВ проводятся через экземпляр RtConnection,
// поскольку каждая нить должна иметь свое собвстенное подключение к БДРВ.
// Подключение с помощью mco_db_connect осуществляется в конструкторе ConnectionImpl,
// которому для работы нужен доступ к экземпляру DatabaseRtapImpl.
RtConnection::RtConnection(RtDatabase* _rtap_db) :
  m_database(_rtap_db),
  // Так как указатель на реализацию класса фактической работы с БДРВ (DatabaseRtapImpl)
  // передается явно, экземпляр RtConnection может работать только с XDB формата RTAP.
  //
  // TODO: проверить, целесообразно ли переделать ConnectionImpl на работу
  // с базовым классом DatabaseImpl вместо DatabaseRtapImpl?
  m_impl(new ConnectionImpl(_rtap_db->getImpl())),
  m_last_error(rtE_NONE)
{
  // Вызвать mco_db_connect() для получения хендла на БДРВ.
  // Для каждого потока требуется вызвать mco_db_connect и создать
  // отдельный экземпляр mco_db_h.
}

RtConnection::~RtConnection()
{
  LOG(INFO) << "Destroy RtConnection for database " << m_database->getName();
  delete m_impl;
}

const Error& RtConnection::create(RtPoint* _point)
{
  assert(_point);
  return m_impl->create(&_point->info());
}

// Вернуть ссылку на экземпляр текущей среды
RtDatabase* RtConnection::database()
{
  return m_database;
}

const Error& RtConnection::write(RtPoint* _point)
{
  return m_impl->write(_point->info());
}

// Найти точку с указанным тегом и прочитать все значения её атрибутов
// Название тега дано в усеченной нотации, без указания конкретного атрибута
RtPoint* RtConnection::locate(const char* _tag)
{
  RtPoint *located = NULL;
  assert(_tag);

  // В теге не должно быть точки - читаем все атрибуты разом
  if (strrchr(_tag, '.'))
  {
    LOG(ERROR) << "RtConnection::locate() shouldn't accept attribute name: " << _tag;
  }
  assert(strrchr(_tag, '.') == 0);

  rtap_db::Point* data = m_impl->locate(_tag);

  if (data)
    located = new RtPoint(*data);

  return located;
}

// Найти указанную точку и прочитать её заданный атрибут.
// Название тега дано в полной нотации: "название.атрибут".
//
// Входное значение:
//   AttributeInfo_t::name
// Выходные значения:
//   type - тип атрибута
//   value - значение
//   quality - качество
//   признак успешности чтения значения точки
const Error& RtConnection::read(AttributeInfo_t* output)
{
  return m_impl->read(output);
}

const Error& RtConnection::read(std::string& _group_name, int* _size, SubscriptionPoints_t* _output)
{
  return m_impl->read(_group_name, _size, _output);
}

// Прочитать значение заданного атрибута точки
// Входное значение:
//   AttributeInfo_t::name
//   type - тип атрибута
//   value - значение
// Выходные значения:
//   Признак успешности чтения значения точки
const Error& RtConnection::write(const AttributeInfo_t* input)
{
  return m_impl->write(input);
}

// Интерфейс управления БД - Контроль выполнения
const Error& RtConnection::ControlDatabase(rtDbCq& info)
{
  info.act_type = CONTROL;
  return m_impl->Control(info);
}

// Интерфейс управления БД - Контроль Точек
const Error& RtConnection::QueryDatabase(rtDbCq& info)
{
  info.act_type = QUERY;
  return m_impl->Query(info);
}

// Интерфейс управления БД - Контроль выполнения
const Error& RtConnection::ConfigDatabase(rtDbCq& info)
{
  info.act_type = CONFIG;
  return m_impl->Config(info);
}

