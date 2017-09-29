#ifndef EXCHANGE_CONFIG_H
#define EXCHANGE_CONFIG_H
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <vector>
#include <map>

// Название сокета для связи между общим интерфейсом СС и интерфейсом её реализации
#define s_SA_INTERNAL_EXCHANGE_ENDPOINT "inproc://sys_acq_control"
// Названия разделов конфигурационного файла
#define s_COMMON        "common"    // общие сведения о работе модуля
#define s_MODBUS        "modbus"    // сведения, специфичные для работы протокола MODBUS
#define s_OPC           "opc"       // сведения, специфичные для работы протокола OPC
#define s_SERVICE       "service"   // информация по основному и резервным каналам связи с СС
#define s_PARAMETERS    "parameters"    // общий раздел, содержащий параметры
#define s_ACQUISITION   "acquisition"   // параметры, подлежащие модулем приёму от системы сбора
#define s_TRANSMISSION  "transmission"  // параметры, подлежащие передаче модулем в адрес СС

#define s_SA_MODE_MODBUS_TCP    "MODBUS-TCP"
#define s_SA_MODE_MODBUS_RTU    "MODBUS-RTU"
#define s_SA_MODE_OPC_UA        "OPC-UA"

#define s_TS        "TS"
#define s_TSA       "TSA"
#define s_TM        "TM"
#define s_AL        "AL"
// TODO: Убрал точку данного типа вообще из списка параметров. Это и так понятно, что
// должно где-то храниться состояние самой системы сбора. Для этого предназначена
// таблица SMAD в SMAD, где содержится список всех СС, подключенных к данной SQLite БД.
//#define s_SA        "SA"

#define s_PARAM_NAME      "NAME"
#define s_PARAM_INCLUDE   "INCLUDE"
#define s_PARAM_TYPE      "TYPE"
#define s_PARAM_TYPESUP   "TYPESUP"
#define s_PARAM_ADDR      "ADDR"
// NB: Инженерный диапазон используется только для типов обработки HR и IR
#define s_PARAM_LOW_FAN   "LOW_FAN"
#define s_PARAM_HIGH_FAN  "HIGH_FAN"
// Физический диапазон аналоговых значений
#define s_PARAM_LOW_FIS   "LOW_FIS"
#define s_PARAM_HIGH_FIS  "HIGH_FIS"
#define s_PARAM_FACTOR    "FACTOR"

#define s_MBUS_TYPE_SUPPORT_HC  "HC"
#define s_MBUS_TYPE_SUPPORT_IC  "IC"
#define s_MBUS_TYPE_SUPPORT_HR  "HR"
#define s_MBUS_TYPE_SUPPORT_IR  "IR"
#define s_MBUS_TYPE_SUPPORT_FSC "FSC"   // Force Single Coil
#define s_MBUS_TYPE_SUPPORT_PSR "PSR"   // Preset Single Register
#define s_MBUS_TYPE_SUPPORT_FHR "FHR"
#define s_MBUS_TYPE_SUPPORT_FP  "FP"
#define s_MBUS_TYPE_SUPPORT_DP  "DP"
#define code_MBUS_TYPE_SUPPORT_HC  1
#define code_MBUS_TYPE_SUPPORT_IC  2
#define code_MBUS_TYPE_SUPPORT_HR  3
#define code_MBUS_TYPE_SUPPORT_IR  4
#define code_MBUS_TYPE_SUPPORT_FSC 5 // Force Single Coil
#define code_MBUS_TYPE_SUPPORT_PSR 6 // Preset Single Register
#define code_MBUS_TYPE_SUPPORT_FHR 103
#define code_MBUS_TYPE_SUPPORT_FP  104
#define code_MBUS_TYPE_SUPPORT_DP  105
#define MODBUS_TCP_SLAVE 0xFF   // Идентификатор слейва протокола MODBUS/TCP

