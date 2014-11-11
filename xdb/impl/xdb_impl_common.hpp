#pragma once
#ifndef XDB_CORE_COMMON_HPP
#define XDB_CORE_COMMON_HPP

#include <vector>

namespace xdb {

    // Внутренние состояния базы данных
    typedef enum {
        DB_STATE_UNINITIALIZED = 1, // первоначальное состояние
        DB_STATE_INITIALIZED   = 2, // инициализирован runtime
        DB_STATE_ATTACHED      = 3, // вызван mco_db_open
        DB_STATE_CONNECTED     = 4, // вызван mco_db_connect
        DB_STATE_DISCONNECTED  = 5, // вызван mco_db_disconnect
        DB_STATE_CLOSED        = 6  // вызван mco_db_close
    } DBState_t;

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

    // Позиции бит в флагах, передаваемых конструктору
    typedef enum
    {
      OF_POS_CREATE   = 1, // создать БД в случае, если это не было сделано ранее
      OF_POS_READONLY = 2, // открыть в режиме "только для чтения"
      OF_POS_RDWR     = 3, // открыть в режиме "чтение|запись" (по умолчанию)
      OF_POS_TRUNCATE = 4, // открыть пустую базу, удалив данные в существующем экземпляре
      OF_POS_LOAD_SNAP= 5, // открыть базу, заполнив ее данными из последнего снапшота
      OF_POS_SAVE_SNAP= 6  // сохранить дамп базы после завершения работы
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
    rtCONTROL_SAVE_XSD      = 17  /* Сохранить XML Schema базы */
} TypeOfControl;

/*
 * Database query actions
 * Типы запросов на контроль структуры БДРВ
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
    rtQUERY_PTS_IN_CLASS    = 27,      /* get points matching a class */
    rtQUERY_PTS_IN_CATEG    = 28,      /* get points matching categories */
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
    rtCONFIG_ATTR_AIN       = 29       /* change the AIN of an attribute */
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
    uint16_t        addrCnt;   /* count of addresses */
    //rtDbAddress     *addr;     /* addresses of Points */
    std::vector<std::string>    tags; // Теги Точек
    void            *buffer;   /* buffer for data */
    uint32_t        size;      /* size of buffer */
    uint32_t        actual;    /* actual size of data */
    rtDbCqAction    action;    /* action to perform */
} rtDbCq;

/*
 * При изменении перечня синхронизировать его с rtad_db.xsd
 */
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
#define RTDB_ATT_FUNCTION       "FUNCTION"
#define RTDB_ATT_CONVERTCOEFF   "CONVERTCOEFF"
#define RTDB_ATT_INHIB          "INHIB"
#define RTDB_ATT_MNVALPHY       "MNVALPHY"
#define RTDB_ATT_MXPRESSURE     "MXPRESSURE"
#define RTDB_ATT_MXVALPHY       "MXVALPHY"
#define RTDB_ATT_NMFLOW         "NMFLOW"
#define RTDB_ATT_STATUS         "STATUS"
#define RTDB_ATT_SUPPLIERMODE   "SUPPLIERMODE"
#define RTDB_ATT_SUPPLIERSTATE  "SUPPLIERSTATE"
#define RTDB_ATT_SYNTHSTATE     "SYNTHSTATE"
#define RTDB_ATT_DELEGABLE      "DELEGABLE"
#define RTDB_ATT_ETGCODE        "ETGCODE"
#define RTDB_ATT_SITEFLG        "SITEFLG"
#define RTDB_ATT_LOCALFLAG      "LOCALFLAG"
#define RTDB_ATT_ALARMBEGIN     "ALARMBEGIN"
#define RTDB_ATT_ALARMBEGINACK  "ALARMBEGINACK"
#define RTDB_ATT_ALARMENDACK    "ALARMENDACK"
#define RTDB_ATT_ALARMSYNTH     "ALARMSYNTH"
#define RTDB_ATT_L_TYPINFO      "L_TYPINFO"
#define RTDB_ATT_L_EQT          "L_EQT"
#define RTDB_ATT_L_SA           "L_SA"
#define RTDB_ATT_CONFREMOTECMD  "CONFREMOTECMD"
#define RTDB_ATT_FLGREMOTECMD   "FLGREMOTECMD"
#define RTDB_ATT_FLGMAINTENANCE "FLGMAINTENANCE"
#define RTDB_ATT_NAMEMAINTENANCE "NAMEMAINTENANCE"
#define RTDB_ATT_REMOTECONTROL  "REMOTECONTROL"
#define RTDB_ATT_TSSYNTHETICAL  "TSSYNTHETICAL"
#define RTDB_ATT_NONEXE         "NONEXE  "
#define RTDB_ATT_RESPNEG        "RESPNEG"
#define RTDB_ATT_ACTIONTYP      "ACTIONTYP"
#define RTDB_ATT_VALIDCHANGE    "VALIDCHANGE"
#define RTDB_ATT_VAL_LABEL      "VAL_LABEL"
#define RTDB_ATT_LINK_HIST      "LINK_HIST"
#define RTDB_ATT_ACQMOD         "ACQMOD"
#define RTDB_ATT_L_DIPL         "L_DIPL"
#define RTDB_ATT_ALDEST         "ALDEST"
#define RTDB_ATT_INHIBLOCAL     "INHIBLOCAL"
#define RTDB_ATT_UNITY          "UNITY"

}

#endif
