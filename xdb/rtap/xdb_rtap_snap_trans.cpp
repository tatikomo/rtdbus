#include <assert.h>
#include <map>
#include <string>
#include <stdlib.h> // atoi()
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
typedef std:pair  <const std::string, xdb::DbType_t> DbTypesHashPair_t;
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

xdb::recordType getRecordType(char *typeEnreg)
{
   if (strncmp(typeEnreg, "C ", TYPE_ENREG_SIZE) == 0)
      return C_TYPE;
   else if (strncmp(typeEnreg, "I ", TYPE_ENREG_SIZE) == 0)
      return I_TYPE;
   else if (strncmp(typeEnreg, "S ", TYPE_ENREG_SIZE) == 0)
      return S_TYPE;
   else if (strncmp(typeEnreg, "V ", TYPE_ENREG_SIZE) == 0)
      return V_TYPE;
   else if (strncmp(typeEnreg, "DV", TYPE_ENREG_SIZE) == 0)
      return DV_TYPE;
   else if (strncmp(typeEnreg, "T ", TYPE_ENREG_SIZE) == 0)
      return T_TYPE;
   else if (strncmp(typeEnreg, "F ", TYPE_ENREG_SIZE) == 0)
      return F_TYPE;
   else if (strncmp(typeEnreg, "DF", TYPE_ENREG_SIZE) == 0)
      return DF_TYPE;
   else if (strncmp(typeEnreg, "A ", TYPE_ENREG_SIZE) == 0)
      return A_TYPE;
   else if (strncmp(typeEnreg, "H ", TYPE_ENREG_SIZE) == 0)
      return H_TYPE;
   else if (strncmp(typeEnreg, "J ", TYPE_ENREG_SIZE) == 0)
      return J_TYPE;
   else if (strncmp(typeEnreg, "LF", TYPE_ENREG_SIZE) == 0)
      return LF_TYPE;
   else if (strncmp(typeEnreg, "CF", TYPE_ENREG_SIZE) == 0)
      return CF_TYPE;
   else if (strncmp(typeEnreg, "AV", TYPE_ENREG_SIZE) == 0)
      return AV_TYPE;
   else if (strncmp(typeEnreg, "C0", TYPE_ENREG_SIZE) == 0)
      return C0_TYPE;
   else if (strncmp(typeEnreg, "L0", TYPE_ENREG_SIZE) == 0)
      return L0_TYPE;
   else
      return UNKNOWN_RECORD_TYPE;  
}

// buffer - строка из входного файла
// 2 байта = тип точки:
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
bool copyPoint(char* buffer, char* instance)
{
  bool status = true;
  char               aliasInst[NAME_SIZE+1];
  char               className[NAME_SIZE+1];
  char               pointName[NAME_SIZE+1];
  char               aliasFather[NAME_SIZE+1];
  char              *aux;
  
  /* extracts the data from the buffer */
  aux = buffer + TYPE_ENREG_SIZE;
  strncpy(aliasInst, aux, NAME_SIZE);
  aliasInst[NAME_SIZE] = CNULL;
  skipStr(aliasInst); 

  aux += NAME_SIZE;
  strncpy(className, aux, NAME_SIZE);
  className[NAME_SIZE] = CNULL; 
  skipStr(className); 

  aux += NAME_SIZE;
  strncpy(pointName, aux, NAME_SIZE);
  pointName[NAME_SIZE] = CNULL;

  aux += NAME_SIZE;
  strncpy(aliasFather, aux, NAME_SIZE);
  aliasFather[NAME_SIZE] = CNULL;

  /* init output parameter */
  strcpy(instance, aliasInst);

  LOG(INFO) << className<<":"<<pointName<<"("<<aliasInst<<") "<<"father "<<aliasFather;

  return status;
}

