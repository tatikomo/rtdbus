#include <assert.h>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdlib.h> // atoi()
#include <string.h> // memcpy()
#include <stdio.h>  // sprintf()
#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_common.hpp"
#include "xdb_rtap_snap.hpp"

using namespace xdb;

static const std::string UNIVNAME_STRING = "UNIVNAME";
static const std::string OBJCLASS_STRING = "OBJCLASS";

/*------------------*/
/* Global variables */
/*------------------*/
#define LINE_BUFFER_LEN    255
char     currentAttrName [NAME_SIZE+1];   
char     currentStrDeType[DETYPE_SIZE+1];
rtDeType currentDeType;
 	
#ifdef __SUNPRO_CC
typedef std::pair <std::string, DbType_t> DbTypesHashPair_t;
typedef std::map  <std::string, DbType_t> DbTypesHash_t;
#else
typedef std::pair <const std::string, DbType_t> DbTypesHashPair_t;
typedef std::map  <const std::string, DbType_t> DbTypesHash_t;
#endif
typedef DbTypesHash_t::iterator DbTypesHashIterator_t;

typedef std::map  <std::string, int> DbPointsTypeHash_t;
typedef std::pair <std::string, int> DbPointsTypeHashPair_t;
DbTypesHash_t       dbTypesHash;
DbPointsTypeHash_t  dbPointsTypeHash;


/* statistic counters */
int nbClass;
int nbClassFailed;
int nbAlias;
int nbAliasFailed;
int nbInst;
int nbInstFailed;
int nbCeDefAdded;
int nbCeDefFailed;
int nbPointActivated;
int nbPointActivationFailed;
int nbHist;
int nbHistFailed;

// ###############################################################
// Заполнить структуру AttributeInfo_t по заданным DbType_t и 
// строковому представлению значения
bool getAttrValue(DbType_t, AttributeInfo_t*, const std::string&);
// ###############################################################

// Создать точку c заданным именем и без атрибутов, подключив её к ROOT
//
bool initBranch(const char* pointName)
{
  assert(pointName);
  LOG(INFO) << "creates point root/"<<pointName;
  return true;
}

// Удалить точку c заданным именем
//
bool deleteBranch(const char* pointName)
{
  assert(pointName);
  LOG(INFO) << "delete point "<<pointName;
  return true;
}

#if 0
rtDeType extractDeType(char* laChaine)
{
  int j;

  skipStr(laChaine);

  for(j = 1; j < GOF_D_BDR_MAX_DE_TYPE; j++)
  {
    if(strcmp(rtDataElem[j].name, laChaine) == 0)
      break;
  }

  if (j < GOF_D_BDR_MAX_DE_TYPE)
      return((rtDeType)j);
  else 
      return(rtUNDEFINED);
}

#endif

recordType xdb::getRecordType(std::string& typeEnreg)
{
   if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "C ") == 0)
      return C_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "I ") == 0)
      return I_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "S ") == 0)
      return S_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "V ") == 0)
      return V_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "DV") == 0)
      return DV_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "T ") == 0)
      return T_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "F ") == 0)
      return F_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "DF") == 0)
      return DF_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "A ") == 0)
      return A_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "H ") == 0)
      return H_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "J ") == 0)
      return J_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "LF") == 0)
      return LF_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "CF") == 0)
      return CF_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "AV") == 0)
      return AV_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "C0") == 0)
      return C0_TYPE;
   else if (typeEnreg.compare(0, TYPE_ENREG_SIZE, "L0") == 0)
      return L0_TYPE;
   else
      return UNKNOWN_RECORD_TYPE;  
}

/*
 all RTAP data types from /opt/rtap/A.08.60/include/rtap/dataElem.h
 */
void LoadDbTypesDictionary()
{
   /* TODO: release dbTypesHash memory */
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtLOGICAL].name,    DB_TYPE_LOGICAL));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtINT8].name,       DB_TYPE_INT8));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtUINT8].name,      DB_TYPE_UINT8));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtINT16].name,      DB_TYPE_INT16));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtUINT16].name,     DB_TYPE_UINT16));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtINT32].name,      DB_TYPE_INT32));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtUINT32].name,     DB_TYPE_UINT32));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtFLOAT].name,      DB_TYPE_FLOAT));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtDOUBLE].name,     DB_TYPE_DOUBLE));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtBYTES4].name,     DB_TYPE_BYTES4));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtBYTES8].name,     DB_TYPE_BYTES8));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtBYTES12].name,    DB_TYPE_BYTES12));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtBYTES16].name,    DB_TYPE_BYTES16));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtBYTES20].name,    DB_TYPE_BYTES20));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtBYTES32].name,    DB_TYPE_BYTES32));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtBYTES48].name,    DB_TYPE_BYTES48));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtBYTES64].name,    DB_TYPE_BYTES64));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtBYTES80].name,    DB_TYPE_BYTES80));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtBYTES128].name,   DB_TYPE_BYTES128));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtBYTES256].name,   DB_TYPE_BYTES256));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtDB_XREF].name,    DB_TYPE_INT64)); // autoid_t eXtremeDB
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtDATE].name,       DB_TYPE_ABSTIME));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtTIME_OF_DAY].name,DB_TYPE_ABSTIME));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtABS_TIME].name,   DB_TYPE_ABSTIME));
   dbTypesHash.insert(DbTypesHashPair_t(rtDataElem[rtUNDEFINED].name,  DB_TYPE_UNDEF));

   // TODO: release memory
   // NB: не используется, удалить?
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("DISP_TABLE_H",
                           GOF_D_BDR_OBJCLASS_DISP_TABLE_H));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("DISP_TABLE_J",
                           GOF_D_BDR_OBJCLASS_DISP_TABLE_J));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("DISP_TABLE_M",  
                           GOF_D_BDR_OBJCLASS_DISP_TABLE_M));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("DISP_TABLE_QH",
                           GOF_D_BDR_OBJCLASS_DISP_TABLE_QH));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("FIXEDPOINT",
                           GOF_D_BDR_OBJCLASS_FIXEDPOINT));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("HIST_SET",
                           GOF_D_BDR_OBJCLASS_HIST_SET));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("HIST_TABLE_H",
                           GOF_D_BDR_OBJCLASS_HIST_TABLE_H));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("HIST_TABLE_J",
                           GOF_D_BDR_OBJCLASS_HIST_TABLE_J));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("HIST_TABLE_M",
                           GOF_D_BDR_OBJCLASS_HIST_TABLE_M));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("HIST_TABLE_QH",
                           GOF_D_BDR_OBJCLASS_HIST_TABLE_QH));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("HIST_TABLE_SAMPLE",
                           GOF_D_BDR_OBJCLASS_HIST_TABLE_SAMPLE));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("TIME_AVAILABLE",
                           GOF_D_BDR_OBJCLASS_TIME_AVAILABLE));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("config",
                           GOF_D_BDR_OBJCLASS_CONFIG));
}

