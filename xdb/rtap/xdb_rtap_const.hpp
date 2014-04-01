#pragma once
#ifndef GEV_XDB_RTAP_CONST_H_
#define GEV_XDB_RTAP_CONST_H_

#include <map>
#include <string>
#include <stdint.h>

#include "mco.h"
#include "config.h"

namespace xdb
{

#define SHORTLABEL_LENGTH   16
#define CODE_LENGTH         8
#define LABEL_LENGTH        80
#define UNIVNAME_LENGTH     32
#define NAME_LENGTH         19

/* NB: GOF_D_BDR_OBJCLASS... definitions are copied from $GOF/inc/gof_bdr_d.h file */
/* values of the OBJCLASS attributes */
/* ================================= */
/* -------------------------------------  Informations objects */
#define GOF_D_BDR_OBJCLASS_TS          0
#define GOF_D_BDR_OBJCLASS_TM          1
#define GOF_D_BDR_OBJCLASS_TR          2
#define GOF_D_BDR_OBJCLASS_TSA         3
#define GOF_D_BDR_OBJCLASS_TSC         4
#define GOF_D_BDR_OBJCLASS_TC          5
#define GOF_D_BDR_OBJCLASS_AL          6
#define GOF_D_BDR_OBJCLASS_ICS         7
#define GOF_D_BDR_OBJCLASS_ICM         8
#define GOF_D_BDR_OBJCLASS_REF         9
/* -------------------------------------  Network objects       */
#define GOF_D_BDR_OBJCLASS_PIPE       11
#define GOF_D_BDR_OBJCLASS_BORDER     12
#define GOF_D_BDR_OBJCLASS_CONSUMER   13
#define GOF_D_BDR_OBJCLASS_CORRIDOR   14
#define GOF_D_BDR_OBJCLASS_PIPELINE   15
/* -------------------------------------  Equipments objects    */
/* NB: update GOF_D_BDR_OBJCLASS_EQTLAST when adding an Equipments objects */
#define GOF_D_BDR_OBJCLASS_TL         16

#define GOF_D_BDR_OBJCLASS_VA         19
#define GOF_D_BDR_OBJCLASS_SC         20
#define GOF_D_BDR_OBJCLASS_ATC        21
#define GOF_D_BDR_OBJCLASS_GRC        22
#define GOF_D_BDR_OBJCLASS_SV         23
#define GOF_D_BDR_OBJCLASS_SDG        24
#define GOF_D_BDR_OBJCLASS_RGA        25
#define GOF_D_BDR_OBJCLASS_SSDG       26
#define GOF_D_BDR_OBJCLASS_BRG        27
#define GOF_D_BDR_OBJCLASS_SCP        28
#define GOF_D_BDR_OBJCLASS_STG        29
#define GOF_D_BDR_OBJCLASS_DIR        30
#define GOF_D_BDR_OBJCLASS_DIPL       31
#define GOF_D_BDR_OBJCLASS_METLINE    32
#define GOF_D_BDR_OBJCLASS_ESDG       33
#define GOF_D_BDR_OBJCLASS_SVLINE     34
#define GOF_D_BDR_OBJCLASS_SCPLINE    35
#define GOF_D_BDR_OBJCLASS_TLLINE     36
#define GOF_D_BDR_OBJCLASS_INVT       37
#define GOF_D_BDR_OBJCLASS_AUX1       38
#define GOF_D_BDR_OBJCLASS_AUX2       39
/* -------------------------------------  Fixed objects    */
#define GOF_D_BDR_OBJCLASS_SITE       45
/* -------------------------------------  System objects    */
/* NB: update GOF_D_BDR_OBJCLASS_SYSTLAST when adding an System objects */
#define GOF_D_BDR_OBJCLASS_SA         50
/* -------------------------------------  Alarms objects    */
#define GOF_D_BDR_OBJCLASS_GENAL      51
#define GOF_D_BDR_OBJCLASS_USERAL     52
#define GOF_D_BDR_OBJCLASS_HISTSET    53

#define GOF_D_BDR_OBJCLASS_EQTFIRST   16
#define GOF_D_BDR_OBJCLASS_EQTLAST    39
#define GOF_D_BDR_OBJCLASS_SYSTFIRST  50
#define GOF_D_BDR_OBJCLASS_SYSTLAST   50

#define GOF_D_BDR_OBJCLASS_NBMAX      54
/* NB это нестандарт, в ГОФО таких констант нет */
#define GOF_D_BDR_OBJCLASS_DISP_TABLE_H  75
#define GOF_D_BDR_OBJCLASS_DISP_TABLE_J  76
#define GOF_D_BDR_OBJCLASS_DISP_TABLE_M  77
#define GOF_D_BDR_OBJCLASS_DISP_TABLE_QH 78
#define GOF_D_BDR_OBJCLASS_FIXEDPOINT    79 
#define GOF_D_BDR_OBJCLASS_HIST_SET      80
#define GOF_D_BDR_OBJCLASS_HIST_TABLE_H  81
#define GOF_D_BDR_OBJCLASS_HIST_TABLE_J  82
#define GOF_D_BDR_OBJCLASS_HIST_TABLE_M  83
#define GOF_D_BDR_OBJCLASS_HIST_TABLE_QH 84
#define GOF_D_BDR_OBJCLASS_HIST_TABLE_SAMPLE 85
#define GOF_D_BDR_OBJCLASS_TIME_AVAILABLE 86
#define GOF_D_BDR_OBJCLASS_CONFIG     87

#define GOF_D_BDR_OBJCLASS_LASTUSED   87
#define GOF_D_BDR_OBJCLASS_UNUSED    127

#if 0
typedef char   shortlabel_t[SHORTLABEL_LENGTH+1];
typedef char   longlabel_t[LABEL_LENGTH+1];
typedef char   univname_t[UNIVNAME_LENGTH+1];
typedef char   code_t[CODE_LENGTH+1];
#else
typedef std::string   shortlabel_t;
typedef std::string   longlabel_t;
typedef std::string   univname_t;
typedef std::string   code_t;
#endif

#define D_MISSING_OBJCODE   "MISSING"

typedef enum
{
  DB_TYPE_UNDEF     = 0,
  DB_TYPE_INT8      = 1,
  DB_TYPE_UINT8     = 2,
  DB_TYPE_INT16     = 3,
  DB_TYPE_UINT16    = 4,
  DB_TYPE_INT32     = 5,
  DB_TYPE_UINT32    = 6,
  DB_TYPE_INT64     = 7,
  DB_TYPE_UINT64    = 8,
  DB_TYPE_FLOAT     = 9,
  DB_TYPE_DOUBLE    = 10,
  DB_TYPE_BYTES     = 11, // переменная длина строки
  DB_TYPE_BYTES4    = 12,
  DB_TYPE_BYTES8    = 13,
  DB_TYPE_BYTES12   = 14,
  DB_TYPE_BYTES16   = 15,
  DB_TYPE_BYTES20   = 16,
  DB_TYPE_BYTES32   = 17,
  DB_TYPE_BYTES48   = 18,
  DB_TYPE_BYTES64   = 19,
  DB_TYPE_BYTES80   = 20,
  DB_TYPE_BYTES128  = 21,
  DB_TYPE_BYTES256  = 22,
  DB_TYPE_LAST      = 23 // fake type, used for limit array types
} DbType_t;

// NB: формат хранения строк UTF-8
typedef struct
{
  uint16_t size; // 16384
  char *data;
} variable_t;

#define ELEMENT_DESCRIPTION_MAXLEN  15
typedef enum
{
    rtRESERVED0  = 0,
    rtLOGICAL    = 1,
    rtINT8       = 2,
    rtUINT8      = 3,
    rtINT16      = 4,
    rtUINT16     = 5,
    rtINT32      = 6,
    rtUINT32     = 7,
    rtFLOAT      = 8,
    rtDOUBLE     = 9,
    rtPOLAR      = 10,
    rtRECTANGULAR= 11,
    rtRESERVED12 = 12,
    rtRESERVED13 = 13,
    rtRESERVED14 = 14,
    rtRESERVED15 = 15,
    rtBYTES4     = 16,
    rtBYTES8     = 17,
    rtBYTES12    = 18,
    rtBYTES16    = 19,
    rtBYTES20    = 20,
    rtBYTES32    = 21,
    rtBYTES48    = 22,
    rtBYTES64    = 23,
    rtBYTES80    = 24,
    rtBYTES128   = 25,
    rtBYTES256   = 26,
    rtRESERVED27 = 27,
    rtDB_XREF    = 28,
    rtDATE       = 29,
    rtTIME_OF_DAY= 30,
    rtASB_TIME   = 31,
    rtUNDEFINED  = 32
} rtDeType;

#define GOF_D_BDR_MAX_DE_TYPE rtUNDEFINED

// Таблица описателей типов данных БДРВ
typedef struct
{
  // порядковый номер
  DbType_t   code;
  const char name[ELEMENT_DESCRIPTION_MAXLEN+1];
  // размер типа данных
  size_t     size;
} DbTypeDescription_t;

// Элемент соответствия между кодом типа RTAP и БДРВ
typedef struct
{
  rtDeType de_type; // код RTAP
  DbType_t db_type; // код eXtremeDB
} DeTypeToDbTypeLink;

// Элемент соответствия между кодом типа БДРВ и RTAP
typedef struct
{
  DbType_t db_type; // код eXtremeDB
  rtDeType de_type; // код RTAP
} DbTypeToDeTypeLink;

// Таблица описателей типов данных RTAP
typedef struct
{
  // порядковый номер
  rtDeType code;
  // описание
  char name[ELEMENT_DESCRIPTION_MAXLEN+1];
  // размер типа данных
  size_t size;
} DeTypeDescription_t;


typedef union
{
  int8_t   val_int8;
  uint8_t  val_uint8;
  int16_t  val_int16;
  uint16_t val_uint16;
  int32_t  val_int32;
  uint32_t val_uint32;
  int64_t  val_int64;
  uint64_t val_uint64;
  variable_t val_bytes; // NB: максимальное значение строки =16384
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

typedef std::map  <const std::string, xdb::AttributeInfo_t> AttributeMap_t;
typedef std::map  <const std::string, xdb::AttributeInfo_t>::iterator AttributeMapIterator_t;
typedef std::pair <const std::string, xdb::AttributeInfo_t> AttributeMapPair_t;

/* общие сведения по точке базы данных */
typedef struct
{
  int16_t        objclass;
  univname_t     alias;
  AttributeMap_t attributes;
  univname_t     parent_alias;
  univname_t     code;
  autoid_t       id_SA;
  autoid_t       id_unity;
} PointDescription_t;

/* Хранилище набора атрибутов, их типов, кода и описания для каждого objclass */
typedef struct
{
  char              name[UNIVNAME_LENGTH+1];  // название класса
  int8_t            code;                     // код класса
  // NB: Используется указатель AttributeMap_t* для облегчения 
  // статической инициализации массива
  AttributeMap_t   *attributes_pool; // набор атрибутов с доступом по имени
} ClassDescription_t;

extern ClassDescription_t ClassDescriptionTable[];
extern const DeTypeDescription_t rtDataElem[];
// Получить универсальное имя на основе его алиаса
extern int GetPointNameByAlias(univname_t&, univname_t&);

// Хранилище описаний типов данных БДРВ
extern const DbTypeDescription_t DbTypeDescription[];
extern const DeTypeToDbTypeLink DeTypeToDbType[];
}

#endif