bool addScalar(char *buffer, char *alias, attrCategory* category, char *scalarValue)
{
  bool  status = false;
  char  attrName[NAME_SIZE+1];
  char  categ[CATEGORY_SIZE+1];
  char  rmask[MASK_SIZE+1];
  char  wmask[MASK_SIZE+1];
  char  deType[DETYPE_SIZE+1];
  char  value[VALUE_SIZE+1];
  char *aux;

  /* extracts the data from the buffer */
  aux = buffer + TYPE_ENREG_SIZE;
  strncpy(attrName, aux, NAME_SIZE);
  attrName[NAME_SIZE] = CNULL;
  skipStr(attrName);

  aux += NAME_SIZE;
  strncpy(categ, aux, CATEGORY_SIZE);
  categ[CATEGORY_SIZE] = CNULL;
   
  aux += CATEGORY_SIZE;
  strncpy(rmask, aux, MASK_SIZE);
  rmask[MASK_SIZE] = CNULL;

  aux += MASK_SIZE;
  strncpy(wmask, aux, MASK_SIZE);
  wmask[MASK_SIZE] = CNULL;

  aux += MASK_SIZE;
  strncpy(deType, aux, DETYPE_SIZE);
  deType[DETYPE_SIZE] = CNULL;  
  skipStr(deType); 

  aux += DETYPE_SIZE;
  strncpy(value, aux, VALUE_SIZE);
  value[VALUE_SIZE] = CNULL;
  skipStr(value);

  /* init output parameters */
  strcpy(scalarValue, value); 
  if (strncmp(categ, STR_PUBLIC, CATEGORY_SIZE) == 0)
    *category = PUBLIC;
  else if (strncmp(categ, STR_PRIVATE, CATEGORY_SIZE) == 0)
    *category = PRIVATE;
  else
    LOG(ERROR)<<"Must be "<<STR_PUBLIC<<" or "<<STR_PRIVATE<<" : "<<categ<<" invalid",

  /* init global variables for the attribute name and DeType */
  strcpy(currentAttrName, attrName); 
  strcpy(currentStrDeType, deType);
  currentDeType = extractDeType(deType);

  return status;
}

bool extractRow(char *buffer, int* row)
{
   bool status = true;
   char  rang[NUMERIC_SIZE + 1];
   char *aux;

   aux = buffer + TYPE_ENREG_SIZE;
   strncpy(rang, aux, NUMERIC_SIZE);
   rang[NUMERIC_SIZE] = CNULL;
   *row = atoi(rang);
   return status;
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
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("DISP_TABLE_H",  GOF_D_BDR_OBJCLASS_DISP_TABLE_H));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("DISP_TABLE_J",  GOF_D_BDR_OBJCLASS_DISP_TABLE_J));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("DISP_TABLE_M",  GOF_D_BDR_OBJCLASS_DISP_TABLE_M));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("DISP_TABLE_QH", GOF_D_BDR_OBJCLASS_DISP_TABLE_QH));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("FIXEDPOINT",    GOF_D_BDR_OBJCLASS_FIXEDPOINT));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("HIST_SET",      GOF_D_BDR_OBJCLASS_HIST_SET));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("HIST_TABLE_H",  GOF_D_BDR_OBJCLASS_HIST_TABLE_H));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("HIST_TABLE_J",  GOF_D_BDR_OBJCLASS_HIST_TABLE_J));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("HIST_TABLE_M",  GOF_D_BDR_OBJCLASS_HIST_TABLE_M));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("HIST_TABLE_QH", GOF_D_BDR_OBJCLASS_HIST_TABLE_QH));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("HIST_TABLE_SAMPLE", GOF_D_BDR_OBJCLASS_HIST_TABLE_SAMPLE));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("TIME_AVAILABLE",GOF_D_BDR_OBJCLASS_TIME_AVAILABLE));
   dbPointsTypeHash.insert(DbPointsTypeHashPair_t("config",        GOF_D_BDR_OBJCLASS_CONFIG));

   /* Используется в ts.c в качестве словаря соответствия между алиасами и универсальными именами */
   //dbAliasHash = g_hash_table_new(g_str_hash, g_str_equal);
}

/*
 * По каждому классу точек прочитать справочные данные
 *
 */