/*
 * По каждому классу точек прочитать справочные данные
 *
 */
int xdb::processClassFile(const char* fpath)
{
  int        objclass;
  int        loadedClasses = 0;
  char       fname[255];
  std::string s_skip;
  std::string s_univname;
  std::string s_access;
  std::string s_type;
  std::string s_default_value;
  DbType_t    db_type;
  AttributeInfo_t *p_attr_info; 
  std::string line;
  std::string::size_type first;
  std::string::size_type second;
  std::istringstream iss;

  assert(fpath);
  LoadDbTypesDictionary();

  for (objclass=0; objclass <= GOF_D_BDR_OBJCLASS_LASTUSED; objclass++)
  {
    ClassDescriptionTable[objclass].attributes_pool = NULL;
    if (0 == strncmp(ClassDescriptionTable[objclass].name,
                 D_MISSING_OBJCODE,
                 TAG_NAME_MAXLEN))
      continue;

    sprintf(fname, "%s/%02d_%s.dat",
            fpath,
            ClassDescriptionTable[objclass].code,
            ClassDescriptionTable[objclass].name);

    std::ifstream ifs(fname);

    if (ifs.is_open())
    {
      while (getline(ifs, line))
      {
        /* пропускать строки, начинающиеся с символов [ALJCVTFD#] */
        if (0 == line.find_first_of("ALJCVTFD#"))
        {
          // 'A': CE
          // 'L': словарные значения поля таблицы
          // 'J': Свойство точки "Enabled|Disabled"
          // 'C': окончание перечисления словарных значений поля таблицы
          // 'V': вектор
          // 'T': таблица
          // 'F': поле таблицы
          // 'D': элемент вектора
          // '#': комментарий
          continue;
        }

        if (0 == line.find_first_of("I"))
        {
          // начало новой точки
          // создать массив лексем
          continue;
        }

        if (0 == line.find_first_of("S"))
        {
          // S OBJCLASS           PUB        rtUINT8        0
          // создать массив лексем
          iss.clear();
          iss.str(line);
          if (iss >> s_skip >> s_univname >> s_access >> s_type)
          {
             // Если в iss еще остались данные, и тип атрибута символьный,
             // нужно получить все это содержимое вместе с кавычками и пробелами.
             // NB: Приведенный здесь подход работает, если кавычки встречаются
             // у атрибутов только в колонке значения.
             s_default_value.clear();
             first = line.find('\"');
             if (first != std::string::npos)
             {
                second = line.rfind('\"');
                if (second != std::string::npos)
                {
                    if (first < second)
                    {
                      s_default_value = line.substr(first+1, (second-first-1));
                    }
                }
             }
          }
          else
          {
            LOG(ERROR) << "Error parsing '" << line << "', should be 4 fields" << std::endl;
          }
        }

        /* type может быть: строковое, с плав. точкой, целое */
        if (false == GetDbTypeFromString(s_type, db_type))
        {
                /* ошибка определения типа атрибута */
                LOG(ERROR)<<"Given attribute type '"<<s_type
                          <<"' is unknown for class '"<<s_univname<<"'";
        }

        /*
          Добавить для экземпляра данного objclass перечень атрибутов,
          подлежащих чтению из instances_total.dat, и их родовые типы
          (целое, дробное, строка, ...)
        */
        if (!ClassDescriptionTable[objclass].attributes_pool)
             ClassDescriptionTable[objclass].attributes_pool = new AttributeMap_t;

        p_attr_info = new AttributeInfo_t;
        p_attr_info->name.assign(s_univname);
        // Присвоить значение атрибуту в соответствии с полученным типом
        getAttrValue(db_type, p_attr_info, s_default_value);
#if defined VERBOSE
        std::cout << "objclass: " << objclass << " attr_name: "
                  << s_univname << " type: " << s_type
                  << " type_code: " << db_type << std::endl;
#endif

        // Поместить новый атрибут в список атрибутов класса
        ClassDescriptionTable[objclass].attributes_pool->insert(
                    AttributeMapPair_t(s_univname,  *p_attr_info)
                    );
        delete p_attr_info;
      }

      ifs.close();
      loadedClasses++;
      LOG(INFO) << "Class file "<<fname<<" loaded";
    }
    else
    {
      LOG(ERROR) << "Ошибка "<<ifs.rdstate()<<" чтения входного файла "<<fname;
    }
  }

  return loadedClasses;
}

