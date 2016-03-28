#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <iostream>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "modbus.h"
#include "sqlite3.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include "exchange_modbus_cli.hpp"
#include "exchange_modbus_server_sim.hpp"

// ==========================================================================================
RTDBUS_Modbus_server::RTDBUS_Modbus_server(const char* config_filename)
  : m_ctx(NULL),
    m_socket(-1),
    m_debug(true),
    m_header_length(0),
    mb_mapping(NULL),
    m_query(NULL),
    m_config(NULL),
    m_config_filename(),
    m_UT_BITS_ADDRESS(0),
    m_UT_BITS_NB(0),
    m_UT_INPUT_BITS_ADDRESS(0),
    m_UT_INPUT_BITS_NB(0),
    m_UT_REGISTERS_ADDRESS(0),
    m_UT_REGISTERS_NB(0),
    m_UT_INPUT_REGISTERS_ADDRESS(0),
    m_UT_INPUT_REGISTERS_NB(0)
{
  m_config = new ExchangeConfig(config_filename);
}

// ==========================================================================================
RTDBUS_Modbus_server::~RTDBUS_Modbus_server()
{
  delete m_config;

  if (m_socket != -1) {
    close(m_socket);
  }

  modbus_mapping_free(mb_mapping);

  modbus_close(m_ctx);
  modbus_free(m_ctx);

  delete [] m_query;
}

// ==========================================================================================
int RTDBUS_Modbus_server::init()
{
  int status = 0;
  const std::vector <sa_network_address_t>& list = m_config->server_list();

  if (0 == (status = load_config())) {

    switch(m_config->channel()) {
      case SA_MODE_MODBUS_TCP:
        m_ctx = modbus_new_tcp("127.0.0.1", list[0].port_num);
        m_query = new uint8_t[MODBUS_TCP_MAX_ADU_LENGTH];
        break;

      case SA_MODE_MODBUS_RTU:
        m_ctx = modbus_new_rtu(m_config->rtu_info().dev_name,
                               m_config->rtu_info().speed,
                               m_config->rtu_info().parity,
                               m_config->rtu_info().nbit,
                               m_config->rtu_info().start_stop);
        m_query = new uint8_t[MODBUS_RTU_MAX_ADU_LENGTH];
        modbus_set_slave(m_ctx, m_config->modbus_specific().slave_idx);
        break;

      default:
        status = -1;
    }
  }

  if (-1 != status) {
    m_header_length = modbus_get_header_length(m_ctx);

    modbus_set_debug(m_ctx, m_debug);

    mb_mapping = modbus_mapping_new(
        // Размер битовых полей
        m_UT_BITS_ADDRESS + m_UT_BITS_NB,
        m_UT_INPUT_BITS_ADDRESS + m_UT_INPUT_BITS_NB,
        // Размер полей с плав. запятой
        m_UT_REGISTERS_ADDRESS + m_UT_REGISTERS_NB,
        m_UT_INPUT_REGISTERS_ADDRESS + m_UT_INPUT_REGISTERS_NB);

    if (mb_mapping == NULL) {
       LOG(ERROR) << "Failed to allocate the mapping: " << modbus_strerror(errno);
       status = -1;
    }
  }

  return status;
}

