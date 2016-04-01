#ifndef EXCHANGE_CONFIG_H
#define EXCHANGE_CONFIG_H
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <vector>
#include <map>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netdb.h>

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
// должно где-то храниться состояние самой сиистемы сбора. Для этого предназначена
// таблица SMAD, где содержится список всех СС, подключенный к данной SQLite БД SMAD.
#define s_SA        "SA"

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
#define s_COMMON_BYTE_ORDER "BYTE_ORDER"    // Порядок байт СС
#define s_COMMON_SUB       "SUB"        // Признак необходимости вычитания единицы из адреса 
#define s_COMMON_NAME      "NAME"       // Тег системы сбора (СС)
#define s_COMMON_SMAD      "SMAD"       // Название файла SQLite, содержащего данные СС
#define s_COMMON_CHANNEL   "CHANNEL"    // Тип канала доступа к серверу, TCP|RTU
#define s_COMMON_TRACE     "TRACE"      // Глубина трассировки логов
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
// Направления потока данных - от сервера (0) или к серверу (1)
typedef enum {
  SA_FLOW_ACQUISITION = 0,
  SA_FLOW_DIFFUSION   = 1
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
// Структура хранения данных в БД SMAD
typedef struct {
  char      name[sizeof(wchar_t)*TAG_NAME_MAXLEN + 1]; // название тега
  int       address;    // MODBUS-адрес регистра
  uint64_t  ref;        // Идентификатор текущего описателя в БД
  sa_param_type_t     type;     // Тип параметра (аналоговый, дискретный, тревога,...)
// NB: нужно сравнивать не числовые коды функций, а типы обработки,
// поскольку для некоторых функций числовые коды можно менять (FHR к примеру)
  int       support;// Тип обработки параметра
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

typedef struct {
  char host_name[20 + 1];
  char port_name[20 + 1];
//  struct sockaddr ip_addr;
//  int port_num;
} sa_network_address_t;

typedef struct {
  int timeout;      // Таймаут ожидания ответа в секундах
  int repeat_nb;    // Количество последовательных попыток передачи данных при сбое
  int error_nb;     // Количество последовательных ошибок связи до диагностики разрыва связи
  sa_endianess_t byte_order;   // Порядок байт СС
  int subtract;     // Признак необходимости вычитания единицы из адреса 
  std::string name; // Тег системы сбора (СС)
  std::string smad; // Название файла SQLite, содержащего данные СС
  sa_connection_type_t channel;      // Тип канала доступа к серверу, MODBUS-{TCP|RTU}, OPC-UA
  int trace_level;  // Глубина трассировки логов
} sa_common_t;

typedef struct {
  int lala;
} sa_command_info_t;

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
} mbus_common_info_t;

typedef struct {
  int lala;
} opc_common_info_t;

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
#endif

