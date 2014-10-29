// Not copyrighted - public domain.
//
// This sample parser implementation was generated by CodeSynthesis XSD,
// an XML Schema to C++ data binding compiler. You may use it in your
// programs without any restrictions.
//

#include <iostream>

#include "rtap_db.hxx"
#include "rtap_db-pimpl.hxx"

namespace rtap_db
{

  rtap_db::Point  *current_class_instance = NULL;

  // AttrNameType_pimpl
  //

  void AttrNameType_pimpl::
  pre ()
  {
  }

  rtap_db::AttrNameType& AttrNameType_pimpl::
  post_AttrNameType ()
  {
    const ::std::string& v (post_string ());

    // TODO
    //
#if VERBOSE
    std::cout << "\t\t\tAttrNameType_pimpl::post_AttrNameType:" << v << std::endl;
#endif
    m_impl.assign(v);
    return m_impl;
  }

  // Tag_pimpl
  //

  void Tag_pimpl::
  pre ()
  {
  }

  rtap_db::Tag& Tag_pimpl::
  post_Tag ()
  {
    const ::std::string& v (post_string ());

    // TODO
    //
#if VERBOSE
    std::cout << "\t\tTag_pimpl::post_Tag:" << v << std::endl;
#endif
    m_impl.assign(v);
    return m_impl;
  }

  // AttributeType_pimpl
  //

  void AttributeType_pimpl::
  pre ()
  {
  }

  rtap_db::AttributeType& AttributeType_pimpl::
  post_AttributeType ()
  {
    const ::std::string& v (post_string ());

    // TODO
    //
#if VERBOSE
    std::cout << "\t\t\tAttributeType_pimpl::post_AttributeType:" << v << std::endl;
#endif
    m_impl.assign(v);
    return m_impl;
  }

  // PointKind_pimpl
  //

  void PointKind_pimpl::
  pre ()
  {
  }

  rtap_db::PointKind& PointKind_pimpl::
  post_PointKind ()
  {
    const ::std::string& v (post_string ());

    // TODO
    //
#if VERBOSE
    std::cout << "\t\tPointKind_pimpl::post_PointKind:" << v << std::endl;
#endif
    m_impl.assign(v);
    return m_impl;
  }

  // RTDB_STRUCT_pimpl
  //

  void RTDB_STRUCT_pimpl::
  pre (rtap_db::Points& class_list)
  {
//    std::cout << "RTDB_STRUCT_pimpl::pre"<<std::endl;
    m_classes = &class_list;
  }

  void RTDB_STRUCT_pimpl::
  pre (const char* fname)
  {
    m_classes = NULL;
    std::cout << "NOTE: construct eXtremeDB from XML-file " << fname << std::endl;
  }

  void RTDB_STRUCT_pimpl::
  Point (rtap_db::Point& Point)
  {
    // TODO
    //
#if VERBOSE
    std::cout << "RTDB_STRUCT_pimpl::Point("<<Point.code()<<", "<<Point.tag()<<")"<<std::endl;
#endif
    if (m_classes)
    {
      m_classes->push_back(Point);
    }
    else
    {
      std::cout << "Read: "<< Point.tag() << std::endl;
    }
    delete &Point;
  }

  rtap_db::Points* RTDB_STRUCT_pimpl::
  post_RTDB_STRUCT ()
  {
#if VERBOSE
    std::cout << "Job done!" << std::endl;
#endif
    return m_classes;
  }

  // Point_pimpl
  //

  void Point_pimpl::
  pre ()
  {
    // NB экземпляр удаляется в RTDB_STRUCT_pimpl::Point
    m_impl = new Point;
#if VERBOSE
    std::cout << "\t\tPoint::m_impl=" << m_impl << std::endl;
#endif
    m_impl->m_class_code = 0;
    current_class_instance = m_impl;
  }

  void Point_pimpl::
  Code (rtap_db::Code Code)
  {
    // TODO
    //
#if VERBOSE
    std::cout << "\tPoint_pimpl::Code = "<<static_cast<unsigned int>(Code)<<std::endl;
#endif
    m_impl->m_class_code = static_cast<unsigned int>(Code);
  }

