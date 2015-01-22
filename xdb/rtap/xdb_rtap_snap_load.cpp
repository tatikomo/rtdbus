#include <assert.h>
#include <xercesc/util/PlatformUtils.hpp>

#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_rtap_snap.hpp"
#include "xdb_rtap_database.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"
#include "xdb_rtap_point.hpp"

// Загрузка НСИ из XML файла формата rtap_db_dict.xsd
#include "dat/rtap_db_dict.hxx"
#include "dat/rtap_db_dict-pimpl.hxx"
#include "dat/rtap_db_dict-pskel.hxx"

// Загрузка контента из XML файла формата rtap_db.xsd
#include "dat/rtap_db.hpp"
#include "dat/rtap_db.hxx"
#include "dat/rtap_db-pimpl.hxx"
#include "dat/rtap_db-pskel.hxx"

using namespace xdb;
using namespace xercesc;

// ------------------------------------------------------------
// Загрузить в БДРВ свежепрочитанные из XML таблицы НСИ
void applyDictionariesToDB(RtEnvironment*,
                        rtap_db_dict::Dictionaries_t&);
// ------------------------------------------------------------
// Загрузить в БДРВ свежепрочитанные из XML данные
void applyClassListToDB(RtEnvironment*,
                        rtap_db_dict::Dictionaries_t&,
                        rtap_db::Points&);
// ------------------------------------------------------------
// Загрузить НСИ БДРВ
bool loadXmlDictionaries(RtEnvironment* env,
                         const char* filename,
                         rtap_db_dict::Dictionaries_t&);
// ------------------------------------------------------------
// Загрузить содержимое БДРВ
bool loadXmlContent(RtEnvironment* env,
                    const char* filename,
                    rtap_db::Points&);
// ------------------------------------------------------------

// Прочитать все содержимое БД, расположенное в виде набора XML-файлов
// формата XScheme (rtap_db.xsd и rtap_db_dict.xsd)
bool xdb::loadFromXML(RtEnvironment* env, const char* filename)
{
  rtap_db::Points point_list;
  rtap_db_dict::Dictionaries_t dictionary;
  bool status = false;

  status = loadXmlDictionaries(env, filename, dictionary);
  if (!status)
  {
    LOG(ERROR) << "Dictionary '" << env->getName() << "' doesn't load";
    return false;
  }

  status = loadXmlContent(env, filename, point_list);
  if (!status)
  {
    LOG(ERROR) << "Content '" << env->getName() << "' doesn't load";
    return false;
  }

  applyClassListToDB(env, dictionary, point_list);

#ifdef USE_EXTREMEDB_HTTP_SERVER
  // При использовании встроенного в БДРВ HTTP-сервера можно
  // посмотреть статистику свежесозданного экземпляра до его закрытия
  puts("Press any key to exit....");
  int c = getchar();
#endif

  return status;
}

