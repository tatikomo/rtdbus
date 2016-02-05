// ==============================================================
// Общие типы данных как для системно-зависимой части (xdb/impl),
// так и для интерфейсной части (xdm/rtap и xdb/broker).
//
// 2015/07/07
// ==============================================================
#pragma once
#ifndef XDB_COMMON_HPP
#define XDB_COMMON_HPP

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include <string>
#include <map>

#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>

namespace xdb {

#define D_DATE_FORMAT_STR           "%d-%m-%Y %T"
#define D_DATE_FORMAT_LEN           20
#define D_DATE_FORMAT_W_MSEC_ST     D_DATE_FORMAT ## ".%06ld"
#define D_DATE_FORMAT_W_MSEC_LEN    (D_DATE_FORMAT_LEN + 7)

#define D_MISSING_OBJCODE   "MISSING"

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

// ====================================================================
//
// При изменении перечня синхронизировать его с rtad_db.xsd
//
// ====================================================================
#define RTDB_ATT_TAG        "TAG"       // Атрибуты TAG и OBJCLASS - специальный случай,
#define RTDB_ATT_OBJCLASS   "OBJCLASS"  // Они не в виде атрибутов, а в виде полей XDBpoint
#define RTDB_ATT_LABEL      "LABEL"
#define RTDB_ATT_SHORTLABEL "SHORTLABEL"
#define RTDB_ATT_VALID      "VALID"
#define RTDB_ATT_VALIDACQ   "VALIDACQ"
#define RTDB_ATT_DATEHOURM  "DATEHOURM"
#define RTDB_ATT_DATERTU    "DATERTU"
#define RTDB_ATT_MINVAL     "MINVAL"
#define RTDB_ATT_MAXVAL     "MAXVAL"
#define RTDB_ATT_VALEX      "VALEX"
#define RTDB_ATT_VAL        "VAL"
#define RTDB_ATT_VALACQ     "VALACQ"
#define RTDB_ATT_VALMANUAL  "VALMANUAL"
#define RTDB_ATT_ALINHIB    "ALINHIB"
#define RTDB_ATT_DISPP      "DISPP"
#define RTDB_ATT_CURRENT_SHIFT_TIME "CURRENT_SHIFT_TIME"
#define RTDB_ATT_PREV_SHIFT_TIME    "PREV_SHIFT_TIME"
#define RTDB_ATT_PREV_DISPATCHER    "PREV_DISPATCHER"
#define RTDB_ATT_FUNCTION       "FUNCTION"
#define RTDB_ATT_CONVERTCOEFF   "CONVERTCOEFF"
#define RTDB_ATT_GRADLEVEL      "GRADLEVEL"
#define RTDB_ATT_HISTOTYPE      "HISTOTYPE"
#define RTDB_ATT_INHIB          "INHIB"
#define RTDB_ATT_MNVALPHY       "MNVALPHY"
#define RTDB_ATT_MXPRESSURE     "MXPRESSURE"
#define RTDB_ATT_MXVALPHY       "MXVALPHY"
#define RTDB_ATT_NMFLOW         "NMFLOW"
#define RTDB_ATT_STATUS         "STATUS"
#define RTDB_ATT_SUPPLIER       "SUPPLIER"
#define RTDB_ATT_SUPPLIERMODE   "SUPPLIERMODE"
#define RTDB_ATT_SUPPLIERSTATE  "SUPPLIERSTATE"
#define RTDB_ATT_SYNTHSTATE     "SYNTHSTATE"
#define RTDB_ATT_DELEGABLE      "DELEGABLE"
#define RTDB_ATT_TYPE           "TYPE"
#define RTDB_ATT_L_NETTYPE      "L_NETTYPE"
#define RTDB_ATT_L_NET          "L_NET"
#define RTDB_ATT_L_TL           "L_TL"
#define RTDB_ATT_L_TM           "L_TM"
#define RTDB_ATT_L_EQTORBORUPS  "L_EQTORBORUPS"
#define RTDB_ATT_L_EQTORBORDWN  "L_EQTORBORDWN"
#define RTDB_ATT_L_TYPINFO      "L_TYPINFO"
#define RTDB_ATT_L_EQT          "L_EQT"
#define RTDB_ATT_L_EQTTYP       "L_EQTTYP"
#define RTDB_ATT_L_SA           "L_SA"
#define RTDB_ATT_L_DIPL         "L_DIPL"
#define RTDB_ATT_L_CONSUMER     "L_CONSUMER"
#define RTDB_ATT_L_CORRIDOR     "L_CORRIDOR"
#define RTDB_ATT_L_PIPE         "L_PIPE"
#define RTDB_ATT_L_PIPELINE     "L_PIPELINE"
#define RTDB_ATT_NMPRESSURE     "NMPRESSURE"
#define RTDB_ATT_KMREFUPS       "KMREFUPS"
#define RTDB_ATT_KMREFDWN       "KMREFDWN"
#define RTDB_ATT_LOCALFLAG      "LOCALFLAG"
#define RTDB_ATT_ALARMBEGIN     "ALARMBEGIN"
#define RTDB_ATT_ALARMBEGINACK  "ALARMBEGINACK"
#define RTDB_ATT_ALARMENDACK    "ALARMENDACK"
#define RTDB_ATT_ALARMSYNTH     "ALARMSYNTH"
#define RTDB_ATT_DATEAINS       "DATEAINS"
#define RTDB_ATT_MXFLOW         "MXFLOW"
#define RTDB_ATT_PLANPRESSURE   "PLANPRESSURE"
#define RTDB_ATT_PLANFLOW       "PLANFLOW"
#define RTDB_ATT_CONFREMOTECMD  "CONFREMOTECMD"
#define RTDB_ATT_FLGREMOTECMD   "FLGREMOTECMD"
#define RTDB_ATT_FLGMAINTENANCE "FLGMAINTENANCE"
#define RTDB_ATT_NAMEMAINTENANCE   "NAMEMAINTENANCE"
#define RTDB_ATT_REMOTECONTROL  "REMOTECONTROL"
#define RTDB_ATT_TSSYNTHETICAL  "TSSYNTHETICAL"
#define RTDB_ATT_ACTIONTYP      "ACTIONTYP"
#define RTDB_ATT_VALIDCHANGE    "VALIDCHANGE"
#define RTDB_ATT_VAL_LABEL      "VAL_LABEL"
#define RTDB_ATT_LINK_HIST      "LINK_HIST"
#define RTDB_ATT_ACQMOD         "ACQMOD"
#define RTDB_ATT_ALDEST         "ALDEST"
#define RTDB_ATT_INHIBLOCAL     "INHIBLOCAL"
#define RTDB_ATT_UNITY          "UNITY"
#define RTDB_ATT_UNITYCATEG     "UNITYCATEG"

// Индексы атрибутов в массиве соответствия "Атрибут => Размер атрибута"
// Алгоритм получения:
//   1) bin/xdb_snap -g > types_tmp.out
//   2) grep "objclass:" types_tmp.out | while read x1 class x2 name x3 stype x4 itype; do echo $class" "$name" "$stype" "$itype; done > types.out
//   3) i=0; cat types.out| awk '{print $2}' | sort | uniq | while read a; do echo "#define RTDB_ATT_IDX_$a  $i"; i=$((i+1)); done

#define RTDB_ATT_IDX_ACTIONTYP      0
#define RTDB_ATT_IDX_ALARMBEGIN     1
#define RTDB_ATT_IDX_ALARMBEGINACK  2
#define RTDB_ATT_IDX_ALARMENDACK    3
#define RTDB_ATT_IDX_ALARMSYNTH     4
#define RTDB_ATT_IDX_ALDEST         5
#define RTDB_ATT_IDX_ALINHIB        6
#define RTDB_ATT_IDX_CONFREMOTECMD  7
#define RTDB_ATT_IDX_CONVERTCOEFF   8
#define RTDB_ATT_IDX_CURRENT_SHIFT_TIME  9
#define RTDB_ATT_IDX_DATEAINS       10
#define RTDB_ATT_IDX_DATEHOURM      11
#define RTDB_ATT_IDX_DATERTU        12
#define RTDB_ATT_IDX_DELEGABLE      13
#define RTDB_ATT_IDX_DISPP          14
#define RTDB_ATT_IDX_FLGMAINTENANCE 15
#define RTDB_ATT_IDX_FLGREMOTECMD   16
#define RTDB_ATT_IDX_FUNCTION       17
#define RTDB_ATT_IDX_GRADLEVEL      18
#define RTDB_ATT_IDX_HISTOTYPE      19

#define RTDB_ATT_IDX_INHIB          20
#define RTDB_ATT_IDX_INHIBLOCAL     21
#define RTDB_ATT_IDX_KMREFDWN       22
#define RTDB_ATT_IDX_KMREFUPS       23
#define RTDB_ATT_IDX_LABEL          24
#define RTDB_ATT_IDX_L_CONSUMER     25
#define RTDB_ATT_IDX_L_CORRIDOR     26
#define RTDB_ATT_IDX_L_DIPL         27
#define RTDB_ATT_IDX_L_EQT          28
#define RTDB_ATT_IDX_L_EQTORBORDWN  29
#define RTDB_ATT_IDX_L_EQTORBORUPS  30
#define RTDB_ATT_IDX_L_EQTTYP       31
#define RTDB_ATT_IDX_LINK_HIST      32
#define RTDB_ATT_IDX_L_NET          33
#define RTDB_ATT_IDX_L_NETTYPE      34
#define RTDB_ATT_IDX_LOCALFLAG      35
#define RTDB_ATT_IDX_L_PIPE         36
#define RTDB_ATT_IDX_L_PIPELINE     37
#define RTDB_ATT_IDX_L_SA           38
#define RTDB_ATT_IDX_L_TL           39
#define RTDB_ATT_IDX_L_TM           40
#define RTDB_ATT_IDX_L_TYPINFO      41
#define RTDB_ATT_IDX_MAXVAL         42
#define RTDB_ATT_IDX_MINVAL         43
#define RTDB_ATT_IDX_MNVALPHY       44
#define RTDB_ATT_IDX_MXFLOW         45
#define RTDB_ATT_IDX_MXPRESSURE     46
#define RTDB_ATT_IDX_MXVALPHY       47
#define RTDB_ATT_IDX_NAMEMAINTENANCE  48
#define RTDB_ATT_IDX_NMFLOW         49
#define RTDB_ATT_IDX_NMPRESSURE     50
#define RTDB_ATT_IDX_OBJCLASS       51
#define RTDB_ATT_IDX_PLANFLOW       52
#define RTDB_ATT_IDX_PLANPRESSURE   53
#define RTDB_ATT_IDX_PREV_DISPATCHER  54
#define RTDB_ATT_IDX_PREV_SHIFT_TIME  55
#define RTDB_ATT_IDX_REMOTECONTROL  56
#define RTDB_ATT_IDX_SHORTLABEL     57
#define RTDB_ATT_IDX_STATUS         58
#define RTDB_ATT_IDX_SUPPLIER       59
#define RTDB_ATT_IDX_SUPPLIERMODE   60
#define RTDB_ATT_IDX_SUPPLIERSTATE  61
#define RTDB_ATT_IDX_SYNTHSTATE     62
#define RTDB_ATT_IDX_TSSYNTHETICAL  63
#define RTDB_ATT_IDX_TYPE           64
#define RTDB_ATT_IDX_UNITY          65
#define RTDB_ATT_IDX_UNITYCATEG     66
#define RTDB_ATT_IDX_UNIVNAME       67
#define RTDB_ATT_IDX_VAL            68
#define RTDB_ATT_IDX_VALACQ         69
#define RTDB_ATT_IDX_VALEX          70
#define RTDB_ATT_IDX_VALID          71
#define RTDB_ATT_IDX_VALIDACQ       72
#define RTDB_ATT_IDX_VALIDCHANGE    73
#define RTDB_ATT_IDX_VALMANUAL      74
// Последнее значение, индекс несуществующего атрибута
#define RTDB_ATT_IDX_UNEXIST        75

// ====================================================================
//
typedef std::string   shortlabel_t;
typedef std::string   longlabel_t;
typedef std::string   univname_t;
typedef std::string   code_t;

typedef enum
{
  ATTR_OK        = 0, /* no known error on data */
  ATTR_SUSPECT   = 1, /* depends on a suspect, error or disabled value */
  ATTR_ERROR     = 2, /* calc. engine got math error */
  ATTR_DISABLED  = 3, /* calc. engine operation indicator disabled */
  ATTR_NOT_FOUND = 4  /* attribute is not found in database */
} Quality_t;

typedef union
{
  //uint8 common;
  uint64_t common;
  //uint4 part[2];
  uint32_t part[2];
  struct timeval tv; // uint32 sec, uint32 usec
} datetime_t;

// NB: формат хранения строк UTF-8
typedef struct
{
  uint16_t     size; // 16384
  char        *varchar; // ссылка на данные типа [BYTES4..BYTES256]
  std::string *val_string;  // ссылка на данные типа DB_TYPE_BYTES
} dynamic_part;

#define CATEGORY_WORK          0x0001   // В рабочем состоянии
#define CATEGORY_ANALOG_TYPE   0x0002   // Аналоговый тип атрибута VAL
#define CATEGORY_DISCRETE_TYPE 0x0004   // Дискретный тип атрибута VAL
#define CATEGORY_BINARY_TYPE   0x0008   // Двоичный тип атрибута VAL
#define CATEGORY_EXIST_IN_SBS  0x0010   // Присутствует в одной из Групп Подписок
#define CATEGORY_HAS_HISTORY   0x0020   // Имеет предысторию
#define CATEGORY_7             0x0040   //
#define CATEGORY_8             0x0080   //
#define CATEGORY_9             0x0100   //
#define CATEGORY_10            0x0200   //
#define CATEGORY_11            0x0400   //
#define CATEGORY_12            0x0800   //
#define CATEGORY_13            0x1000   //
#define CATEGORY_14            0x2000   //
#define CATEGORY_15            0x4000   //
#define CATEGORY_16            0x8000   //

// Типы данных атрибута VAL Точки
// используется при проверке БД Истории
enum ProcessingType_t {
    PROCESSING_UNKNOWN  = 0,
    PROCESSING_ANALOG   = 1,
    PROCESSING_DISCRETE = 2,
    PROCESSING_BINARY   = 3
};

// Значения битов отдельных Категорий Точек БДРВ
// NB: Ширина поля 16 бит обусловлена разрядностью поля rtDbCq.addrCnt
typedef uint16_t category_type_t;

// Одно элементарное значение из БДРВ
typedef union
{
  bool     val_bool;
  int8_t   val_int8;
  uint8_t  val_uint8;
  int16_t  val_int16;
  uint16_t val_uint16;
  int32_t  val_int32;
  uint32_t val_uint32;
  int64_t  val_int64;
  uint64_t val_uint64;
  float    val_float;
  double   val_double;
  struct   timeval val_time;
} fixed_part;

typedef struct
{
  fixed_part   fixed;
  dynamic_part dynamic;
} AttrVal_t;

// Коды элементарных типов данных eXtremeDB, которые мы используем
// Копия используется в файле sinf.proto
typedef enum
{
  DB_TYPE_UNDEF     = 0,
  DB_TYPE_LOGICAL   = 1,
  DB_TYPE_INT8      = 2,
  DB_TYPE_UINT8     = 3,
  DB_TYPE_INT16     = 4,
  DB_TYPE_UINT16    = 5,
  DB_TYPE_INT32     = 6,
  DB_TYPE_UINT32    = 7,
  DB_TYPE_INT64     = 8,
  DB_TYPE_UINT64    = 9,
  DB_TYPE_FLOAT     = 10,
  DB_TYPE_DOUBLE    = 11,
  DB_TYPE_BYTES     = 12, // переменная длина строки
  DB_TYPE_BYTES4    = 13,
  DB_TYPE_BYTES8    = 14,
  DB_TYPE_BYTES12   = 15,
  DB_TYPE_BYTES16   = 16,
  DB_TYPE_BYTES20   = 17,
  DB_TYPE_BYTES32   = 18,
  DB_TYPE_BYTES48   = 19,
  DB_TYPE_BYTES64   = 20,
  DB_TYPE_BYTES80   = 21,
  DB_TYPE_BYTES128  = 22,
  DB_TYPE_BYTES256  = 23,
  DB_TYPE_ABSTIME   = 24,
  DB_TYPE_LAST      = 25 // fake type, used for limit array types
} DbType_t;

// Внутренние состояния базы данных
typedef enum {
    DB_STATE_UNINITIALIZED = 1, // первоначальное состояние
    DB_STATE_INITIALIZED   = 2, // инициализирован runtime
    DB_STATE_ATTACHED      = 3, // вызван mco_db_open
    DB_STATE_CONNECTED     = 4, // вызван mco_db_connect
    DB_STATE_DISCONNECTED  = 5, // вызван mco_db_disconnect
    DB_STATE_CLOSED        = 6  // вызван mco_db_close
} DBState_t;

typedef enum {
    CONNECTION_INVALID = 0,
    CONNECTION_VALID   = 1
} ConnectionState_t;

typedef enum
{
  APP_MODE_UNKNOWN    =-1,
  APP_MODE_LOCAL      = 0,
  APP_MODE_REMOTE     = 1
} AppMode_t;

typedef enum
{
  APP_STATE_UNKNOWN =-1,
  APP_STATE_GOOD    = 0,
  APP_STATE_BAD     = 1
} AppState_t;

typedef enum
{
  ENV_STATE_UNKNOWN = 0, // первоначальное состояние
  ENV_STATE_BAD     = 1, // критическая ошибка
  ENV_STATE_INIT    = 2, // среда проинициализирована, БД еще не открыта
  ENV_STATE_DB_OPEN = 3, // среда инициализирована, БД открыта
  ENV_STATE_LOADING = 4,      // среда в процессе запуска
  ENV_STATE_SHUTTINGDOWN = 5  // среда в процессе останова
} EnvState_t;

typedef enum
{
  ENV_SHUTDOWN_SOFT = 0,
  ENV_SHUTDOWN_HARD = 1
} EnvShutdownOrder_t;

// Коды элементарных типов данных старой БДРВ RTAP
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
    rtABS_TIME   = 31,
    rtUNDEFINED  = 32
} rtDeType;

