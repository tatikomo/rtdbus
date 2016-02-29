#include <vector>
#include <time.h>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "xdb_common.hpp"

using namespace xdb;

// Таблица соответствия "Тип данных как индекс" => "размер строковой части для set_s_value()"
// NB: синхронизировать ее с таблицей типов в xdb/common/xdb_common.hpp
const uint16_t xdb::var_size[xdb::DB_TYPE_LAST+1] = {
    /* DB_TYPE_UNDEF     = 0  */  0,
    /* DB_TYPE_LOGICAL   = 1  */  0,
    /* DB_TYPE_INT8      = 2  */  0,
    /* DB_TYPE_UINT8     = 3  */  0,
    /* DB_TYPE_INT16     = 4  */  0,
    /* DB_TYPE_UINT16    = 5  */  0,
    /* DB_TYPE_INT32     = 6  */  0,
    /* DB_TYPE_UINT32    = 7  */  0,
    /* DB_TYPE_INT64     = 8  */  0,
    /* DB_TYPE_UINT64    = 9  */  0,
    /* DB_TYPE_FLOAT     = 10 */  0,
    /* DB_TYPE_DOUBLE    = 11 */  0,
    /* DB_TYPE_BYTES     = 12 */  65535, // переменная длина строки
    /* DB_TYPE_BYTES4    = 13 */  4 * sizeof(wchar_t),
    /* DB_TYPE_BYTES8    = 14 */  8 * sizeof(wchar_t),
    /* DB_TYPE_BYTES12   = 15 */  12 * sizeof(wchar_t),
    /* DB_TYPE_BYTES16   = 16 */  16 * sizeof(wchar_t),
    /* DB_TYPE_BYTES20   = 17 */  20 * sizeof(wchar_t),
    /* DB_TYPE_BYTES32   = 18 */  32 * sizeof(wchar_t),
    /* DB_TYPE_BYTES48   = 19 */  48 * sizeof(wchar_t),
    /* DB_TYPE_BYTES64   = 20 */  64 * sizeof(wchar_t),
    /* DB_TYPE_BYTES80   = 21 */  80 * sizeof(wchar_t),
    /* DB_TYPE_BYTES128  = 22 */  128 * sizeof(wchar_t),
    /* DB_TYPE_BYTES256  = 23 */  256 * sizeof(wchar_t),
    /* DB_TYPE_ABSTIME   = 24 */  sizeof(timeval),
    /* DB_TYPE_LAST      = 25 */  0
};