// 
bool getAttrValue(DbType_t db_type,
    // OUTPUT
    AttributeInfo_t* p_attr_info,
    // INPUT
    const std::string& given_value)
{
    bool status = true;
    struct tm given_time;
    std::string::size_type point_pos;

    assert(p_attr_info);

    p_attr_info->type = db_type;
    switch(db_type)
    {
      // TODO: учитывать данное ограничение размера строки
      // NB: сейчас ограничение не применяется, используется
      // фактическая поданная длина
      // TODO: проверить работу с UTF-8
      case DB_TYPE_BYTES:
        if (!given_value.empty())
        {
          p_attr_info->value.dynamic.val_string = new std::string(given_value);
        }
        else p_attr_info->value.dynamic.val_string = NULL;
      break;

      case DB_TYPE_BYTES4:
      case DB_TYPE_BYTES8:
      case DB_TYPE_BYTES12:
      case DB_TYPE_BYTES16:
      case DB_TYPE_BYTES20:
      case DB_TYPE_BYTES32:
      case DB_TYPE_BYTES48:
      case DB_TYPE_BYTES64:
      case DB_TYPE_BYTES80:
      case DB_TYPE_BYTES128:
      case DB_TYPE_BYTES256:
        p_attr_info->value.dynamic.size = given_value.size();
        // для непустого значения
        if (p_attr_info->value.dynamic.size)
        {
           p_attr_info->value.dynamic.varchar = new char[p_attr_info->value.dynamic.size + 1];
           memcpy(p_attr_info->value.dynamic.varchar,
               given_value.data(),
               given_value.length());
           p_attr_info->value.dynamic.varchar[p_attr_info->value.dynamic.size] = '\0';
        }
        else
        {
/*          LOG(ERROR) << "LEAK?:" << p_attr_info->name
                     << " type:" << p_attr_info->type
                     << " size:" << p_attr_info->value.dynamic.size
                     << " data:" << (void*)p_attr_info->value.dynamic.varchar;*/
          p_attr_info->value.dynamic.varchar = NULL;
        }
        break;

      case DB_TYPE_LOGICAL:
              p_attr_info->value.fixed.val_bool = (atoi(given_value.c_str()) > 0);
              break;
      case DB_TYPE_INT8:
              p_attr_info->value.fixed.val_int8 = atoi(given_value.c_str());
              break;
      case DB_TYPE_UINT8:
              p_attr_info->value.fixed.val_uint8 = atoi(given_value.c_str());
              break;
      case DB_TYPE_INT16:
              p_attr_info->value.fixed.val_int16 = atoi(given_value.c_str());
              break;
      case DB_TYPE_UINT16:
              p_attr_info->value.fixed.val_uint16 = atoi(given_value.c_str());
              break;
      case DB_TYPE_INT32:
              p_attr_info->value.fixed.val_int32 = atoi(given_value.c_str());
              break;
      case DB_TYPE_UINT32:
              p_attr_info->value.fixed.val_uint32 = atoi(given_value.c_str());
              break;
      case DB_TYPE_INT64:
              p_attr_info->value.fixed.val_int64 = atoll(given_value.c_str());
              break;
      case DB_TYPE_UINT64:
              p_attr_info->value.fixed.val_uint64 = atoll(given_value.c_str());
              break;
      case DB_TYPE_FLOAT:
              p_attr_info->value.fixed.val_float = atof(given_value.c_str());
              break;
      case DB_TYPE_DOUBLE:
              p_attr_info->value.fixed.val_double = atof(given_value.c_str());
              break;
      case DB_TYPE_ABSTIME:
              strptime(given_value.c_str(), D_DATE_FORMAT_STR, &given_time);
              p_attr_info->value.fixed.val_time.tv_sec = given_time.tm_sec;
//              std::cout << "snap date:" << given_value.c_str()
//                        << " sec=" << given_time.tm_sec
//                        << std::endl; //1
              point_pos = given_value.find_last_of('.');
              // Если точка найдена, и она не последняя в строке
              if ((point_pos != std::string::npos) && point_pos != given_value.size())
                 p_attr_info->value.fixed.val_time.tv_usec = atoi(given_value.substr(point_pos + 1).c_str());
              break;

      default:
        status = false;
    }

    return status;
}

