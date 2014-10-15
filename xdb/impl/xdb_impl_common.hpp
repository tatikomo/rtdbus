#pragma once
#ifndef XDB_CORE_COMMON_HPP
#define XDB_CORE_COMMON_HPP

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
      OF_POS_LOAD_SNAP= 5  // открыть базу, заполнив ее данными из последнего снапшота
    } FlagPos_t;

#define RTDB_ATT_CODE       "CODE"
#define RTDB_ATT_UNIVNAME   "UNIVNAME"
#define RTDB_ATT_OBJCLASS   "OBJCLASS"
#define RTDB_ATT_LABEL      "LABEL"
#define RTDB_ATT_SHORTLABEL "SHORTLABEL"
#define RTDB_ATT_L_SA       "L_SA"
#define RTDB_ATT_VALID      "VALID"
#define RTDB_ATT_VALIDACQ   "VALIDACQ"
#define RTDB_ATT_DATEHOURM  "DATEHOURM"
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
#define RTDB_ATT_REMOTECONTROL  "REMOTECONTROL"
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