// ==========================================================================================
// Прочитать конфигурационный файл
int RTDBUS_Modbus_server::load_config()
{
  int status = 0;
 
  // загрузка всей конфигурации
  if (0 == (status = m_config->load())) {

    // Подставить действительный код функции, используемый для запросов данных параметров
    m_supply_info[MBUS_TYPE_SUPPORT_HC].actual = m_config->modbus_specific().actual_HC_FUNCTION;
    m_supply_info[MBUS_TYPE_SUPPORT_IC].actual = m_config->modbus_specific().actual_IC_FUNCTION;
    m_supply_info[MBUS_TYPE_SUPPORT_HR].actual = m_config->modbus_specific().actual_HR_FUNCTION;
    m_supply_info[MBUS_TYPE_SUPPORT_IR].actual = m_config->modbus_specific().actual_IR_FUNCTION;
    m_supply_info[MBUS_TYPE_SUPPORT_FHR].actual= m_config->modbus_specific().actual_FHR_FUNCTION;
    m_supply_info[MBUS_TYPE_SUPPORT_FP].actual = m_config->modbus_specific().actual_FP_FUNCTION;
    m_supply_info[MBUS_TYPE_SUPPORT_DP].actual = m_config->modbus_specific().actual_DP_FUNCTION;

    // определить границы параметров
    int i = 0;
    for (sa_parameters_t::iterator itr = m_config->acquisitions().begin();
         itr != m_config->acquisitions().end();
         itr++)
    {
      const sa_parameter_info_t &info = (*itr);
      LOG(INFO) << ++i << "/" << m_config->acquisitions().size()
            << " "
            << info.name << ":"
            << info.address << ":"
            << info.type << ":"
            << info.support << ":"
            << info.low_fan << ":"
            << info.high_fan << ":"
            << info.low_fis << ":"
            << info.high_fis;

      if ((MBUS_TYPE_SUPPORT_HC <= info.support) && (info.support < MBUS_TYPE_SUPPORT_UNK)) {
        m_supply_info[info.support].use = true;

        if (info.address > m_supply_info[info.support].min_max.last)
          m_supply_info[info.support].min_max.last = info.address;

        if (info.address < m_supply_info[info.support].min_max.first)
          m_supply_info[info.support].min_max.first = info.address;
      }
    } // Конец перебора всех хранимых сервером параметров


    for (i = MBUS_TYPE_SUPPORT_HC;
         i <= MBUS_TYPE_SUPPORT_DP;
         i++)
    {
      if (true == m_supply_info[i].use) {
        LOG(INFO) << "Will use support type=" << i
                  << " first_register_number=" << m_supply_info[i].min_max.first
                  << " last_register_number=" << m_supply_info[i].min_max.last;
        switch(i) {
          case MBUS_TYPE_SUPPORT_HC:    // 0
            m_UT_BITS_ADDRESS = m_supply_info[i].min_max.first;
            m_UT_BITS_NB = (m_supply_info[i].min_max.last - m_supply_info[i].min_max.first) + 1;
            break;

          case MBUS_TYPE_SUPPORT_IC:    // 1
            m_UT_INPUT_BITS_ADDRESS = m_supply_info[i].min_max.first;
            m_UT_INPUT_BITS_NB = (m_supply_info[i].min_max.last - m_supply_info[i].min_max.first) + 1;
            break;

          case MBUS_TYPE_SUPPORT_HR:    // 2
            m_UT_REGISTERS_ADDRESS = m_supply_info[i].min_max.first;
            m_UT_REGISTERS_NB = (m_supply_info[i].min_max.last - m_supply_info[i].min_max.first) + 1;
            break;

          case MBUS_TYPE_SUPPORT_IR:    // 3
            m_UT_INPUT_REGISTERS_ADDRESS = m_supply_info[i].min_max.first;
            m_UT_INPUT_REGISTERS_NB = (m_supply_info[i].min_max.last - m_supply_info[i].min_max.first) + 1;
            break;

          case MBUS_TYPE_SUPPORT_FSC:   // 4
          case MBUS_TYPE_SUPPORT_PSR:   // 5
            LOG(ERROR) << "Not yet supported";
            assert(0==1);
            break;

          case MBUS_TYPE_SUPPORT_FHR:   // 6
          case MBUS_TYPE_SUPPORT_FP:    // 7
            m_UT_INPUT_REGISTERS_ADDRESS = m_supply_info[i].min_max.first;
            m_UT_INPUT_REGISTERS_NB = (m_supply_info[i].min_max.last - m_supply_info[i].min_max.first) + 1;

            // Проверить возможность нештатного использования штатных кодов функций MODBUS
            if ((m_supply_info[i].actual == code_MBUS_TYPE_SUPPORT_HR)
              ||(m_supply_info[i].actual == code_MBUS_TYPE_SUPPORT_IR))
            {
              m_UT_INPUT_REGISTERS_NB *= 2;
            }
            break;

          case MBUS_TYPE_SUPPORT_DP:    // 8
            m_UT_INPUT_REGISTERS_ADDRESS = m_supply_info[i].min_max.first;
            m_UT_INPUT_REGISTERS_NB = (m_supply_info[i].min_max.last - m_supply_info[i].min_max.first) + 1;

            // Проверить возможность нештатного использования штатных кодов функций MODBUS
            if ((m_supply_info[i].actual == code_MBUS_TYPE_SUPPORT_HR)
              ||(m_supply_info[i].actual == code_MBUS_TYPE_SUPPORT_IR))
            {
              m_UT_INPUT_REGISTERS_NB *= 4;
            }
            break;
        }
      }
    }
  } // успешная загрузка всей конфигурации

  return status;
}

