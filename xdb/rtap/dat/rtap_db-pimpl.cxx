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

  rtap_db::Class * current_class_instance = NULL;
  rtap_db::ClassesList* m_classes;


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
//    std::cout << "\t\t\tAttrNameType_pimpl::post_AttrNameType:" << v << std::endl;
    m_impl.assign(v);
    return m_impl;
  }

  // ClassType_pimpl
  //

  void ClassType_pimpl::
  pre ()
  {
  }

  rtap_db::ClassType& ClassType_pimpl::
  post_ClassType ()
  {
    const ::std::string& v (post_string ());

    // TODO
    //
//    std::cout << "\t\tClassType_pimpl::post_ClassType:" << v << std::endl;
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
//    std::cout << "\t\t\tAttributeType_pimpl::post_AttributeType:" << v << std::endl;
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
//    std::cout << "\t\tPointKind_pimpl::post_PointKind:" << v << std::endl;
    m_impl.assign(v);
    return m_impl;
  }

  // RTDB_STRUCT_pimpl
  //

  void RTDB_STRUCT_pimpl::
  pre (rtap_db::ClassesList* class_list)
  {
    std::cout << "RTDB_STRUCT_pimpl::pre"<<std::endl;
    m_classes = class_list;
  }

  void RTDB_STRUCT_pimpl::
  Class (rtap_db::Class& Class)
  {
    // TODO
    //
    std::cout << "RTDB_STRUCT_pimpl::Class("<<Class.code()<<", "<<Class.name()<<")"<<std::endl;
  }

  void RTDB_STRUCT_pimpl::
  post_RTDB_STRUCT ()
  {
    std::cout << "Job done!" << std::endl;
  }

  // Class_pimpl
  //

  void Class_pimpl::
  pre ()
  {
    m_impl.m_class_code = 0;
    current_class_instance = &m_impl;
  }

  void Class_pimpl::
  Code (rtap_db::Code Code)
  {
    // TODO
    //
    std::cout << "\tClass_pimpl::Code = "<<static_cast<unsigned int>(Code)<<std::endl;
    m_impl.m_class_code = static_cast<unsigned int>(Code);
  }

  void Class_pimpl::
  Name (rtap_db::ClassType& Name)
  {
    // TODO
    //
    std::cout << "\tClass_pimpl::Name = "<<Name<<std::endl;
    m_impl.m_class_name.assign(Name);
  }

  void Class_pimpl::
  Attr ()
  {
    std::cout << "\tClass_pimpl::Attr: count = "<< m_impl.m_attributes.size()<<std::endl;
  }

  rtap_db::Class& Class_pimpl::
  post_Class ()
  {
    // TODO
    //
    std::cout << "\tClass_pimpl::post_Class("
        << m_impl.code() << ","
        << m_impl.name() << ","
        << m_impl.m_attributes.size() <<")"
    << std::endl;

    m_classes->push_back(m_impl);

    return m_impl;
  }

  // Code_pimpl
  //
  void Code_pimpl::
  pre ()
  {
    std::cout << "\t\tCode_pimpl::pre" << std::endl;
  }

  rtap_db::Code Code_pimpl::
  post_Code ()
  {
    long long v (post_integer ());

    // TODO
    //
  //  std::cout << "\t\tCode_pimpl::post_Code:" << v << std::endl;
    m_impl = v;
    return m_impl;
  }

  // Attr_pimpl
  //

  void Attr_pimpl::
  pre ()
  {
    std::cout << "\t\tAttr_pimpl::pre" << std::endl;
    m_class = current_class_instance;
  }

  void Attr_pimpl::
  Kind (rtap_db::PointKind& Kind)
  {
    // TODO
    std::cout << "\t\tAttr_pimpl::Kind = " << Kind << std::endl;
    m_impl.m_attrib_string_kind.assign(Kind);
  }

  void Attr_pimpl::
  Accessibility (rtap_db::Accessibility& Accessibility)
  {
    // TODO
    //
    std::cout << "\t\tAttr_pimpl::Accessibility = " << Accessibility << std::endl;
    m_impl.m_accessibility.assign(Accessibility);
  }

  void Attr_pimpl::
  DeType (rtap_db::AttributeType& DeType)
  {
    // TODO
    //
    std::cout << "\t\tAttr_pimpl::DeType = " << DeType << std::endl;
    m_impl.m_attrib_type.assign(DeType);
  }

  void Attr_pimpl::
  AttrName (rtap_db::AttrNameType& AttrName)
  {
    // TODO
    //
    std::cout << "\t\tAttr_pimpl::AttrName = " << AttrName << std::endl;
    m_impl.m_name.assign(AttrName);
  }

  void Attr_pimpl::
  Value (const ::std::string& Value)
  {
    // TODO
    //
    std::cout << "\t\tAttr_pimpl::Value = " << Value << std::endl;
    m_impl.m_value.assign(Value);
  }

  void Attr_pimpl::
  post_Attr ()
  {
  /*
    std::cout << "\t\tAttr_pimpl::post_Attr("
        << m_impl.kind() << ", " << m_impl.type() << ", " << m_impl.accessibility()
        << ")" << std::endl;
    */
    // Атрибут помещает себя в список своего Класса
    m_class->m_attributes.push_back(m_impl);
  }

  // Accessibility_pimpl
  //

  void Accessibility_pimpl::
  pre ()
  {
    std::cout << "\t\tAccessibility_pimpl::pre" << std::endl;
  }

  rtap_db::Accessibility& Accessibility_pimpl::
  post_Accessibility ()
  {
    const ::std::string& v (post_string ());

    // TODO
    //
//    std::cout << "\t\t\tAccessibility_pimpl::post_Accessibility:" << v << std::endl;
    m_impl.assign(v);
    return m_impl;
  }
}

