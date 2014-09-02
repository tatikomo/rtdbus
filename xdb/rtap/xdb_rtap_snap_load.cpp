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
void applyClassListToDB(RtEnvironment*, rtap_db::ClassesList&);
// ------------------------------------------------------------
//
bool xdb::loadFromXML(RtEnvironment* env, const char* filename)
{
  bool status = false;
  rtap_db::ClassesList class_list;

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

    RTDB_STRUCT_p.pre (&class_list);
    doc_p.parse (filename);
    RTDB_STRUCT_p.post_RTDB_STRUCT ();

    LOG(INFO) << "Parsing XML is over, processed " << class_list.size() << " element(s)";

    applyClassListToDB(env, class_list);
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

  return status;
}

void applyClassListToDB(RtEnvironment* env, rtap_db::ClassesList &class_list)
{
  unsigned int class_item;
  unsigned int attribute_item;
  RtConnection* conn = env->getConnection();
  RtPoint* new_point = NULL;
  rtap_db::Attrib   attrib;


  for (class_item=0; class_item<class_list.size(); class_item++)
  {
    new_point = new RtPoint(/*env, */class_list[class_item]);
    conn->create(new_point);

#if 0
      std::cout << "\tCODE:  " << class_list[class_item].code() << std::endl;
      std::cout << "\tNAME:  '" << class_list[class_item].name() << "'" << std::endl;
      std::cout << "\t#ATTR: " << class_list[class_item].m_attributes.size() << std::endl;
      if (class_list[class_item].m_attributes.size())
      {
        for (attribute_item=0;
             attribute_item<class_list[class_item].m_attributes.size();
             attribute_item++)
        {
          std::cout << "\t\t" << class_list[class_item].m_attributes[attribute_item].name()
                    << " : "  << class_list[class_item].m_attributes[attribute_item].value()
                    << " : "  << class_list[class_item].m_attributes[attribute_item].kind()
                    << " : "  << class_list[class_item].m_attributes[attribute_item].type()
                    << " : "  << class_list[class_item].m_attributes[attribute_item].accessibility()
                    << std::endl;
        }
      }
#endif
       delete new_point;
   }
}
