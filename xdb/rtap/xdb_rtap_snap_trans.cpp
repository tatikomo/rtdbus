#include <assert.h>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <stdlib.h> // atoi()
#include <string.h> // memcpy()
#include <stdio.h>  // fopen()
#include <time.h>   // timeval
#include "glog/logging.h"

#include "config.h"
#include "xdb_rtap_const.hpp"
#include "xdb_rtap_snap_main.hpp"
#include "xdb_rtap_snap_trans.hpp"

using namespace xdb;

/*------------------*/
/* Global variables */
/*------------------*/
#define LINE_BUFFER_LEN    255
char     currentAttrName [NAME_SIZE+1];   
char     currentStrDeType[DETYPE_SIZE+1];
rtDeType currentDeType;
 	
typedef std::map  <const std::string, xdb::DbType_t> DbTypesHash_t;
typedef std::map  <const std::string, xdb::DbType_t>::iterator DbTypesHashIterator_t;
typedef std::pair <const std::string, xdb::DbType_t> DbTypesHashPair_t;
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


// Создать точку c заданным именем и без атрибутов, подключив её к ROOT
//
bool initBranch(const char* pointName)
{
  assert(pointName);
  LOG(INFO) << "creates point root/"<<pointName;
}

// Удалить точку c заданным именем
//
bool deleteBranch(const char* pointName)
{
  assert(pointName);
  LOG(INFO) << "delete point "<<pointName;
}

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

xdb::recordType xdb::getRecordType(std::string& typeEnreg)
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
   dbTypesHash.insert(DbTypesHashPair_t("rtUNDEFINED",  DB_TYPE_UNDEF));
   dbTypesHash.insert(DbTypesHashPair_t("rtLOGICAL",    DB_TYPE_INTEGER8));
   dbTypesHash.insert(DbTypesHashPair_t("rtINT8",       DB_TYPE_INTEGER8));
   dbTypesHash.insert(DbTypesHashPair_t("rtUINT8",      DB_TYPE_INTEGER8));
   dbTypesHash.insert(DbTypesHashPair_t("rtINT16",      DB_TYPE_INTEGER16));
   dbTypesHash.insert(DbTypesHashPair_t("rtUINT16",     DB_TYPE_INTEGER16));
   dbTypesHash.insert(DbTypesHashPair_t("rtINT32",      DB_TYPE_INTEGER32));
   dbTypesHash.insert(DbTypesHashPair_t("rtUINT32",     DB_TYPE_INTEGER32));
   dbTypesHash.insert(DbTypesHashPair_t("rtFLOAT",      DB_TYPE_FLOAT));
   dbTypesHash.insert(DbTypesHashPair_t("rtDOUBLE",     DB_TYPE_DOUBLE));
   dbTypesHash.insert(DbTypesHashPair_t("rtBYTES4",     DB_TYPE_BYTES));
   dbTypesHash.insert(DbTypesHashPair_t("rtBYTES8",     DB_TYPE_BYTES));
   dbTypesHash.insert(DbTypesHashPair_t("rtBYTES12",    DB_TYPE_BYTES));
   dbTypesHash.insert(DbTypesHashPair_t("rtBYTES16",    DB_TYPE_BYTES));
   dbTypesHash.insert(DbTypesHashPair_t("rtBYTES20",    DB_TYPE_BYTES));
   dbTypesHash.insert(DbTypesHashPair_t("rtBYTES32",    DB_TYPE_BYTES));
   dbTypesHash.insert(DbTypesHashPair_t("rtBYTES48",    DB_TYPE_BYTES));
   dbTypesHash.insert(DbTypesHashPair_t("rtBYTES64",    DB_TYPE_BYTES));
   dbTypesHash.insert(DbTypesHashPair_t("rtBYTES80",    DB_TYPE_BYTES));
   dbTypesHash.insert(DbTypesHashPair_t("rtBYTES128",   DB_TYPE_BYTES));
   dbTypesHash.insert(DbTypesHashPair_t("rtBYTES256",   DB_TYPE_BYTES));
   dbTypesHash.insert(DbTypesHashPair_t("rtDB_XREF",    DB_TYPE_INTEGER64)); // autoid_t eXtremeDB
   dbTypesHash.insert(DbTypesHashPair_t("rtDATE",       DB_TYPE_INTEGER64));
   dbTypesHash.insert(DbTypesHashPair_t("rtTIME_OF_DAY",DB_TYPE_INTEGER64));
   dbTypesHash.insert(DbTypesHashPair_t("rtABS_TIME",   DB_TYPE_INTEGER64));

   /* TODO release memory */
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

   /* Используется в ts.c в качестве словаря соответствия между алиасами и универсальными именами */
   //dbAliasHash = g_hash_table_new(g_str_hash, g_str_equal);
}

