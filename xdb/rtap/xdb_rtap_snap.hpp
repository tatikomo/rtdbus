#pragma once
#ifndef XDB_RTAP_SNAP_HPP
#define XDB_RTAP_SNAP_HPP

#include <string>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_common.hpp"

namespace xdb {

class RtEnvironment;

#define INSTANCES_FILE_XML "instances_total.xml"
#define INSTANCES_FILE_DAT "instances_total.dat"

/*--------------------------*/
/* buffer sizes             */
/*--------------------------*/
#define RECORD_SIZE       500
#define CE_RECORD_SIZE   2100
#define ALIAS_RECORD_SIZE 100

#define rtMAX_FIELD_CNT              255     /* allows fields 0 to 254 */
/*--------------------------*/
/* return codes             */
/*--------------------------*/
//#define OK                  1
//#define NOK                 0
#define IGNORED             2 

/*--------------------------*/
/* characters               */
/*--------------------------*/
#define CNULL             '\0'
#define COMPLETE_GEN       'C'
#define MODIF_GEN          'M'
#define ADDA_GEN           'A'
#define ADDC_GEN           'S'
#define ADDI_GEN           'I'

/*-------------------------------------------*/
/* strings                                   */
/*-------------------------------------------*/
#define STR_RAM            "RAM "
#define STR_DISK           "DISK"
#define STR_NATURAL        "Natural     "
#define STR_USER_DEF       "User defined"
#define STR_ENABLED        "Enabled"
#define STR_OPTIMISED      "Optimized"
#define STR_PREFIXE_CLONE  "CLONE_"
#define STR_PUBLIC         "PUB"
#define STR_PRIVATE        "PRV"
#define STR_LINK_HIST_NULL "@[]"
 
/*-------------------------------------------*/
/* Rtap adress formats                       */
/*-------------------------------------------*/
#define FORMAT_ADDR_ALIAS       "%s"
#define FORMAT_ADDR_ATTR        "%s.%s"
#define FORMAT_ADDR_VECTOR_ELT  "%s.%s(%d)"
#define FORMAT_ADDR_TABLE_ELT   "%s.%s(%d:%d,%d:%d)"
#define FORMAT_ADDR_DB_XREF     "@[%d]"
#define FORMAT_DB_ADDR_DB_XREF  "@[%d.%d]"

/*-------------------------------------------*/
/* names of points and attributes            */
/*-------------------------------------------*/
#define kRootName          "root"
#define kRootClass         "ROOT"
#define kGenClassesPoint   "Gen_Classes"
#define kGenClonesPoint    "Gen_Clones"
#define kClassConfigPoint  "class config"
#define kIndexAttr         "index"
#define kNamesAttr         "names"
#define kClassRefAttr      "CLASS_REFERENCE"

#define  FMT_ATT_VAL_FIELD_DEBUT_LIG	"DF%05ld"
#define  FMT_ATT_VAL_FIELD_SUITE_LIG	"%05ld%s@"
/* */
#ifdef CLASS
#define  FMT_CLASSE			"C %05ld%-19s%-4s%-12s%-9s\n"
#define  FMT_ATT_VAL_SCALAR		"S %-19s%-3s%-4s%-4s%-15s%s\n"
#define  FMT_ATT_VECTOR			"V %-19s%-3s%-4s%-4s%-15s%05ld\n"
#define  FMT_ATT_VAL_VECTOR		"DV%05ld%s\n"
#define  FMT_ATT_TABLE			"T %-19s%-3s%-4s%-4s%05ld%05ld\n"
#define  FMT_ATT_TABLE_FIELD		"F %-19s%-15s\n"
#else
#define  FMT_INSTANCE			"I %-19s%-19s%-19s%-19s\n"
#define  FMT_ATT_VAL_SCALAR		"S %-19s%-15s%s\n"
#define  FMT_ATT_VECTOR			"V %-19s%s\n"
#define  FMT_ATT_VAL_VECTOR		"DV%05ld%s\n"
#define  FMT_ATT_TABLE			"T %s\n"
#define  FMT_ATT_TABLE_FIELD		"F %-19s%s\n"
#define  FMT_ALIAS_CE_DEFINITION	"I %-19s%s\n"
#define  FMT_CE_DEFINITION		"A %-19s%s\n"
#endif

/*--------------------------------*/
/*   record field lengthes Rtap   */ 
/*--------------------------------*/
#define SEPARATOR           '@'
#define POINT               '.'
#define CATEGORY_SIZE        3
#define TYPE_ENREG_SIZE      2
#define NUMERIC_SIZE         5
#define NAME_SIZE           19
#define RESIDENCE_SIZE       4
#define CE_ORDER_SIZE       12
#define CE_INDICATOR_SIZE    9
#define MASK_SIZE            4
#define DETYPE_SIZE         15
#define VALUE_SIZE         255
#define CE_DEF_SIZE       2000
// Начальная позиция поля значения в файле instances
#define VALUE_POSITION      36

#define NAME_LENGTH         19
#define TYPE_ENREG_SIZE      2

/*-------------------------------------------------*/
/* type of formats (class file or instance file)   */
/*-------------------------------------------------*/
typedef enum 
{
    CLASS_FORMAT,
    INSTANCE_FORMAT,
    UNKNOWN_FORMAT
} formatType;

/*-------------------------------------------------*/
/* type of attributes : public or private          */
/*-------------------------------------------------*/
typedef enum
{
    PUBLIC  = 1,
    PRIVATE = 2,
    UNKNOWN = 3
} attrCategory;

typedef enum
{
    C_TYPE  = 1,
    I_TYPE  = 2,
    S_TYPE  = 3,
    V_TYPE  = 4,
    DV_TYPE = 5,
    T_TYPE  = 6,
    F_TYPE  = 7,
    DF_TYPE = 8,
    A_TYPE  = 9,
    J_TYPE  = 10,
    H_TYPE  = 11,
    LF_TYPE = 12,
    CF_TYPE = 13,
    AV_TYPE = 14,
    C0_TYPE = 15,
    L0_TYPE = 16,
    UNKNOWN_RECORD_TYPE = 17
} recordType;


typedef enum 
{
  LOAD_FROM_XML = 1,
  SAVE_TO_XML   = 2
} Commands_t;

// ------------------------------------------------------------
bool loadFromXML(RtEnvironment*, const char*);

bool saveToXML(RtEnvironment*, const char*);

bool translateInstance(const char*);

bool initBranch(const char*);

bool deleteBranch(const char*);

bool copyPoint(char*, char*);

recordType getRecordType(std::string&);

//void skipStr(char* laChaine);

//bool extractRow(char*, int*);

int processClassFile(const char*);

bool processInstanceFile(const char*);

// Сброс законченного набора атрибутов точки в XML-файл
std::string& dump_point(
    // IN
    int class_idx,
    const std::string& pointName,
    // IN-OUT
    xdb::AttributeMap_t& attributes_given,
    // OUT
    std::string& dump);

bool addClassPoint(char *buffer,
                   int classNum,
                   char* className,
                   char* cloneName,
                   char* buffAttrRef);

bool addScalar(char*, char*, attrCategory*, char*);

bool setInfoScalar(char*, formatType, attrCategory*, std::string&);

bool setInfoVector(char*, formatType, attrCategory*);

bool setInfoTable(char*, formatType, attrCategory*);

bool initFieldTable(std::string&, char*[], int);

bool GetDbTypeFromString(std::string&, DbType_t&);

const char* GetDbNameFromType(const DbType_t& db_t);

} // namespace xdb

#endif