// Раздел "common"
#define s_COMMON_TIMEOUT   "TIMEOUT"    // Таймаут ожидания ответа в секундах
#define s_COMMON_REPEAT_NB "REPEAT_NB"  // Количество последовательных попыток передачи данных при сбое
#define s_COMMON_ERROR_NB  "ERROR_NB"   // Количество последовательных ошибок связи до диагностики разрыва связи
#define s_COMMON_BYTE_ORDER      "BYTE_ORDER" // Порядок байт СС
#define s_COMMON_BYTE_ORDER_DCBA "DCBA"
#define s_COMMON_BYTE_ORDER_ABCD "ABCD"
// TODO: насколько важно знать самой системе сбора свой тип?
#define s_COMMON_NATURE      "NATURE"   // Тип системы сбора
#define s_COMMON_SUB         "SUB"      // Признак необходимости вычитания единицы из адреса
#define s_COMMON_NAME        "NAME"     // Тег системы сбора (СС)
#define s_COMMON_SMAD        "SMAD"     // Название файла SQLite, содержащего данные СС
#define s_COMMON_CHANNEL     "CHANNEL"  // Тип канала доступа к серверу, TCP|RTU
#define s_COMMON_TRACE       "TRACE"    // Глубина трассировки логов
// Раздел MODBUS
#define s_MODBUS_SLAVE_IDX      "SLAVE_IDX"     // Идентификатор ведомого устройства (slave identity)
#define s_MODBUS_HC_FUNCTION    "HC_FUNCTION"   // Код функции HC
#define s_MODBUS_IC_FUNCTION    "IC_FUNCTION"   // Код функции IC
#define s_MODBUS_HR_FUNCTION    "HR_FUNCTION"   // Код функции HR
#define s_MODBUS_IR_FUNCTION    "IR_FUNCTION"   // Код функции IR
#define s_MODBUS_FSC_FUNCTION   "FSC_FUNCTION"  // Код функции FSC, запись бита
#define s_MODBUS_PSR_FUNCTION   "PSR_FUNCTION"  // Код функции PSR, запись байта
#define s_MODBUS_FHR_FUNCTION   "FHR_FUNCTION"  // Код функции FHR
#define s_MODBUS_FP_FUNCTION    "FP_FUNCTION"   // Код функции FP
#define s_MODBUS_DP_FUNCTION    "DP_FUNCTION"   // Код функции DP
#define s_MODBUS_DELEGATION_COIL_ADDRESS    "DELEGATION_COIL_ADDRESS"
#define s_MODBUS_DELEGATION_2DIPL_WRITE     "DELEGATION_2DIPL_WRITE"
#define s_MODBUS_DELEGATION_2SCC_WRITE      "DELEGATION_2SCC_WRITE"
#define s_MODBUS_TELECOMAND_LATCH_ADDRESS   "TELECOMAND_LATCH_ADDRESS"
#define s_MODBUS_VALIDITY_OFFSET            "VALIDITY_OFFSET"
#define s_MODBUS_DUBIOUS_VALUE              "DUBIOUS_VALUE" // Константа-признак недостоверного значения
// Раздел "config"
// Раздел "service", группа MODBUS-TCP
#define s_SERVICE_HOST   "HOST"     // Адрес сервера
#define s_SERVICE_PORT   "PORT"     // Порт сервера
// Раздел "service", группа MODBUS-RTU
#define s_SERVICE_DEVICE "DEVICE"   // Устройство
#define s_SERVICE_SPEED  "SPEED"    // Скорость обмена с портом
#define s_SERVICE_PARITY "PARITY"   // Чётность
#define s_SERVICE_PARITY_N "N"      // Чётность = NONE
#define s_SERVICE_PARITY_E "E"      // Чётность = EVEN
#define s_SERVICE_PARITY_O "O"      // Чётность = ODD
#define s_SERVICE_NBIT   "NBIT"     // Старт-стоповых бит
#define s_SERVICE_FLOW   "FLOW"     // Контроль потока
#define s_SERVICE_FLOW_SOFTWARE   "SOFTWARE"    // Программный контроль потока
#define s_SERVICE_FLOW_HARDWARE   "HARDWARE"    // Аппаратный контроль потока

