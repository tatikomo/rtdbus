#if !defined XDB_RTAP_SNAP_TRANS_H_
#define XDB_RTAP_SNAP_TRANS_H_
#pragma once

#include "config.h"
#include "xdb_rtap_const.hpp"

namespace xdb
{

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
#define OK                  1
#define NOK                 0
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
    PUBLIC,
    PRIVATE,
    UNKNOWN
} attrCategory;

typedef enum
{
    C_TYPE,
    I_TYPE,
    S_TYPE,
    V_TYPE,
    DV_TYPE,
    T_TYPE,
    F_TYPE,
    DF_TYPE,
    A_TYPE,
    J_TYPE,
    H_TYPE,
    LF_TYPE,
    CF_TYPE,
    AV_TYPE,
    C0_TYPE,
    L0_TYPE,
    UNKNOWN_RECORD_TYPE
} recordType;

#define TYPE_ENREG_SIZE 2

bool initBranch(const char*);

bool deleteBranch(const char*);

bool copyPoint(char*, char*);

xdb::recordType getRecordType(std::string&);

void skipStr(char* laChaine);

bool extractRow(char*, int*);

bool processClassFile(const char*);

bool processInstanceFile(const char*);

bool addClassPoint(char *buffer,
                   int classNum,
                   char* className,
                   char* cloneName,
                   char* buffAttrRef);

bool addScalar(char*, char*, attrCategory*, char*);

bool setInfoScalar(char*, formatType, attrCategory*, std::string&);

bool setInfoVector(char*, formatType, attrCategory*);

bool setInfoTable(char*, formatType, attrCategory*);

bool initFieldTable(char*, char*[], int);

bool GetDbTypeFromString(std::string&, xdb::DbType_t&);

char* GetNextWord(char**, char*);

}
#endif
