#include <assert.h>
#include <xercesc/util/PlatformUtils.hpp>

#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_rtap_snap.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"
#include "xdb_rtap_point.hpp"

#include "dat/rtap_db.hpp"
#include "dat/rtap_db.hxx"
#include "dat/rtap_db-pimpl.hxx"
#include "dat/rtap_db-pskel.hxx"

using namespace xdb;
using namespace xercesc;

// ------------------------------------------------------------
void applyClassListToDB(RtEnvironment*, rtap_db::Points&);
// ------------------------------------------------------------
//
bool xdb::loadFromXML(RtEnvironment* env, const char* filename)
{
  bool status = false;
  rtap_db::Points point_list;

  assert(env);
  assert(filename);
  LOG(INFO) << "Load '"<< env->getName() <<"' content from "<<filename;

  try
  {
    XMLPlatformUtils::Initialize("UTF-8");
  }
  catch (const XMLException& toCatch)
  {
    LOG(ERROR) << "Error during initialization! :" << toCatch.getMessage();
    return status;
  }

  // Допустимым форматом хранения XML-снимков БДРВ является встроенный в eXtremeDB формат.
  // Это снимает зависимость от библиотеки xerces в runtime луркера.
  try
  {
    // Instantiate individual parsers.
    //
    ::rtap_db::RTDB_STRUCT_pimpl RTDB_STRUCT_p;
    ::rtap_db::Point_pimpl Point_p;
    ::rtap_db::Code_pimpl Code_p;
    ::rtap_db::PointType_pimpl PointType_p;
    ::rtap_db::Attr_pimpl Attr_p;
    ::rtap_db::PointKind_pimpl PointKind_p;
    ::rtap_db::Accessibility_pimpl Accessibility_p;
    ::rtap_db::AttributeType_pimpl AttributeType_p;
    ::rtap_db::AttrNameType_pimpl AttrNameType_p;
    ::xml_schema::string_pimpl string_p;

    // Connect the parsers together.
    //
    RTDB_STRUCT_p.parsers (Point_p);

    Point_p.parsers (Code_p,
                     PointType_p,
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

    RTDB_STRUCT_p.pre (&point_list);
    doc_p.parse (filename);
    RTDB_STRUCT_p.post_RTDB_STRUCT ();

    LOG(INFO) << "Parsing XML is over, processed " << point_list.size() << " element(s)";

    applyClassListToDB(env, point_list);

    saveToXML(env, filename);
  }
  catch (const ::xml_schema::exception& e)
  {
    LOG(ERROR) << e;
    status = false;
  }
  catch (const std::ios_base::failure&)
  {
    LOG(ERROR) << filename << ": error: i/o failure";
    status = false;
  }

  puts("Press any key to exit....");
  int c = getchar();
  return status;
}

void applyClassListToDB(RtEnvironment* env, rtap_db::Points &point_list)
{
  unsigned int class_item;
  unsigned int attribute_item;
  RtConnection* conn = env->getConnection();
  RtPoint* new_point = NULL;
  rtap_db::Attrib   attrib;

  for (class_item=0; class_item<point_list.size(); class_item++)
  {
    new_point = new RtPoint(point_list[class_item]);
    conn->create(new_point);
    delete new_point;
  }
}
