#pragma once
#ifndef XDB_RTAP_POINT_HPP
#define XDB_RTAP_POINT_HPP

#include <string>
#include <vector>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_impl_error.hpp"
#include "xdb_rtap_const.hpp"
#include "xdb_rtap_filter.hpp"

namespace xdb {

class RtAttribute;
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
    RtPoint();
    void          addPoint(univname_t name);
    const Error&  setName(univname_t);

    // Получить тип хранилища данной точки
    RtResidence   getResidence() const;
    // Получить алиас точки
    RtAttribute*  getAlias() const;
    // Получить количество атрибутов точки
    int           getAttibuteCount();
    // Получить количество атрибутов точки, подходящих под данный шаблон
    int           getAttibuteCount(const char*);
    // Получить все атрибуты точки
    RtAttribute*  getAttributes();
    // Получить все атрибуты точки, подходящие под данный шаблон
    RtAttribute*  getAttributes(const char*);
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

    // Запись множества значений атрибутов данной точки
    const Error&  write(std::vector<std::string> attrNames, RtData* data);
    // Запись значения заданного атрибута данной точки
    const Error&  write(std::string& attrName, RtData* data);

  private:
    DISALLOW_COPY_AND_ASSIGN(RtPoint);
    Error         m_last_error;
    RtResidence   m_residence;
};

} // namespace xdb

#endif

