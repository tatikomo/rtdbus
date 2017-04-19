#pragma once
#ifndef XDB_RTAP_DB_HXX
#define XDB_RTAP_DB_HXX

#include <string>
#include <vector>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// неиспользуемый макс. номер (см. ObjClass_t в rtap_db_dict.hxx)
#define OBJCLASS_UNUSED 127

namespace rtap_db
{
  // Значение Атрибута
  //
  typedef std::string Value;

  // Код Класса
  // TODO: аналог задан в rtap_db_dict.hxx, объединить
  typedef unsigned int Objclass;

  // Тэг, ранее "универсальное имя точки"
  // 
  typedef std::string Tag;

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
  typedef std::vector<rtap_db::Attrib> AttributeList;

  // Основной класс-хранилище экземпляров для каждого OBJCLASS (Rtap)
  struct Point
  {
    // 
    //
    Objclass objclass() const
    {
      return m_objclass;
    }

    const std::string& tag () const
    {
      return m_tag;
    }

    uint64_t tag_id() const
    {
      return m_tag_id;
    }

    void clear()
    {
      m_tag.clear();
      m_objclass = OBJCLASS_UNUSED;
      m_attributes.clear();
    }

    int attrib_qty()
    {
      return m_attributes.size();
    }

    rtap_db::Attrib* attrib(const char* _attr_name)
    {
      std::string given_attr_name = _attr_name;

      return attrib(given_attr_name);
    }

    rtap_db::Attrib* attrib(const std::string& _attr_name)
    {
      rtap_db::Attrib* located = NULL;

      // По заданному имени атрибута вернуть его данные из списка m_attributes
      // TODO ускорить поиск
      if (m_attributes.size())
      {
        for (unsigned int attribute_item=0;
             attribute_item < m_attributes.size();
             attribute_item++)
        {
           if (0 == (_attr_name.compare(m_attributes[attribute_item].name())))
           {
             located = &m_attributes[attribute_item];
             break;
           }
        }
      }

      return located;
    }

    rtap_db::Attrib& attrib(unsigned int idx)
    {
      return m_attributes[idx];
    }

    AttributeList& attributes()
    {
      return m_attributes;
    }

    Objclass     m_objclass;
    Tag          m_tag;
    uint64_t     m_tag_id;
    AttributeList m_attributes;
  };

  // Список Классов
  typedef std::vector<rtap_db::Point> Points;

} // namespace

#endif
