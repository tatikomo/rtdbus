#pragma once
#if !defined XDB_RTAP_POINT_H_
#define XDB_RTAP_POINT_H_

#include <string>
#include <vector>

#include "config.h"
#include "xdb_rtap_const.hpp"
#include "xdb_rtap_error.hpp"
#include "xdb_rtap_filter.hpp"

namespace xdb
{
class RtAttribute;
class RtDbConnection;
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
    void            addPoint(univname_t name);
    const RtError&  setName(univname_t);

    // Получить тип хранилища данной точки
    RtResidence     getResigence() const;
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
    RtDbConnection* getDbConnection();
    // Получить родительскую точку
    RtPoint*        getParent();
    // Залочить данную точку на указанное количество милисекунд
    const RtError&  lock(long);
    const RtError&  unlock();
    // Перенести блокировку точки указанному процессу
    const RtError&  transferLock(int procid/*rtdbProcessId*/, long lockTime);

    // Получить список имен точек, в соответствии с заданным критерием
    const RtError& matchPoints(RtPointFilter::ScopeType, int level, RtPointFilter*);
    const RtError& matchPoints(RtPointFilter*);

    // Запись множества значений атрибутов данной точки
    const RtError&  write(std::vector<std::string> attrNames, RtData* data);
    // Запись значения заданного атрибута данной точки
    const RtError&  write(std::string& attrName, RtData* data);

  private:
    DISALLOW_COPY_AND_ASSIGN(RtPoint);
    RtError      m_last_error;
    RtResidence  m_residence;
};

}
#endif