// Соответствие между индексом атрибута, и его типом и размером
const AttrTypeDescription_t xdb::AttrTypeDescription[] = 
{
//#include "dat/attr_type.gen"
// NB: Поле size для типов данных DB_TYPE_BYTES* определяет максимальный,
// а не фактический размер, поскольку в кодировке UTF-8 символ может занимать
// как 1 (латиница), так и 2 байта (кириллица).
//    (index)
//    |
//    |                             Указатель на строку с названием атрибута (name)
//    |                             |
//    |                             |                       Тип данных (type)
//    |                             |                       |
//    |                             |                       |               размер атрибута в байтах, макс. 32кБ (size)
//    |                             |                       |               |
    { RTDB_ATT_IDX_ACTIONTYP,       RTDB_ATT_ACTIONTYP,     DB_TYPE_UINT16, sizeof(uint16_t) },   // 0
    { RTDB_ATT_IDX_ALARMBEGIN,      RTDB_ATT_ALARMBEGIN,    DB_TYPE_UINT32, sizeof(uint32_t) },   // 1
    { RTDB_ATT_IDX_ALARMBEGINACK,   RTDB_ATT_ALARMBEGINACK, DB_TYPE_UINT32, sizeof(uint32_t) },   // 2
    { RTDB_ATT_IDX_ALARMENDACK,     RTDB_ATT_ALARMENDACK,   DB_TYPE_UINT32, sizeof(uint32_t) },   // 3
    { RTDB_ATT_IDX_ALARMSYNTH,      RTDB_ATT_ALARMSYNTH,    DB_TYPE_UINT8,  sizeof(uint8_t) },    // 4
    { RTDB_ATT_IDX_ALDEST,          RTDB_ATT_ALDEST,        DB_TYPE_LOGICAL,sizeof(bool) },       // 5
    { RTDB_ATT_IDX_ALINHIB,         RTDB_ATT_ALINHIB,       DB_TYPE_LOGICAL,sizeof(bool) },       // 6
    { RTDB_ATT_IDX_CONFREMOTECMD,   RTDB_ATT_CONFREMOTECMD, DB_TYPE_LOGICAL,sizeof(bool) },       // 7
    { RTDB_ATT_IDX_CONVERTCOEFF,    RTDB_ATT_CONVERTCOEFF,  DB_TYPE_DOUBLE, sizeof(double) },     // 8
    { RTDB_ATT_IDX_CURRENT_SHIFT_TIME,
                                    RTDB_ATT_CURRENT_SHIFT_TIME,
                                                            DB_TYPE_ABSTIME,sizeof(timeval) },    // 9
    { RTDB_ATT_IDX_DATEAINS,        RTDB_ATT_DATEAINS,      DB_TYPE_BYTES12,sizeof(wchar_t)*12 }, // 10
    { RTDB_ATT_IDX_DATEHOURM,       RTDB_ATT_DATEHOURM,     DB_TYPE_ABSTIME,sizeof(timeval) },    // 11
    { RTDB_ATT_IDX_DATERTU,         RTDB_ATT_DATERTU,       DB_TYPE_ABSTIME,sizeof(timeval) },    // 12
    { RTDB_ATT_IDX_DELEGABLE,       RTDB_ATT_DELEGABLE,     DB_TYPE_UINT8,  sizeof(uint8_t) },    // 13
    { RTDB_ATT_IDX_DISPP,           RTDB_ATT_DISPP,         DB_TYPE_BYTES20,sizeof(wchar_t)*20 }, // 14
    { RTDB_ATT_IDX_FLGMAINTENANCE,  RTDB_ATT_FLGMAINTENANCE,DB_TYPE_LOGICAL,sizeof(bool) },       // 15
    { RTDB_ATT_IDX_FLGREMOTECMD,    RTDB_ATT_FLGREMOTECMD,  DB_TYPE_LOGICAL,sizeof(bool) },       // 16
    { RTDB_ATT_IDX_FUNCTION,        RTDB_ATT_FUNCTION,      DB_TYPE_BYTES20,sizeof(wchar_t)*20 }, // 17
    { RTDB_ATT_IDX_GRADLEVEL,       RTDB_ATT_GRADLEVEL,     DB_TYPE_DOUBLE, sizeof(double) },     // 18
    { RTDB_ATT_IDX_HISTOTYPE,       RTDB_ATT_HISTOTYPE,     DB_TYPE_UINT16, sizeof(uint16_t) },   // 19
    { RTDB_ATT_IDX_INHIB,           RTDB_ATT_INHIB,         DB_TYPE_LOGICAL,sizeof(bool) },       // 20
    { RTDB_ATT_IDX_INHIBLOCAL,      RTDB_ATT_INHIBLOCAL,    DB_TYPE_LOGICAL,sizeof(bool) },       // 21
    { RTDB_ATT_IDX_KMREFDWN,        RTDB_ATT_KMREFDWN,      DB_TYPE_DOUBLE, sizeof(double) },     // 22
    { RTDB_ATT_IDX_KMREFUPS,        RTDB_ATT_KMREFUPS,      DB_TYPE_DOUBLE, sizeof(double) },     // 23
    { RTDB_ATT_IDX_LABEL,           RTDB_ATT_LABEL,         DB_TYPE_BYTES20,sizeof(wchar_t)*20 }, // 24
    { RTDB_ATT_IDX_L_CONSUMER,      RTDB_ATT_L_CONSUMER,    DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 25
    { RTDB_ATT_IDX_L_CORRIDOR,      RTDB_ATT_L_CORRIDOR,    DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 26
    { RTDB_ATT_IDX_L_DIPL,          RTDB_ATT_L_DIPL,        DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 27
    { RTDB_ATT_IDX_L_EQT,           RTDB_ATT_L_EQT,         DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 28
    { RTDB_ATT_IDX_L_EQTORBORDWN,   RTDB_ATT_L_EQTORBORDWN, DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 29
    { RTDB_ATT_IDX_L_EQTORBORUPS,   RTDB_ATT_L_EQTORBORUPS, DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 30
    { RTDB_ATT_IDX_L_EQTTYP,        RTDB_ATT_L_EQTTYP,      DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 31
    { RTDB_ATT_IDX_LINK_HIST,       RTDB_ATT_LINK_HIST,     DB_TYPE_UINT64, sizeof(uint64_t) },   // 32
    { RTDB_ATT_IDX_L_NET,           RTDB_ATT_L_NET,         DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 33
    { RTDB_ATT_IDX_L_NETTYPE,       RTDB_ATT_L_NETTYPE,     DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 34
    { RTDB_ATT_IDX_LOCALFLAG,       RTDB_ATT_LOCALFLAG,     DB_TYPE_UINT8,  sizeof(uint8_t) },    // 35
    { RTDB_ATT_IDX_L_PIPE,          RTDB_ATT_L_PIPE,        DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 36
    { RTDB_ATT_IDX_L_PIPELINE,      RTDB_ATT_L_PIPELINE,    DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 37
    { RTDB_ATT_IDX_L_SA,            RTDB_ATT_L_SA,          DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 38
    { RTDB_ATT_IDX_L_TL,            RTDB_ATT_L_TL,          DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 39
    { RTDB_ATT_IDX_L_TM,            RTDB_ATT_L_TM,          DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 40
    { RTDB_ATT_IDX_L_TYPINFO,       RTDB_ATT_L_TYPINFO,     DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 41
    { RTDB_ATT_IDX_MAXVAL,          RTDB_ATT_MAXVAL,        DB_TYPE_DOUBLE, sizeof(double) },     // 42
    { RTDB_ATT_IDX_MINVAL,          RTDB_ATT_MINVAL,        DB_TYPE_DOUBLE, sizeof(double) },     // 43
    { RTDB_ATT_IDX_MNVALPHY,        RTDB_ATT_MNVALPHY,      DB_TYPE_DOUBLE, sizeof(double) },     // 44
    { RTDB_ATT_IDX_MXFLOW,          RTDB_ATT_MXFLOW,        DB_TYPE_DOUBLE, sizeof(double) },     // 45
    { RTDB_ATT_IDX_MXPRESSURE,      RTDB_ATT_MXPRESSURE,    DB_TYPE_DOUBLE, sizeof(double) },     // 46
    { RTDB_ATT_IDX_MXVALPHY,        RTDB_ATT_MXVALPHY,      DB_TYPE_DOUBLE, sizeof(double) },     // 47
    { RTDB_ATT_IDX_NAMEMAINTENANCE, RTDB_ATT_NAMEMAINTENANCE,
                                                            DB_TYPE_BYTES12,sizeof(wchar_t)*12 }, // 48
    { RTDB_ATT_IDX_NMFLOW,          RTDB_ATT_NMFLOW,        DB_TYPE_DOUBLE, sizeof(double) },     // 49
    { RTDB_ATT_IDX_NMPRESSURE,      RTDB_ATT_NMPRESSURE,    DB_TYPE_DOUBLE, sizeof(double) },     // 50
// NB: OBJCLASS - особый случай, этот атрибут более не является самостоятельным,
// став принадлежностью XDBPoint
    { RTDB_ATT_IDX_OBJCLASS,        RTDB_ATT_OBJCLASS,      DB_TYPE_UINT8,  sizeof(uint8_t) },    // 51
    { RTDB_ATT_IDX_PLANFLOW,        RTDB_ATT_PLANFLOW,      DB_TYPE_DOUBLE, sizeof(double) },     // 52
    { RTDB_ATT_IDX_PLANPRESSURE,    RTDB_ATT_PLANPRESSURE,  DB_TYPE_DOUBLE, sizeof(double) },     // 53
    { RTDB_ATT_IDX_PREV_DISPATCHER, RTDB_ATT_PREV_DISPATCHER,
                                                            DB_TYPE_BYTES12,sizeof(wchar_t)*12 }, // 54
    { RTDB_ATT_IDX_PREV_SHIFT_TIME, RTDB_ATT_PREV_SHIFT_TIME,
                                                            DB_TYPE_UINT64, sizeof(uint64_t) },   // 55
    { RTDB_ATT_IDX_REMOTECONTROL,   RTDB_ATT_REMOTECONTROL, DB_TYPE_UINT8,  sizeof(uint8_t) },    // 56
    { RTDB_ATT_IDX_SHORTLABEL,      RTDB_ATT_SHORTLABEL,    DB_TYPE_BYTES16,sizeof(wchar_t)*16 }, // 57
    { RTDB_ATT_IDX_STATUS,          RTDB_ATT_STATUS,        DB_TYPE_UINT8,  sizeof(uint8_t) },    // 58
    { RTDB_ATT_IDX_SUPPLIER,        RTDB_ATT_SUPPLIER,      DB_TYPE_BYTES16,sizeof(wchar_t)*16 }, // 59
    { RTDB_ATT_IDX_SUPPLIERMODE,    RTDB_ATT_SUPPLIERMODE,  DB_TYPE_BYTES16,sizeof(wchar_t)*16 }, // 60
    { RTDB_ATT_IDX_SUPPLIERSTATE,   RTDB_ATT_SUPPLIERSTATE, DB_TYPE_BYTES16,sizeof(wchar_t)*16 }, // 61
    { RTDB_ATT_IDX_SYNTHSTATE,      RTDB_ATT_SYNTHSTATE,    DB_TYPE_UINT8,  sizeof(uint8_t) },    // 62
    { RTDB_ATT_IDX_TSSYNTHETICAL,   RTDB_ATT_TSSYNTHETICAL, DB_TYPE_UINT8,  sizeof(uint8_t) },    // 63
    { RTDB_ATT_IDX_TYPE,            RTDB_ATT_TYPE,          DB_TYPE_UINT32, sizeof(uint32_t) },   // 64
    { RTDB_ATT_IDX_UNITY,           RTDB_ATT_UNITY,         DB_TYPE_BYTES20,sizeof(wchar_t)*20 }, // 65
    { RTDB_ATT_IDX_UNITYCATEG,      RTDB_ATT_UNITYCATEG,    DB_TYPE_UINT8,  sizeof(uint8_t) },    // 66
// NB: UNIVNAME - особый случай, этот атрибут более не является самостоятельным,
// став принадлежностью XDBPoint
// Размерность определяется значением TAG_NAME_MAXLEN в файле config.h
    { RTDB_ATT_IDX_UNIVNAME,        RTDB_ATT_TAG,           DB_TYPE_BYTES32,sizeof(wchar_t)*TAG_NAME_MAXLEN }, // 67
// NB: VAL - особый случай, может быть как целочисленным, так и double
    { RTDB_ATT_IDX_VAL,             RTDB_ATT_VAL,           DB_TYPE_DOUBLE, sizeof(double) },     // 68
// NB: VALACQ - особый случай, может быть как целочисленным, так и double 
    { RTDB_ATT_IDX_VALACQ,          RTDB_ATT_VALACQ,        DB_TYPE_DOUBLE, sizeof(double) },     // 69
    { RTDB_ATT_IDX_VALEX,           RTDB_ATT_VALEX,         DB_TYPE_DOUBLE, sizeof(double) },     // 70
    { RTDB_ATT_IDX_VALID,           RTDB_ATT_VALID,         DB_TYPE_UINT8,  sizeof(uint8_t) },    // 71
    { RTDB_ATT_IDX_VALIDACQ,        RTDB_ATT_VALIDACQ,      DB_TYPE_UINT8,  sizeof(uint8_t) },    // 72
    { RTDB_ATT_IDX_VALIDCHANGE,     RTDB_ATT_VALIDCHANGE,   DB_TYPE_UINT8,  sizeof(uint8_t) },    // 73
// NB: VALMANUAL - особый случай, может быть как целочисленным, так и double
    { RTDB_ATT_IDX_VALMANUAL,       RTDB_ATT_VALMANUAL,     DB_TYPE_DOUBLE, sizeof(double) },     // 74
    { RTDB_ATT_IDX_UNEXIST,         "",                     DB_TYPE_UNDEF,  0 }                   // 75
};


// Перекодировочная таблица замены типам RTAP на тип eXtremeDB
// Индекс элемента - код типа данных RTAP
// Значение элемента - соответствующий индексу тип данных в eXtremeDB
//
const DeTypeToDbTypeLink DeTypeToDbType[] = 
{
    { rtRESERVED0,  DB_TYPE_UNDEF },
    { rtLOGICAL,    DB_TYPE_LOGICAL },
    { rtINT8,       DB_TYPE_INT8 },
    { rtUINT8,      DB_TYPE_UINT8 },
    { rtINT16,      DB_TYPE_INT16 },
    { rtUINT16,     DB_TYPE_UINT16 },
    { rtINT32,      DB_TYPE_INT32 },
    { rtUINT32,     DB_TYPE_UINT32 },
    { rtFLOAT,      DB_TYPE_FLOAT },
    { rtDOUBLE,     DB_TYPE_DOUBLE },
    { rtPOLAR,      DB_TYPE_UNDEF },
    { rtRECTANGULAR,DB_TYPE_UNDEF },
    { rtRESERVED12, DB_TYPE_UNDEF },
    { rtRESERVED13, DB_TYPE_UNDEF },
    { rtRESERVED14, DB_TYPE_UNDEF },
    { rtRESERVED15, DB_TYPE_UNDEF },
    { rtBYTES4,     DB_TYPE_BYTES4 },
    { rtBYTES8,     DB_TYPE_BYTES8 },
    { rtBYTES12,    DB_TYPE_BYTES12 },
    { rtBYTES16,    DB_TYPE_BYTES16 },
    { rtBYTES20,    DB_TYPE_BYTES20 },
    { rtBYTES32,    DB_TYPE_BYTES32 },
    { rtBYTES48,    DB_TYPE_BYTES48 },
    { rtBYTES64,    DB_TYPE_BYTES64 },
    { rtBYTES80,    DB_TYPE_BYTES80 },
    { rtBYTES128,   DB_TYPE_BYTES128 },
    { rtBYTES256,   DB_TYPE_BYTES256 },
    { rtRESERVED27, DB_TYPE_UNDEF },
    { rtDB_XREF,    DB_TYPE_UINT64 },
    { rtDATE,       DB_TYPE_ABSTIME },
    { rtTIME_OF_DAY,DB_TYPE_ABSTIME },
    { rtABS_TIME,   DB_TYPE_ABSTIME },
    { rtUNDEFINED,  DB_TYPE_UNDEF },
};

// Соответствие между кодом типа БДРВ и его аналогом в RTAP
const DbTypeToDeTypeLink DbTypeToDeType[] = 
{
  { DB_TYPE_UNDEF,  rtUNDEFINED },
  { DB_TYPE_LOGICAL,rtLOGICAL },
  { DB_TYPE_INT8,   rtINT8 },
  { DB_TYPE_UINT8,  rtUINT8 },
  { DB_TYPE_INT16,  rtINT16 },
  { DB_TYPE_UINT16, rtUINT16 },
  { DB_TYPE_INT32,  rtINT32 },
  { DB_TYPE_UINT32, rtUINT32 },
  { DB_TYPE_INT64,  rtUNDEFINED }, // RTAP не поддерживает
  { DB_TYPE_UINT64, rtUNDEFINED }, // RTAP не поддерживает
  { DB_TYPE_FLOAT,  rtFLOAT },
  { DB_TYPE_DOUBLE, rtDOUBLE },
  { DB_TYPE_BYTES,  rtUNDEFINED }, // RTAP не поддерживает
  { DB_TYPE_BYTES4, rtBYTES4 },
  { DB_TYPE_BYTES8, rtBYTES8 },
  { DB_TYPE_BYTES12, rtBYTES12 },
  { DB_TYPE_BYTES16, rtBYTES16 },
  { DB_TYPE_BYTES20, rtBYTES20 },
  { DB_TYPE_BYTES32, rtBYTES32 },
  { DB_TYPE_BYTES48, rtBYTES48 },
  { DB_TYPE_BYTES64, rtBYTES64 },
  { DB_TYPE_BYTES80 , rtBYTES80 },
  { DB_TYPE_BYTES128, rtBYTES128 },
  { DB_TYPE_BYTES256, rtBYTES256 },
  { DB_TYPE_ABSTIME,  rtABS_TIME },
  { DB_TYPE_LAST,     rtUNDEFINED }
};

// Описание типов данных БДРВ
// NB: длина символьных типов расчитана, исходя из двухбайтовых кодировок,
// чтобы иметь возможность хранения русского текста в формате UTF-8.
const DbTypeDescription_t xdb::DbTypeDescription[] = 
{
/* 00 */  { DB_TYPE_UNDEF,   "UNDEF",   0 },
/* 01 */  { DB_TYPE_LOGICAL, "LOGICAL", sizeof(bool) },
/* 02 */  { DB_TYPE_INT8,    "INT8",    sizeof(int8_t) },
/* 03 */  { DB_TYPE_UINT8,   "UINT8",   sizeof(uint8_t) },
/* 04 */  { DB_TYPE_INT16,   "INT16",   sizeof(int16_t) },
/* 05 */  { DB_TYPE_UINT16,  "UINT16",  sizeof(uint16_t) },
/* 06 */  { DB_TYPE_INT32,   "INT32",   sizeof(int32_t) },
/* 07 */  { DB_TYPE_UINT32,  "UINT32",  sizeof(uint32_t) },
/* 08 */  { DB_TYPE_INT64,   "INT64",   sizeof(int64_t) },
/* 09 */  { DB_TYPE_UINT64,  "UINT64",  sizeof(uint64_t) },
/* 10 */  { DB_TYPE_FLOAT,   "FLOAT",   sizeof(float) },
/* 11 */  { DB_TYPE_DOUBLE,  "DOUBLE",  sizeof(double) },
/* 12 */  { DB_TYPE_BYTES,   "BYTES",   0 },    // может варьироваться
/* 13 */  { DB_TYPE_BYTES4,  "BYTES4",  sizeof(wchar_t)*4 },
/* 14 */  { DB_TYPE_BYTES8,  "BYTES8",  sizeof(wchar_t)*8 },
/* 15 */  { DB_TYPE_BYTES12, "BYTES12", sizeof(wchar_t)*12 },
/* 16 */  { DB_TYPE_BYTES16, "BYTES16", sizeof(wchar_t)*16 },
/* 17 */  { DB_TYPE_BYTES20, "BYTES20", sizeof(wchar_t)*20 },
/* 18 */  { DB_TYPE_BYTES32, "BYTES32", sizeof(wchar_t)*32 },
/* 19 */  { DB_TYPE_BYTES48, "BYTES48", sizeof(wchar_t)*48 },
/* 20 */  { DB_TYPE_BYTES64, "BYTES64", sizeof(wchar_t)*64 },
/* 21 */  { DB_TYPE_BYTES80 ,"BYTES80", sizeof(wchar_t)*80 },
/* 22 */  { DB_TYPE_BYTES128,"BYTES128",sizeof(wchar_t)*128 },
/* 23 */  { DB_TYPE_BYTES256,"BYTES256",sizeof(wchar_t)*256 },
/* 24 */  { DB_TYPE_ABSTIME, "ABS_TIME",sizeof(timeval) },
/* 25 */  { DB_TYPE_LAST,    "LAST",    0 }
};

// Описание типов данных RTAP
// NB: длина символьных типов расчитана исходя из однобайтовых кодировок.
// То есть при хранении русского в UTF-8 строки могут обрезАться.
// Для типов данных XDB принят размер символа в 2 байта (wchar_t)
const DeTypeDescription_t xdb::rtDataElem[] =
{
    { rtRESERVED0,   "RESERVED0",     0 },
    { rtLOGICAL,     "rtLOGICAL",  sizeof(bool) },
    { rtINT8,        "rtINT8",     sizeof(int8_t) },
    { rtUINT8,       "rtUINT8",    sizeof(uint8_t) },
    { rtINT16,       "rtINT16",    sizeof(int16_t) },
    { rtUINT16,      "rtUINT16",   sizeof(uint16_t) },
    { rtINT32,       "rtINT32",    sizeof(int32_t) },
    { rtUINT32,      "rtUINT32",   sizeof(uint32_t) },
    { rtFLOAT,       "rtFLOAT",    sizeof(float) },
    { rtDOUBLE,      "rtDOUBLE",   sizeof(double) },
    { rtPOLAR,       "rtPOLAR",       0 }, // not used
    { rtRECTANGULAR, "rtRECTANGULAR", 0 }, // not used
    { rtRESERVED12,  "RESERVED12",    0 }, // not used
    { rtRESERVED13,  "RESERVED13",    0 }, // not used
    { rtRESERVED14,  "RESERVED14",    0 }, // not used
    { rtRESERVED15,  "RESERVED15",    0 }, // not used
    { rtBYTES4,      "rtBYTES4",   sizeof(uint8_t)*4 },
    { rtBYTES8,      "rtBYTES8",   sizeof(uint8_t)*8 },
    { rtBYTES12,     "rtBYTES12",  sizeof(uint8_t)*12 },
    { rtBYTES16,     "rtBYTES16",  sizeof(uint8_t)*16 },
    { rtBYTES20,     "rtBYTES20",  sizeof(uint8_t)*20 },
    { rtBYTES32,     "rtBYTES32",  sizeof(uint8_t)*32 },
    { rtBYTES48,     "rtBYTES48",  sizeof(uint8_t)*48 },
    { rtBYTES64,     "rtBYTES64",  sizeof(uint8_t)*64 },
    { rtBYTES80,     "rtBYTES80",  sizeof(uint8_t)*80 },
    { rtBYTES128,    "rtBYTES128", sizeof(uint8_t)*128 },
    { rtBYTES256,    "rtBYTES256", sizeof(uint8_t)*256 },
    { rtRESERVED27,  "RESERVED27",    0 },
    { rtDB_XREF,     "rtDB_XREF",     0 }, // TODO: возможно, это д.б. символьная ссылка
    { rtDATE,        "rtDATE",        sizeof(timeval) },
    { rtTIME_OF_DAY, "rtTIME_OF_DAY", sizeof(timeval) },
    { rtABS_TIME,    "rtABS_TIME",    sizeof(timeval) },
    { rtUNDEFINED,   "rtUNDEFINED",   0 }
};

// Динамическая таблица описаний классов
// Заполняется из шаблонных файлов вида XX_YYY.dat
// где 
//  XX = номер класса
//  YYY = название класса
ClassDescription_t xdb::ClassDescriptionTable[] =  {
//        Имя (name)
//        |         Objclass (code) 
//        |         |                       тип атрибутов {VAL|VALMANUAL|VALACQ} (val_type)
//        |         |                       |               ссылка на набор атрибутов (attributes_pool)
//        |         |                       |               |
/*  0*/ {"TS",      GOF_D_BDR_OBJCLASS_TS,  DB_TYPE_UINT16, 0},           /* Телесигнализация */
/*  1*/ {"TM",      GOF_D_BDR_OBJCLASS_TM,  DB_TYPE_DOUBLE, 0},           /* Телеизмерение */
/*  2*/ {"TR",      GOF_D_BDR_OBJCLASS_TR,  DB_TYPE_DOUBLE, 0},           /* Телерегулировка */
/*  3*/ {"TSA",     GOF_D_BDR_OBJCLASS_TSA, DB_TYPE_UINT16, 0},
/*  4*/ {"TSC",     GOF_D_BDR_OBJCLASS_TSC, DB_TYPE_UINT16, 0},
/*  5*/ {"TC",      GOF_D_BDR_OBJCLASS_TC,  DB_TYPE_UINT16, 0},           /* Телеуправление */
/*  6*/ {"AL",      GOF_D_BDR_OBJCLASS_AL,  DB_TYPE_UINT16, 0},           /* Полученный аварийный сигнал */
/*  7*/ {"ICS",     GOF_D_BDR_OBJCLASS_ICS, DB_TYPE_UINT16, 0},           /* Двоичные расчетные данные */
/*  8*/ {"ICM",     GOF_D_BDR_OBJCLASS_ICM, DB_TYPE_DOUBLE, 0},           /* Аналоговые расчетные данные */
/*  9*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* "REF" - Не используется */
/* 10*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* -------------------------------------  Network objects       */
/* 11*/ {"PIPE",    GOF_D_BDR_OBJCLASS_PIPE,     DB_TYPE_UINT16, 0},      /* Труба */
/* 12*/ {"BORDER",  GOF_D_BDR_OBJCLASS_BORDER,   DB_TYPE_UINT16, 0},      /* Граница */
/* 13*/ {"CONSUMER",GOF_D_BDR_OBJCLASS_CONSUMER, DB_TYPE_UINT16, 0},      /* Потребитель */
/* 14*/ {"CORRIDOR",GOF_D_BDR_OBJCLASS_CORRIDOR, DB_TYPE_UINT16, 0},      /* Коридор */
/* 15*/ {"PIPELINE",GOF_D_BDR_OBJCLASS_PIPELINE, DB_TYPE_UINT16, 0},      /* Трубопровод */
/* -------------------------------------  Equipments objects    */
/* NB: update GOF_D_BDR_OBJCLASS_EQTLAST when adding an Equipments objects */
/* 16*/ {"TL",      GOF_D_BDR_OBJCLASS_TL,       DB_TYPE_UINT16, 0},      /* Линейный участок */
/* 17*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 18*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 19*/ {"VA",      GOF_D_BDR_OBJCLASS_VA,       DB_TYPE_UINT16, 0},      /* Кран */
/* 20*/ {"SC",      GOF_D_BDR_OBJCLASS_SC,       DB_TYPE_UINT16, 0},      /* Компрессорная станция */
/* 21*/ {"ATC",     GOF_D_BDR_OBJCLASS_ATC,      DB_TYPE_UINT16, 0},      /* Цех */
/* 22*/ {"GRC",     GOF_D_BDR_OBJCLASS_GRC,      DB_TYPE_UINT16, 0},      /* Агрегат */
/* 23*/ {"SV",      GOF_D_BDR_OBJCLASS_SV,       DB_TYPE_UINT16, 0},      /* Крановая площадка */
/* 24*/ {"SDG",     GOF_D_BDR_OBJCLASS_SDG,      DB_TYPE_UINT16, 0},      /* Газораспределительная станция */
/* 25*/ {"RGA",     GOF_D_BDR_OBJCLASS_RGA,      DB_TYPE_UINT16, 0},      /* Станция очистного поршня */
/* 26*/ {"SSDG",    GOF_D_BDR_OBJCLASS_SSDG,     DB_TYPE_UINT16, 0},      /* Выход ГРС */
/* 27*/ {"BRG",     GOF_D_BDR_OBJCLASS_BRG,      DB_TYPE_UINT16, 0},      /* Газораспределительный блок */
/* 28*/ {"SCP",     GOF_D_BDR_OBJCLASS_SCP,      DB_TYPE_UINT16, 0},      /* Пункт замера расхода */
/* 29*/ {"STG",     GOF_D_BDR_OBJCLASS_STG,      DB_TYPE_UINT16, 0},      /* Установка подготовки газа */
/* 30*/ {"DIR",     GOF_D_BDR_OBJCLASS_DIR,      DB_TYPE_UINT16, 0},      /* Предприятие */
/* 31*/ {"DIPL",    GOF_D_BDR_OBJCLASS_DIPL,     DB_TYPE_UINT16, 0},      /* ЛПУ */
/* 32*/ {"METLINE", GOF_D_BDR_OBJCLASS_METLINE,  DB_TYPE_UINT16, 0},      /* Газоизмерительная нитка */
/* 33*/ {"ESDG",    GOF_D_BDR_OBJCLASS_ESDG,     DB_TYPE_UINT16, 0},      /* Вход ГРС */
/* 34*/ {"SVLINE",  GOF_D_BDR_OBJCLASS_SVLINE,   DB_TYPE_UINT16, 0},      /* Точка КП на газопроводе */
/* 35*/ {"SCPLINE", GOF_D_BDR_OBJCLASS_SCPLINE,  DB_TYPE_UINT16, 0},      /* Нитка ПЗРГ */
/* 36*/ {"TLLINE",  GOF_D_BDR_OBJCLASS_TLLINE,   DB_TYPE_UINT16, 0},      /* Нитка ЛУ */
/* 37*/ {"INVT",    GOF_D_BDR_OBJCLASS_INVT,     DB_TYPE_UINT16, 0},      /* Инвертор */
/* 38*/ {"AUX1",    GOF_D_BDR_OBJCLASS_AUX1,     DB_TYPE_UINT16, 0},      /* Вспом объект I-го уровня */
/* 39*/ {"AUX2",    GOF_D_BDR_OBJCLASS_AUX2,     DB_TYPE_UINT16, 0},      /* Вспом объект II-го уровня */
/* 40*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 41*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 42*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 43*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 44*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* -------------------------------------  Fixed objects    */
/* 45*/ {"SITE",    GOF_D_BDR_OBJCLASS_SITE,     DB_TYPE_UNDEF, 0},       /* Корневая точка с информацией о локальном объекте */
/* 46*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 47*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 48*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 49*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* -------------------------------------  System objects    */
/* NB: update GOF_D_BDR_OBJCLASS_SYSTLAST when adding an System objects */
/* 50*/ {"SA",      GOF_D_BDR_OBJCLASS_SA,       DB_TYPE_UINT16, 0},         /* Система сбора данных */
/* -------------------------------------  Alarms objects    */
/* 51*/ {"GENAL",   GOF_D_BDR_OBJCLASS_GENAL,    DB_TYPE_UNDEF, 0},         /*  */
/* 52*/ {"USERAL",  GOF_D_BDR_OBJCLASS_USERAL,   DB_TYPE_UNDEF, 0},         /*  */
/* 53*/ {"HISTSET", GOF_D_BDR_OBJCLASS_HISTSET,  DB_TYPE_UNDEF, 0},         /*  */

/* 54*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 55*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 56*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 57*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 58*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 59*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */

/* 60*/ {"DRIVE_TYPE",      GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 61*/ {"COMPRESSOR_TYPE", GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 62*/ {"GRC_TYPE",        GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 63*/ {"ATC_TYPE",        GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 64*/ {"SC_TYP",          GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 65*/ {"SDG_TYP",         GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 66*/ {"SCP_TYP",         GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 67*/ {D_MISSING_OBJCODE, GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /* Missing */
/* 68*/ {"PIPE_TYPE",       GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 69*/ {"VA_TYPE",         GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 70*/ {"TS_TYPE",         GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 71*/ {"AL_TYPE",         GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 72*/ {"TSC_TYPE",        GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 73*/ {"AUX_TYPE",        GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 74*/ {"VERSION",         GOF_D_BDR_OBJCLASS_FIXEDPOINT, DB_TYPE_UINT16, 0}, /*  */
/* 75*/ {"DISP_TABLE_H",    GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 76*/ {"DISP_TABLE_J",    GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 77*/ {"DISP_TABLE_M",    GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 78*/ {"DISP_TABLE_QH",   GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 79*/ {"FIXEDPOINT",      GOF_D_BDR_OBJCLASS_FIXEDPOINT, DB_TYPE_UNDEF, 0}, /*  */
/* 80*/ {"HIST_SET",        GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 81*/ {"HIST_TABLE_H",    GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 82*/ {"HIST_TABLE_J",    GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 83*/ {"HIST_TABLE_M",    GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 84*/ {"HIST_TABLE_QH",   GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 85*/ {"HIST_TABLE_SAMPLE",GOF_D_BDR_OBJCLASS_UNUSED,DB_TYPE_UNDEF,0}, /*  */
/* 86*/ {"TIME_AVAILABLE",  GOF_D_BDR_OBJCLASS_UNUSED, DB_TYPE_UNDEF, 0}, /*  */
/* 87*/ {"config",          GOF_D_BDR_OBJCLASS_FIXEDPOINT, DB_TYPE_UNDEF, 0}  /*  */
};