bool processClassFile(const char* fname)
{
  bool status = false;
  int        objclass;
  FILE*      rfile;
  char       fpath[255];
  univname_t s_univname;
  univname_t s_access;
  univname_t s_type;
  DbType_t   db_type;
  AttributeInfo_t* p_attr_info; 
  char*      p_line   = NULL;
  char*      p_cursor = NULL;
  char       fline[LINE_BUFFER_LEN+1];

  LoadDbTypesDictionary();

  for (objclass=0; objclass <= GOF_D_BDR_OBJCLASS_LASTUSED; objclass++)
  {
    if (strcmp(ObjClassDescrTable[objclass].name, D_MISSING_OBJCODE)==0)
      continue;

    sprintf(fpath, "/export/home/users/stmd/SCADA/dat/dict/%02d_%s.dat",
            ObjClassDescrTable[objclass].code, ObjClassDescrTable[objclass].name);

    g_printf("%s\n", fpath);

    if ((rfile = fopen(fpath, "r"))!=NULL)
    {
      while (!feof(rfile) && fgets(fline, LINE_BUFFER_LEN, rfile))
      {
        /* пропускать строки, начинающиеся с символов [C#TV] */
        switch ((int)fline[0])
        {
          case 'C':
          case 'V':
          case 'T':
          case 'F':
          case 'D':
          case '#':
               break;
          default:
              p_line = p_cursor = g_strdup(fline);
              p_cursor = g_strstrip(p_line);
              p_cursor += 2; /* skip first symbol */
              p_cursor = GetNextWord(&p_cursor, (char*)s_univname);
              p_cursor = GetNextWord(&p_cursor, (char*)s_access);
              p_cursor = GetNextWord(&p_cursor, (char*)s_type);
              g_free(p_line);
              /* type может быть: строковое, с плав. точкой, целое */
              if (!GetDbTypeFromString((const char*)s_type, &db_type))
              {
                /* ошибка определения типа атрибута */
                g_error("Given attribute type '%s' is unknown for class '%s'",
                    s_type, s_univname);
              }
              g_printf("%-8s attribute %-18s type %-10s:%d\n", 
                    ObjClassDescrTable[objclass].name, s_univname, s_type, db_type);

              /*
                 Добавить для экземпляра данного objclass перечень атрибутов,
                 подлежащих чтению из instances_total.dat, и их родовые типы
                 (целое, дробь, строка)
               */
              p_attr_info = (AttributeInfo_t*) g_new(AttributeInfo_t, 1);
              g_assert(p_attr_info != NULL);
              memset((void*)p_attr_info, '\0', sizeof(AttributeInfo_t));
//              g_printf("CREATE NEW AttributeInfo_t for objclassdescr at %p\n", p_attr_info);
              strcpy(p_attr_info->name, s_univname /*, sizeof(p_attr_info->name)*/);
              p_attr_info->db_type = db_type;

              ObjClassDescrTable[objclass].attr_info_list = 
                    g_slist_append(ObjClassDescrTable[objclass].attr_info_list, p_attr_info);
        }
      }
    }
  }

  return status;
}