// Макс. длина имени файла-устройства для режима RS-232/485/USB
#define MAX_DEVICE_FILENAME_LENGTH      40
#define MAX_CONFIG_SA_FILENAME_LENGTH   400

// Для совместимости с ГОФО
typedef char    gof_t_UniversalName[32];
typedef int32_t gof_t_ExchangeId;

// Начнем нумерацию кодов ошибок после общего кода "Ошибка" (NOK)
enum {
  ESG_ESG_D_ERR_CANNOTOPENFILE = NOK + 1, //	Cannot open file
  ESG_ESG_D_ERR_CANNOTREADFILE, //Cannot read from file
  ESG_ESG_D_ERR_CANNOTWRITFILE, //Cannot write into file

  ESG_ESG_D_WAR_STARTINGESDL,   //Starting ESDL
  ESG_ESG_D_WAR_ESDLTERMINATED, //End of ESDL treatments. Waiting to be killed

  ESG_ESG_D_ERR_CANNOTCLOSESMAD,    //Cannot close SMAD for site

  ESG_ESG_D_ERR_INEXISTINGENTRY,    //Entry does not exist in Oper Data Table
  ESG_ESG_D_ERR_ACQSITETABLEFULL,   //Oper Data Acquisition Sites Table full
  ESG_ESG_D_WAR_CANNOTUPDATECYCLE,  //Cannot update Oper Data Cycle Table
  ESG_ESG_D_ERR_CYCLETABLEFULL, //Oper Data Cycles Table full

  ESG_ESG_D_ERR_UNKNOWNMESS,    //Unknown received message by ESDL
  ESG_ESG_D_ERR_BADMESS,    //Bad received message by ESDL

  ESG_ESG_D_ERR_NOACQSITE,  //Oper Data Table for acquisition sites is empty

  ESG_ESG_D_ERR_MAXREQSREACHED, //Request(s) rejected : max. number reached
  ESG_ESG_D_ERR_MAXCYCSREACHED, //Cycle(s) rejected : max. number reached
  ESG_ESG_D_ERR_MAXSITESREACHED,    //Site(s) rejected : max. number reached

  ESG_ESG_D_ERR_BADCONFFILE,    //Configuration file not correct
  ESG_ESG_D_ERR_UNKNOWNREQTYPE, //Unknown request type
  ESG_ESG_D_ERR_BADSTATEMODEVAL,    //Bad value of the state or mode
  ESG_ESG_D_ERR_BADENTRYID, //Empty string for identifier of Oper Data Table entry
  ESG_ESG_D_WAR_BADREQTYPE, //Bad received request type
  ESG_ESG_D_ERR_SMADEMPTY,  //SMAD for site has zero record
  ESG_ESG_D_ERR_ALLOCACQDATAID, //Acquired data universal names allocation
  ESG_ESG_D_ERR_ALLOCACQDATAINTID,  //Acquired data internal idents allocation
  ESG_ESG_D_ERR_ALLOCACQDATAEXFL,   //Acquired data existing indicators allocation
  ESG_ESG_D_ERR_ACQDATAUNKNOWN, //Acquired data does not exist in DIPL data base
  ESG_ESG_D_WAR_CYCLEWOACQSITE, //Cycle without acquisition site declaration
  ESG_ESG_D_ERR_EQUIPWOACQSITE, //Equipment without acquisition site associated
  ESG_ESG_D_ERR_INFOWOACQSITE,  //Information without acquisition site associated
  ESG_ESG_D_WAR_BADREQCLASS,    //Bad request class

  ESG_ESG_D_ERR_BADALLOC,   //Cannot allocate memory

  ESG_ESG_D_ERR_UNKNOWNFLAG,    //Unknown flag for consulting or updating
  ESG_ESG_D_ERR_INEXISTINGREQ,  //Inexisting request in In Progress List
  ESG_ESG_D_WAR_BADCYCFAMILY,   //Bad cycle family