#define GOF_D_BDR_MAX_DE_TYPE rtUNDEFINED

/* перечень значимых атрибутов и их типов */
typedef struct
{
#if CMAKE_COMPILER_IS_GNUCC && (CMAKE_CXX_COMPILER_VERSION >= 483)
  univname_t name = "";    /* имя атрибута */
  DbType_t   type = DB_TYPE_UNDEF;    /* его тип - целое, дробь, строка */
  Quality_t  quality = ATTR_ERROR; /* качество атрибута (из БДРВ) */
  AttrVal_t  value = { {0}, {0, NULL, NULL} };   /* значение атрибута */
#else
  univname_t name;    /* имя атрибута */
  DbType_t   type;    /* его тип - целое, дробь, строка */
  Quality_t  quality; /* качество атрибута (из БДРВ) */
  AttrVal_t  value;   /* значение атрибута */
#endif
} AttributeInfo_t;

// Позиции бит в флагах, передаваемых конструктору
typedef enum
{
  OF_POS_CREATE   = 1, // создать БД в случае, если это не было сделано ранее
  OF_POS_READONLY = 2, // открыть в режиме "только для чтения"
  OF_POS_RDWR     = 3, // открыть в режиме "чтение|запись" (по умолчанию)
  OF_POS_TRUNCATE = 4, // открыть пустую базу, удалив данные в существующем экземпляре
  OF_POS_LOAD_SNAP= 5, // открыть базу, заполнив ее данными из последнего снапшота
  OF_POS_SAVE_SNAP= 6, // сохранить дамп базы после завершения работы
  OF_POS_REGISTER_EVENT = 7 // активировать обработчики событий БДРВ
} FlagPos_t;