bool loadXmlDictionaries(RtEnvironment* env,
                         const char* filename,
                         rtap_db_dict::Dictionaries_t& dict)
{
  const char *rtap_dict_fname = "rtap_dict.xml";

  LOG(INFO) << "BEGIN Load '"<< env->getName() << "' dictionaries from " << rtap_dict_fname;

  try
  {
    ::rtap_db_dict::Dictionaries_pimpl Dictionaries_p;
    ::rtap_db_dict::UNITY_LABEL_pimpl UNITY_LABEL_p;
    ::rtap_db_dict::UnityDimensionType_pimpl UnityDimensionType_p;
    ::rtap_db_dict::UnityDimensionEntry_pimpl UnityDimensionEntry_p;
    ::rtap_db_dict::UnityIdEntry_pimpl UnityIdEntry_p;
    ::rtap_db_dict::UnityLabelEntry_pimpl UnityLabelEntry_p;
    ::rtap_db_dict::UnityDesignationEntry_pimpl UnityDesignationEntry_p;
    ::rtap_db_dict::VAL_LABEL_pimpl VAL_LABEL_p;
    ::rtap_db_dict::ObjClassEntry_pimpl ObjClassEntry_p;
    ::rtap_db_dict::ValueEntry_pimpl ValueEntry_p;
    ::rtap_db_dict::ValueLabelEntry_pimpl ValueLabelEntry_p;
    ::rtap_db_dict::CE_ITEM_pimpl CE_ITEM_p;
    ::rtap_db_dict::Identifier_pimpl Identifier_p;
    ::rtap_db_dict::ActionScript_pimpl ActionScript_p;
    ::rtap_db_dict::INFO_TYPES_pimpl INFO_TYPES_p;
    ::rtap_db_dict::InfoTypeEntry_pimpl InfoTypeEntry_p;
    ::xml_schema::string_pimpl string_p;

    // Connect the parsers together
    //
    Dictionaries_p.parsers (UNITY_LABEL_p,
                            VAL_LABEL_p,
                            CE_ITEM_p,
                            INFO_TYPES_p);

    UNITY_LABEL_p.parsers (UnityDimensionType_p,
                           UnityDimensionEntry_p,
                           UnityIdEntry_p,
                           UnityLabelEntry_p,
                           UnityDesignationEntry_p);

    VAL_LABEL_p.parsers (ObjClassEntry_p,
                         ValueEntry_p,
                         ValueLabelEntry_p);

    CE_ITEM_p.parsers (Identifier_p,
                  ActionScript_p);

    INFO_TYPES_p.parsers (ObjClassEntry_p,
                          InfoTypeEntry_p,
                          string_p);

    // Parse the XML document.
    //
    ::xml_schema::document doc_p (
      Dictionaries_p,
      "http://www.example.com/rtap_db_dict",
      "Dictionaries");

    // TODO: передать внутрь пустые списки НСИ для их инициализации
    Dictionaries_p.pre (dict);
    doc_p.parse (rtap_dict_fname);
    Dictionaries_p.post_Dictionaries ();

    // Загрузить в БДРВ свежепрочитанные из XML таблицы НСИ
    applyDictionariesToDB(env, dict);
  }
  catch (const ::xml_schema::exception& e)
  {
    LOG(ERROR) << e;
    return false;
  }
  catch (const std::ios_base::failure&)
  {
    LOG(ERROR) << rtap_dict_fname << ": error: i/o failure";
    return false;
  }

  LOG(INFO) << "END Load '"<< env->getName() << "' dictionaries from " << rtap_dict_fname;

  return true;
}