  ESG_ESG_D_ERR_UNKNOWNREPLY,   //Unknown reply identication
  ESG_ESG_D_ERR_CANNOTSENDMSG,  //Message cannot be sent
  ESG_ESG_D_ERR_NOACQSITE_HCPU, //No acquisition site found for the high CPU load

  ESG_ESG_D_ERR_BADMULRESPSPARAM,   //Parameter error for request multi-responses
  ESG_ESG_D_ERR_MULRESPSTABLEFULL,  //Request multi-responses Table full
  ESG_ESG_D_ERR_INEXISTINGENVVAR,   //Inexisting environment variable

  ESG_ESG_D_ERR_CANNOTLOCK, //Smad lock cannot be performed

  ESG_ESG_D_ERR_CANNOTDELFILE,  //Cannot delete file
  ESG_ESG_D_ERR_TOOMANYOPEN,    //Too many open file
  ESG_ESG_D_ERR_CLOSENOTOPEN,   //Cannot close file : maybe not correctly open
  ESG_ESG_D_ERR_NOTWRITABLE,    //File is not writable
  ESG_ESG_D_ERR_NOTREADABLE,    //File is not readable
  ESG_ESG_D_ERR_EOF,    //End of file entcountered

  ESG_ESG_D_ERR_UNEXPARAMVAL,   //Unexpected value for a parameter
  ESG_ESG_D_ERR_RESERVEMPTY,    //Reserved table is empty
  ESG_ESG_D_ERR_BADDEDFORMAT,   //Bad EDI ACSCII format for elementary data
  ESG_ESG_D_ERR_DEDLGTOOSHORT,  //Buffer for coded elementary data too short
  ESG_ESG_D_ERR_BADEDINTERTYPE, //Bad internal type for elementary data
  ESG_ESG_D_ERR_BADDEDIDENT,    //Bad elementary data identifier
  ESG_ESG_D_ERR_MAXINFOSREACHED,    //Teleinfo(s) rejected : max. number reached
  ESG_ESG_D_ERR_TRANSASCUTCTIME,    //Cannot transforme ascii UTC time to internal
  ESG_ESG_D_ERR_BADDCDIDENT,    //Bad composed data identifier
  ESG_ESG_D_ERR_NBEDINTERDCD,   //Internal and DCD composed data different ED number
  ESG_ESG_D_ERR_DCDLGTOOSHORT,  //Buffer for coded elementary data too short

  ESG_ESG_D_ERR_BADLISTEN,  //Problem listening named pipes (select)
  ESG_ESG_D_ERR_CHILDFAILURE,   //Failure activating child process

  ESG_ESG_D_ERR_NOTOPEN,    //File is not open via file services
  ESG_ESG_D_ERR_BADFILEOP,  //Incorrect file operation
  ESG_ESG_D_ERR_BUFFSIZE,   //Buffer size
  ESG_ESG_D_ERR_BADCATEG,   //Bad category
  ESG_ESG_D_ERR_BADDCDDEDLIST,  //Bad elementary data list in DCD
  ESG_ESG_D_ERR_ODMTABLE,   //Incoherence in ODM table

  ESG_ESG_D_ERR_TABLEFULL,  //Table full
  ESG_ESG_D_ERR_BADARGS,    //Non correct process parameters
  ESG_ESG_D_ERR_ANYEXCHINF, //Any info has not been found
  ESG_ESG_D_ERR_BADINFOCATEGORY,    //Bad info category
  ESG_ESG_D_ERR_BADSEGLABEL,    //Bad segment label
  ESG_ESG_D_ERR_BADSNDRCVID,    //Bad sender-receiver identifier
  ESG_ESG_D_ERR_CRELISTREQENTRY,    //Bad request entry in progress list creating
  ESG_ESG_D_ERR_CONSLISTREQENTRY,   //Bad request entry in progress list consulting
  ESG_ESG_D_ERR_DELELISTREQENTRY,   //Bad request entry in progress list deleting
  ESG_ESG_D_ERR_REQAUTORISATION,    //Request autorisation err. processing
  ESG_ESG_D_WAR_ACQTERMINATED,  //End of ACQ treatments. Waiting to be killed
  ESG_ESG_D_WAR_DIFFTERMINATED, //End of DIFF treatments. Waiting to be killed
  ESG_ESG_D_ERR_NOSTATESEG, //State exchange contains no state segment

