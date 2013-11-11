#if !defined GEV_XDB_RTAP_ATTRIBUTE_H_
#define GEV_XDB_RTAP_ATTRIBUTE_H_
#pragma once

#include <string>
#include "config.h"
#include "xdb_rtap_const.hpp"
#include "xdb_rtap_data.hpp"
#include "xdb_rtap_point.hpp"
#include "xdb_rtap_error.hpp"

namespace xdb
{

class RtDbConnection;
class RtData;
class RtPoint;

class RtAttribute
{
  public:
    RtAttribute();
    ~RtAttribute();
    // Вернуть алиас атрибута
    const char* getAlias() const;
    // Вернуть тип атрибута
    DbType_t    getAttributeType() const;
    // Вернуть объект-подключение
    RtDbConnection* getConnection();
    // Вернуть определение СЕ
    std::string getDefinition() const;
    //
    int getDeType();
    // Получить point-представление точки, к которой 
    // принадлежит этот атрибут
    RtPoint* getPoint();
    //
    const char* getPointAlias() const;
    //
    const char* getPointName() const;
    // Прочитать значение экземпляра
    RtData* read();
    // Записать значение экземпляра
    const RtError& write(RtData&);

  private:
    DISALLOW_COPY_AND_ASSIGN(RtAttribute);
    RtError    m_last_error;
    univname_t m_univname;
    univname_t m_alias;
    DbType_t   m_type;
};

}

#endif
