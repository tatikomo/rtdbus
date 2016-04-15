#ifndef EXCHANGE_MODBUS_CLIENT_IMPL_H
#define EXCHANGE_MODBUS_CLIENT_IMPL_H
#pragma once

// Общесистемные заголовочные файлы
#include <map>
#include <sys/socket.h>

// Служебные заголовочные файлы сторонних утилит
#include "modbus.h"
//#include "rapidjson/document.h"

// Служебные файлы RTDBUS
#include "exchange_config_sac.hpp"

// ===============================================================================================

// Специфичные вещи для реализации протокола ELE
#define MAX_MODBUS_FUNC_NAME_LEN	3   // symbolic modbus function name length
#define NMAXZAPRFANUC               512 // Макс. число подзапросов в адрес СС за один цикл опроса
// The only 3 EGSA messages may be present at now:
// 00 - global control [send accept = 0]
// 01 - acquisition	 [send accept = 0]
// 07 - telecommand	 [send accept = 0]
// 13 - initialization [send accept = 1]
#define Negsa_message           3
#define SIZEHEADIP              6
#define SIZECRC                 2
#define LENSEND                 64
#define LENPRIEM                2048
#define SIZERECEIVEERROR        3
#define SIZERECEIVEZAPR         3

// Коды запросов с верхнего уровня
#define	Negsa_glob_control      0
#define Negsa_acquisition       1
#define Negsa_initialization    13

class InternalSMAD;

// ---------------------------------------------------------
// Справочная таблица по всем используемым MODBUS-функциям
typedef struct {
  mbus_type_support_t support;
  int	originCode;	// const, оригинальный числовой код функции (до изменения в конфиге)
  int	actualCode;	// используемый числовой код функции (задается в конфиге)
  char	name[MAX_MODBUS_FUNC_NAME_LEN+1]; // const
  int	maxZaprosLength;                  // const
  // Addresses boundaries for each used modbus function
  bool  used;       // sign this function is used in acquisition
} MBusFuncDesription;

/* struct for useful attributes for message from EGSA */
typedef struct  {
    int MessageEGSA; /* Id message from EGSA */
    int NZapr;       /* Number of messages to HOST */
    int SendAccept;  /* send accept if == 1 */
} SReqEGSAAttr;

// Набор состояний модуля
typedef enum {
  // Ещё не подключён, все в порядке
  STATUS_OK = 1,
  // Ещё не подключён, InternalSMAD загружена
  STATUS_OK_SMAD_LOAD,
  // Подключён, все в порядке
  STATUS_OK_CONNECTED,
  // Не подключён, требуется переподключение
  STATUS_OK_NOT_CONNECTED,
  // Подключён, требуется переподключение
  STATUS_FAIL_NEED_RECONNECTED,
  // Не подключён, переподключение нек удалось
  STATUS_FAIL_TO_RECONNECT,
  // Нормальное завершение работы
  STATUS_OK_SHUTDOWN,
  // Нет возможности продолжать работу из-за проблем с InternalSMAD
  STATUS_FATAL_SMAD,
  // Нет возможности продолжать работу из-за проблем с конфигурационными файлами
  STATUS_FATAL_CONFIG,
  // Нет возможности продолжать работу из-за проблем с ОС
  STATUS_FATAL_RUNTIME
} client_status_t;