  ESG_ESG_D_ERR_BADSUBTYPEATTNB,    //Incorrect number of attributes in sub-type
  ESG_ESG_D_ERR_NOHISTTISUBSCRIPT,  //No subscription for the historic period
  ESG_ESG_D_ERR_NOHISTTISUBTYPE,    //No sub-type for the historic period
  ESG_ESG_D_ERR_TILEDFORHISTACQ,    //No Ti identifier for Ti historic acquisition
  ESG_ESG_D_ERR_HHISTTIFREEREQCONT, //No free content in local request HHistTi
  ESG_ESG_D_ERR_NOMOREHHISTTIREQ,   //No more HHistTi request in period table
  ESG_ESG_D_ERR_NOTIFORHHISTTIREQ,  //No Ti for HHistTi request

  ESG_ESG_D_ERR_INEXPCOMPDATA,  //Inexpected composed data
  ESG_ESG_D_ERR_INCOMPLIST, //The Dir and DIPL list are incompatible

  ESG_ESG_D_ERR_PERIODNFOUND,   //The period to update has not been found
  ESG_ESG_D_WAR_BADREQPRIORITY, //Request priority different than normal/urgent
  ESG_ESG_D_ERR_NONCORRECTDATA, //Incorrect data in BDTR
  ESG_ESG_D_ERR_NOPERIODIHHISTFILE, //No period in HHISTINFSACQ file
  ESG_ESG_D_ERR_QUERYDB,    //Error in query DataBase
  ESG_ESG_D_ERR_BADCOMMAND, //Bad command
  ESG_ESG_D_ERR_DBREAD, //Error in reading data base
};

typedef enum {
  SOFTWARE  = 0,
  HARDWARE  = 1
} rtu_flow_control_t;

// ---------------------------------------------------------
// Виды обработки полученных значений
// NB: Используются как индексы записей соответствующих функций в таблице mbusDescr
typedef enum {
  MBUS_TYPE_SUPPORT_HC  = 0, // "HC"
  MBUS_TYPE_SUPPORT_IC  = 1, // "IC"
  MBUS_TYPE_SUPPORT_HR  = 2, // "HR"
  MBUS_TYPE_SUPPORT_IR  = 3, // "IR"
  MBUS_TYPE_SUPPORT_FSC = 4, // Force Single Coil, Write Bit, запись бита
  MBUS_TYPE_SUPPORT_PSR = 5, // Preset Single Register, Write Byte, запись байта
  MBUS_TYPE_SUPPORT_FHR = 6, // "FHR"
  MBUS_TYPE_SUPPORT_FP  = 7, // "FP"
  MBUS_TYPE_SUPPORT_DP  = 8, // "DP"
  MBUS_TYPE_SUPPORT_UNK = 9  //
} mbus_type_support_t;

// ---------------------------------------------------------
// Режим работы сервера
typedef enum {
  SA_MODE_UNKNOWN    = 0,
  SA_MODE_MODBUS_RTU = 1,
  SA_MODE_MODBUS_TCP = 2,
  SA_MODE_OPC_UA     = 3
} sa_connection_type_t; //mbus_mode_t;

// ---------------------------------------------------------
typedef enum {
  SA_BYTEORDER_ABCD   = 0,
  SA_BYTEORDER_DCBA   = 1,
  SA_BYTEORDER_CDAB   = 2
} sa_endianess_t;

// ---------------------------------------------------------
// Направления потока данных - от сервера (1) или к серверу (2)
typedef enum {
  SA_FLOW_UNKNOWN     = 0,
  SA_FLOW_ACQUISITION = 1,
  SA_FLOW_DIFFUSION   = 2
} sa_flow_direction_t; //mbus_flow_t;

