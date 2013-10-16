#ifndef GEV_XDB_RTAP_CONST_H_
#define GEV_XDB_RTAP_CONST_H_
#pragma once

#include <vector>
#include <stdint.h>

#include "mco.h"
#include "config.h"

namespace xdb
{

#define SHORTLABEL_LENGTH   16
#define CODE_LENGTH         8
#define LABEL_LENGTH        80
#define UNIVNAME_LENGTH     32

/* NB: GOF_D_BDR_OBJCLASS... definitions are copied from $GOF/inc/gof_bdr_d.h file */
/* values of the OBJCLASS attributes */
/* ================================= */
/* -------------------------------------  Informations objects */
#define GOF_D_BDR_OBJCLASS_TS        0
#define GOF_D_BDR_OBJCLASS_TM        1
#define GOF_D_BDR_OBJCLASS_TR        2
#define GOF_D_BDR_OBJCLASS_TSA        3
#define GOF_D_BDR_OBJCLASS_TSC        4
#define GOF_D_BDR_OBJCLASS_TC        5
#define GOF_D_BDR_OBJCLASS_AL        6
#define GOF_D_BDR_OBJCLASS_ICS        7
#define GOF_D_BDR_OBJCLASS_ICM        8
#define GOF_D_BDR_OBJCLASS_REF        9
/* -------------------------------------  Network objects       */
#define GOF_D_BDR_OBJCLASS_PIPE        11
#define GOF_D_BDR_OBJCLASS_BORDER    12
#define GOF_D_BDR_OBJCLASS_CONSUMER    13
#define GOF_D_BDR_OBJCLASS_CORRIDOR    14
#define GOF_D_BDR_OBJCLASS_PIPELINE    15
/* -------------------------------------  Equipments objects    */
/* NB: update GOF_D_BDR_OBJCLASS_EQTLAST when adding an Equipments objects */
#define GOF_D_BDR_OBJCLASS_TL        16


#define GOF_D_BDR_OBJCLASS_VA        19
#define GOF_D_BDR_OBJCLASS_SC        20
#define GOF_D_BDR_OBJCLASS_ATC        21
#define GOF_D_BDR_OBJCLASS_GRC        22
#define GOF_D_BDR_OBJCLASS_SV        23
#define GOF_D_BDR_OBJCLASS_SDG        24
#define GOF_D_BDR_OBJCLASS_RGA        25
#define GOF_D_BDR_OBJCLASS_SSDG        26
#define GOF_D_BDR_OBJCLASS_BRG        27
#define GOF_D_BDR_OBJCLASS_SCP        28
#define GOF_D_BDR_OBJCLASS_STG        29
#define GOF_D_BDR_OBJCLASS_DIR        30
#define GOF_D_BDR_OBJCLASS_DIPL        31
#define GOF_D_BDR_OBJCLASS_METLINE    32
#define GOF_D_BDR_OBJCLASS_ESDG        33
#define GOF_D_BDR_OBJCLASS_SVLINE    34
#define GOF_D_BDR_OBJCLASS_SCPLINE    35
#define GOF_D_BDR_OBJCLASS_TLLINE    36
#define GOF_D_BDR_OBJCLASS_INVT        37
#define GOF_D_BDR_OBJCLASS_AUX1         38
#define GOF_D_BDR_OBJCLASS_AUX2         39
/* -------------------------------------  Fixed objects    */
#define GOF_D_BDR_OBJCLASS_SITE        45
/* -------------------------------------  System objects    */
/* NB: update GOF_D_BDR_OBJCLASS_SYSTLAST when adding an System objects */
#define GOF_D_BDR_OBJCLASS_SA        50
/* -------------------------------------  Alarms objects    */
#define GOF_D_BDR_OBJCLASS_GENAL    51
#define GOF_D_BDR_OBJCLASS_USERAL    52
#define GOF_D_BDR_OBJCLASS_HISTSET    53

#define GOF_D_BDR_OBJCLASS_EQTFIRST     16
#define GOF_D_BDR_OBJCLASS_EQTLAST      39
#define GOF_D_BDR_OBJCLASS_SYSTFIRST    50
#define GOF_D_BDR_OBJCLASS_SYSTLAST     50

#define GOF_D_BDR_OBJCLASS_NBMAX        54

#define GOF_D_BDR_OBJCLASS_FIXEDPOINT    79 /* NB это нестандарт, в ГОФО такой константы нет */
#define GOF_D_BDR_OBJCLASS_LASTUSED     87 /* NB это нестандарт, в ГОФО такой константы нет */
#define GOF_D_BDR_OBJCLASS_UNUSED    127

typedef char   shortlabel_t[SHORTLABEL_LENGTH+1];
typedef char   longlabel_t[LABEL_LENGTH+1];
typedef char   univname_t[UNIVNAME_LENGTH+1];
typedef char   code_t[CODE_LENGTH+1];

#define D_MISSING_OBJCODE   "MISSING"

typedef enum
{
  DB_TYPE_UNDEF     = 0,
  DB_TYPE_INTEGER8  = 1,
  DB_TYPE_INTEGER16 = 2,
  DB_TYPE_INTEGER32 = 3,
  DB_TYPE_INTEGER64 = 4,
  DB_TYPE_FLOAT     = 5,
  DB_TYPE_DOUBLE    = 6,
  DB_TYPE_BYTES     = 7,
  DB_TYPE_LAST      = 8 // fake type, used for limit array types
} DbType_t;

typedef struct
{
  uint16_t size;
  unsigned char *data;
} variable_t;

typedef union
{
  int64_t  val_int64;
  int32_t  val_int32;
  int16_t  val_int16;
  int8_t   val_int8;
  variable_t val_bytes; /* TODO: определить максимальное значение строки */
  float    val_float;
  double   val_double;
} AttrVal_t;

/* перечень значимых атрибутов и их типов */
typedef struct
{
  univname_t name;      /* имя атрибута */
  DbType_t   db_type;   /* его тип - целое, дробь, строка */
  AttrVal_t  value;     /* значение атрибута */
} AttributeInfo_t;

/* общие сведения по точке базы данных */
typedef struct
{
  int16_t       objclass;
  univname_t    alias;
  std::vector   <AttributeInfo_t> attr_info_list;
  univname_t    parent_alias;
  univname_t    code;
  autoid_t      id_SA;
  autoid_t      id_unity;
} PointDescription_t;

/* Хранилище набора атрибутов, их типов, кода и описания для каждого objclass */
typedef struct
{
  univname_t        name;   /* название класса */
  int8_t            code;   /* код класса */
  std::vector       <AttributeInfo_t> attr_info_list;
} ObjClassDescr_t;

extern ObjClassDescr_t ObjClassDescrTable[];
/* Получить универсальное имя на основе его алиаса */
extern int GetPointNameByAlias(univname_t, univname_t);

}

#endif