// Экранировать символы '&', '<', '>' на '&amp;', '&lt;', '&gt'
// перед использованием строки в составе XML
std::string getValueAsString(AttributeInfo_t* attr_info, bool masquerade)
{
  std::string s_val;
  std::stringstream ss;
  std::string::size_type found;
  time_t given_time;
  static char s_date[D_DATE_FORMAT_LEN + 1];

  assert(attr_info);
  switch(attr_info->type)
  {
      case DB_TYPE_BYTES:
        s_val.assign(*attr_info->value.dynamic.val_string);
      break;
      case DB_TYPE_BYTES4:
      case DB_TYPE_BYTES8:
      case DB_TYPE_BYTES12:
      case DB_TYPE_BYTES16:
      case DB_TYPE_BYTES20:
      case DB_TYPE_BYTES32:
      case DB_TYPE_BYTES64:
      case DB_TYPE_BYTES80:
      case DB_TYPE_BYTES128:
      case DB_TYPE_BYTES256:
        s_val.assign(attr_info->value.dynamic.varchar, attr_info->value.dynamic.size);
        break;

      case DB_TYPE_LOGICAL:
        ss << static_cast<unsigned int>(attr_info->value.fixed.val_bool);
        s_val.assign(ss.str());
        break;
      case DB_TYPE_INT8:
        // NB: простой вывод int8 значением < '0' в поток проводит к
        // занесению туда непечатных символов, нужно приводить int8 к int16
        ss << static_cast<signed int>(attr_info->value.fixed.val_int8);
        s_val.assign(ss.str());
        break;
      case DB_TYPE_UINT8:
        ss << static_cast<unsigned int>(attr_info->value.fixed.val_uint8);
        s_val.assign(ss.str());
        break;

      case DB_TYPE_INT16:
        ss << attr_info->value.fixed.val_int16;
        s_val.assign(ss.str());
        break;
      case DB_TYPE_UINT16:
        ss << attr_info->value.fixed.val_uint16;
        s_val.assign(ss.str());
        break;

      case DB_TYPE_INT32:
        ss << attr_info->value.fixed.val_int32;
        s_val.assign(ss.str());
        break;
      case DB_TYPE_UINT32:
        ss << attr_info->value.fixed.val_uint32;
        s_val.assign(ss.str());
        break;

      case DB_TYPE_INT64:
        ss << attr_info->value.fixed.val_int64;
        s_val.assign(ss.str());
        break;
      case DB_TYPE_UINT64:
        ss << attr_info->value.fixed.val_uint64;
        s_val.assign(ss.str());
        break;

      case DB_TYPE_FLOAT:
        ss << attr_info->value.fixed.val_float;
        s_val.assign(ss.str());
        break;

      case DB_TYPE_DOUBLE:
        ss << attr_info->value.fixed.val_double;
        s_val.assign(ss.str());
        break;

      case DB_TYPE_ABSTIME:
        given_time = attr_info->value.fixed.val_time.tv_sec;
        strftime(s_date, D_DATE_FORMAT_LEN, D_DATE_FORMAT_STR, localtime(&given_time));
//              std::cout << "snap sec:" << given_time
//                        << " date:" << s_date
//                        << std::endl; //1
        ss << s_date << "." << attr_info->value.fixed.val_time.tv_usec;
        s_val.assign(ss.str());
        break;

      default:
        s_val.assign("<unknown>");
  }

  if (masquerade)
  {
    while (std::string::npos != (found=s_val.find('&')))
    {
      s_val.erase(found, 1);
      s_val.insert(found, "&amp;");
    }

    while (std::string::npos != (found=s_val.find('<')))
    {
      s_val.erase(found, 1);
      s_val.insert(found, "&lt;");
    }

    while (std::string::npos != (found=s_val.find('>')))
    {
      s_val.erase(found, 1);
      s_val.insert(found, "&gt;");
    }

  }
  return s_val;
}