// ---------------------------------------------------------
// Типы параметров
typedef enum {
  SA_PARAM_TYPE_UNK = 0,
  SA_PARAM_TYPE_TM  = 1,
  SA_PARAM_TYPE_TS  = 2,
  SA_PARAM_TYPE_TSA = 3,
  SA_PARAM_TYPE_AL  = 4,
  SA_PARAM_TYPE_SA  = 5
} sa_param_type_t; //mbus_param_type_t

// ---------------------------------------------------------
// Уровень системы сбора
typedef enum {
  LEVEL_UNKNOWN  = 0,    // Неопределенный тип
  LEVEL_LOCAL    = 1,    // Локальная система автоматизации
  LEVEL_LOWER    = 30,   // Подчиненный (нижестоящий) уровень (СЛТМ, САУ КЦ, ...)
  LEVEL_ADJACENT = 31,   // Смежный аналогичный уровень (Соседний объект)
  LEVEL_UPPER    = 50    // Вышестоящий уровень
} sa_object_level_t;

// ---------------------------------------------------------
// Значения атрибута SYNTHSTATE
// NB: Значения синхронизированы со схемой rtap_db, структура SynthState, а также с ГОФО:
// GOF_D_BDR_SYNTHSTATE_UNR	    0
// GOF_D_BDR_SYNTHSTATE_OP		1
// GOF_D_BDR_SYNTHSTATE_PREOP	2
//
typedef enum {
  SYNTHSTATE_UNREACH    = 0,
  SYNTHSTATE_OPER       = 1,
  SYNTHSTATE_PRE_OPER   = 2
} synthstate_t;

// ---------------------------------------------------------
// Категория телеметрии из ESG словарей обмена со смежными системами
typedef enum {
  TM_CATEGORY_OTHER     = 0,
  TM_CATEGORY_PRIMARY   = 1,
  TM_CATEGORY_SECONDARY = 2,
  TM_CATEGORY_TERTIARY  = 4,
  TM_CATEGORY_EXPLOITATION = 8,
  TM_CATEGORY_ALL       = 15,
} telemetry_category_t;

// ---------------------------------------------------------
// definition of the nature of a Sac (site of acquisition)
typedef enum {
  GOF_D_SAC_NATURE_DIR  = 0,
  GOF_D_SAC_NATURE_DIPL,    // 1
  GOF_D_SAC_NATURE_GEKO,    // 2
  GOF_D_SAC_NATURE_A86,     // 3
  GOF_D_SAC_NATURE_FANUC,   // 4
  GOF_D_SAC_NATURE_SLTM,    // 5
  GOF_D_SAC_NATURE_ZOND,    // 6
  GOF_D_SAC_NATURE_STI,     // 7
  GOF_D_SAC_NATURE_SUPERRTU,// 8
  GOF_D_SAC_NATURE_GPRI,    // 9
  GOF_D_SAC_NATURE_ESTM,    // 10
  GOF_D_SAC_NATURE_EELE,    // 11
  GOF_D_SAC_NATURE_ESIR,    // 12
  GOF_D_SAC_NATURE_EIMP,    // 13
  GOF_D_SAC_NATURE_SDEC,    // 14
  GOF_D_SAC_NATURE_SCCSC,   // 15
  GOF_D_SAC_NATURE_EUNK     // 16 (NB: должен идти последним элементом)
} gof_t_SacNature;

// ---------------------------------------------------------
// текущее состояние подключения к SMAD
typedef enum {
  STATE_OK      = 1,
  STATE_DISCONNECTED = 2,
  STATE_ERROR   = 3
} smad_connection_state_t;