/*--------------------------------------------------------------*/
/* Common part for each request */
typedef struct {
 unsigned char IdModbusServer;          // Идентификатор modbus-слейва.
 // TODO: сейчас в конфиге можно приводить параметры, которые собираются с одного слейва,
 // но в будущем можно попробовать объединить параметы для разных серверов в одном конфиге.
 unsigned char NumberModbusFunction;    // Актуальный числовой код используемой MODBUS-функции
 int           SupportType;
 int           StartAddress;            // Начальный регистр запроса
 int           QuantityRegisters;       // Количество запрашиваемых регистров данного типа обработки
 // NB: под "регистром" здесь понимается элементарный тип данных, переносящий значение
 // одного тега от СС к АСУТП
 // Протокол MODBUS оперирует понятием "регистр" по-другому - имеется ввиду простой двухбайтовый регистр,
 // содержащий некое значение. Далее из двух подобных регисторов требуется склеить одно значение
 // в формате с плав. запятой.
 sa_flow_direction_t   Direction;       // Направление движения данных
 // NB: [Direction = 0] приём от сервера, [Direction = 1] передача в адрес сервера
 int           SizeSendedPayload;       // Размер запроса [header + crc]
 int           SizeRequestedPayload;    // Одидаемый размер ответа [awaiting bytes]
 int           AmountReceptedData;      // Фактически полученный размер ответа
 uint8_t      *RequestPayload;          // Содержимое буфера для передачи в сервер [LENSEND]
 unsigned char un_Try;                  // Количество последовательных попыток передачи
 unsigned char lg_Receive;              // Признак получения ответа на этот запрос
 char          ReceivedQuality;         // Признак качества полученного ответа
 unsigned char lg_FatalError;           // Признак обнаружения фатальной ошибки
} ModbusOrderDescription;

/* struct for send message  to HOST            */
typedef struct {
    ModbusOrderDescription  ReqModbusAttr[NMAXZAPRFANUC];
    SReqEGSAAttr            ReqEGSAAttr;
} SRequest;

// ---------------------------------------------------------
class RTDBUS_Modbus_client {
  public:
    RTDBUS_Modbus_client(const char*);
   ~RTDBUS_Modbus_client();
    client_status_t connect();
    client_status_t run();

  private:
    DISALLOW_COPY_AND_ASSIGN(RTDBUS_Modbus_client);
    // Итерация запросов сервера СС
    client_status_t ask();
    // На основе прочитанных данных автоматически построить план запросов к СС и вывести его в лог
    int make_request_plan();
    int calculate();
    // Разбор буфера ответа от СС на уровне байтов
    int parse_response(ModbusOrderDescription&, uint8_t*, uint16_t*);
    int parse_HC_IC(address_map_t*, ModbusOrderDescription&, uint8_t*);
    int parse_HR_IR(address_map_t*, ModbusOrderDescription&, uint16_t*);
    int parse_FHR(address_map_t*, ModbusOrderDescription&, uint16_t*);
    int parse_FP(address_map_t*, ModbusOrderDescription&, uint16_t*);
    int parse_DP(address_map_t*, ModbusOrderDescription&, uint16_t*);
    void set_validity(int);

    int read_config();     // Точка входа в функции загрузки конфигурационных файлов
    int load_parameters(); // загрузка параметров обмена
    int load_commands();   // загрузка команд управления

    // Подключиться к внутренней БД для хранения там полученных данных
    client_status_t connect_to_smad();
    // Инициализировать значения всех параметров в InternalSMAD
    client_status_t init_smad_parameters();
    // Завершить формированое запроса
    void polish_order(int, int, ModbusOrderDescription&);
    int resolve_addr_info(const char*, const char*, int&);

    // -------------------------------------------------------------------
    static MBusFuncDesription mbusDescr[];

    AcquisitionSystemConfig* m_config;
    const char*     m_config_filename;
    sa_common_t     m_sa_common;
    // Номер подключения к серверу по порядку в секции SERVICE
    size_t          m_connection_idx;
    // Код состояния модуля
    client_status_t m_status;

    modbus_t *m_ctx;
    // Размер заголовка запроса
    int m_header_length;
    // Параметры самого интерфейсного модуля

    InternalSMAD    *m_smad;
    // Хранилище соответствия "адрес" - "информация по параметру" для каждого типа обработки,
    // что требуется для поиска интересующих регистров в составе ответа при его разборе.
    address_map_t m_HC_map;
    address_map_t m_IC_map;
    address_map_t m_HR_map;
    address_map_t m_IR_map;
    address_map_t m_FHR_map;
    address_map_t m_FP_map;
    address_map_t m_DP_map;
    // Вектор запросов в адрес СС
    std::vector<ModbusOrderDescription> m_actual_orders_description;
};

#endif