// Сброс законченного набора атрибутов точки в XML-файл
std::string& xdb::dump_point(
    // IN
    int class_idx,
    const std::string& pointName,
    // IN-OUT
    AttributeMap_t& attributes_given,
    // OUT
    std::string& dump)
{
    // Получить доступ к Атрибутам из шаблонных файлов Классов
    AttributeMap_t         *attributes_template;
    AttributeInfo_t        *element;
    AttributeMapIterator_t  it_given;
    std::stringstream       class_item_presentation;
    AttributeMapIterator_t  it_attr_pool;
    std::string             univname;

    if (class_idx == GOF_D_BDR_OBJCLASS_UNUSED)
    {
      LOG(ERROR) << "Processing unsupported class for point " << pointName;
      dump.clear();
      return dump;
    }

    if (NULL != (attributes_template = ClassDescriptionTable[class_idx].attributes_pool))
    {
        class_item_presentation
                  << "<rtdb:Point>" << std::endl
                  << "  <rtdb:Objclass>"<< (int)class_idx <<"</rtdb:Objclass>" << std::endl;

        // Найти тег БДРВ (атрибут ".UNIVNAME")
        it_given = attributes_given.find(UNIVNAME_STRING);
        if (it_given != attributes_given.end())
        {
          univname.assign(it_given->second.value.dynamic.varchar,
                          it_given->second.value.dynamic.size);
        }
        else
        {
          LOG(WARNING) << "RTDB tag 'UNIVNAME' not found for point " << pointName;
          univname.assign("/");
          univname+=pointName;
        }

        class_item_presentation << "  <rtdb:Tag>"<< univname <<"</rtdb:Tag>" << std::endl;

        for (AttributeMapIterator_t it=attributes_template->begin();
             it!=attributes_template->end();
             ++it)
        {
          // TODO: удалить проверку UNIVNAME и OBJCLASS после редактирования словарей ??_*.dat
          // Атрибут БДРВ UNIVNAME пропустить, он уже сохранен как атрибут XML Тег
          if (!it->second.name.compare(UNIVNAME_STRING))
            continue;

          // Атрибут БДРВ OBJCLASS пропустить, он уже сохранен как атрибут XML
          if (!it->second.name.compare(OBJCLASS_STRING))
            continue;

          // Сохранить остальные атрибуты
          class_item_presentation
                << "  <rtdb:Attr>" << std::endl
        		<< "    <rtdb:AttrName>" << it->second.name
                    << "</rtdb:AttrName>" << std::endl
                << "    <rtdb:Kind>SCALAR</rtdb:Kind>" << std::endl
        		<< "    <rtdb:Accessibility>PUBLIC</rtdb:Accessibility>" << std::endl
        		<< "    <rtdb:DbType>" << GetDbNameFromType(it->second.type)
                    << "</rtdb:DbType>" << std::endl;

          // Если Атрибут из шаблона найден во входном перечне Атрибутов, то
          //    (1) Значения по умолчанию следует брать из входного перечня
          // Иначе
          //    (2) Значения по умолчанию брать из шаблона
          it_given = attributes_given.find(it->first);
          if (it_given != attributes_given.end())
          {
            // Этот Атрибут есть во входном перечне -> (1)
            element = &it_given->second;
          }
          else
          {
            // Этот Атрибут есть только в шаблоне Атрибутов -> (2)
            element = &it->second;
          }

          class_item_presentation
            << "    <rtdb:Value>"
            << getValueAsString(element, true) <<"</rtdb:Value>"<< std::endl
            << "  </rtdb:Attr>"<< std::endl;
        }

        class_item_presentation << "</rtdb:Point>" << std::endl;
        dump.assign(class_item_presentation.str());
    }
    else
    {
      dump.clear();
    }

    // Удалить из attributes_given динамически выделенные данные (тип данных DB_TYPE_BYTES)
    for (it_attr_pool  = attributes_given.begin();
         it_attr_pool != attributes_given.end();
         ++it_attr_pool)
    {
      switch(it_attr_pool->second.type)
      {
        case DB_TYPE_BYTES:
          delete it_attr_pool->second.value.dynamic.val_string;
        break;
        case DB_TYPE_BYTES4:
        case DB_TYPE_BYTES8:
        case DB_TYPE_BYTES12:
        case DB_TYPE_BYTES16:
        case DB_TYPE_BYTES20:
        case DB_TYPE_BYTES32:
        case DB_TYPE_BYTES48:
        case DB_TYPE_BYTES64:
        case DB_TYPE_BYTES80:
        case DB_TYPE_BYTES128:
        case DB_TYPE_BYTES256:
          delete[] it_attr_pool->second.value.dynamic.varchar;
          break;
        default:
          // nothing to do here
          break;
      }
    }

    attributes_given.clear();
    return dump;
}