/*
 * Типы запросов на контроль структуры БДРВ
 */
typedef enum
{
    rtCONTROL_CE_OPER       = 0,  /* set CE operation indicator */
    rtCONTROL_LOCK_PT       = 1,  /* lock Points */
    rtCONTROL_SET_CWP       = 2,  /* set current working position */
    rtCONTROL_SNAPSHOT      = 3,  /* force snapshot */
    rtCONTROL_UNLOCK_PT     = 4,  /* unlock Points */
    //    rtCONTROL_XFER_LOCK     5,  /* transfer locks to another process */
    rtCONTROL_RUN_CE        = 6,  /* run calculation engine on pts */
    //    rtCONTROL_ADD_USAGE     7,  /* add usage flags */
    //    rtCONTROL_DEL_USAGE     8,  /* delete usage flags */
    rtCONTROL_SET_CFI       = 9,  /* set configuration interlock */
    rtCONTROL_REL_CFI       = 10, /* release configuration interlock */
    //    rtCONTROL_SET_USAGE     11, /* set usage flags array */
    rtCONTROL_ENABLE_SNAPS  = 12, /* enable snapshotting */
    rtCONTROL_DISABLE_SNAPS = 13, /* disable snapshotting */
    //    rtCONTROL_REUSE_PLINS   14, /* allow reuse of PLINs */
    rtCONTROL_RDR_CFI       = 15, /* obtain a multi-reader CFI */
    rtCONTROL_CLR_CFI       = 16, /* clear a dead process's zombie CFI */
    // Далее - специфичные для XDB
    rtCONTROL_SAVE_XSD      = 17, /* Сохранить XML Schema базы */
    rtCONTROL_CHECK_CONN    = 18  // Проверить статус имеющихся подключений
} TypeOfControl;