bool loadXmlContent(RtEnvironment* env, const char* filename, rtap_db::Points& pl)
{
  bool status = false;

  assert(env);
  assert(filename);
  LOG(INFO) << "Load '"<< env->getName() <<"' content from "<<filename;

  // Допустимым форматом хранения XML-снимков БДРВ является встроенный
  // в eXtremeDB формат (Native-XDB-XML, N-XML).
  // Это снимает зависимость от библиотеки xerces в runtime Брокера.
  try
  {
    // Instantiate individual parsers.
    //
    ::rtap_db::RTDB_STRUCT_pimpl RTDB_STRUCT_p;
    ::rtap_db::Point_pimpl       Point_p;
    ::rtap_db::Objclass_pimpl    Objclass_p;
    ::rtap_db::Tag_pimpl         Tag_p;
    ::rtap_db::Attr_pimpl        Attr_p;
    ::rtap_db::PointKind_pimpl   PointKind_p;
    ::rtap_db::Accessibility_pimpl  Accessibility_p;
    ::rtap_db::AttributeType_pimpl  AttributeType_p;
    ::rtap_db::AttrNameType_pimpl   AttrNameType_p;
    ::xml_schema::string_pimpl      string_p;

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

    RTDB_STRUCT_p.pre (pl);
    doc_p.parse (filename);
    RTDB_STRUCT_p.post_RTDB_STRUCT ();

    LOG(INFO) << "Parsing XML content is over, processed " << pl.size() << " element(s)";
    status = true;
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

  return status;
}

// Загрузить в БДРВ свежепрочитанные из XML таблицы НСИ
void applyDictionariesToDB(RtEnvironment* env, rtap_db_dict::Dictionaries_t& dict)
{
  rtDbCq command;
  RtConnection* conn = env->getConnection();

  LOG(INFO) << "applyDictionariesToDB";
  // Загрузить в БДРВ таблицы:
  // DICT_TSC_VAL_LABEL (элемент XML: VAL_LABEL)
  command.addrCnt = dict.values_dict.size();
  command.tags = new std::vector<std::string>();
  command.tags->push_back("DICT_TSC_VAL_LABEL");
  command.action.config = rtCONFIG_ADD_TABLE;
  command.buffer = static_cast<void*>(&dict.values_dict);
  conn->ConfigDatabase(command);
  LOG(INFO) << "Creating VAL_LABEL DICT: " << conn->getLastError().what();
  delete command.tags;

  // DICT_UNITY_ID (элемент XML: UNITY_LABEL)
  command.addrCnt = dict.unity_dict.size();
  command.tags = new std::vector<std::string>();
  command.tags->push_back("DICT_UNITY_ID");
  command.action.config = rtCONFIG_ADD_TABLE;
  command.buffer = static_cast<void*>(&dict.unity_dict);
  conn->ConfigDatabase(command);
  LOG(INFO) << "Creating UNITY DICT: " << conn->getLastError().what();
  delete command.tags;

  // XDB_CE (элемент XML: UNITY_LABEL)
  command.addrCnt = dict.macros_dict.size();
  command.tags = new std::vector<std::string>();
  command.tags->push_back("XDB_CE");
  command.action.config = rtCONFIG_ADD_TABLE;
  command.buffer = static_cast<void*>(&dict.macros_dict);
  conn->ConfigDatabase(command);
  LOG(INFO) << "Creating CE DICT: " << conn->getLastError().what();
  delete command.tags;

  //
  // TODO: Эти данные хранить в реляционной СУБД:
  // DICT_OBJCLASS_CODE (элемент XML: )
  // DICT_SIMPLE_TYPE   (элемент XML: )
  // DICT_INFOTYPES     (элемент XML: INFO_TYPES)
}

void applyClassListToDB(RtEnvironment* env, rtap_db_dict::Dictionaries_t& dict, rtap_db::Points &pl)
{
  unsigned int index;
  RtConnection* conn = env->getConnection();
  RtPoint* new_point = NULL;
  rtap_db::Attrib   attrib;
  int ClassCounter[GOF_D_BDR_OBJCLASS_LASTUSED+1];
  int objclass;
  int sum;

  // Обнулим счетчики количеств созданных экземпляров по типам Точек
  memset(static_cast<void*>(ClassCounter), '\0', sizeof(ClassCounter));

  for (index=0; index<pl.size(); index++)
  {
    new_point = new RtPoint(pl[index]);
    conn->create(new_point);

    // Обновим счетчик класса
    objclass = pl[index].objclass();
    if ((GOF_D_BDR_OBJCLASS_TS < objclass) && (objclass < GOF_D_BDR_OBJCLASS_LASTUSED))
    {
      ClassCounter[objclass]++;
    }
    else
    {
      LOG(ERROR) << "objclass "<< objclass << " exceeds limit";
    }

    delete new_point;
  }

  // Вывод статистики
  for (sum=0, objclass=0; objclass<GOF_D_BDR_OBJCLASS_LASTUSED; objclass++)
  {
    if (ClassCounter[objclass])
    {
      sum += ClassCounter[objclass];

      LOG(INFO) << "Class " << objclass << " '"
                << ClassDescriptionTable[objclass].name
                << "' : " << ClassCounter[objclass] << std::endl;
    }
  }
  LOG(INFO) << "Summary processed : " << sum << " point(s)";
}