/*
 * По каждому классу точек прочитать справочные данные
 *
 */
int xdb::processClassFile(const char* fpath)
{
  bool status = false;
  int        objclass;
  int        loadedClasses = 0;
  char       fname[255];
  std::string s_skip;
  std::string s_univname;
  std::string s_access;
  std::string s_type;
  std::string s_default_value;
  DbType_t    db_type;
  AttributeInfo_t* p_attr_info; 
  std::string line;
  std::string::size_type found;

  assert(fpath);
  LoadDbTypesDictionary();

  for (objclass=0; objclass <= GOF_D_BDR_OBJCLASS_LASTUSED; objclass++)
  {
    ObjClassDescrTable[objclass].attr_info_list = NULL;
    if (!strncmp(ObjClassDescrTable[objclass].name, D_MISSING_OBJCODE, UNIVNAME_LENGTH))
      continue;

    sprintf(fname, "%s/%02d_%s.dat",
            fpath,
            ObjClassDescrTable[objclass].code,
            ObjClassDescrTable[objclass].name);

    std::ifstream ifs(fname);

    if (ifs.is_open())
    {
      while (getline(ifs, line))
      {
        /* пропускать строки, начинающиеся с символов [ALJCVTFD#] */
        if (0 /*std::string::npos*/ == (found = line.find_first_of("ALJCVTFD#")))
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

        if (0 /*std::string::npos*/ == (found = line.find_first_of("I")))
        {
          // начало новой точки
          // создать массив лексем
          //
          // std::cout << line << std::endl;
          // std::istringstream iss(line);
          // std::cout << "new point: " << line << std::endl;
          continue;
        }

        if (0 /*std::string::npos*/ == (found = line.find_first_of("S")))
        {
          // S OBJCLASS           PUB        rtUINT8        0
          // создать массив лексем
          std::istringstream iss(line);
          if (iss >> s_skip >> s_univname >> s_access >> s_type)
          {
//            std::cout << "OK: "<<s_univname<<" : "<<s_type;

            if (iss >> s_default_value)
            {
//              std::cout << " : "<<s_default_value<<std::endl;
              //   iss >> std::ws;
            }
            else
            {
              // нет значения по умолчанию для этого атрибута
              s_default_value.clear();
            }
            
            if (iss.eof()) {
//              std::cout << std::endl;
            }
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
        if (!ObjClassDescrTable[objclass].attr_info_list)
             ObjClassDescrTable[objclass].attr_info_list = new att_list_t;

        p_attr_info = (AttributeInfo_t*) new AttributeInfo_t;
        p_attr_info->name.assign(s_univname.c_str());
        p_attr_info->db_type = db_type;
        // Присвоить значение атрибуту в соответствии с полученным типом
        switch(p_attr_info->db_type)
        {
          case xdb::DB_TYPE_BYTES:
            p_attr_info->value.val_bytes.size = s_default_value.size();
            // для пустого значения
            if (p_attr_info->value.val_bytes.size)
            {
               p_attr_info->value.val_bytes.data = new char[p_attr_info->value.val_bytes.size + 1];
               memcpy(p_attr_info->value.val_bytes.data,
                   s_default_value.c_str(),
                   p_attr_info->value.val_bytes.size);
            }
            else p_attr_info->value.val_bytes.data = NULL;
            break;

          case xdb::DB_TYPE_INTEGER8:
                  p_attr_info->value.val_int8 = atoi(s_default_value.c_str());
                  break;
          case xdb::DB_TYPE_INTEGER16:
                  p_attr_info->value.val_int16 = atoi(s_default_value.c_str());
                  break;
          case xdb::DB_TYPE_INTEGER32:
                  p_attr_info->value.val_int32 = atoi(s_default_value.c_str());
                  break;
          case xdb::DB_TYPE_INTEGER64:
                  p_attr_info->value.val_int64 = atoi(s_default_value.c_str());
                  break;
          case xdb::DB_TYPE_FLOAT:
                  p_attr_info->value.val_float = atof(s_default_value.c_str());
                  break;
          case xdb::DB_TYPE_DOUBLE:
                  p_attr_info->value.val_double = atof(s_default_value.c_str());
                  break;
        }

        ObjClassDescrTable[objclass].attr_info_list->push_back(*p_attr_info);
        delete p_attr_info;
      }

      ifs.close();
      loadedClasses++;
      LOG(INFO) << "Class file "<<fname<<" loaded";
    }
    else
    {
//      LOG(ERROR) << "Ошибка "<<ifs.rdstate()<<" чтения входного файла "<<fname;
    }
  }

  return loadedClasses;
}

//
// Прочитать сгенерированный файл с содержимым БДРВ
// Часть атрибутов и/или значений по-умолчанию может 
// отсутствовать, в этом случае нужно брать их из 
// структуры ObjClassDescrTable[]
//
bool xdb::processInstanceFile(const char* fpath)
{
  bool status = false;
  std::string buffer;
  std::string type;
  std::string value;
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
  std::string instance_file_name(fpath);
  std::string::size_type found;
  char *rc;
  int               indiceTab = 0;
  int               colonne;
  int               colvect;
  int               ligne;
  int               fieldCount;
  int               classCounter;
  xdb::recordType   typeRecord;
  attrCategory      attrCateg;
  char*             tableStrDeType[rtMAX_FIELD_CNT];

  /*------------------------------------*/
  /* opens the file of the Rtap classes */
  /*------------------------------------*/
  instance_file_name += "/instances_total.dat";
  std::ifstream ifs(instance_file_name.c_str());

  if (ifs.is_open())
  {
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
      /*------------------------------------------*/
      /* reads the file and processes each record */
      /*------------------------------------------*/
      classCounter = 0;

      /* research the type of the record */
      typeRecord = getRecordType(buffer);
      // TODO: проверить, что происходит с памятью при разборе большого файла
      // поскольку istringstream не удаляется
      std::istringstream iss(buffer);
//      LOG(INFO) << "\tparse " << buffer;
      value.clear();

      switch(typeRecord)
      {
         /*----------*/
         /* INSTANCE */
         /*----------*/
         case I_TYPE :
            status = false;
            //instanceAlias.clear();

            if (indiceTab)
            {
               // writes all the data of the previous class
              LOG(INFO) << "DUMP " << instanceAlias;
//              LOG(INFO) << "I_TYPE Table";
            }

            // NB: Если длина алиаса равна 19 символов, во входном файле значение
            // instanceAlias склеивается с className. 
            // TODO: Предусмотреть обработку этого случая
            if (iss >> type >> instanceAlias)
            {
              if (NAME_LENGTH < instanceAlias.size()-1)
              {
                LOG(INFO) << "#" << instanceAlias;

                className = instanceAlias.substr(NAME_LENGTH, strlen("ClassXX"));
                instanceAlias.resize(NAME_LENGTH);
              }
              else {
                iss >> className;
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
           /* adds the scalar in the class --> init currentAttrName, currentDeType */
           // Получить в глобальных переменных currentAttrName и currentDeType 
           // значения "Название атрибута" и "Тип данных" соответственно.
           // NB: По умолчанию в файле инстанса для всех атрибутов доступ PUBLIC
           //
           if (iss >> type >> currentAttrName >> type)
           {
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
                    value = buffer.substr(first, ++second);
                }
             }

           /* sets the data structure with the new value of the scalar */
           //status = initNewScalarValue(instanceAlias, value);
             LOG (INFO) << " Разбирается "<< instanceAlias << "." << currentAttrName << " : " << type << " : " << value;
           }
           else
           {
             LOG(ERROR) << "Ошибка разбора " << instanceAlias << "." << currentAttrName;
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
           std::istringstream(buffer.substr(2, 7)) >> ligne;
           LOG(INFO) << "ligne="<<ligne;
         break;

         /*------------*/
         /* TABLE DATA */
         /*------------*/
         case LF_TYPE :
           std::istringstream(buffer.substr(2, 7)) >> ligne;
           LOG(INFO) << "ligne="<<ligne;
         break;

         /*------------*/
         /* TABLE DATA */
         /*------------*/
         case CF_TYPE :
           std::istringstream(buffer.substr(2, 7)) >> colonne;
           LOG(INFO) << "ligne="<<colonne;
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
           status = false;
           break;

         default:
           LOG (INFO) << "default";
           status = false;
      }
    }
  }
  else
  {
    LOG(INFO) << "Unable to open file '"<<instance_file_name<<"': "<<strerror(errno);
    return false;
  }

  /*------------------------------------*/
  /* adds all the values in the Rtap DB */
  /*------------------------------------*/
  if (indiceTab != 0)
  {
    LOG(INFO) << "Add " << indiceTab << " value(s)";
  }
  return status;
}

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
  LOG(INFO) << "initFieldTable";
  return status;
}


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

void xdb::skipStr(char* laChaine)
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
bool xdb::GetDbTypeFromString(std::string& s_t, xdb::DbType_t& db_t)
{
  bool status = false;
  DbTypesHashIterator_t it;

  db_t = DB_TYPE_UNDEF;

  it = dbTypesHash.find(s_t);
  if (it != dbTypesHash.end())
  {
    db_t= it->second;
  }

  if (db_t != DB_TYPE_UNDEF)
    status = true;

  return status;
}

