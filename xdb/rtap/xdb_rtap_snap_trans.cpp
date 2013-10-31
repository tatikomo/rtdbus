#include <assert.h>
#include "glog/logging.h"

#include "config.h"
#include "xdb_rtap_snap_main.hpp"
#include "xdb_rtap_snap_trans.hpp"

using namespace xdb;


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

recordType
getRecordType(char *typeEnreg)
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
bool copyPoint(const char* buffer, const char* instance)
{
  bool status = false;
  return status;
}

bool processClassFile(const char* fname)
{
  bool status = false;
  return status;
}

bool processInstanceFile(const char* fname)
{
  bool status = false;
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

   classId = (rtUInt16)atoi(classKey);
  return status;
}