/*
 * Database query actions
 * Типы запросов на контроль структуры БДРВ
 * В RTAP есть числовое понятие Категории, предназначенное для хранение битовой маски
 * различных категорий данной точки.
 *
 * TODO: стоит ли перенести понятие Категории в БДРВ, для создания единого запроса по
 * выгрузки информации о Точках нужной категории? Маска категории может формироваться
 * на этапе генерации БДРВ, а может быть синтетической. В первом случае генератор БДРВ
 * формирует маску Категории, основываясь на свойстве генерируемой сейчас Точки.
 * В последнем случае информация о категории нигде в явном не хранится, но при запросе
 * выборки соответствующих данных поисковый движок будет учитывать характеристики Точки.
 *
 * К примеру, запрос на получение списка Точек с плав. точкой, имеющих предысторию:
 *   (1) rtQUERY_PTS_IN_CATEG(CATEG_ANALOG | CATEG_HISTORY_EXISTS)
 *   или
 *   (2) rtQUERY_PTS_IN_CATEG(CATEG_ANALOG_HISTORY_EXISTS)
 */
typedef enum
{
    /* general: no address */
    rtQUERY_CONN_INFO       = 0,       /* get connection information */
    //    rtQUERY_CATEG_NAMES     = 1,       /* get category names (all 32) */
    //    rtQUERY_GROUP_NAMES     = 2,       /* get group names (all 4) */
    rtQUERY_SPACE           = 3,       /* get free space info */
    rtQUERY_CFI             = 4,       /* get Config. interlock data */
    //    rtQUERY_USAGE_NAMES     = 5,       /* get usage flag names (all 256) */
    /* Point level */
    rtQUERY_ALIAS           = 10,      /* get Point Alias */
    rtQUERY_ATTR_CNT        = 11,      /* get Attribute count of Point */
    rtQUERY_ATTR_NAMES      = 12,      /* get Attribute names for Point */
    rtQUERY_ATTR_ORDER      = 13,      /* get Attribute order list */
    rtQUERY_CATEGORIES      = 14,      /* get Point categories */
    rtQUERY_CE_OPER         = 15,      /* get CE operation indicator */
    rtQUERY_EXPR_ORDER      = 16,      /* get expression order type */
    rtQUERY_FIRST_CHILD     = 17,      /* get first child PLIN */
    rtQUERY_LRL             = 18,      /* get logical reference list */
    rtQUERY_NEXT_SIBLING    = 19,      /* get next sibling PLIN */
    rtQUERY_PARENT          = 20,      /* get parent PLIN */
    rtQUERY_PT_NAME         = 21,      /* get rtPointName of Point */
    rtQUERY_RESIDENCE       = 22,      /* get Point residence */
    rtQUERY_USAGE           = 23,      /* get Point usage flags */
    rtQUERY_ATTR_IDS        = 24,      /* get Attribute ID numbers (AINs) */
    //    rtQUERY_ALPHA_ATTRS     = 25,      /* get sorted attribute names */
    rtQUERY_PT_CLASS        = 26,      /* get point class */
    rtQUERY_PTS_IN_CLASS    = 27,      /* get points matching a class */    // есть в RTDB
    rtQUERY_PTS_IN_CATEG    = 28,      /* get points matching categories */ // есть в RTDB
    rtQUERY_CE_DEP_REF      = 29,      /* get pts with CE ref's to a pt */
    rtQUERY_CE_DEP_UPD      = 30,      /* get pts CE would update for a pt */
    /* Attribute level */
    rtQUERY_ATTRIBUTE       = 40,      /* get Attribute specs (type, size) */
    rtQUERY_ATTR_ACCESS     = 41,      /* get read write access to Attribute */
    rtQUERY_ATTR_NAME       = 42,      /* get name of Attribute */
    rtQUERY_DE_TYPE         = 43,      /* get data-element type(s) */
    rtQUERY_EVENT           = 44,      /* get event triggers */
    rtQUERY_FIELD_NAMES     = 45,      /* get field names */
    rtQUERY_GROUPS          = 46,      /* get Attribute access groups */
    rtQUERY_DEFINITION      = 47,      /* get unparsed definition */
    /* any address level */
    rtQUERY_DIRECT          = 60,      /* get direct equiv of sym addr */
    rtQUERY_SYM_ABS         = 61,      /* get abs-path equiv of given addr */
    rtQUERY_SYM_ALIAS       = 62,      /* get alias equiv of given addr */
    rtQUERY_SYM_REL         = 63,      /* get rel-path equiv of given addr */
    rtQUERY_DIRECT_ATTR     = 64,      /* get fully-specified direct addr */
    // Нет аналогов в ГОФО
    rtQUERY_SBS_LIST_ARMED      = 65,  // Получить список активных SBS групп с измененными элементами
    rtQUERY_SBS_POINTS_ARMED    = 66,  // Получить список измененных точек для указанной группы
    rtQUERY_SBS_POINTS_DISARM   = 67,  // Сбросить для всех измененных точек признак модификации
    rtQUERY_SBS_POINTS_DISARM_BY_LIST = 68,  // Сбросить флаг модификации для измененных точек из списка
    rtQUERY_SBS_READ_POINTS_ARMED = 69,// Прочитать список модифицированных атрибутов указанной группы
} TypeOfQuery;

