// Not copyrighted - public domain.
//
// This sample parser implementation was generated by CodeSynthesis XSD,
// an XML Schema to C++ data binding compiler. You may use it in your
// programs without any restrictions.
//

#include "rtap_db-pimpl.hxx"

#include <iostream>

const char *toto = NULL;

int
main (int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "usage: " << argv[0] << " file.xml" << std::endl;
    return 1;
  }

  try
  {
    // Instantiate individual parsers.
    //
    ::rtap_db::RTDB_STRUCT_pimpl RTDB_STRUCT_p;
    ::rtap_db::Point_pimpl Point_p;
    ::rtap_db::Objclass_pimpl Objclass_p;
    ::rtap_db::Tag_pimpl Tag_p;
    ::rtap_db::Attr_pimpl Attr_p;
    ::rtap_db::AttrNameType_pimpl AttrNameType_p;
    ::rtap_db::PointKind_pimpl PointKind_p;
    ::rtap_db::Accessibility_pimpl Accessibility_p;
    ::rtap_db::AttributeType_pimpl AttributeType_p;
    ::xml_schema::string_pimpl string_p;

    // Connect the parsers together.
    //
    RTDB_STRUCT_p.parsers (Point_p);

    Point_p.parsers (Objclass_p,
                     Tag_p,
                     Attr_p);

    Attr_p.parsers (AttrNameType_p,
                    PointKind_p,
                    Accessibility_p,
                    AttributeType_p,
                    string_p);

    // Parse the XML document.
    //
    ::xml_schema::document doc_p (
      RTDB_STRUCT_p,
      "http://www.example.com/rtap_db",
      "RTDB_STRUCT");

    RTDB_STRUCT_p.pre (argv[1]);
    doc_p.parse (argv[1]);
    RTDB_STRUCT_p.post_RTDB_STRUCT ();
  }
  catch (const ::xml_schema::exception& e)
  {
    std::cerr << e << std::endl;
    return 1;
  }
  catch (const std::ios_base::failure&)
  {
    std::cerr << argv[1] << ": error: io failure" << std::endl;
    return 1;
  }
}

