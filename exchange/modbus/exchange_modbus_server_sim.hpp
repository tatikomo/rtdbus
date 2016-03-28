#ifndef MODBUS_SERVER_SIM_IMPL_H
#define MODBUS_SERVER_SIM_IMPL_H
#pragma once

// Общесистемные заголовочные файлы
// Служебные заголовочные файлы сторонних утилит
#include "sqlite3.h"
#include "modbus.h"
#include "rapidjson/document.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"

// Структуры для хранения информации из конфига СС для формирования ответа
typedef struct { int first; int last; } min_max_t;
// supply:  Тип обработки
// actual:  Код функции MODBUS, используемый для обработки данного блока параметров
// use:     Признак использования кода
// min_max: Минимальное и максимальное значение регистра для данного типа обработки
// data:    Указатель на область памяти, где размещаются параметры
typedef struct { int supply; int actual; bool use; min_max_t min_max; uint8_t* data; } supply_info_t;

// ---------------------------------------------------------
class RTDBUS_Modbus_server {
  public:
    RTDBUS_Modbus_server(const char*);
   ~RTDBUS_Modbus_server();
    int run();

  private:
    // Инициализация работы
    int init();
    // Загрузить всю информацию для симуляции из конфига имитируемой СС
    int load_config();
    // Имитировать изменения данных областях, соответствующих запросам
    void simulate_values();

    // Конфигурационный файл системы сбора
    modbus_t   *m_ctx;
    int         m_port_num;
    int         m_socket;
    bool        m_debug;
    int         m_header_length;
    // Область с данными
    modbus_mapping_t *mb_mapping;
    // Область полученного от Клиента запроса
    uint8_t    *m_query;
    // Доступ к конфигурационным данным СС
    ExchangeConfig* m_config;

    supply_info_t m_supply_info[MBUS_TYPE_SUPPORT_UNK] = 
    {
    { MBUS_TYPE_SUPPORT_HC, code_MBUS_TYPE_SUPPORT_HC, false, { 65535, 0 }, NULL },
    { MBUS_TYPE_SUPPORT_IC, code_MBUS_TYPE_SUPPORT_IC, false, { 65535, 0 }, NULL },
    { MBUS_TYPE_SUPPORT_HR, code_MBUS_TYPE_SUPPORT_HR, false, { 65535, 0 }, NULL },
    { MBUS_TYPE_SUPPORT_IR, code_MBUS_TYPE_SUPPORT_IR, false, { 65535, 0 }, NULL },
    { MBUS_TYPE_SUPPORT_FSC,code_MBUS_TYPE_SUPPORT_FSC,false, { 65535, 0 }, NULL },
    { MBUS_TYPE_SUPPORT_PSR,code_MBUS_TYPE_SUPPORT_PSR,false, { 65535, 0 }, NULL },
    { MBUS_TYPE_SUPPORT_FHR,code_MBUS_TYPE_SUPPORT_FHR,false, { 65535, 0 }, NULL },
    { MBUS_TYPE_SUPPORT_FP, code_MBUS_TYPE_SUPPORT_FP, false, { 65535, 0 }, NULL },
    { MBUS_TYPE_SUPPORT_DP, code_MBUS_TYPE_SUPPORT_DP, false, { 65535, 0 }, NULL }
    };

    char  m_config_filename[MAX_CONFIG_SA_FILENAME_LENGTH + 1];

    int m_UT_BITS_ADDRESS;
    int m_UT_BITS_NB;

    int m_UT_INPUT_BITS_ADDRESS;
    int m_UT_INPUT_BITS_NB;

    int m_UT_REGISTERS_ADDRESS;
    int m_UT_REGISTERS_NB;

    int m_UT_INPUT_REGISTERS_ADDRESS;
    int m_UT_INPUT_REGISTERS_NB;
};

#endif