// ---------------------------------------------------------
// Структура хранения данных в БД SMAD
typedef struct {
  char      name[sizeof(wchar_t)*TAG_NAME_MAXLEN + 1]; // название тега
  int       address;            // MODBUS-адрес регистра
  uint64_t  ref;                // Идентификатор текущего описателя в БД
  sa_param_type_t     type;     // Тип параметра (аналоговый, дискретный, тревога,...)
  // NB: нужно сравнивать не числовые коды функций, а типы обработки,
  // поскольку для некоторых функций числовые коды можно менять (FHR к примеру)
  int       support;            // Тип обработки параметра
  time_t    rtdbus_timestamp;   // Отметка времени получения параметра
  time_t    sa_timestamp;       // Отметка времени Системы сбора о получении ей параметра
  int       valid;              // Автоматически сформированная достоверность
  int       valid_acq;          // Достоверность, полученная от СС
  // специфичная часть данных для дискретных объектов
  int       i_value;            // Целочисленное значение
  // специфичная часть данных для аналоговых объектов
  double    g_value;            // Значение с плав. запятой
  int       low_fan;            // Нижняя граница инженерного диапазона
  int       high_fan;           // Верхняя граница инженерного диапазона
  double    low_fis;            // Нижняя физическая граница измерения
  double    high_fis;           // Верхняя физическая граница измерения
  double    factor;             // Коэффициент конвертации
} sa_parameter_info_t;

// ---------------------------------------------------------
// Структура хранения описателя сервера в режиме MODBUS-RTU
typedef struct {
  char dev_name[MAX_DEVICE_FILENAME_LENGTH + 1];
  char parity;
  int speed;
  rtu_flow_control_t flow_control;
  int nbit;
  int start_stop;
} sa_rtu_info_t;

// ---------------------------------------------------------
typedef struct {
  char host_name[20 + 1];
  char port_name[20 + 1];
//  struct sockaddr ip_addr;
//  int port_num;
} sa_network_address_t;

// ---------------------------------------------------------
typedef struct {
  int timeout;      // Таймаут ожидания ответа в секундах
  int repeat_nb;    // Количество последовательных попыток передачи данных при сбое
  int error_nb;     // Количество последовательных ошибок связи до диагностики разрыва связи
  sa_endianess_t byte_order;    // Порядок байт СС
  gof_t_SacNature nature;       // Тип системы сбора
  int subtract;     // Признак необходимости вычитания единицы из адреса
  std::string name; // Тег системы сбора (СС)
  std::string smad; // Название файла SQLite, содержащего данные СС
  sa_connection_type_t channel;      // Тип канала доступа к серверу, MODBUS-{TCP|RTU}, OPC-UA
  int trace_level;  // Глубина трассировки логов
} sa_common_t;

// ---------------------------------------------------------
typedef struct {
  int lala;
} sa_command_info_t;

// ---------------------------------------------------------
typedef struct {
  int slave_idx;
  int actual_HC_FUNCTION;  // фактический код функции для способа обработки HC
  int actual_IC_FUNCTION;
  int actual_HR_FUNCTION;
  int actual_IR_FUNCTION;
  int actual_FSC_FUNCTION;
  int actual_PSR_FUNCTION;
  int actual_FHR_FUNCTION;
  int actual_FP_FUNCTION;
  int actual_DP_FUNCTION;
  int delegation_coil_address;
  int delegation_2dipl_write;
  int delegation_2scc_write;
  int telecomand_latch_address;
  int validity_offset;
  double dubious_value;         // Константа-признак недостоверного значения
} mbus_common_info_t;

// ---------------------------------------------------------
typedef struct {
  int lala;
} opc_common_info_t;

// ---------------------------------------------------------
typedef union {
  mbus_common_info_t    mbus;
  opc_common_info_t     opc;
} sa_protocol_t;

// ---------------------------------------------------------
// Тип данных для хранения списка параметров из конфигурационных файлов систем сбора
typedef std::vector<sa_parameter_info_t> sa_parameters_t;
typedef std::vector<sa_command_info_t> sa_commands_t;
// соответствие между числовым адресом параметра и его описанием
typedef std::map <int, sa_parameter_info_t> address_map_t;

// ---------------------------------------------------------
// Тип телеинформации из словаря DED_ELEMTYPES
typedef enum {
  TM_TYPE_UNKNOWN   = 0,
  TM_TYPE_INTEGER   = 1,    // I
  TM_TYPE_TIME      = 2,    // T
  TM_TYPE_STRING    = 3,    // S
  TM_TYPE_REAL      = 4,    // R
  TM_TYPE_LOGIC     = 5     // L
} elemtype_t;