  void Point_pimpl::
  Tag (rtap_db::Tag& univname)
  {
    // TODO
    //
#if VERBOSE
    std::cout << "\tPoint_pimpl::Tag = "<<univname<<std::endl;
#endif
    m_impl->m_tag.assign(univname);
  }

  void Point_pimpl::
  Attr ()
  {
#if VERBOSE
    std::cout << "\tPoint_pimpl::Attr: count = "<< m_impl->m_attributes.size()<<std::endl;
#endif
  }

  rtap_db::Point& Point_pimpl::
  post_Point ()
  {
    // TODO
    //
#if VERBOSE
    std::cout << "\tPoint_pimpl::post_Point("
        << m_impl->code() << ","
        << m_impl->tag() << ","
        << m_impl->m_attributes.size() <<")"
    << std::endl;
#endif

    return *m_impl;
  }

  Point_pimpl::~Point_pimpl()
  {
    // TODO Вызывается только один раз - проверить причины
#if VERBOSE
    std::cout << "\tPoint_pimpl::~Point_pimpl()" << std::endl;
#endif
  }


  // Code_pimpl
  //
  void Code_pimpl::
  pre ()
  {
#if VERBOSE
    std::cout << "\t\tCode_pimpl::pre" << std::endl;
#endif
  }

  rtap_db::Code Code_pimpl::
  post_Code ()
  {
    long long v (post_integer ());

    // TODO
    //
#if VERBOSE
    std::cout << "\t\tCode_pimpl::post_Code:" << v << std::endl;
#endif
    m_impl = v;
    return m_impl;
  }

  // Attr_pimpl
  //

  void Attr_pimpl::
  pre ()
  {
//    std::cout << "\t\tAttr_pimpl::pre" << std::endl;
  }

  void Attr_pimpl::
  Kind (rtap_db::PointKind& Kind)
  {
    // TODO
#if VERBOSE
    std::cout << "\t\tAttr_pimpl::Kind = " << Kind << std::endl;
#endif
    m_impl.m_attrib_string_kind.assign(Kind);
  }

  void Attr_pimpl::
  Accessibility (rtap_db::Accessibility& Accessibility)
  {
    // TODO
    //
#if VERBOSE
    std::cout << "\t\tAttr_pimpl::Accessibility = " << Accessibility << std::endl;
#endif
    m_impl.m_accessibility.assign(Accessibility);
  }

  void Attr_pimpl::
  DeType (rtap_db::AttributeType& DeType)
  {
    // TODO
    //
#if VERBOSE
    std::cout << "\t\tAttr_pimpl::DeType = " << DeType << std::endl;
#endif
    m_impl.m_attrib_type.assign(DeType);
  }

  void Attr_pimpl::
  AttrName (rtap_db::AttrNameType& AttrName)
  {
    // TODO
    //
#if VERBOSE
    std::cout << "\t\tAttr_pimpl::AttrName = " << AttrName << std::endl;
#endif
    m_impl.m_name.assign(AttrName);
  }

  void Attr_pimpl::
  Value (const ::std::string& Value)
  {
    // TODO
    //
#if VERBOSE
    std::cout << "\t\tAttr_pimpl::Value = " << Value << std::endl;
#endif
    m_impl.m_value.assign(Value);
  }

  void Attr_pimpl::
  post_Attr ()
  {
#if VERBOSE
    std::cout << "\t\tAttr_pimpl::post_Attr("
        << m_impl.kind() << ", " << m_impl.type() << ", " << m_impl.accessibility()
        << ")" << std::endl;
#endif
    // Атрибут помещает себя в список своего Класса
    current_class_instance->m_attributes.push_back(m_impl);
  }

  // Accessibility_pimpl
  //

  void Accessibility_pimpl::
  pre ()
  {
#if VERBOSE
    std::cout << "\t\tAccessibility_pimpl::pre" << std::endl;
#endif
  }

  rtap_db::Accessibility& Accessibility_pimpl::
  post_Accessibility ()
  {
    const ::std::string& v (post_string ());

    // TODO
    //
#if VERBOSE
    std::cout << "\t\t\tAccessibility_pimpl::post_Accessibility:" << v << std::endl;
#endif
    m_impl.assign(v);
    return m_impl;
  }
}