//
// Прочитать сгенерированный файл с содержимым БДРВ
// Часть атрибутов и/или значений по-умолчанию может 
// отсутствовать, в этом случае нужно брать их из 
// структуры ClassDescriptionTable[]
//
bool xdb::processInstanceFile(const char* fpath)
{
  bool status = false;
  std::string buffer;
  std::string type;
  std::string value;
  // Хранение Атрибутов точки до момента их сброса на диск
  AttributeMap_t attributes;
  // Буфер для хранения состояния прочитанного Атрибута
  AttributeInfo_t attr_info; 
  // Сконвертированный из строки тип Атрибута
  DbType_t    db_type;
  int         class_idx = GOF_D_BDR_OBJCLASS_UNUSED;
  // NB: значения могут быть строковыми, и содержать пробелы
  // В таком случае необходимо читать до конца кавычек.
  // Или до конца строки, поскольку поле "Значение" является
  // последним в строке.
  std::string::size_type first;
  std::string::size_type second;
  univname_t  className;
  univname_t  pointName;
  univname_t  aliasFather;
  univname_t  instanceAlias;
  std::string instance_file_name(fpath); // полное имя файла instances
  std::string xml_file_name(fpath);      // полное имя выходного XML-файла
  std::string point_view;
  std::istringstream iss;
  int         indiceTab = 0;
  int         colonne;
  int         colvect;
  int         ligne;
  int         input_file_line = 0; // номер текущей строки во входном файле
  int         fieldCount = 0;
  recordType  typeRecord;
//  attrCategory      attrCateg;
  char*       tableStrDeType[rtMAX_FIELD_CNT];

  /*------------------------------------*/
  /* opens the file of the Rtap classes */
  /*------------------------------------*/
  instance_file_name += "/instances_total.dat";
  xml_file_name += "/instances_total.xml";
  std::ifstream ifs(instance_file_name.c_str());
  std::ofstream ofs(xml_file_name.c_str());

  if (!ofs.is_open())
  {
    LOG(ERROR) << "Unable to open output file '"<<xml_file_name<<"': "<<strerror(errno);
    return false;
  }
  ofs << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl
      << "<rtdb:RTDB_STRUCT xmlns:rtdb=\"http://www.example.com/rtap_db\"" << std::endl
      << "  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
      << "  xsi:schemaLocation=\"http://www.example.com/rtap_db rtap_db.xsd\">" << std::endl;

  if (!ifs.is_open())
  {
    LOG(ERROR) << "Unable to open input file '"<<instance_file_name<<"': "<<strerror(errno);
    return false;
  }

    // 2 первый байта = тип точки:
    //  "I "
    //  "S "
    //  "V "
    //  "DV"
    //  "T "
    //  "F "
    //  "DF"
    //  "A "
    //  "H "
    //  "J "
    //  "LF"
    //  "CF"
    //  "AV"
    //  "C0"
    //  "L0"
    //  
    while (getline(ifs, buffer))
    {
      input_file_line++;
      if (buffer.empty())
        continue;

      /*------------------------------------------*/
      /* reads the file and processes each record */
      /*------------------------------------------*/

      /* research the type of the record */
      typeRecord = xdb::getRecordType(buffer);
      // TODO: проверить, что происходит с памятью при разборе большого файла
      // поскольку istringstream не удаляется
      iss.clear();
      iss.str(buffer);
      value.clear();

      switch(typeRecord)
      {
         /*----------*/
         /* INSTANCE */
         /*----------*/
         case I_TYPE :
            status = false;

            if (indiceTab)
            {
               // writes all the data of the previous class
              xdb::dump_point(class_idx, pointName, attributes, point_view);
              if (false == point_view.empty())
              {
                ofs << point_view;
              }
            }

            // NB: Если длина алиаса равна 19 символов, во входном файле значение
            // instanceAlias склеивается с className. 
            if (iss >> type >> instanceAlias)
            {
              if (NAME_LENGTH < instanceAlias.size()-1)
              {
                className = instanceAlias.substr(NAME_LENGTH, strlen("ClassXX"));
                instanceAlias.resize(NAME_LENGTH);
              }
              else {
                iss >> className;
              }

              // Обработка случая склейки алиаса с полем Class
              if (std::string::npos != className.find("Class"))
              {
                // Получить числовой идентификатор Класса из строки:
                // className вида Class[0-9][0-9]
                class_idx = atoi(className.substr(5,2).c_str());
              }
              else
              {
                // Помимо названий вида ClassXX, допускаются:
                // [87] config
                // [75] FIXEDPOINT
                // [??] HISH_SITE
                // [??] HIST
                // [53 или 83] HIST_SET
                // [81] HIST_TABLE_H
                // [82] HIST_TABLE_J
                // [83] HIST_TABLE_M
                // [84] HIST_TABLE_QH
                //
                if (!className.compare("FIXEDPOINT"))
                  class_idx = GOF_D_BDR_OBJCLASS_FIXEDPOINT;
                else if (!className.compare("HIST_SET"))
                  class_idx = GOF_D_BDR_OBJCLASS_HIST_SET;
                else if (!className.compare("HIST_TABLE_H"))
                  class_idx = GOF_D_BDR_OBJCLASS_HIST_TABLE_H;
                else if (!className.compare("HIST_TABLE_J"))
                  class_idx = GOF_D_BDR_OBJCLASS_HIST_TABLE_J;
                else if (!className.compare("HIST_TABLE_M"))
                  class_idx = GOF_D_BDR_OBJCLASS_HIST_TABLE_M;
                else if (!className.compare("HIST_TABLE_QH"))
                  class_idx = GOF_D_BDR_OBJCLASS_HIST_TABLE_QH;
                else if (!className.compare("config"))
                  class_idx = GOF_D_BDR_OBJCLASS_CONFIG;
                else
                {
                  LOG(ERROR) << "Unknown class code: " << className;
                  class_idx = GOF_D_BDR_OBJCLASS_UNUSED;
                }
              }

              if (iss >> pointName >> aliasFather)
              {
                indiceTab++;
                status = true;
              }
            }
            else {
              LOG (WARNING) << "Check engine - " << instanceAlias << " fails";
            }
         break;

         /*--------*/
         /* SCALAR */
         /*--------*/
         case S_TYPE :
           // adds the scalar in the class --> init currentAttrName, currentDeType
           // Получить в глобальных переменных currentAttrName и currentDeType 
           // значения "Название атрибута" и "Тип данных" соответственно.
           // NB: По умолчанию в файле инстанса для всех атрибутов доступ PUBLIC
           //
           if (iss >> type >> currentAttrName >> type)
           {
             // type может быть: строковое, с плав. точкой, целое
             if (GetDbTypeFromString(type, db_type))
             {
               attr_info.name.assign(currentAttrName); // имя атрибута

               // Если в iss еще остались данные, и тип атрибута символьный,
               // нужно получить все это содержимое вместе с кавычками и пробелами.
               // NB: Приведенный здесь подход работает, если кавычки встречаются
               // у атрибутов только в колонке значения.
               first = buffer.find('\"');
               if (first != std::string::npos)
               {
                  second = buffer.rfind('\"');
                  if (second != std::string::npos)
                  {
                    if (first < second)
                    {
                      // Взять все содержимое, кроме крайних кавычек
                      value = buffer.substr(first+1, (second-first-1));
                    }
                  }
               }
               else // Строковое поле, но кавычек нет - так бывает (например, у UNIVNAME)
               {
                  if (VALUE_POSITION < buffer.size())
                  {
                    // В этом случае берем весь остаток строки
                    value = buffer.substr(VALUE_POSITION);
                  }
               }

               //memset(&attr_info.value, '\0', sizeof(attr_info.value));
               // Присвоить значение атрибуту в соответствии с полученным типом
               if (!getAttrValue(db_type, &attr_info, value))
               {
                 LOG(ERROR) << "Unable process type info '" << type
                            << "' for " << currentAttrName;
               }

               attributes.insert(AttributeMapPair_t(currentAttrName, attr_info));
             }
             else
             {
                /* ошибка определения типа атрибута */
                LOG(ERROR)<<"Given attribute type '"<<type
                          <<"' is unknown for attribute '"<<currentAttrName<<"'";
             }

           /* sets the data structure with the new value of the scalar */
           //status = initNewScalarValue(instanceAlias, value);
//         LOG (INFO) << " Разбирается "<< instanceAlias << "."
//                    << currentAttrName << " : " << type << " : " << value;
           }
           else
           {
             LOG(ERROR) << "Error parsing " << instanceAlias << "." << currentAttrName;
           }

         break;

         /*--------*/
         /* VECTOR */
         /*--------*/
         case V_TYPE :  /* and DV_TYPE */
           /* sets vector info ---> init currentAttrName, currentDeType */
           //status = setInfoVector(buffer, INSTANCE_FORMAT, &attrCateg);
           colvect = 0;
         break;

         /*-------------*/
         /* VECTOR DATA */
         /*-------------*/
         case DV_TYPE :
           /* initializes the data structure with the new value */
           //rc = initNewVectorValue(buffer, instanceAlias);
         break;

         /*-------------*/
         /* VECTOR DATA */
         /*-------------*/
         case AV_TYPE :
           /* initializes the data structure with the new value */
           colvect++;
         break;

         /*--------*/
         /* TABLE  */
         /*--------*/
         case T_TYPE :
           /* sets table info ---> init currentAttrName */
           //status = setInfoTable(buffer, INSTANCE_FORMAT, &attrCateg); 
           fieldCount = 0;
           colonne = 0;
           ligne = 0;
         break;

         /*-------------*/
         /* TABLE FIELD */
         /*-------------*/
         case F_TYPE : 
           /* initializes the fields data of tableField structure */
           status = initFieldTable(buffer, tableStrDeType, fieldCount);
           fieldCount++;
         break;

         /*------------*/
         /* TABLE DATA */
         /*------------*/
         case DF_TYPE :
           // ligne = buffer[со 2 по 7 позицию]
           // TODO: создание istringstream довольно затратно, переделать на повторное использование
           std::istringstream(buffer.substr(2, 7)) >> ligne;
//           LOG(INFO) << "ligne="<<ligne;
         break;

         /*------------*/
         /* TABLE DATA */
         /*------------*/
         case LF_TYPE :
           std::istringstream(buffer.substr(2, 7)) >> ligne;
//           LOG(INFO) << "ligne="<<ligne;
         break;

         /*------------*/
         /* TABLE DATA */
         /*------------*/
         case CF_TYPE :
           std::istringstream(buffer.substr(2, 7)) >> colonne;
//           LOG(INFO) << "ligne="<<colonne;
         break;

         /*------------*/
         /* TABLE DATA */
         /*------------*/
         case C0_TYPE :
           colonne = 0;
         break;

         /*------------*/
         /* TABLE DATA */
         /*------------*/
         case L0_TYPE :
           ligne = 0;
         break;

         /*----------------------*/
         /* LINK_HIST            */
         case H_TYPE :
         /* CE DEFINITION        */
         case A_TYPE :
         /* ALIAS CE DEFINITION  */
         /*----------------------*/
         case J_TYPE :
//           LOG (INFO) << "(" << typeRecord 
//            << ") : LINK_HIST|CE_DEFINITION|ALIAS_CE_DEFINITION : "
//            << buffer;
           break;

         default:
           LOG (INFO) << "#" << input_file_line 
            << " : (" << typeRecord << ") : UNKNOWN : '" << buffer <<"'";
      }
    }

  ifs.close();
  ofs << "</rtdb:RTDB_STRUCT>" << std::endl;
  ofs.close();

  /*------------------------------------*/
  /* adds all the values in the Rtap DB */
  /*------------------------------------*/
  if (indiceTab != 0)
  {
    LOG(INFO) << "Add " << indiceTab << " value(s)";
    status = true; // TODO Что делать с ранее полученным статусом false?
  }
  return status;
}

