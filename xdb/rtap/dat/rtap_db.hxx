#if !defined XDB_RTAP_DAT_H_
#define XDB_RTAP_DAT_H_
#pragma once

#include <string>
#include <vector>

#include "config.h"

namespace rtap_db
{
  // Значение Атрибута
  //
  typedef std::string Value;

  // Код Класса
  //
  typedef unsigned int Code;

  // Наименование типа Класса - TS, TM, TSA,...
  // 
  typedef std::string ClassType;

  // Наименование Класса
  // 
  typedef std::string ClassName;

  // Наименование атрибута
  // 
  typedef std::string AttrNameType;

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


  // ===================================================================
  //
  // Атрибут класса
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

    void name(const std::string& _name)
    {
      m_name = _name;
    }

    void value(const std::string& _value)
    {
      m_value = _value;
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

    const std::string& name()
    {
      return m_name;
    }

    const std::string& value()
    {
      return m_value;
    }

    PointKind       m_attrib_string_kind;
    AttributeType   m_attrib_type;
    Accessibility   m_accessibility;
    AttrNameType    m_name;
    Value           m_value;
  };

  // Набор Атрибутов Класса
  typedef std::vector<rtap_db::Attrib> AttibuteList;

  // Основной класс-хранилище экземпляров для каждого OBJCLASS (Rtap)
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

    void clear()
    {
      m_class_code = 0;
      m_class_name.clear();
      m_attributes.clear();
    }

    Code      m_class_code;
    ClassName m_class_name;
    AttibuteList m_attributes;
  };

  // Список Классов
  typedef std::vector<rtap_db::Class> ClassesList;

} // namespace

#endif