// ==========================================================================================
int RTDBUS_Modbus_server::run()
{
  int rc;

  if (0 == (rc = init()))
  {
    if (m_config->channel() == SA_MODE_MODBUS_TCP)
    {
      m_socket = modbus_tcp_listen(m_ctx, 1);
      modbus_tcp_accept(m_ctx, &m_socket);
    }
    else
    {
        rc = modbus_connect(m_ctx);
        if (rc == -1)
        {
          LOG(ERROR) << "Unable to connect: " << modbus_strerror(errno);
          return -1;
        }
    }

    for (;;) {
        do {
            rc = modbus_receive(m_ctx, m_query);
            /* Filtered queries return 0 */
        } while (rc == 0);

        // The connection is not closed on errors which require on reply such as bad CRC in RTU
        if (rc == -1 && errno != EMBBADCRC) {
            /* Quit */
            break;
        }
        
        simulate_values();

        rc = modbus_reply(m_ctx, m_query, rc, mb_mapping);
        if (rc == -1) {
            break;
        }
    }

  }

  return rc;
}

// ==========================================================================================
void RTDBUS_Modbus_server::simulate_values()
{
  int i = 0;

    for (sa_parameters_t::iterator itr = m_config->acquisitions().begin();
         itr != m_config->acquisitions().end();
         itr++)
    {
      const sa_parameter_info_t &info = (*itr);
#if 0
      LOG(INFO) << ++i << "/" << m_config->acquisitions().size()
            << " "
            << info.name << ":"
            << info.address << ":"
            << info.type << ":"
            << info.support << ":"
            << info.low_fan << ":"
            << info.high_fan << ":"
            << info.low_fis << ":"
            << info.high_fis;
#endif

      // Если тип обработки этого параметра используется
      if (true == m_supply_info[info.support].use) {
        m_supply_info[info.support].use = true;

        switch(m_supply_info[info.support].supply)
        {
          case MBUS_TYPE_SUPPORT_HC:
            // используется область памяти mb_mapping.tab_bits
            for (i=0; i < m_UT_BITS_NB; i++) {
                mb_mapping->tab_bits[i] = (uint8_t) (255.0*rand() / (RAND_MAX + 1.0));
                mb_mapping->tab_bits[i] = mb_mapping->tab_bits[i] % 2;
            }
            break;

          case MBUS_TYPE_SUPPORT_IC:
            // используется область памяти mb_mapping.tab_input_bits
            for (i=0; i < m_UT_INPUT_BITS_NB; i++) {
                mb_mapping->tab_input_bits[i] = (uint8_t) (255.0*rand() / (RAND_MAX + 1.0));
                mb_mapping->tab_input_bits[i] = mb_mapping->tab_input_bits[i] % 2;
            }
            break;

          case MBUS_TYPE_SUPPORT_HR:
            // используется область памяти mb_mapping.tab_registers
            for (i=0; i < m_UT_REGISTERS_NB; i++) {
                mb_mapping->tab_registers[i] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
            }
            break;

          case MBUS_TYPE_SUPPORT_IR:
          case MBUS_TYPE_SUPPORT_FHR:
          case MBUS_TYPE_SUPPORT_FP:
          case MBUS_TYPE_SUPPORT_DP:
            // используется область памяти mb_mapping.tab_input_registers
            for (i=0; i < m_UT_INPUT_REGISTERS_NB; i++) {
                mb_mapping->tab_input_registers[i] = ~mb_mapping->tab_input_registers[i];
            }
            break;

          default:
            LOG(ERROR) << "Unsupported processing type: " << info.support;
        }





//        if (info.address > m_supply_info[info.support].min_max.last)
//          m_supply_info[info.support].min_max.last = info.address;
//
//        if (info.address < m_supply_info[info.support].min_max.first)
//          m_supply_info[info.support].min_max.first = info.address;
      }
    } // Конец перебора всех хранимых сервером параметров


}

// ==========================================================================================
int main(int argc, char* argv[])
{
  int  opt;
  char config_filename[2000 + 1] = "BI4500.json";
  RTDBUS_Modbus_server *mbus_server = NULL;
  int rc;

#ifndef VERBOSE
  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();
#endif

  LOG(INFO) << "RTDBUS MODBUS server simulator " << argv[0] << " START";

  while ((opt = getopt (argc, argv, "c:")) != -1)
  {
     switch (opt)
     {
       case 'c': // Конфигурационный файл системы сбора
         strncpy(config_filename, optarg, 2000);
         config_filename[2000] = '\0';
         break;

       case '?':
         if (optopt == 'n')
           fprintf (stderr, "Option -%c requires an argument.\n", optopt);
         else if (isprint (optopt))
           fprintf (stderr, "Unknown option `-%c'.\n", optopt);
         else
           fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
         return 1;

       default:
         abort ();
     }
  }

  mbus_server = new RTDBUS_Modbus_server(config_filename);

  rc = mbus_server->run();
  
  return rc;
}