#if 0
bool setInfoVector(char *buffer, formatType leFormat, attrCategory* category)
{
   bool  status = true;
   char  attrName[NAME_SIZE+1];
   char  categ[CATEGORY_SIZE+1];
   char  deType[DETYPE_SIZE+1];
   char *pAttr;
   char *pCateg;
   char *pType;

   /* extracts the data from the buffer */
   pAttr = buffer + TYPE_ENREG_SIZE;
   pCateg = pAttr + NAME_SIZE;
   if (leFormat == CLASS_FORMAT)
      pType = pCateg + CATEGORY_SIZE + (MASK_SIZE * 2);
   else
      pType = pAttr + NAME_SIZE;

   strncpy(attrName, pAttr, NAME_SIZE);
   attrName[NAME_SIZE] = CNULL;  
   skipStr(attrName);  

   if (leFormat == CLASS_FORMAT)
      strncpy(categ, pCateg, CATEGORY_SIZE);
   else
      /* instance => surcharge d'attributs publics uniquement */
      strncpy(categ, STR_PUBLIC, CATEGORY_SIZE);
   categ[CATEGORY_SIZE] = CNULL;
   skipStr(categ);

   strncpy(deType, pType, DETYPE_SIZE);
   deType[DETYPE_SIZE] = CNULL;
   skipStr(deType); 

   /* init output parameter */
   if (strncmp(categ, STR_PUBLIC, CATEGORY_SIZE) == 0)
      *category = PUBLIC;
   else if (strncmp(categ, STR_PRIVATE, CATEGORY_SIZE) == 0)
      *category = PRIVATE;
   else
      LOG(ERROR) << "Must be 'PUBLIC' or 'PRIVATE', not " << categ;

   /* init global variables for the attribute name and DeType */
   strcpy(currentAttrName, attrName); 
   strcpy(currentStrDeType, deType);
   currentDeType = extractDeType(deType);

   LOG(INFO)<<categ<<" | "<<attrName<<"["<<deType<<"]";

   return status;   
}