/*
 * database configuration actions
 */
typedef enum
{
    //    rtCONFIG_CATEGORIES     = 1,       /* set point categories */
    //    rtCONFIG_EXPR_ORDER     = 2,       /* change expression order indic */
    rtCONFIG_DEFINITION     = 3,       /* change expression */
    //    rtCONFIG_GROUPS         = 4,       /* set attr access groups */
    rtCONFIG_PT_NAME        = 5,       /* change the name of a point */
    rtCONFIG_MOVE_POINT     = 6,       /* move a point (branch) */
    //    rtCONFIG_ALIAS          = 7,       /* change the alias of a point */
    //    rtCONFIG_ADD_NULL_PT    = 8,       /* add a null point */
    //    rtCONFIG_RESIDENCE      = 9,       /* change point residence */
    rtCONFIG_DEL_BRANCH     = 10,      /* delete branch */
    rtCONFIG_COPY_BRANCH    = 11,      /* copy branch */
    rtCONFIG_COPY_POINT     = 12,      /* copy point */
    //    rtCONFIG_PT_CLASS       = 13,      /* change point class */
    rtCONFIG_DEL_ATTR       = 14,      /* delete attribute */
    rtCONFIG_COPY_ATTR      = 15,      /* copy attribute */
    rtCONFIG_ATTR_NAME      = 16,      /* change attribute name */
    rtCONFIG_ADD_SCALAR     = 20,      /* add scalar attribute */
    rtCONFIG_ADD_VECTOR     = 21,      /* add vector attribute */
    rtCONFIG_ADD_TABLE      = 22,      /* add table attribute */
    rtCONFIG_DEL_BR_CHK     = 23,      /* check if branch deletable */
    rtCONFIG_RECEL_CNT      = 24,      /* change record count of vector/table */
    rtCONFIG_DE_TYPE        = 25,      /* change attribute/field DE type */
    rtCONFIG_FIELD_NAME     = 26,      /* rename a field in a table */
    rtCONFIG_ADD_FIELD      = 27,      /* add a field to a table */
    rtCONFIG_DEL_FIELD      = 28,      /* delete a field from a table */
    rtCONFIG_ATTR_AIN       = 29,      /* change the AIN of an attribute */
    // У следующих кодов нет аналогов в RTAP
    rtCONFIG_ADD_GROUP_SBS  = 30,      /* Создать группу подписки */
    rtCONFIG_DEL_GROUP_SBS  = 31,      /* Удалить группу подписки */
    rtCONFIG_ENABLE_GROUP_SBS  = 32,   /* Активировать группу подписки */
    rtCONFIG_SUSPEND_GROUP_SBS = 33    /* Приостановить группу подписки */
} TypeOfConfig;

