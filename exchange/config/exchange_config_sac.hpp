#ifndef EXCHANGE_CONFIG_SAC_H
#define EXCHANGE_CONFIG_SAC_H
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <vector>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "rapidjson/document.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"

// ---------------------------------------------------------
class AcquisitionSystemConfig
{
  public:
    AcquisitionSystemConfig(const char*);
   ~AcquisitionSystemConfig();

    // Загрузка конфига целиком
    int load();
    // Загрузка из конфига параметров, требуемых к приему (in) и к передаче (out)
    // (разделы PARAMETERS:ACQUISITION, PARAMETERS:TRANSMISSION)
    int load_parameters(sa_parameters_t* in, sa_parameters_t* out);
    // загрузка основной конфигурации: разделы COMMON, CONFIG, SERVERS
    int load_common(sa_common_t&);
    // загрузка основной конфигурации MODBUS
    int load_modbus(sa_protocol_t&);
    // загрузка основной конфигурации OPC-UA
    int load_opc(sa_protocol_t&);
    // загрузка команд управления: раздел PARAMETERS:COMMAND
    int load_commands(sa_commands_t&);

    // COMMON
    int timeout()   { return m_sa_common.timeout; };
    int repeat_nb() { return m_sa_common.repeat_nb; };
    int error_nb()  { return m_sa_common.error_nb; };
    sa_endianess_t byte_order() { return m_sa_common.byte_order; };
    gof_t_SacNature nature() { return m_sa_common.nature; };
    bool address_subtraction()  { return m_sa_common.subtract; };  // Признак необходимости вычитания единицы из адреса 
    std::string& name() { return m_sa_common.name; };              // Тег системы сбора (СС)
    int trace_level()   { return m_sa_common.trace_level; };       // Глубина трассировки логов
    std::string& smad_filename()   { return m_sa_common.smad; };   // Название файла SQLite, содержащего данные СС
    sa_connection_type_t channel() { return m_sa_common.channel; };// Тип канала доступа к серверу, MODBUS-{TCP|RTU}, OPC-UA
    const std::vector <sa_network_address_t>& server_list() { return m_server_address; }

    sa_parameters_t& acquisitions() { return m_sa_parameters_input; };
    sa_parameters_t& transmissions() { return m_sa_parameters_output; };
    sa_commands_t& commands() { return m_sa_commands; };

    // MODBUS
    const mbus_common_info_t& modbus_specific() const { return m_protocol.mbus; };
    // RS-232|485
    const std::vector <sa_rtu_info_t>& rtu_list() const { return m_rtu_list; };

  private:
    DISALLOW_COPY_AND_ASSIGN(AcquisitionSystemConfig);
    char   *m_config_filename;
    bool    m_config_has_good_format;
    rapidjson::Document m_document;
    sa_parameters_t m_sa_parameters_input;
    sa_parameters_t m_sa_parameters_output;
    sa_commands_t   m_sa_commands;
    sa_common_t     m_sa_common;
    // Информация по TCP-подключению - может быть несколько доступных серверов
    std::vector <sa_network_address_t> m_server_address;
    // Информация по RTU-подключениям - может быть нескoлько асинхронных линий
    std::vector <sa_rtu_info_t> m_rtu_list;
    sa_protocol_t m_protocol;
    // Выполнить проверки возможности указанных ограничений для параметра.
    bool check_constrains(const sa_parameter_info_t&);
};

#endif