// ---------------------------------------------------------
// Тип телеинформации из словаря ESG
typedef enum {
  TM_CLASS_UNKNOWN      = 0,    // ?
  TM_CLASS_INFORMATION  = 1,    // I
  TM_CLASS_ALARM        = 2,    // A
  TM_CLASS_STATE        = 3,    // S
  TM_CLASS_ORDER        = 4,    // O
  TM_CLASS_HISTORIZATION= 5,    // P
  TM_CLASS_HISTORIC     = 6,    // H
  TM_CLASS_THRESHOLD    = 7,    // T
  TM_CLASS_REQUEST      = 8,    // R
  TM_CLASS_CHANGEHOUR   = 9,    // G
  TM_CLASS_INCIDENT     = 10    // D
} elemtype_class_t;

// ==============================================================================
// Данные для чтения конфигурации раздела конфигурации ESG_LOCSTRUCTS
typedef enum {
  FIELD_TYPE_UNKNOWN    = 0,
  FIELD_TYPE_LOGIC      = 1,
  FIELD_TYPE_INT8       = 2,
  FIELD_TYPE_UINT8      = 3,
  FIELD_TYPE_INT16      = 4,
  FIELD_TYPE_UINT16     = 5,
  FIELD_TYPE_INT32      = 6,
  FIELD_TYPE_UINT32     = 7,
  FIELD_TYPE_FLOAT      = 8,
  FIELD_TYPE_DOUBLE     = 9,
  FIELD_TYPE_DATE       = 10,
  FIELD_TYPE_STRING     = 11,
} field_type_t;

// Internal elementary data
// ------------------------
// internal strings
//   . string pointer
//   . string length
// --------------------------------
typedef struct {
  char*  ps_String;
  size_t i_LgString;
} ech_t_InternalString;

// internal data types
// --------------------------------
typedef union {
  bool      b_Logical;
  uint8_t   o_Uint8;
  uint16_t  h_Uint16;
  uint32_t  i_Uint32;
  int8_t    o_Int8;
  int16_t   h_Int16;
  int32_t   i_Int32;
  float     f_Float;
  double    g_Double;
  timeval   d_Timeval;
  ech_t_InternalString r_Str;
} ech_t_InternalVal;

// elementary data
// ---------------
//
// all internal data types to code to or to decode from EDI format
//   . for coding :
//      IN : type and valeur of data are set
//           (for string fixed/variable the length of the string is set)
//   . for decoding :
//      IN : type of data is set, for string length is set
//           (maximum length of the reserved buffer string)
//      OUT : valeur of data is set, for string length is set
//           (the length of the received string)
// --------------------------------
typedef struct {
	field_type_t type;
	ech_t_InternalVal u_val;
} esg_esg_edi_t_StrElementaryData;

//
// qualify of composed data
//   - b_QualifyUse --> used for coding Composed data
//       Input parameter
//          set to TRUE : if qualify "L3800" has to be taken to account
//          otherwise set to FALSE
//   - i_QualifyValue is set :
//          for coding by the caller of EDI services
//          for decoding by EDI services
//   - b_QualifyExist --> Output parameter updated by decoding procedure
//          (i.e EDI services) to TRUE : qualify "L3800" exists in the DCD entity
// --------------------------------
typedef struct {
  bool b_QualifyUse;
  int  i_QualifyValue;
  bool b_QualifyExist;
} esg_esg_edi_t_StrQualifyComposedData;

#define ECH_D_COMPELEMNB		    20

//
// internal composed data table
//   . number of internal elementary data
//   . elementary data table
// --------------------------------
typedef struct {
  size_t i_NbEData;
  esg_esg_edi_t_StrElementaryData ar_EDataTable[ECH_D_COMPELEMNB];
} esg_esg_edi_t_StrComposedData;

#endif

