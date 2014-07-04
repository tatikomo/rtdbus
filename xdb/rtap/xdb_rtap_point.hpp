#pragma once
#if !defined XDB_RTAP_POINT_HPP
#define XDB_RTAP_POINT_HPP

#include <string>
#include <vector>

#include "config.h"
#include "xdb_core_error.hpp"
#include "xdb_rtap_const.hpp"
#include "xdb_rtap_filter.hpp"

namespace xdb {
namespace rtap {

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
    void            addPoint(xdb::core::univname_t name);
    const xdb::core::Error&  setName(xdb::core::univname_t);

    // Получить тип хранилища данной точки
    RtResidence     getResidence() const;
    // Получить алиас точки
    RtAttribute*    getAlias() const;
    // Получить количество атрибутов точки
    int             getAttibuteCount();
    // Получить количество атрибутов точки, подходящих под данный шаблон
    int             getAttibuteCount(const char*);
    // Получить все атрибуты точки
    RtAttribute*    getAttributes();
    // Получить все атрибуты точки, подходящие под данный шаблон
    RtAttribute*    getAttributes(const char*);
    // Вернуть все дочерние точки
    RtPoint*        getChildren();
    // Вернуть объект подключения к БД
    RtConnection*   getDbConnection();
    // Получить родительскую точку
    RtPoint*        getParent();
    // Залочить данную точку на указанное количество милисекунд
    const xdb::core::Error&  lock(long);
    const xdb::core::Error&  unlock();
    // Перенести блокировку точки указанному процессу
    const xdb::core::Error&  transferLock(int procid/*rtdbProcessId*/, long lockTime);

    // Получить список имен точек, в соответствии с заданным критерием
    const xdb::core::Error& matchPoints(RtPointFilter::ScopeType, int level, RtPointFilter*);
    const xdb::core::Error& matchPoints(RtPointFilter*);

    // Запись множества значений атрибутов данной точки
    const xdb::core::Error&  write(std::vector<std::string> attrNames, RtData* data);
    // Запись значения заданного атрибута данной точки
    const xdb::core::Error&  write(std::string& attrName, RtData* data);

  private:
    DISALLOW_COPY_AND_ASSIGN(RtPoint);
    xdb::core::Error      m_last_error;
    RtResidence  m_residence;
};

} // namespace rtap
} // namespace xdb

#endif

