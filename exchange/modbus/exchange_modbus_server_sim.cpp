#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "modbus.h"
#include "sqlite3.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include "exchange_modbus_cli.hpp"
#include "exchange_modbus_server_sim.hpp"
#include "exchange_config_sac.hpp"

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
  m_config = new AcquisitionSystemConfig(config_filename);
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

// TODO: Устранить дублирование - такой же код находится в клиенте MODBUS
int RTDBUS_Modbus_server::resolve_addr_info(const char* host_name, const char* port_name, int& port_num)
{
  int rc = OK;
//  struct addrinfo *servinfo;  // указатель на результаты
  struct addrinfo hints, *res, *p;
  char ipstr[INET6_ADDRSTRLEN];
  void *addr;
  int status;

  memset(&hints, 0, sizeof(hints)); // убедимся, что структура пуста
  hints.ai_family = AF_INET;        // IPv4 (AF_UNSPEC = неважно, IPv4 или IPv6)
  hints.ai_socktype = SOCK_STREAM;  // TCP stream-sockets

  if ((status = getaddrinfo(host_name, port_name, &hints, &res)) != 0) {
    LOG(ERROR) << "Resolving " << host_name << ":" << port_name
               << " - getaddrinfo(): " << gai_strerror(status);
    rc = NOK;
  }
  else {
      // Имя и порт успешно разрешились, продолжим
      for(p = res;p != NULL; p = p->ai_next) {
        
        // получаем указатель на адрес, по разному в разных протоколах
        if (p->ai_family == AF_INET) { // IPv4
          struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
          addr = &(ipv4->sin_addr);
          port_num = htons(ipv4->sin_port);

          // преобразуем IP в строку и выводим его:
          inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
          LOG(INFO) << ipstr;
        }
        else { // IPv6
          LOG(ERROR) << "IPv6 for '" << host_name << "' is forbidden";
          rc = NOK;
        }
      }
      freeaddrinfo(res); // free the linked list

      if (OK == rc) {
        LOG(INFO) << "Resolve '" << host_name << "' as " << ipstr
                  << ", port '" << port_name << "' as " << (unsigned int) port_num;
      }
  }

  return rc;
}

// ==========================================================================================
int RTDBUS_Modbus_server::init()
{
  int status = OK;
  const char* localhost = "127.0.0.1";
  const std::vector <sa_network_address_t>& list = m_config->server_list();
  int port_num;

  if (OK == (status = load_config())) {

    switch(m_config->channel()) {
      case SA_MODE_MODBUS_TCP:
        if (OK == resolve_addr_info(localhost, list[0].port_name, port_num)) {

          m_ctx = modbus_new_tcp(localhost, port_num);
          if (m_ctx) {
            modbus_set_slave(m_ctx, m_config->modbus_specific().slave_idx);
            m_query = new uint8_t[MODBUS_TCP_MAX_ADU_LENGTH];
          }
          else {
            LOG(ERROR) << "Unable to create context:" << modbus_strerror(errno);
            status = NOK;
          }
        }
        else status = NOK;
        break;

      case SA_MODE_MODBUS_RTU:
        LOG(INFO) << "Use RTU mode";
        m_ctx = modbus_new_rtu(m_config->rtu_list()[0].dev_name,
                               m_config->rtu_list()[0].speed,
                               m_config->rtu_list()[0].parity,
                               m_config->rtu_list()[0].nbit,
                               m_config->rtu_list()[0].start_stop);
        if (m_ctx) {
          m_query = new uint8_t[MODBUS_RTU_MAX_ADU_LENGTH];
          modbus_set_slave(m_ctx, m_config->modbus_specific().slave_idx);
        }
        else {
          LOG(ERROR) << "Unable to open device " << m_config->rtu_list()[0].dev_name
                     << " - " << modbus_strerror(errno);
          status = NOK;
        }
        break;

      default:
        status = NOK;
    }
  }

  if (OK == status) {
    m_header_length = modbus_get_header_length(m_ctx);

    modbus_set_debug(m_ctx, m_debug);

    mb_mapping = modbus_mapping_new(
        // Размер битовых полей
        m_UT_BITS_ADDRESS + m_UT_BITS_NB,
        m_UT_INPUT_BITS_ADDRESS + m_UT_INPUT_BITS_NB,
        // Размер полей с плав. запятой
        m_UT_REGISTERS_ADDRESS + m_UT_REGISTERS_NB,
        m_UT_INPUT_REGISTERS_ADDRESS + m_UT_INPUT_REGISTERS_NB);

    LOG(INFO) << "BITS_ADDRESS=" << m_UT_BITS_ADDRESS << " BITS_NB=" << m_UT_BITS_NB
              << ", INPUT_BITS_ADDRESS=" << m_UT_INPUT_BITS_ADDRESS << " INPUT_BITS_NB=" << m_UT_INPUT_BITS_NB
              << ", REGISTERS_ADDRESS=" << m_UT_REGISTERS_ADDRESS << " REGISTERS_NB=" << m_UT_REGISTERS_NB
              << ", INPUT_REGISTERS_ADDRESS=" << m_UT_INPUT_REGISTERS_ADDRESS << " INPUT_REGISTERS_NB=" << m_UT_INPUT_REGISTERS_NB;

    if (mb_mapping == NULL) {
       LOG(ERROR) << "Failed to allocate the mapping: " << modbus_strerror(errno);
       status = NOK;
    }
  }

  return status;
}