bool processInstanceFile(const char* fname)
{
  bool status = false;
  char *rc;
  FILE             *ficInst;
  int               indiceTab;
  int               colonne;
  int               colvect;
  int               ligne;
  int               fieldCount;
  int               classCounter;
  recordType        typeRecord;
  attrCategory      attrCateg;
  char              buffer[RECORD_SIZE];
  char              classAlias[NAME_SIZE+1];
  char              instanceAlias[NAME_SIZE+1];
  char              value[VALUE_SIZE + 1];
  char*             tableStrDeType[rtMAX_FIELD_CNT];

  /*------------------------------------*/
  /* opens the file of the Rtap classes */
  /*------------------------------------*/
  if ((ficInst = fopen(fname,"r")) == NULL)
  {
    LOG(INFO) << "Unable to open file '"<<fname<<"': "<<strerror(errno);
    return false;
  }

  /*------------------------------------------*/
  /* reads the file and processes each record */
  /*------------------------------------------*/
  classCounter = 0;

  while (NULL != (rc=fgets(buffer, RECORD_SIZE, ficInst)))
  {
      /* research the type of the record */
      typeRecord = getRecordType(buffer);

      switch(typeRecord)
      {
         /*----------*/
         /* INSTANCE */
         /*----------*/
         case I_TYPE :
            if (indiceTab)
            {
              LOG(INFO) << "I_TYPE Table";
            }

            status = copyPoint(buffer, instanceAlias);
         break;

         /*--------*/
         /* SCALAR */
         /*--------*/
         case S_TYPE :
           /* adds the scalar in the class --> init currentAttrName, currentDeType */
           status = setInfoScalar(buffer, INSTANCE_FORMAT, &attrCateg, value);
           /* sets the data structure with the new value of the scalar */
           //status = initNewScalarValue(instanceAlias, value);
         break;

         /*--------*/
         /* VECTOR */
         /*--------*/
         case V_TYPE :  /* and DV_TYPE */
           /* sets vector info ---> init currentAttrName, currentDeType */
           status = setInfoVector(buffer, INSTANCE_FORMAT, &attrCateg);
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
           status = setInfoTable(buffer, INSTANCE_FORMAT, &attrCateg); 
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
           status = extractRow(buffer, &ligne);
         break;

         /*------------*/
         /* TABLE DATA */
         /*------------*/
         case LF_TYPE :
           status = extractRow(buffer, &ligne);
         break;

         /*------------*/
         /* TABLE DATA */
         /*------------*/
         case CF_TYPE :
           status = extractRow(buffer, &colonne);
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
           status = false;
      }
  }
  fclose(ficInst);

  /*------------------------------------*/
  /* adds all the values in the Rtap DB */
  /*------------------------------------*/
  if (indiceTab != 0)
  {
    LOG(INFO) << "Add table values";
  }

  return status;
}

bool setInfoScalar(char *buffer, formatType leFormat, attrCategory* category, char *scalarValue)
{
   bool  status = true;
   char  attrName[NAME_SIZE+1];
   char  categ[CATEGORY_SIZE+1];
   char  deType[DETYPE_SIZE+1];
   char  value[VALUE_SIZE+1];
   char *pAttr;
   char *pCateg;
   char *pType;
   char *pValue;

   /* extracts the data from the buffer */
   pAttr = buffer + TYPE_ENREG_SIZE;
   pCateg = pAttr + NAME_SIZE;
   if (leFormat == CLASS_FORMAT)
      pType = pCateg + CATEGORY_SIZE + (MASK_SIZE * 2);
   else
      pType = pAttr + NAME_SIZE;
   pValue = pType + DETYPE_SIZE;

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

   strncpy(value, pValue, VALUE_SIZE);
   value[VALUE_SIZE] = CNULL;
   skipStr(value);

   /* init output parameters */
   strcpy(scalarValue, value);
   if (strncmp(categ, STR_PUBLIC, CATEGORY_SIZE) == 0)
      *category = PUBLIC;
   else if (strncmp(categ, STR_PRIVATE, CATEGORY_SIZE) == 0)
      *category = PRIVATE;
   else
      LOG(ERROR) << "Must be 'PUBLIC' or 'PRIVATE', not '"<<categ<<"'";

   /* init global variables for the attribute name and DeType */
   strcpy(currentAttrName, attrName); 
   strcpy(currentStrDeType, deType);
   currentDeType = extractDeType(deType);

   LOG(INFO)<<categ<<" | "<<attrName<<"["<<deType<<"] = "<<scalarValue;
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

bool xdb::translateInstance(const char* fname)
{
  bool status = false;
  LOG(INFO) << "translate input file "<<fname;

  processClassFile(fname);
  processInstanceFile(fname);

  return status;
}

bool initFieldTable(char *buffer, char* tableStrDeType[], int fieldCount)
{
  bool status = true;
  LOG(INFO) << "initFieldTable";
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
   skipStr(pointName); 

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


