#include <assert.h>

#include "glog/logging.h"

#include "config.h"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_snap.hpp"

using namespace xdb;

bool xdb::loadFromXML(RtEnvironment* env, const char* filename)
{
  bool status = false;

  assert(env);
  assert(filename);
//  LOG(INFO) << "Load '"<< database_name <<"' content from "<<filename;
  LOG(INFO) << "Load '"<< env->getName() <<"' content from "<<filename;

  // Допустимым форматом хранения XML-снимков БДРВ является встроенный в eXtremeDB формат.
  // Это снимает зависимость от библиотеки xerces в runtime луркера.
#if 0
  try
  {
    // Instantiate individual parsers.
    //
    ::rtap_db::RTDB_STRUCT_pimpl RTDB_STRUCT_p;
    ::rtap_db::Class_pimpl Class_p;
    ::rtap_db::Code_pimpl Code_p;
    ::rtap_db::ClassType_pimpl ClassType_p;
    ::rtap_db::Attr_pimpl Attr_p;
    ::rtap_db::PointKind_pimpl PointKind_p;
    ::rtap_db::Accessibility_pimpl Accessibility_p;
    ::rtap_db::AttributeType_pimpl AttributeType_p;
    ::rtap_db::AttrNameType_pimpl AttrNameType_p;
    ::xml_schema::string_pimpl string_p;

    // Connect the parsers together.
    //
    RTDB_STRUCT_p.parsers (Class_p);

    Class_p.parsers (Code_p,
                     ClassType_p,
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

    //RTDB_STRUCT_p.pre (&class_list);
    RTDB_STRUCT_p.pre ("NO");
    doc_p.parse (filename);
    RTDB_STRUCT_p.post_RTDB_STRUCT ();
  }
  catch (const ::xml_schema::exception& e)
  {
    std::cerr << e << std::endl;
    status = false;
  }
  catch (const std::ios_base::failure&)
  {
    std::cerr << filename << ": error: i/o failure" << std::endl;
    status = false;
  }

#endif
  return status;
}

