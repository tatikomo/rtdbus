#include <vector>
#include <time.h>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "xdb_common.hpp"

using namespace xdb;

// Таблица соответствия "тип данных как индекс" => "размер строковой части для set_s_value()"
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
    { RTDB_ATT_IDX_ACTIONTYP,       DB_TYPE_UINT16, sizeof(uint16_t) },   // 0
    { RTDB_ATT_IDX_ALARMBEGIN,      DB_TYPE_UINT32, sizeof(uint32_t) },   // 1
    { RTDB_ATT_IDX_ALARMBEGINACK,   DB_TYPE_UINT32, sizeof(uint32_t) },   // 2
    { RTDB_ATT_IDX_ALARMENDACK,     DB_TYPE_UINT32, sizeof(uint32_t) },   // 3
    { RTDB_ATT_IDX_ALARMSYNTH,      DB_TYPE_UINT8,  sizeof(uint8_t) },    // 4
    { RTDB_ATT_IDX_ALDEST,          DB_TYPE_LOGICAL,sizeof(bool) },       // 5
    { RTDB_ATT_IDX_ALINHIB,         DB_TYPE_LOGICAL,sizeof(bool) },       // 6
    { RTDB_ATT_IDX_CONFREMOTECMD,   DB_TYPE_LOGICAL,sizeof(bool) },       // 7
    { RTDB_ATT_IDX_CONVERTCOEFF,    DB_TYPE_DOUBLE, sizeof(double) },     // 8
    { RTDB_ATT_IDX_CURRENT_SHIFT_TIME, DB_TYPE_ABSTIME, sizeof(timeval) },// 9
    { RTDB_ATT_IDX_DATEAINS,        DB_TYPE_BYTES12,sizeof(wchar_t)*12 }, // 10
    { RTDB_ATT_IDX_DATEHOURM,       DB_TYPE_ABSTIME,sizeof(timeval) },    // 11
    { RTDB_ATT_IDX_DATERTU,         DB_TYPE_ABSTIME,sizeof(timeval) },    // 12
    { RTDB_ATT_IDX_DELEGABLE,       DB_TYPE_UINT8,  sizeof(uint8_t) },    // 13
    { RTDB_ATT_IDX_DISPP,           DB_TYPE_BYTES20,sizeof(wchar_t)*20 }, // 14
    { RTDB_ATT_IDX_FLGMAINTENANCE,  DB_TYPE_LOGICAL,sizeof(bool) },       // 15
    { RTDB_ATT_IDX_FLGREMOTECMD,    DB_TYPE_LOGICAL,sizeof(bool) },       // 16
    { RTDB_ATT_IDX_FUNCTION,        DB_TYPE_BYTES20,sizeof(wchar_t)*20 }, // 17
    { RTDB_ATT_IDX_GRADLEVEL,       DB_TYPE_DOUBLE, sizeof(double) },     // 18
    { RTDB_ATT_IDX_INHIB,           DB_TYPE_LOGICAL,sizeof(bool) },    // 19
    { RTDB_ATT_IDX_INHIBLOCAL,      DB_TYPE_LOGICAL,sizeof(bool) },    // 20
    { RTDB_ATT_IDX_KMREFDWN,        DB_TYPE_DOUBLE, sizeof(double) },     // 21
    { RTDB_ATT_IDX_KMREFUPS,        DB_TYPE_DOUBLE, sizeof(double) },     // 22
    { RTDB_ATT_IDX_LABEL,           DB_TYPE_BYTES20,sizeof(wchar_t)*20 }, // 23
    { RTDB_ATT_IDX_L_CONSUMER,      DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 24
    { RTDB_ATT_IDX_L_CORRIDOR,      DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 25
    { RTDB_ATT_IDX_L_DIPL,          DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 26
    { RTDB_ATT_IDX_L_EQT,           DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 27
    { RTDB_ATT_IDX_L_EQTORBORDWN,   DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 28
    { RTDB_ATT_IDX_L_EQTORBORUPS,   DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 29
    { RTDB_ATT_IDX_L_EQTTYP,        DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 30
    { RTDB_ATT_IDX_LINK_HIST,       DB_TYPE_UINT64, sizeof(uint64_t) },   // 31
    { RTDB_ATT_IDX_L_NET,           DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 32
    { RTDB_ATT_IDX_L_NETTYPE,       DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 33
    { RTDB_ATT_IDX_LOCALFLAG,       DB_TYPE_UINT8,  sizeof(uint8_t) },    // 34
    { RTDB_ATT_IDX_L_PIPE,          DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 35
    { RTDB_ATT_IDX_L_PIPELINE,      DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 36
    { RTDB_ATT_IDX_L_SA,            DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 37
    { RTDB_ATT_IDX_L_TL,            DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 38
    { RTDB_ATT_IDX_L_TM,            DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 39
    { RTDB_ATT_IDX_L_TYPINFO,       DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 40
    { RTDB_ATT_IDX_MAXVAL,          DB_TYPE_DOUBLE, sizeof(double) },     // 41
    { RTDB_ATT_IDX_MINVAL,          DB_TYPE_DOUBLE, sizeof(double) },     // 42
    { RTDB_ATT_IDX_MNVALPHY,        DB_TYPE_DOUBLE, sizeof(double) },     // 43
    { RTDB_ATT_IDX_MXFLOW,          DB_TYPE_DOUBLE, sizeof(double) },     // 44
    { RTDB_ATT_IDX_MXPRESSURE,      DB_TYPE_DOUBLE, sizeof(double) },     // 45
    { RTDB_ATT_IDX_MXVALPHY,        DB_TYPE_DOUBLE, sizeof(double) },     // 46
    { RTDB_ATT_IDX_NAMEMAINTENANCE, DB_TYPE_BYTES12,sizeof(wchar_t)*12 }, // 47
    { RTDB_ATT_IDX_NMFLOW,          DB_TYPE_DOUBLE, sizeof(double) },     // 48
    { RTDB_ATT_IDX_NMPRESSURE,      DB_TYPE_DOUBLE, sizeof(double) },     // 49
// NB: OBJCLASS - особый случай, этот атрибут более не является самостоятельным,
// став принадлежностью XDBPoint
    { RTDB_ATT_IDX_OBJCLASS,        DB_TYPE_UINT8,  sizeof(uint8_t) },    // 50
    { RTDB_ATT_IDX_PLANFLOW,        DB_TYPE_DOUBLE, sizeof(double) },     // 51
    { RTDB_ATT_IDX_PLANPRESSURE,    DB_TYPE_DOUBLE, sizeof(double) },     // 52
    { RTDB_ATT_IDX_PREV_DISPATCHER, DB_TYPE_BYTES12,sizeof(wchar_t)*12 }, // 53
    { RTDB_ATT_IDX_PREV_SHIFT_TIME, DB_TYPE_UINT64, sizeof(uint64_t) },   // 54
    { RTDB_ATT_IDX_REMOTECONTROL,   DB_TYPE_UINT8,  sizeof(uint8_t) },    // 55
    { RTDB_ATT_IDX_SHORTLABEL,      DB_TYPE_BYTES16,sizeof(wchar_t)*16 }, // 56
    { RTDB_ATT_IDX_STATUS,          DB_TYPE_UINT8,  sizeof(uint8_t) },    // 57
    { RTDB_ATT_IDX_SUPPLIER,        DB_TYPE_BYTES16,sizeof(wchar_t)*16 }, // 58
    { RTDB_ATT_IDX_SUPPLIERMODE,    DB_TYPE_BYTES16,sizeof(wchar_t)*16 }, // 59
    { RTDB_ATT_IDX_SUPPLIERSTATE,   DB_TYPE_BYTES16,sizeof(wchar_t)*16 }, // 60
    { RTDB_ATT_IDX_SYNTHSTATE,      DB_TYPE_UINT8,  sizeof(uint8_t) },    // 61
    { RTDB_ATT_IDX_TSSYNTHETICAL,   DB_TYPE_UINT8,  sizeof(uint8_t) },    // 62
    { RTDB_ATT_IDX_TYPE,            DB_TYPE_UINT32, sizeof(uint32_t) },   // 63
    { RTDB_ATT_IDX_UNITY,           DB_TYPE_BYTES20,sizeof(wchar_t)*20 }, // 64
    { RTDB_ATT_IDX_UNITYCATEG,      DB_TYPE_UINT8,  sizeof(uint8_t) },    // 65
// NB: UNIVNAME - особый случай, этот атрибут более не является самостоятельным,
// став принадлежностью XDBPoint
// Размерность определяется значением TAG_NAME_MAXLEN в файле config.h
    { RTDB_ATT_IDX_UNIVNAME,        DB_TYPE_BYTES32,sizeof(wchar_t)*32 }, // 66
// NB: VAL - особый случай, может быть как целочисленным, так и double 
    { RTDB_ATT_IDX_VAL,             DB_TYPE_DOUBLE, sizeof(double) },     // 67
// NB: VALACQ - особый случай, может быть как целочисленным, так и double 
    { RTDB_ATT_IDX_VALACQ,          DB_TYPE_DOUBLE, sizeof(double) },     // 68
    { RTDB_ATT_IDX_VALEX,           DB_TYPE_DOUBLE, sizeof(double) },     // 69
    { RTDB_ATT_IDX_VALID,           DB_TYPE_UINT8,  sizeof(uint8_t) },    // 70
    { RTDB_ATT_IDX_VALIDACQ,        DB_TYPE_UINT8,  sizeof(uint8_t) },    // 71
    { RTDB_ATT_IDX_VALIDCHANGE,     DB_TYPE_UINT8,  sizeof(uint8_t) },    // 72
// NB: VALMANUAL - особый случай, может быть как целочисленным, так и double 
    { RTDB_ATT_IDX_VALMANUAL,       DB_TYPE_DOUBLE, sizeof(double) },     // 73
    { RTDB_ATT_IDX_UNEXIST,         DB_TYPE_UNDEF,  0 }                   // 74
};


// Таблица тегов Точки, при изменении которых запись этой точки в группе подписки активируется
// (точки считается изменившейся)
// Атрибуты точки типа XDBPoint и связанные с ней паспортные характеристики
const char* xdb::attributes_for_subscription_group[] = {
    RTDB_ATT_TAG,
    RTDB_ATT_OBJCLASS,
    RTDB_ATT_STATUS,
    RTDB_ATT_VALID,
    RTDB_ATT_VALIDACQ,
    RTDB_ATT_VAL
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
  { DB_TYPE_UNDEF,  "UNDEF",    0 },
  { DB_TYPE_LOGICAL,"LOGICAL",  sizeof(bool) },
  { DB_TYPE_INT8,   "INT8",     sizeof(int8_t) },
  { DB_TYPE_UINT8,  "UINT8",    sizeof(uint8_t) },
  { DB_TYPE_INT16,  "INT16",    sizeof(int16_t) },
  { DB_TYPE_UINT16, "UINT16",   sizeof(uint16_t) },
  { DB_TYPE_INT32,  "INT32",    sizeof(int32_t) },
  { DB_TYPE_UINT32, "UINT32",   sizeof(uint32_t) },
  { DB_TYPE_INT64,  "INT64",    sizeof(int64_t) },
  { DB_TYPE_UINT64, "UINT64",   sizeof(uint64_t) },
  { DB_TYPE_FLOAT,  "FLOAT",    sizeof(float) },
  { DB_TYPE_DOUBLE, "DOUBLE",   sizeof(double) },
  { DB_TYPE_BYTES,  "BYTES",    0 },    // может варьироваться
  { DB_TYPE_BYTES4, "BYTES4",   sizeof(wchar_t)*4 },
  { DB_TYPE_BYTES8, "BYTES8",   sizeof(wchar_t)*8 },
  { DB_TYPE_BYTES12, "BYTES12", sizeof(wchar_t)*12 },
  { DB_TYPE_BYTES16, "BYTES16", sizeof(wchar_t)*16 },
  { DB_TYPE_BYTES20, "BYTES20", sizeof(wchar_t)*20 },
  { DB_TYPE_BYTES32, "BYTES32", sizeof(wchar_t)*32 },
  { DB_TYPE_BYTES48, "BYTES48", sizeof(wchar_t)*48 },
  { DB_TYPE_BYTES64, "BYTES64", sizeof(wchar_t)*64 },
  { DB_TYPE_BYTES80 ,"BYTES80",    sizeof(wchar_t)*80 },
  { DB_TYPE_BYTES128,"BYTES128",   sizeof(wchar_t)*128 },
  { DB_TYPE_BYTES256,"BYTES256",   sizeof(wchar_t)*256 },
  { DB_TYPE_ABSTIME, "ABS_TIME",   sizeof(timeval) },
  { DB_TYPE_LAST,    "LAST",    0 }
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

// Динамическая таблдица описаний классов
// Заполняется из шаблонных файлов вида XX_YYY.dat
// где 
//  XX = номер класса
//  YYY = название класса
ClassDescription_t xdb::ClassDescriptionTable[] = {
    {"TS",      GOF_D_BDR_OBJCLASS_TS, 0},          /* Телесигнализация */
    {"TM",      1, 0},          /* Телеизмерение */
    {"TR",      2, 0},          /* Телерегулировка */
    {"TSA",     3, 0},
    {"TSC",     4, 0},
    {"TC",      5, 0},          /* Телеуправление */
    {"AL",      6, 0},          /* Полученный аварийный сигнал */
    {"ICS",     7, 0},          /* Двоичные расчетные данные */
    {"ICM",     8, 0},          /* Аналоговые расчетные данные */
    {D_MISSING_OBJCODE,  9, 0}, /* "REF" - Не используется */
    {D_MISSING_OBJCODE, 10, 0}, /* Missing */
/* -------------------------------------  Network objects       */
    {"PIPE",    11, 0},         /* Труба */
    {"BORDER",  12, 0},         /* Граница */
    {"CONSUMER",13, 0},         /* Потребитель */
    {"CORRIDOR",14, 0},         /* Коридор */
    {"PIPELINE",15, 0},         /* Трубопровод */
/* -------------------------------------  Equipments objects    */
/* NB: update GOF_D_BDR_OBJCLASS_EQTLAST when adding an Equipments objects */
    {"TL",      16, 0},         /* Линейный участок */
    {D_MISSING_OBJCODE, 17, 0}, /* Missing */
    {D_MISSING_OBJCODE, 18, 0}, /* Missing */
    {"VA",      19, 0},         /* Кран */
    {"SC",      20, 0},         /* Компрессорная станция */
    {"ATC",     21, 0},         /* Цех */
    {"GRC",     22, 0},         /* Агрегат */
    {"SV",      23, 0},         /* Крановая площадка */
    {"SDG",     24, 0},         /* Газораспределительная станция */
    {"RGA",     25, 0},         /* Станция очистного поршня */
    {"SSDG",    26, 0},         /* Выход ГРС */
    {"BRG",     27, 0},         /* Газораспределительный блок */
    {"SCP",     28, 0},         /* Пункт замера расхода */
    {"STG",     29, 0},         /* Установка подготовки газа */
    {"DIR",     30, 0},         /* Предприятие */
    {"DIPL",    31, 0},         /* ЛПУ */
    {"METLINE", 32, 0},         /* Газоизмерительная нитка */
    {"ESDG",    33, 0},         /* Вход ГРС */
    {"SVLINE",  34, 0},         /* Точка КП на газопроводе */
    {"SCPLINE", 35, 0},         /* Нитка ПЗРГ */
    {"TLLINE",  36, 0},         /* Нитка ЛУ */
    {"INVT",    37, 0},         /* Инвертор */
    {"AUX1",    38, 0},         /* Вспом объект I-го уровня */
    {"AUX2",    39, 0},         /* Вспом объект II-го уровня */
    {D_MISSING_OBJCODE, 40, 0}, /* Missing */
    {D_MISSING_OBJCODE, 41, 0}, /* Missing */
    {D_MISSING_OBJCODE, 42, 0}, /* Missing */
    {D_MISSING_OBJCODE, 43, 0}, /* Missing */
    {D_MISSING_OBJCODE, 44, 0}, /* Missing */
/* -------------------------------------  Fixed objects    */
    {"SITE",    45, 0},         /* Корневая точка с информацией о локальном объекте */
    {D_MISSING_OBJCODE, 46, 0}, /* Missing */
    {D_MISSING_OBJCODE, 47, 0}, /* Missing */
    {D_MISSING_OBJCODE, 48, 0}, /* Missing */
    {D_MISSING_OBJCODE, 49, 0}, /* Missing */
/* -------------------------------------  System objects    */
/* NB: update GOF_D_BDR_OBJCLASS_SYSTLAST when adding an System objects */
    {"SA",      50, 0},         /* Система сбора данных */
/* -------------------------------------  Alarms objects    */
    {"GENAL",   51, 0},         /*  */
    {"USERAL",  52, 0},         /*  */
    {"HISTSET", 53, 0},         /*  */

    {D_MISSING_OBJCODE, 54, 0}, /* Missing */
    {D_MISSING_OBJCODE, 55, 0}, /* Missing */
    {D_MISSING_OBJCODE, 56, 0}, /* Missing */
    {D_MISSING_OBJCODE, 57, 0}, /* Missing */
    {D_MISSING_OBJCODE, 58, 0}, /* Missing */
    {D_MISSING_OBJCODE, 59, 0}, /* Missing */

    {"DRIVE_TYPE",      60, 0}, /*  */
    {"COMPRESSOR_TYPE", 61, 0}, /*  */
    {"GRC_TYPE",        62, 0}, /*  */
    {"ATC_TYPE",        63, 0}, /*  */
    {"SC_TYP",          64, 0}, /*  */
    {"SDG_TYP",         65, 0}, /*  */
    {"SCP_TYP",         66, 0}, /*  */
    {D_MISSING_OBJCODE, 67, 0}, /* Missing */
    {"PIPE_TYPE",       68, 0}, /*  */
    {"VA_TYPE",         69, 0}, /*  */
    {"TS_TYPE",         70, 0}, /*  */
    {"AL_TYPE",         71, 0}, /*  */
    {"TSC_TYPE",        72, 0}, /*  */
    {"AUX_TYPE",        73, 0}, /*  */
    {"VERSION",         74, 0}, /*  */
    {"DISP_TABLE_H",    75, 0}, /*  */
    {"DISP_TABLE_J",    76, 0}, /*  */
    {"DISP_TABLE_M",    77, 0}, /*  */
    {"DISP_TABLE_QH",   78, 0}, /*  */
    {"FIXEDPOINT",      79, 0}, /*  */
    {"HIST_SET",        80, 0}, /*  */
    {"HIST_TABLE_H",    81, 0}, /*  */
    {"HIST_TABLE_J",    82, 0}, /*  */
    {"HIST_TABLE_M",    83, 0}, /*  */
    {"HIST_TABLE_QH",   84, 0}, /*  */
    {"HIST_TABLE_SAMPLE",85,0}, /*  */
    {"TIME_AVAILABLE",  86, 0}, /*  */
    {"config",          87, 0}  /*  */
};

