// Not copyrighted - public domain.
//
// This sample parser implementation was generated by CodeSynthesis XSD,
// an XML Schema to C++ data binding compiler. You may use it in your
// programs without any restrictions.
//

#ifndef RTAP_DB_PIMPL_HXX
#define RTAP_DB_PIMPL_HXX

#include "rtap_db-pskel.hxx"

namespace rtap_db
{
  class AttrNameType_pimpl: public virtual AttrNameType_pskel,
    public ::xml_schema::string_pimpl
  {
    public:
    virtual void
    pre ();

    virtual rtap_db::AttrNameType&
    post_AttrNameType ();

    private:
    AttrNameType m_impl;
  };

  class ClassType_pimpl: public virtual ClassType_pskel,
    public ::xml_schema::string_pimpl
  {
    public:
    virtual void
    pre ();

    virtual rtap_db::ClassType&
    post_ClassType ();

    private:
    ClassType m_impl;
  };

  class AttributeType_pimpl: public virtual AttributeType_pskel,
    public ::xml_schema::string_pimpl
  {
    public:
    virtual void
    pre ();

    virtual rtap_db::AttributeType&
    post_AttributeType ();

    private:
    AttributeType m_impl;
  };

  class PointKind_pimpl: public virtual PointKind_pskel,
    public ::xml_schema::string_pimpl
  {
    public:
    virtual void
    pre ();

    virtual rtap_db::PointKind&
    post_PointKind ();

    private:
    PointKind m_impl;
  };

  class RTDB_STRUCT_pimpl: public virtual RTDB_STRUCT_pskel
  {
    public:
    virtual void
    pre ();

    virtual void
    Class (rtap_db::Class&);

    virtual void
    post_RTDB_STRUCT ();
  };

  class Class_pimpl: public virtual Class_pskel
  {
    public:
    virtual void
    pre ();

    virtual void
    Code (rtap_db::Code);

    virtual void
    Name (rtap_db::ClassType&);

    virtual void
    Attr ();

    virtual rtap_db::Class&
    post_Class ();

    private:
    Class   m_impl;
  };

  class Code_pimpl: public virtual Code_pskel,
    public ::xml_schema::integer_pimpl
  {
    public:
    virtual void
    pre ();

    virtual rtap_db::Code
    post_Code ();

    private:
    Code m_impl;
  };

  class Attr_pimpl: public virtual Attr_pskel
  {
    public:
    virtual void
    pre ();

    virtual void
    Kind (rtap_db::PointKind&);

    virtual void
    Accessibility (rtap_db::Accessibility&);

    virtual void
    DeType (rtap_db::AttributeType&);

    virtual void
    AttrName (rtap_db::AttrNameType&);

    virtual void
    Value (const ::std::string&);

    virtual void
    post_Attr ();

    private:
    Attrib m_impl;
  };

  class Accessibility_pimpl: public virtual Accessibility_pskel,
    public ::xml_schema::string_pimpl
  {
    public:
    virtual void
    pre ();

    virtual rtap_db::Accessibility&
    post_Accessibility ();

    private:
    Accessibility m_impl;
  };
}

#endif // RTAP_DB_PIMPL_HXX