bool setInfoTable(char *buffer, formatType leFormat, attrCategory* category)
{
   bool  status = true;
   char  categ[CATEGORY_SIZE+1];
   char *aux;

   /* extracts the data from the buffer */
   aux = buffer + TYPE_ENREG_SIZE;
   strncpy(currentAttrName, aux, NAME_SIZE); /* init global variable */
   currentAttrName[NAME_SIZE] = CNULL;
   skipStr(currentAttrName);

   if (leFormat == CLASS_FORMAT)
   {
      aux += NAME_SIZE;
      strncpy(categ, aux, CATEGORY_SIZE);
   }
   else
   {
      /* instance => surcharge d'attributs publics uniquement */
      strncpy(categ, STR_PUBLIC, CATEGORY_SIZE);
   }
   categ[CATEGORY_SIZE] = CNULL;
   skipStr(categ);

   /* init output parameter */
   if (strncmp(categ, STR_PUBLIC, CATEGORY_SIZE) == 0)
      *category = PUBLIC;
   else if (strncmp(categ, STR_PRIVATE, CATEGORY_SIZE) == 0)
      *category = PRIVATE;
   else
      LOG(ERROR) << "Must be 'PUBLIC' or 'PRIVATE', not " << categ;

   LOG(INFO)<<categ<<" | "<<currentAttrName;
   return status;   
}
#endif

bool xdb::translateInstance(const char* fpath)
{
  bool status = false;

  status = processClassFile(fpath);
  status = processInstanceFile(fpath);

  return status;
}

bool xdb::initFieldTable(std::string& buffer, char* tableStrDeType[], int fieldCount)
{
  bool status = true;
  assert(!buffer.empty());
  assert(tableStrDeType);
  assert(fieldCount>=0);
//  LOG(INFO) << "initFieldTable";
  return status;
}


#if 0
bool addClassPoint(char *buffer,
                   int classNum,
                   char* className,
                   char* cloneName,
                   char* buffAttrRef)
{
  char classKey[NUMERIC_SIZE+1];
  char residence[RESIDENCE_SIZE+1];
  char ceOrder[CE_ORDER_SIZE+1];
  char ceIndicator[CE_INDICATOR_SIZE+1];
  char pointName[NAME_SIZE+1];
  int  classId;
  char *aux;
  bool status = false;

  /*-----------------------------------*/
  /* extracts the data from the buffer */
  /*-----------------------------------*/
  aux = buffer + TYPE_ENREG_SIZE;
  strncpy(classKey, aux, NUMERIC_SIZE);
  classKey[NUMERIC_SIZE] = CNULL;

  aux += NUMERIC_SIZE;
  strncpy(pointName, aux, NAME_SIZE);
  pointName[NAME_SIZE] = CNULL;
  skipStr(static_cast<char*>(pointName));

  aux += NAME_SIZE;
  strncpy(className, aux, NAME_SIZE);
  className[NAME_SIZE] = CNULL;      /* output parameter */
  skipStr(className);

  aux += NAME_SIZE;
  strncpy(residence, aux, RESIDENCE_SIZE);
  residence[RESIDENCE_SIZE] = CNULL;

  aux += RESIDENCE_SIZE;
  strncpy(ceOrder, aux, CE_ORDER_SIZE);
  ceOrder[CE_ORDER_SIZE] = CNULL;

  aux += CE_ORDER_SIZE;
  strncpy(ceIndicator, aux, CE_INDICATOR_SIZE);
  ceIndicator[CE_INDICATOR_SIZE] = CNULL;

  classId = atoi(classKey);
  return status;
}

void skipStr(char* laChaine)
{
   char *ptr;
   int   lg;

   if ((ptr = strchr(laChaine, '\n')) != NULL)
      *ptr = CNULL;

   lg = strlen(laChaine) - 1;
   while(laChaine[lg] == ' ')
   {
      lg--;
   }
   laChaine[lg+1] = CNULL;
}
#endif

/*
  Получить числовое представление типа данных в словаре на основе строки
  типа rtBYTES...

  Все типы данных в виде строк находятся в хеше в качестве ключей, значениями
  для них являются соответствующие значения типа DbType_t.
  Заполняется хеш в функции LoadDbTypesDictionary()

  Возвращаемое значение перечисляемого типа, может принимать значения:
    - целое
    - с плав. точкой
    - строковое
 */
bool xdb::GetDbTypeFromString(std::string& s_t, DbType_t& db_t)
{
  bool status = false;
  DbTypesHashIterator_t it;

  db_t = DB_TYPE_UNDEF;

  it = dbTypesHash.find(s_t);
  if (it != dbTypesHash.end())
  {
    db_t= it->second;
    status = true;
  }

  return status;
}

// На входе код типа БДРВ, на выходе строковое представление типа, 
// согласно шаблону AttributeType файла rtap_db.xsd
const char* xdb::GetDbNameFromType(DbType_t& db_t)
{
  assert((DB_TYPE_UNDEF < db_t) && (db_t < DB_TYPE_LAST));
  assert(DbTypeDescription[db_t].code == db_t);

  return DbTypeDescription[db_t].name;
}