// Вид используемого структурой rtDbCq действия
typedef enum
{
  CONTROL   = 1,
  QUERY     = 2,
  CONFIG    = 3
} ActionType;

// Объединение команд Управления, Запроса, Контроля
typedef union {
  TypeOfControl control;
  TypeOfQuery   query;
  TypeOfConfig  config;
} rtDbCqAction;

/*
 * database query, control, and config structure
 */
typedef struct
{
    ActionType      act_type;  // тип действия
    // Поле 'addrCnt' может использоваться не только в виде значения количества точек, но и как:
    // 1) хранилище искомого OBJCLASS Точек (для запроса rtQUERY_PTS_IN_CLASS)
    // 2) хранилище битового массива искомых Категорий Точек (запрос rtQUERY_PTS_IN_CATEG)
    // TODO: переделать некрасивое решение, возможно в этих случаях buffer должен указывать
    // не на контейнер результата, а на структуру, содержащую новый атрибут и контейнер
    // результата вместе.
    uint16_t        addrCnt;   /* count of addresses */
    //rtDbAddress     *addr;     /* addresses of Points */
    std::vector<std::string> *tags; // Теги Точек
    void           *buffer;   /* buffer for data */
    uint32_t        size;      /* size of buffer */
    uint32_t        actual;    /* actual size of data */
    rtDbCqAction    action;    /* action to perform */
} rtDbCq;

