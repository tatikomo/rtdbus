#pragma once
#ifndef XDB_RTAP_POINT_HPP
#define XDB_RTAP_POINT_HPP

#include <string>
#include <vector>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "dat/rtap_db.hxx"
#include "xdb_impl_error.hpp"
#include "xdb_rtap_const.hpp"
#include "xdb_rtap_filter.hpp"

namespace xdb {

//class RtEnvironment;
class RtConnection;
class RtPointFilter;
class RtData;

// Тип хранилища: в памяти или на диске
typedef enum
{
  RAM_RESIDENT,
  DISK_RESIDENT
} RtResidence;

class RtPoint
{
  public:
    RtPoint(/*RtEnvironment*,*/ rtap_db::Class&);
    ~RtPoint();

    // Получить тип хранилища данной точки
    RtResidence   getResidence() const;
    // Получить количество атрибутов точки
    int           getAttibuteCount() const;
    // Получить Универсальное Имя точки
    const std::string& getName() const;
    // Получить количество атрибутов точки, подходящих под данный шаблон
    int           getAttibuteCount(const char*) const;
    // Получить все атрибуты точки
    rtap_db::AttibuteList& getAttributes();
    // Получить все атрибуты точки, подходящие под данный шаблон
    //rtap_db::AttibuteList* getAttributes(const char*);
    // Вернуть все дочерние точки
    RtPoint*      getChildren();
    // Вернуть объект подключения к БД
    RtConnection* getDbConnection();
    // Получить родительскую точку
    RtPoint*      getParent();
    // Залочить данную точку на указанное количество милисекунд
    const Error&  lock(long);
    const Error&  unlock();
    // Перенести блокировку точки указанному процессу
    const Error&  transferLock(int procid/*rtdbProcessId*/, long lockTime);

    // Получить список имен точек, в соответствии с заданным критерием
    const Error&  matchPoints(RtPointFilter::ScopeType, int level, RtPointFilter*);
    const Error&  matchPoints(RtPointFilter*);

#if 0
    NB: Стоит перенести все подобные методы на уровень RtConnection или RtEnvironment

    // Полная запись данных точки
    const Error&  write();
    // Запись множества значений атрибутов данной точки
    const Error&  write(std::vector<std::string> attrNames, RtData* data);
    // Запись значения заданного атрибута данной точки
    const Error&  write(std::string& attrName, RtData* data);
#endif

  private:
    DISALLOW_COPY_AND_ASSIGN(RtPoint);
    rtap_db::Class   m_info;
    // Ссылка на атрибут Универсального Имени, т.к. он может часто требоваться
    // NB: Сам атрибут хранится в m_info
    rtap_db::Attrib *m_univname_attr;
//    RtEnvironment *m_environment;
    Error          m_last_error;
    RtResidence    m_residence;
};

} // namespace xdb

#endif

