#if !defined XDB_RTAP_DAT_H_
#define XDB_RTAP_DAT_H_
#pragma once

#include <string>
#include <vector>

#include "config.h"

namespace rtap_db
{
  typedef unsigned int Code;

  // Наименование типа класса - TS, TM, TSA,...
  // 
  typedef std::string ClassType;

  // Наименование класса
  // 
  typedef std::string ClassName;

  // Наименование типа точки - SCALAR, VECTOR, TABLE
  //
  typedef std::string PointKind;

  // Наименование типа атрибута - rtLOGICAL, rtINT8,...
  //
  typedef std::string AttributeType;

  // Наименование типа доступа - PUBLIC, PRIVATE
  //
  typedef std::string Accessibility;

  // Точный размер строкового поля
  typedef unsigned int BytesLength;

  // Основной класс-хранилище экземпляров
  struct Class
  {
    // 
    //
    unsigned int code() const
    {
      return m_class_code;
    }

    const std::string& name () const
    {
      return m_class_name;
    }

    private:
      Code      m_class_code;
      ClassType m_class_type;
      ClassName m_class_name;
};

struct Attrib
{
    void kind(const std::string& _kind)
    {
      m_attrib_string_kind = _kind;
    }

    void type(const std::string& _type)
    {
      m_attrib_type = _type;
    }

    void accessibility(const std::string& _accessibility)
    {
      m_accessibility = _accessibility;
    }

    const std::string& kind() const
    {
      return m_attrib_string_kind;
    }

    const std::string& type() const
    {
      return m_attrib_type;
    }

    const std::string& accessibility() const
    {
      return m_accessibility;
    }

  private:
    PointKind m_attrib_string_kind;
    AttributeType m_attrib_type;
    Accessibility m_accessibility;
};

typedef std::vector<Attrib> attributes;

} // namespace

#endif