// ==========================================================================================
// Прочитать конфигурационный файл
int RTDBUS_Modbus_server::load_config()
{
  int status = OK;
 
  // загрузка всей конфигурации
  if (OK == (status = m_config->load())) {

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

  if (OK == (rc = init()))
  {
    if (m_config->channel() == SA_MODE_MODBUS_TCP)
    {
      m_socket = modbus_tcp_listen(m_ctx, 1);
      if (m_socket > 0) {
        modbus_tcp_accept(m_ctx, &m_socket);
      }
      else {
        LOG(ERROR) << "Unable to listen: " << modbus_strerror(errno);
        return NOK;
      }
    }
    else if (m_config->channel() == SA_MODE_MODBUS_RTU)
    {
        if (modbus_connect(m_ctx) < 0)
        {
          LOG(ERROR) << "Unable to connect: " << modbus_strerror(errno);
          return NOK;
        }
    }
    else {
      LOG(ERROR) << "Unsupported channel type: " << m_config->channel();
      return NOK;
    }

    for (;;) {
        simulate_values();
        do {
            rc = modbus_receive(m_ctx, m_query);
            /* Filtered queries return 0 */
        } while (rc == 0);

        // The connection is not closed on errors which require on reply such as bad CRC in RTU
        if (rc == -1 && errno != EMBBADCRC) {
            rc = NOK;
            /* Quit */
            break;
        }

        if (modbus_reply(m_ctx, m_query, rc, mb_mapping) < 0) {
            rc = NOK;
            break;
        }
    }

  }

  return rc;
}

// ==========================================================================================
void RTDBUS_Modbus_server::simulate_HC(const sa_parameter_info_t &info, int actual, uint8_t* data)
{
  LOG(INFO) << "HC simulate for " << info.name << ":" << info.address << " with #func=" << actual;
}

// ==========================================================================================
void RTDBUS_Modbus_server::simulate_IC(const sa_parameter_info_t &info, int actual, uint8_t* data)
{
  uint8_t val = (uint8_t) (255.0*rand() / (RAND_MAX + 1.0));
  data[info.address] = val % 2;
  LOG(INFO) << "IC simulated val=" << (unsigned int)data[info.address] << " for " << info.name << ":" << info.address << " with #func=" << actual;
}

// ==========================================================================================
void RTDBUS_Modbus_server::simulate_HR(const sa_parameter_info_t &info, int actual, uint16_t* data)
{
  common_float_t common_float;
  common_double_t common_double;

  switch (actual) {
    case code_MBUS_TYPE_SUPPORT_HR:
      // Одно значение - один регистр uint16_t
      data[info.address] += 1;
      if ((float)data[info.address] > info.high_fan)
        data[info.address] = info.low_fan;

      LOG(INFO) << "HR simulated val=" << (unsigned int)data[info.address] << " for " << info.name << ":" << info.address << " with #func=" << actual;
      // data[info.address] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
      break;

    case code_MBUS_TYPE_SUPPORT_FHR:
    case code_MBUS_TYPE_SUPPORT_FP:
      // Одно значение - два последовательных регистра uint16_t
      common_float.i16_val[0] = data[info.address + 0];
      common_float.i16_val[1] = data[info.address + 1];
      common_float.f_val += (info.high_fis - info.low_fis) / 100.0;
      if (common_float.f_val > info.high_fis)
        common_float.f_val = info.low_fis;

      data[info.address + 0] = common_float.i16_val[0];
      data[info.address + 1] = common_float.i16_val[1];

      LOG(INFO) << "HR simulated val=" << common_float.f_val << " for " << info.name << ":" << info.address << " with #func=" << actual;

      //data[info.address + 0] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
      //data[info.address + 1] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
      break;

    case code_MBUS_TYPE_SUPPORT_DP:
      // Одно значение - четыре последовательных регистра uint16_t
      common_double.i16_val[0] = data[info.address + 0];
      common_double.i16_val[1] = data[info.address + 1];
      common_double.i16_val[2] = data[info.address + 2];
      common_double.i16_val[3] = data[info.address + 3];
      common_double.g_val += (info.high_fis - info.low_fis) / 100.0;
      if (common_double.g_val > info.high_fis)
        common_double.g_val = info.low_fis;

      data[info.address + 0] = common_double.i16_val[0];
      data[info.address + 1] = common_double.i16_val[1];
      data[info.address + 2] = common_double.i16_val[2];
      data[info.address + 3] = common_double.i16_val[3];

      LOG(INFO) << "HR simulated val=" << common_double.g_val << " for " << info.name << ":" << info.address << " with #func=" << actual;

      //data[info.address + 0] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
      //data[info.address + 1] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
      //data[info.address + 2] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
      //data[info.address + 3] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
      break;

    default:
      LOG(ERROR) << "Unsupported actual code (" << actual << ") for HR";
      assert(0 == 1);
  }
}

// ==========================================================================================
void RTDBUS_Modbus_server::simulate_FP(const sa_parameter_info_t &info, int actual, uint16_t* data)
{
  common_float_t common_float;

  switch (actual) {
    /*
      // Одно значение - один регистр uint16_t
      data[info.address] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
      LOG(INFO) << "FP simulated val=" << (unsigned int)data[info.address] << " for " << info.name << ":" << info.address << " with #func=" << actual;
      break;
      */

    case code_MBUS_TYPE_SUPPORT_HR:
    case code_MBUS_TYPE_SUPPORT_IR:
    case code_MBUS_TYPE_SUPPORT_FHR:
    case code_MBUS_TYPE_SUPPORT_FP:
      // Одно значение - два последовательных регистра uint16_t
      common_float.i16_val[0] = data[info.address + 0]; //(uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
      common_float.i16_val[1] = data[info.address + 1]; //(uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
      common_float.f_val += 1.0;
      if (common_float.f_val > info.high_fis)
        common_float.f_val = info.low_fis;

      data[info.address + 0] = common_float.i16_val[0];
      data[info.address + 1] = common_float.i16_val[1];

      LOG(INFO) << "FP simulated val=" << common_float.f_val << " for " << info.name << ":" << info.address << " with #func=" << actual;
      break;

    default:
      LOG(ERROR) << "Unsupported actual code (" << actual << ") for FP";
      assert(0 == 1);
  }
}

// ==========================================================================================
void RTDBUS_Modbus_server::simulate_DP(const sa_parameter_info_t &info, int actual, uint16_t* data)
{
  common_double_t common_double;
  common_float_t common_float;

  switch (actual) {
    case code_MBUS_TYPE_SUPPORT_HR:
      // Одно значение - один регистр uint16_t
      data[info.address] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
      break;

    case code_MBUS_TYPE_SUPPORT_FHR:
      // Одно значение - два последовательных регистра uint16_t
      common_float.i16_val[0] = data[info.address + 0];
      common_float.i16_val[1] = data[info.address + 1];
      common_float.f_val += (info.high_fis - info.low_fis) / 100.0;
      if (common_float.f_val > info.high_fis)
        common_float.f_val = info.low_fis;

      data[info.address + 0] = common_float.i16_val[0];
      data[info.address + 1] = common_float.i16_val[1];

      LOG(INFO) << "HR simulated val=" << common_float.f_val << " for " << info.name << ":" << info.address << " with #func=" << actual;
      break;

    case code_MBUS_TYPE_SUPPORT_DP:
      // Одно значение - четыре последовательных регистра uint16_t
      common_double.i16_val[0] = data[info.address + 0];
      common_double.i16_val[1] = data[info.address + 1];
      common_double.i16_val[2] = data[info.address + 2];
      common_double.i16_val[3] = data[info.address + 3];
      common_double.g_val += (info.high_fis - info.low_fis) / 100.0;
      if (common_double.g_val > info.high_fis)
        common_double.g_val = info.low_fis;

      data[info.address + 0] = common_double.i16_val[0];
      data[info.address + 1] = common_double.i16_val[1];
      data[info.address + 2] = common_double.i16_val[2];
      data[info.address + 3] = common_double.i16_val[3];

      LOG(INFO) << "DP simulated val=" << common_double.g_val << " for " << info.name << ":" << info.address << " with #func=" << actual;
      break;

    default:
      LOG(ERROR) << "Unsupported actual code (" << actual << ") for DP";
      assert(0 == 1);
  }
}

// ==========================================================================================
// Для каждого параметра из опроса:
// 1. найти место в буфере ответа
// 2. сгенерировать новое значение, основываясь на текущем времени и физическом диапазоне
void RTDBUS_Modbus_server::simulate_values()
{
 // int i = 0;

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

        switch(m_supply_info[info.support].supply)
        {
          case MBUS_TYPE_SUPPORT_HC:
            // используется область памяти mb_mapping.tab_bits
            simulate_HC(info, m_supply_info[info.support].actual, mb_mapping->tab_bits);
            /*
            for (i=0; i < m_UT_BITS_NB; i++) {
                mb_mapping->tab_bits[i] = (uint8_t) (255.0*rand() / (RAND_MAX + 1.0));
                mb_mapping->tab_bits[i] = mb_mapping->tab_bits[i] % 2;
            }
            */
            break;

          case MBUS_TYPE_SUPPORT_IC:
            // используется область памяти mb_mapping.tab_input_bits
            simulate_IC(info, m_supply_info[info.support].actual, mb_mapping->tab_input_bits);
            /*
            for (i=0; i < m_UT_INPUT_BITS_NB; i++) {
                mb_mapping->tab_input_bits[i] = (uint8_t) (255.0*rand() / (RAND_MAX + 1.0));
                mb_mapping->tab_input_bits[i] = mb_mapping->tab_input_bits[i] % 2;
            }
            */
            break;

          case MBUS_TYPE_SUPPORT_HR:
            // используется область памяти mb_mapping.tab_registers
            simulate_HR(info, m_supply_info[info.support].actual, mb_mapping->tab_registers);
            /*
            for (i=0; i < m_UT_REGISTERS_NB; i++) {
                mb_mapping->tab_registers[i] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
            }
            */
            break;

          case MBUS_TYPE_SUPPORT_IR:
          case MBUS_TYPE_SUPPORT_FHR:
            simulate_HR(info, m_supply_info[info.support].actual, mb_mapping->tab_input_registers);
            break;

          case MBUS_TYPE_SUPPORT_FP:
            simulate_FP(info, m_supply_info[info.support].actual, mb_mapping->tab_input_registers);
            break;

          case MBUS_TYPE_SUPPORT_DP:
            simulate_DP(info, m_supply_info[info.support].actual, mb_mapping->tab_input_registers);
            /*
            // используется область памяти mb_mapping.tab_input_registers
            for (i=0; i < m_UT_INPUT_REGISTERS_NB; i++) {
                //mb_mapping->tab_input_registers[i] = ~mb_mapping->tab_input_registers[i];
                mb_mapping->tab_input_registers[i] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
            }
            */
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
  
  delete mbus_server;

  LOG(INFO) << "RTDBUS MODBUS server simulator " << argv[0] << " FINISH";

#ifndef VERBOSE
  ::google::ShutdownGoogleLogging();
#endif

  return rc;
}