// Тип для хранения связи "идентификатор <-> название"
typedef std::map <uint64_t/*autoid_t*/, std::string> map_id_name_t;
// Тип для хранения связи "название <-> идентификатор"
typedef std::map <std::string, uint64_t/*autoid_t*/> map_name_id_t;

// TODO: проверить, можно ли изолировать связанные с этой константой данные в xdb_rtap_snap.hpp
#define ELEMENT_DESCRIPTION_MAXLEN  15

// Таблица типов данных Атрибутов.
// NB: типы атрибутов одинаковы для всех точек различных OBJCLASS, за исключением трех:
// VALACQ, VALMANUAL, VAL - они есть в двух вариантах - с плав. точкой или целое число.
typedef struct
{
  int       index;  // RTDB_ATT_IDX_...
  const char* name; // указатель на строку с названием атрибута RTDB_ATT_
  DbType_t  type;   // DB_TYPE_...
  // размер типа данных
  uint16_t  size;   // размер атрибута в байтах, макс. 32кБ
} AttrTypeDescription_t;

// Таблица описателей типов данных БДРВ
typedef struct
{
  // порядковый номер
  DbType_t   code;
  const char name[ELEMENT_DESCRIPTION_MAXLEN+1];
  // размер типа данных
  uint16_t   size;
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


// ================================================================================
#ifdef __SUNPRO_CC
typedef std::map  <std::string, AttributeInfo_t> AttributeMap_t;
typedef std::pair <std::string, AttributeInfo_t> AttributeMapPair_t;
#else
typedef std::map  <const std::string, AttributeInfo_t> AttributeMap_t;
typedef std::pair <const std::string, AttributeInfo_t> AttributeMapPair_t;
#endif
typedef AttributeMap_t::iterator AttributeMapIterator_t;

/* общие сведения по точке базы данных */
typedef struct
{
#if CMAKE_COMPILER_IS_GNUCC && (CMAKE_CXX_COMPILER_VERSION >= 483)
  int16_t        objclass = GOF_D_BDR_OBJCLASS_UNUSED;
  univname_t     tag = "";
  int            status = 0;
  AttributeMap_t attributes;
// NB: типы autoid_t определены в MCO
  uint64_t       id_SA = 0;
  uint64_t       id_unity = 0;
#else
  int16_t        objclass;
  univname_t     tag;
  int            status;
  AttributeMap_t attributes;
  uint64_t       id_SA;
  uint64_t       id_unity;
#endif
} PointDescription_t;

/* Хранилище набора атрибутов, их типов, кода и описания для каждого objclass */
typedef struct
{
  char    name[sizeof(wchar_t)*TAG_NAME_MAXLEN + 1]; // название класса
  int8_t  code;                      // код класса
  DbType_t val_type;                 // тип атрибутов {VAL|VALMANUAL|VALACQ} данного класса
  // NB: Используется указатель AttributeMap_t* для облегчения 
  // статической инициализации массива
  AttributeMap_t   *attributes_pool; // набор атрибутов с доступом по имени
} ClassDescription_t;

// Тип для хранения списка точек группы подписки вместе со атрибутами, входящими
// в набор "триггеров", при изменении которых происходит детектирование события
// групп подписки для данной точки.
typedef std::vector<PointDescription_t*> SubscriptionPoints_t;

// ------------------------------------------------------------

extern xdb::ClassDescription_t ClassDescriptionTable[];
extern const xdb::DeTypeDescription_t rtDataElem[];
// Получить универсальное имя на основе его алиаса
extern int GetPointNameByAlias(univname_t&, univname_t&);
// Хранилище описаний типов данных БДРВ
extern const xdb::DbTypeDescription_t DbTypeDescription[];
// Хранилище описаний типов в xdb_rtap_const.cpp
extern const xdb::AttrTypeDescription_t AttrTypeDescription[];
extern const xdb::DeTypeToDbTypeLink  DeTypeToDbType[];
// Таблица соответствий размера данных атрибута его типу
extern const uint16_t var_size[];

// ------------------------------------------------------------


} // namespace xdb

#endif

