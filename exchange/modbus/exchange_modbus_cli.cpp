#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "modbus.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

// Служебные файлы RTDBUS
#include "exchange_modbus_cli.hpp"
#include "exchange_smad_impl.hpp"

using namespace rapidjson;
using namespace std;

typedef struct { int id; const char* name; } modbus_func_by_type_support_t;
static const modbus_func_by_type_support_t func_by_type_support[] = {
  { MBUS_TYPE_SUPPORT_HC, s_MBUS_TYPE_SUPPORT_HC },
  { MBUS_TYPE_SUPPORT_IC, s_MBUS_TYPE_SUPPORT_IC },
  { MBUS_TYPE_SUPPORT_HR, s_MBUS_TYPE_SUPPORT_HR },
  { MBUS_TYPE_SUPPORT_IR, s_MBUS_TYPE_SUPPORT_IR },
  { MBUS_TYPE_SUPPORT_FSC,s_MBUS_TYPE_SUPPORT_FSC },
  { MBUS_TYPE_SUPPORT_PSR,s_MBUS_TYPE_SUPPORT_PSR },
  { MBUS_TYPE_SUPPORT_FHR,s_MBUS_TYPE_SUPPORT_FHR },
  { MBUS_TYPE_SUPPORT_FP, s_MBUS_TYPE_SUPPORT_FP },
  { MBUS_TYPE_SUPPORT_DP, s_MBUS_TYPE_SUPPORT_DP }
};


// Таблица характеристик протокола MODBUS, параметры штатных (1-5) и нештатных (>103) функций
MBusFuncDesription RTDBUS_Modbus_client::mbusDescr[] = {
//      support
//      |                     originCode
//      |                     |                           actualCode Может меняться, задаётся в конфигурации
//      |                     |                           |                           name
//      |                     |                           |                           |                       maxZaprosLength
//      |                     |                           |                           |                       |     used
//      |                     |                           |                           |                       |     |
/* 0 */{MBUS_TYPE_SUPPORT_HC, code_MBUS_TYPE_SUPPORT_HC,  code_MBUS_TYPE_SUPPORT_HC,  s_MBUS_TYPE_SUPPORT_HC, 2000, false},
/* 1 */{MBUS_TYPE_SUPPORT_IC, code_MBUS_TYPE_SUPPORT_IC,  code_MBUS_TYPE_SUPPORT_IC,  s_MBUS_TYPE_SUPPORT_IC, 2000, false},
/* 2 */{MBUS_TYPE_SUPPORT_HR, code_MBUS_TYPE_SUPPORT_HR,  code_MBUS_TYPE_SUPPORT_HR,  s_MBUS_TYPE_SUPPORT_HR,  125, false},
/* 3 */{MBUS_TYPE_SUPPORT_IR, code_MBUS_TYPE_SUPPORT_IR,  code_MBUS_TYPE_SUPPORT_IR,  s_MBUS_TYPE_SUPPORT_IR,  125, false},
/* 4 */{MBUS_TYPE_SUPPORT_FSC,code_MBUS_TYPE_SUPPORT_FSC, code_MBUS_TYPE_SUPPORT_FSC, s_MBUS_TYPE_SUPPORT_FSC,   0, false},
/* 5 */{MBUS_TYPE_SUPPORT_PSR,code_MBUS_TYPE_SUPPORT_PSR, code_MBUS_TYPE_SUPPORT_PSR, s_MBUS_TYPE_SUPPORT_PSR,   0, false},
/* 6 */{MBUS_TYPE_SUPPORT_FHR,code_MBUS_TYPE_SUPPORT_FHR, code_MBUS_TYPE_SUPPORT_FHR, s_MBUS_TYPE_SUPPORT_FHR, 124, false},
/* 7 */{MBUS_TYPE_SUPPORT_FP, code_MBUS_TYPE_SUPPORT_FP,  code_MBUS_TYPE_SUPPORT_FP,  s_MBUS_TYPE_SUPPORT_FP,   62, false},
/* 8 */{MBUS_TYPE_SUPPORT_DP, code_MBUS_TYPE_SUPPORT_DP,  code_MBUS_TYPE_SUPPORT_DP,  s_MBUS_TYPE_SUPPORT_DP,   31, false}
    };  


// ==========================================================================================
// Аргументы:
// 1) Флаг активации режима отладки
// 2) Название системы сбора
RTDBUS_Modbus_client::RTDBUS_Modbus_client(const char* config)
: m_config(NULL),
  m_config_filename(config),
  m_sa_common(),
  m_ctx(NULL),
  m_header_length(0),
  m_smad(NULL)
{
#if 0
  /* 
   * The only 3 EGSA messages may be present at now:
   * 00 - global control [send accept = 0]
   * 01 - acquisition    [send accept = 0]
   * 13 - initialization [send accept = 1]
   */
  m_pFMessages[0].ReqEGSAAttr.MessageEGSA   = Negsa_glob_control;
  m_pFMessages[0].ReqEGSAAttr.SendAccept    = 0;

  m_pFMessages[1].ReqEGSAAttr.MessageEGSA   = Negsa_acquisition;
  m_pFMessages[1].ReqEGSAAttr.SendAccept    = 0;

  m_pFMessages[2].ReqEGSAAttr.MessageEGSA   = Negsa_initialization;
  m_pFMessages[2].ReqEGSAAttr.SendAccept    = 1;
#endif
}

// ==========================================================================================
RTDBUS_Modbus_client::~RTDBUS_Modbus_client()
{
  // Освободить SMAD, отключившись от него
  delete m_smad;

  // Удалить информацию по запросам
  for (std::vector<ModbusOrderDescription>::iterator it = m_actual_orders_description.begin();
       it != m_actual_orders_description.end();
       it++)
  {
    delete [] (*it).RequestPayload;
  }

  delete m_config;

  modbus_close(m_ctx);
  modbus_free(m_ctx);
}

// ==========================================================================================
int RTDBUS_Modbus_client::connect()
{
  const char* fname = "RTDBUS_Modbus_client::conect";
  const std::vector <sa_network_address_t>& list = m_config->server_list();
  struct timeval response_timeout;
  int rc = 0;

  switch(m_config->channel())
  {
    case SA_MODE_MODBUS_TCP:
      assert(list.size() > 0);
      m_ctx = modbus_new_tcp("127.0.0.1", list[0].port_num);
      //1 m_ctx = modbus_new_tcp("192.168.1.141", 502);
      break;

    case SA_MODE_MODBUS_RTU:
      LOG(ERROR) << fname << ": not yet supported: " << m_config->channel();
      m_ctx = modbus_new_rtu("/dev/ttyUSB0", 115200, 'N', 8, 1);
      break;

    default:
      LOG(ERROR) << fname << ": unsupported mode: " << m_config->channel(); 
  }

  if (modbus_connect(m_ctx) == -1) {
    LOG(ERROR) << "Connection failed: " << modbus_strerror(errno);
    rc = -1;
  }
  else {
    modbus_set_debug(m_ctx, TRUE);

    // Установка таймаута
    response_timeout.tv_sec = m_config->timeout();
    response_timeout.tv_usec = 0;
    modbus_set_response_timeout(m_ctx, response_timeout.tv_sec, response_timeout.tv_usec);

    // Может быть разной длины, для RTU=256, для TCP=260
    m_header_length = modbus_get_header_length(m_ctx);
  }

  return rc;
}

// ==========================================================================================
int RTDBUS_Modbus_client::run()
{
  const char* fname = "run";
  bool stop = false;
  int rc = -1;

  if (0 != (rc = read_config()))
  {
      LOG(ERROR) << fname << ": reading configuration files";
      return rc;
  }

  // Открыть SMAD из указанного снимка
  m_smad = new SMAD(m_config->smad_filename().c_str());

  // Открыть SMAD указанной СС
  if (SMAD::STATE_OK != m_smad->connect(m_config->name().c_str())) {
    LOG(ERROR) << "Unable to continue without SMAD " << m_config->name();
    return -1;
  }

  // Загрузить в неё параметры
  rc = init_smad_parameters();

  // На основе прочитанных данных построить план запросов к СС
  if (make_request_plan()) {
    // Сформировано ненулевое количество запросов - есть смысл поработать
    LOG(INFO) << fname << ": connecting";
    if (0 != (rc = connect()))
    {
        LOG(ERROR) << fname << ": connecting to the server";
        return rc;
    }

    LOG(INFO) << fname << ": processing";

    while (!stop) {

      rc = ask();

      switch(rc) {
          case 0:
            // OK
            sleep (m_config->timeout());
            //stop = true;
            break;

          default:
            LOG(ERROR) << fname << ": result code=" << rc;
            stop = true;
      }
    }
  }
  else {
    LOG(WARNING) << "There are no automatic generated orders, exiting";
  }

  return rc;
}

// ==========================================================================================
int RTDBUS_Modbus_client::ask()
{
  const char* fname = "ask";
  unsigned int idx = 0;
  int nb = 0;
  int nb_fail = 0;
  // Размер одного регистра для чтения. Возможны вариации, поскольку штатной функцией
  // могут собираться параметры с нештатным типом обработки, и потребуется увеличивать
  // количество запрашиваемых регистров. К примеру, 10 параметров типа FP собираются
  // функцией HR, в этом случае запрашивать нужно не 10 регистров, а 20:
  // 10 (регистров) * 2 (штатных uint16 регистров MODBUS в одном регистре FP)
  unsigned int single_register_of_uint16_size = 1;
  // Количество ожидаемых к чтению стандартных MODBUS-регистров UINT16
  int num_uint16_registers_to_read = 1;
  uint8_t rsp_8[2000]; // максимальное количество двоичных переменных в одном запросе
  uint16_t rsp_16[MODBUS_TCP_MAX_ADU_LENGTH + 1];

  for (std::vector<ModbusOrderDescription>::iterator it = m_actual_orders_description.begin();
       it != m_actual_orders_description.end();
       it++)
  {
    LOG(INFO) << "processing #" << ++idx << "/" << m_actual_orders_description.size()
              << " order, function:" << (unsigned int) (*it).NumberModbusFunction
              << " slave:" << (unsigned int) (*it).IdModbusServer
              << " direction:" << (unsigned int) (*it).Direction
              << " start:" << (unsigned int) (*it).StartAddress
              // QuantityRegisters есть количество регистров в понимании типа обработки,
              // а не в понимании протокола MODBUS (uint16_t)
              << " quantity:" << (unsigned int) (*it).QuantityRegisters
              << " bytes:" << (unsigned int) (*it).SizeRequestedPayload;

    if (-1 == modbus_set_slave(m_ctx, (*it).IdModbusServer))
    {
        LOG(WARNING) << "Setting slave identity to " << (unsigned int) (*it).IdModbusServer;
    }

    // NB: Возможна ситуация, когда штатной функцией MODBUS можгут обрабатываться регистры
    // с нестандартным типом обработки - FHR, FP, DP. В этом случае следует скорректировать
    // значение количества запрашиваемых регистров.
    // Определим особый режим - штатная функция используется нештатным образом
    switch ((*it).SupportType) {
      case MBUS_TYPE_SUPPORT_FHR:
      case MBUS_TYPE_SUPPORT_FP:
        single_register_of_uint16_size = 2; // один параметр состоит из двух uint16
        break;

      case MBUS_TYPE_SUPPORT_DP:
        single_register_of_uint16_size = 4; // один параметр состоит из четырёх uint16
        break;

      default:
        single_register_of_uint16_size = 1;
    }

    num_uint16_registers_to_read = (*it).QuantityRegisters * single_register_of_uint16_size;
 
    switch((*it).NumberModbusFunction) {
      case code_MBUS_TYPE_SUPPORT_HC:   // 1
        do {
          nb = modbus_read_bits(m_ctx,
                               (*it).StartAddress,
                               (*it).QuantityRegisters,
                               rsp_8);
          if (-1 == nb) {
            LOG(ERROR) << "Unable to process request, #fails=" << ++nb_fail;
          }
        } while ((-1 == nb) && (nb_fail < m_config->error_nb()));

        break;

      case code_MBUS_TYPE_SUPPORT_IC:   // 2
        do {
          nb = modbus_read_input_bits(m_ctx,
                               (*it).StartAddress,
                               (*it).QuantityRegisters,
                               rsp_8);
          if (-1 == nb) {
            LOG(ERROR) << "Unable to process request, #fails=" << ++nb_fail;
          }
        } while ((-1 == nb) && (nb_fail < m_config->error_nb()));
        break;

      case code_MBUS_TYPE_SUPPORT_HR:   // 3
        do {
          nb = modbus_read_registers(m_ctx,
                               (*it).StartAddress,
                               num_uint16_registers_to_read, //1 (*it).QuantityRegisters,
                               rsp_16);
          if (-1 == nb) {
            LOG(ERROR) << "Unable to process request, #fails=" << ++nb_fail;
          }
        } while ((-1 == nb) && (nb_fail < m_config->error_nb()));
        break;

      case code_MBUS_TYPE_SUPPORT_IR:   // 4
        do {
          nb = modbus_read_input_registers(m_ctx,
                               (*it).StartAddress,
                               num_uint16_registers_to_read, //1 (*it).QuantityRegisters,
                               rsp_16);
          if (-1 == nb) {
            LOG(ERROR) << "Unable to process request, #fails=" << ++nb_fail;
          }
        } while ((-1 == nb) && (nb_fail < m_config->error_nb()));
        break;

      // Нештатная функция - используем ранее сфорированный буфер запроса
      case code_MBUS_TYPE_SUPPORT_FHR:  // 103
      case code_MBUS_TYPE_SUPPORT_FP:   // 104
      case code_MBUS_TYPE_SUPPORT_DP:   // 105
        do {
          // Поскольку функция нестандартная, посылаем "сырой" заранее сформированный запрос
          nb = modbus_send_raw_request(m_ctx,
                             (*it).RequestPayload,
                             6 * sizeof(uint8_t));
          if (-1 == nb) {
            LOG(ERROR) << "Unable to send request, #fails=" << ++nb_fail;
          }
          else {
            // Попытаемся получить ответ на запрос
            // Признак "хорошего" ответа - ненулевой код ответа и верное количество прочитанных байт
            nb = modbus_receive_confirmation(m_ctx, rsp_8);
            if (-1 == nb) {
              LOG(ERROR) << "Unable to receive response, #fails=" << ++nb_fail;
            }
          }
        }
        while ((-1 == nb) && (nb_fail < m_config->error_nb()));
        // TODO: Возможно переполнение буфера? Тогда следует читать в буфер максимально
        // допустимой длины MODBUS_TCP_MAX_ADU_LENGTH, а затем скопировать из него нужную часть:
        assert(nb <= (*it).SizeRequestedPayload);
        break;
    }

    if (nb != num_uint16_registers_to_read)
    {
      LOG(ERROR) << fname << ": modbus_read_registers, read=" << nb
                 << ", #await=" << num_uint16_registers_to_read << " #fails=" << nb_fail;
      // при обнаружении обрыва связи установить качество параметров в "недостоверно"
      set_validity(0);
    }
    else
    {
      LOG(INFO) << "Success reading " << (unsigned int) num_uint16_registers_to_read
                << " register(s) of " << (unsigned int) (*it).SizeRequestedPayload << " bytes"
                << " from address " << (unsigned int) (*it).StartAddress
                << ", #uint16=" << nb;
      parse_response((*it), rsp_8, rsp_16);
      nb_fail = 0;
    }
  }

  return nb_fail;
}

// ==========================================================================================
// TODO: Реализация изменения достоверностей всех собираемых параметров на указанное значение
void RTDBUS_Modbus_client::set_validity(int new_validity)
{
  LOG(INFO) << "set new validity to " << new_validity;
}

// ==========================================================================================
// Разбор буфера ответа от СС на уровне байтов
// TODO: проверить, как работает с включённым "substract = 1"
int RTDBUS_Modbus_client::parse_response(ModbusOrderDescription& handler, uint8_t* rsp_8, uint16_t* rsp_16)
{
  int status = 0;
  address_map_t *address_map = NULL;

  switch(handler.SupportType) {
    case MBUS_TYPE_SUPPORT_HC:
      address_map = &m_HC_map;
      status = parse_HC_IC(address_map, handler, rsp_8);
      break;
    case MBUS_TYPE_SUPPORT_IC:
      address_map = &m_IC_map;
      status = parse_HC_IC(address_map, handler, rsp_8);
      break;
    case MBUS_TYPE_SUPPORT_HR:
      address_map = &m_HR_map;
      status = parse_HR_IR(address_map, handler, rsp_16);
      break;
    case MBUS_TYPE_SUPPORT_IR:
      address_map = &m_IR_map;
      status = parse_HR_IR(address_map, handler, rsp_16);
      break;
    case MBUS_TYPE_SUPPORT_FHR:
      address_map = &m_FHR_map;
      status = parse_FHR(address_map, handler, rsp_16);
      break;
    case MBUS_TYPE_SUPPORT_FP:
      address_map = &m_FP_map;
      status = parse_FP(address_map, handler, rsp_16);
      break;
    case MBUS_TYPE_SUPPORT_DP:
      address_map = &m_DP_map;
      status = parse_DP(address_map, handler, rsp_16);
      break;
    default:
      LOG(ERROR) << "Invalid support type:" << handler.SupportType
                 << " for function " << handler.NumberModbusFunction;
      assert (0 == 1);
  }
/*
  switch(handler.NumberModbusFunction) {
    case code_MBUS_TYPE_SUPPORT_HC:
    case code_MBUS_TYPE_SUPPORT_IC:
      status = parse_HC_IC(address_map, handler, rsp_8);
      break;

    case code_MBUS_TYPE_SUPPORT_HR:
    case code_MBUS_TYPE_SUPPORT_IR:
      status = parse_HR_IR(address_map, handler, rsp_16);
      break;

    case code_MBUS_TYPE_SUPPORT_FHR:  // 103
      status = parse_FHR(address_map, handler, rsp_16);
      break;

    case code_MBUS_TYPE_SUPPORT_FP:   // 104
      status = parse_FP(address_map, handler, rsp_16);
      break;

    case code_MBUS_TYPE_SUPPORT_DP:   // 105
      status = parse_DP(address_map, handler, rsp_16);
      break;

    default:
      LOG(ERROR) << "Invalid support type:" << handler.SupportType
                 << " for function " << handler.NumberModbusFunction;
      break;
  } */

  return status;
}

// ==========================================================================================
// Разобрать буфер ответа на основе типа обработки HR и IR.
// Особенностью обработки полученных данных является то, что они представляют из себя смещения
// по шкале инженерного диапазона от 0 до 65535, и для получения физической величины требуется
// провести их нормализацию на основе физического диапазона и смещения по инженерной шкале.
int RTDBUS_Modbus_client::parse_HR_IR(address_map_t* address_map, ModbusOrderDescription& handler, uint16_t* data)
{
  address_map_t::iterator it;
  float          FZnach = USHRT_MAX;// значение регистра, полученное после интерпретации диапазонов
  unsigned short UZnach;
  short          SZnach;
  int status = -1;

  for (int i = 0; i < handler.QuantityRegisters; i++)
  {
    // Выводить информацию только по точкам из конфигурации, пропуская "окна" в принятом наборе регистров
    if ((it = address_map->find(i+handler.StartAddress)) != address_map->end())
    {
      sa_parameter_info_t& item = (*it).second;

      // Определим, может ли значение быть числом со знаком (и >0, и <0)?
      if ((item.low_fan < 0) || (item.high_fan < 0)) {
        SZnach = data[i];
        FZnach = ((float)SZnach * (float)((item.high_fis - item.low_fis) / (item.high_fan - item.low_fan))) + item.low_fis;
        item.g_value = FZnach * item.factor;
#if (VERBOSE > 8)
        printf("S 0x%04X (raw=%f zoom=%f)\n", SZnach, FZnach, item.g_value);
#endif
      }
      else {
        UZnach = data[i];
		FZnach = ((float)UZnach * (float)((item.high_fis - item.low_fis) / (item.high_fan - item.low_fan))) + item.low_fis;
        item.g_value = FZnach * item.factor;
#if (VERBOSE > 8)
        printf("U 0x%04X (raw=%f zoom=%f)\n", UZnach, FZnach, item.g_value);
#endif
      }

      item.rtdbus_timestamp = time(0);

      if ((FZnach == USHRT_MAX) || (FZnach > item.high_fis) || (FZnach < item.low_fis))
      {
        item.valid = 0; //1 GOF_D_ACQ_INVALID;
        LOG(WARNING) << "["<< func_by_type_support[handler.SupportType].name
                     << " as " << (unsigned int) handler.NumberModbusFunction << "] "
                     << item.name
                     << ": Force change validity to invalid (val=" << item.g_value
                     << " out of bounds [" << item.low_fis << ".." << item.high_fis << "])";
      }
      else {
        item.valid = 1; //1 GOF_D_ACQ_VALID;
        LOG(INFO) << "["<< func_by_type_support[handler.SupportType].name
                  << " as " << (unsigned int) handler.NumberModbusFunction << "] "
                  << item.name
                  << ", addr=" << item.address << ", data=" << (unsigned int)data[i]
                  << ", val=" << item.g_value
                  << ", valid=" << item.valid;
      } // конец проверки, что значение не вышло за допустимый диапазон

      // Занести значение в SMAD
      m_smad->push(item);

    } // конец блока обработки значения, присутствующего в конфигурации
  } // конец цикла по всем параметрам из конфигурации

  return status;
}

// ==========================================================================================
// Разобрать буфер ответа на основе типа обработки HC и IC.
int RTDBUS_Modbus_client::parse_HC_IC(address_map_t* address_map, ModbusOrderDescription& handler, uint8_t* data)
{
  address_map_t::iterator it;
  int status = -1;

  for (int i = 0; i < handler.QuantityRegisters; i++)
  {
    // Выводить информацию только по точкам из конфигурации, пропуская "окна" в принятом наборе регистров
    if ((it = address_map->find(i+handler.StartAddress)) != address_map->end())
    {
      sa_parameter_info_t& item = (*it).second;

      LOG(INFO) << "["<< func_by_type_support[handler.SupportType].name
                << " as " << (unsigned int) handler.NumberModbusFunction << "] "
                << (*it).second.name
                << ", address=" << (*it).second.address << ", " << (unsigned int)data[i];

      item.rtdbus_timestamp = time(0);
      item.i_value = (unsigned int)data[i];

      // TODO: получить достоверность дискретов
      item.valid = 1; //1 test

      // Занести значение в SMAD
      m_smad->push(item);
    }
  }

  return status;
}

// ==========================================================================================
// Разобрать буфер ответа на основе типа обработки FHR
// Особенностью обработки полученных данных является то, что физическое значение параметра из
// 32-х бит передаётся в виде двух последовательных 16-ти битовых регистров. Таким образом,
// для получения значения необходимо интерпретировать две последовательно расположенные ячейки
// uint16_t как одну переменную uint32_t, и преобразовать её в тип float.
int RTDBUS_Modbus_client::parse_FHR(address_map_t*, ModbusOrderDescription&, uint16_t*)
{
  int status = -1;

  return status;
}

// ==========================================================================================
// Разобрать буфер ответа на основе типа обработки FP
int RTDBUS_Modbus_client::parse_FP(address_map_t* address_map, ModbusOrderDescription& handler, uint16_t* data)
{
  address_map_t::iterator it;
  int status = -1;
  int register_idx = 0;
  union { float f_value; uint16_t i_values[2]; } common;

  for (int i = 0; i < handler.QuantityRegisters; i++)
  {
    // Выводить информацию только по точкам из конфигурации, пропуская "окна" в принятом наборе регистров
    if ((it = address_map->find(i+handler.StartAddress)) != address_map->end())
    {
      sa_parameter_info_t& item = (*it).second;
      // Один регистр FP есть два регистра uint16 MODBUS
      common.i_values[0] =  (unsigned int)data[register_idx++];
      common.i_values[1] =  (unsigned int)data[register_idx++];

      item.rtdbus_timestamp = time(0);
      item.g_value = common.f_value;

      // TODO: получить достоверность дискретов
      item.valid = 1; //1 test

      LOG(INFO) << "["<< func_by_type_support[handler.SupportType].name
                << " as " << (unsigned int) handler.NumberModbusFunction << "] "
                << item.name
                << ", address=" << item.address << ", " << item.g_value
                << " ([0]=" << common.i_values[0] <<" [1]=" << common.i_values[1] << ")"
                << ", idx=" << register_idx;

      // Занести значение в SMAD
      m_smad->push(item);
    }
  }

  return status;
}

// ==========================================================================================
// Разобрать буфер ответа на основе типа обработки DP
int RTDBUS_Modbus_client::parse_DP(address_map_t*, ModbusOrderDescription&, uint16_t*)
{
  return -1;
}

// ==========================================================================================
// Завершить формированое запроса
void RTDBUS_Modbus_client::polish_order(int support_type, int begin_register, ModbusOrderDescription& order)
{
  // По умолчанию один регистр нашего протокола соответствует одному регистру протокола modbus
  int single_register_size_in_uint16 = 1;
  union { uint8_t a_byte[2]; uint16_t a_word; } toto;
  order.QuantityRegisters = begin_register - order.StartAddress + 1;
  // Для FHR нужно чётное количество регистров, добавим при необходимости до след. чётного числа
  if (order.NumberModbusFunction == code_MBUS_TYPE_SUPPORT_FHR) {
      if (order.QuantityRegisters % 2)
        order.QuantityRegisters++;
  }
  order.SupportType = support_type;

  // Подсчитать ожидаемый размер буфера ответа
  switch(support_type) {
    case MBUS_TYPE_SUPPORT_HC:
    case MBUS_TYPE_SUPPORT_IC:
      order.SizeRequestedPayload = (order.QuantityRegisters - 1)/8+1;
      // Для функции modbus_read_input_bits(), которой собираются эти данные,
      // требуется резервировать в буфере на одну переменную один uint8_t как минимум.
      break;
    case MBUS_TYPE_SUPPORT_FHR:
      single_register_size_in_uint16 = 2;
      // NB: break пропущен специально
    case MBUS_TYPE_SUPPORT_HR:
    case MBUS_TYPE_SUPPORT_IR:
      order.SizeRequestedPayload = order.QuantityRegisters * 2;
      break;
    case MBUS_TYPE_SUPPORT_FP:
      single_register_size_in_uint16 = 2;
      order.SizeRequestedPayload = order.QuantityRegisters * 4;
      break;
    case MBUS_TYPE_SUPPORT_DP:
      single_register_size_in_uint16 = 4;
      order.SizeRequestedPayload = order.QuantityRegisters * 8;
      break;
  }

  // Уменьшим значение начального адреса, если СС считает адреса с 1, а не с 0, и если указан флаг SUB
  if ((m_config->address_subtraction()) && (order.StartAddress)) {
    order.StartAddress--;
  }

  // NB: Протокол MODBUS оперирует регистрами uint16, однако для функций FP и DP
  // требуются регистры размером uint32 и unit64 соответственно.
  // Потому для работы с имитатором, использующим то же самое API, придётся
  // вручную корректировать QuantityRegisters для того, чтобы имитатор отвечал
  // именно таким количеством байт, сколько ожидается получить.
  if ((MBUS_TYPE_SUPPORT_DP == support_type)
    ||(MBUS_TYPE_SUPPORT_FP == support_type)
    ||(MBUS_TYPE_SUPPORT_FHR == support_type))
  {
      // Выделим место под буфер запроса, поскольку используются нестандартные функции
      // и потребуется посылать данные в "сыром" формате.
      LOG(INFO) << "polish_order for '" << func_by_type_support[support_type].name
                << "' as " << (unsigned int) order.NumberModbusFunction
                << ", SRS=" <<single_register_size_in_uint16
                << ", QR=" << order.QuantityRegisters;
      order.RequestPayload = new uint8_t [LENSEND];
      order.RequestPayload[0] = order.IdModbusServer;
      order.RequestPayload[1] = order.NumberModbusFunction;
      toto.a_word = order.StartAddress;
      order.RequestPayload[2] = toto.a_byte[0];
      order.RequestPayload[3] = toto.a_byte[1];
      toto.a_word = single_register_size_in_uint16 * order.QuantityRegisters;
      order.RequestPayload[4] = toto.a_byte[0];
      order.RequestPayload[5] = toto.a_byte[1];
  }
  else order.RequestPayload = NULL;
}

// ==========================================================================================
// На основе прочитанных данных построить план запросов к СС
int RTDBUS_Modbus_client::make_request_plan()
{
  const char* fname = "make_request_plan";

  LOG(INFO) << fname << ": START";
  
  mbusDescr[MBUS_TYPE_SUPPORT_HC].actualCode = m_config->modbus_specific().actual_HC_FUNCTION;
  mbusDescr[MBUS_TYPE_SUPPORT_IC].actualCode = m_config->modbus_specific().actual_IC_FUNCTION;
  mbusDescr[MBUS_TYPE_SUPPORT_HR].actualCode = m_config->modbus_specific().actual_HR_FUNCTION;
  mbusDescr[MBUS_TYPE_SUPPORT_IR].actualCode = m_config->modbus_specific().actual_IR_FUNCTION;
  mbusDescr[MBUS_TYPE_SUPPORT_FHR].actualCode= m_config->modbus_specific().actual_FHR_FUNCTION;
  mbusDescr[MBUS_TYPE_SUPPORT_FP].actualCode = m_config->modbus_specific().actual_FP_FUNCTION;
  mbusDescr[MBUS_TYPE_SUPPORT_DP].actualCode = m_config->modbus_specific().actual_DP_FUNCTION;

  for(int typesup = MBUS_TYPE_SUPPORT_HC; typesup < MBUS_TYPE_SUPPORT_UNK; typesup++)
  {
    if (mbusDescr[typesup].used)
      LOG(INFO) << m_config->name() << " use TYPE_SUPPORT=" << func_by_type_support[typesup].name
                << " [" << typesup << "]";
  }

  int result = calculate();
  
  LOG(INFO) << fname << ": FINISH, #cycles=" << result;

  return result;
}

// ==========================================================================================
int RTDBUS_Modbus_client::read_config()
{
  int status = 0;

  m_config = new ExchangeConfig(m_config_filename);

  if (-1 == (status = m_config->load())) {
    LOG(ERROR) << "бяка config";
  }

  return status;
}

// ==========================================================================================
// На основе ранее прочитанных из конфигурационного файла списков параметров
// создать с именем, соответствующим коду системы сбора, таблицу в SMAD,
// и заполнить её списком параметров. Параллельно собрать такую же информацию
// в виде сортированного по адресу дерева для каждого из типов обработки.
int RTDBUS_Modbus_client::init_smad_parameters() // загрузка параметров обмена
{
  int status = 0;
  // Значения атрибутов прочитанных из конфигурации параметров
//  int include;              // Признак необходимости передачи параметра в БДРВ
  std::string s_type;       // Символьное значения типа параметра
  std::string name;         // Тег БДРВ
  std::string type_support; // Тип обработки параметра

  // Установить ускоренный режим работы журнала транзакций
  m_smad->accelerate(true);

  int i = 0;
  for (sa_parameters_t::iterator itr = m_config->acquisitions().begin();
       itr != m_config->acquisitions().end();
       itr++)
  {
    LOG(INFO) << "Acquisition item " << ++i;
    const sa_parameter_info_t &info = (*itr);

    if (0 == (status = m_smad->setup_parameter(info))) {
      // Запомнить реально собираемые регистры в соответствующих наборах,
      // для последующей идентификации действительных данных в содержимом
      // полученного от сервера буфере
      switch(info.support) {
        case MBUS_TYPE_SUPPORT_HC: m_HC_map.insert(std::pair<int, sa_parameter_info_t>(info.address, info)); break;
        case MBUS_TYPE_SUPPORT_IC: m_IC_map.insert(std::pair<int, sa_parameter_info_t>(info.address, info)); break;
        case MBUS_TYPE_SUPPORT_HR: m_HR_map.insert(std::pair<int, sa_parameter_info_t>(info.address, info)); break;
        case MBUS_TYPE_SUPPORT_IR: m_IR_map.insert(std::pair<int, sa_parameter_info_t>(info.address, info)); break;
        case MBUS_TYPE_SUPPORT_FHR:m_FHR_map.insert(std::pair<int,sa_parameter_info_t>(info.address, info)); break;
        case MBUS_TYPE_SUPPORT_FP: m_FP_map.insert(std::pair<int, sa_parameter_info_t>(info.address, info)); break;
        case MBUS_TYPE_SUPPORT_DP: m_DP_map.insert(std::pair<int, sa_parameter_info_t>(info.address, info)); break;
        default:
          LOG(ERROR) << "Unsupported code: " << info.support;
          assert(0 == 1);
      }
    }

    // чтобы не выбирать потом поиском по базе используемые типы обработки
    // в make_request_plan(), соберем их здесь
    if ((MBUS_TYPE_SUPPORT_HC <= info.support) && (info.support < MBUS_TYPE_SUPPORT_UNK))
      mbusDescr[info.support].used = true;
    else
      LOG(ERROR) << "given typesupport=" << info.support
                 << " exceeds limit:" << MBUS_TYPE_SUPPORT_HC << ".." << MBUS_TYPE_SUPPORT_UNK;
  } // Конец перебора всех собираемых от системы сбора параметров

  // Вернуть режим работы журнала транзакций в норму
  m_smad->accelerate(false);

  return status;
}


// ==========================================================================================
int RTDBUS_Modbus_client::load_commands() // загрузка команд управления
{
  return 0;
}

// ==========================================================================================
int RTDBUS_Modbus_client::calculate()
{
  // Состояние текущего запроса: начали заполнять, закончили заполнять.
  // Нужно для обнаружения факта неоконченного формирования запроса в случае, если запрашиваемых
  // регистров меньше, чем максимально допускает спецификация MODBUS для одного запроса.
  enum order_state_t { BEGIN = 0, END = 1 };
  order_state_t order_state;
  const char* fname = "calculate";
  std::string name;
  int type;
  int current_address;
  int previous_address; // предыдущий прочитанный адрес
  // Количество полезных регистров в запросе, без учёта пропусков
  int useful_registers_in_order;
  address_map_t *address_map = NULL;
  ModbusOrderDescription order;

  for (int support_type = MBUS_TYPE_SUPPORT_HC; support_type < MBUS_TYPE_SUPPORT_UNK; support_type++)
  {
    if (mbusDescr[support_type].used)
    {
      LOG(INFO) << fname << ": Process function " << mbusDescr[support_type].actualCode
                << ", type " << mbusDescr[support_type].name;

      memset(&order, '\0', sizeof(ModbusOrderDescription));
      order_state = BEGIN;

      switch(support_type) {
        case MBUS_TYPE_SUPPORT_HC:  address_map = &m_HC_map;  break;
        case MBUS_TYPE_SUPPORT_IC:  address_map = &m_IC_map;  break;
        case MBUS_TYPE_SUPPORT_HR:  address_map = &m_HR_map;  break;
        case MBUS_TYPE_SUPPORT_IR:  address_map = &m_IR_map;  break;
        case MBUS_TYPE_SUPPORT_FHR: address_map = &m_FHR_map; break;
        case MBUS_TYPE_SUPPORT_FP:  address_map = &m_FP_map;  break;
        case MBUS_TYPE_SUPPORT_DP:  address_map = &m_DP_map;  break;
        default: assert (0 == 1);
      }

      // Общее количество запрашиваемых регистров обнулим
      order.QuantityRegisters = 0;
      // Обнулим количество реальных регистров, требуемых конфигурационным файлом
      useful_registers_in_order = 0;

      // В порядке возрастания адресов информацию по параметрам данного типа обработки 
      for (address_map_t::iterator it = address_map->begin();
           it !=  address_map->end();
           it++)
      {
          // Не читать новые данные, если они остались от предыдущего запроса в связи с его переполнением
          if (order_state != END) {
              name = (*it).second.name;
              current_address = (*it).second.address;
              type = (*it).second.type;
          }
          else it--; // вернуться к предыдущему параметру, оставив значения name и current_address прежними
#if (VERBOSE >=7)
          LOG(INFO) << fname << " :" << m_config->name() << " " << name << "\t"
                    << m_actual_orders_description.size()+1 << " ADDR:"
                    << current_address << " TYPE:" << type << " OT:" << order_state;
#endif

          // Выполнить действия по инициализации полей текущего запроса
          if (!useful_registers_in_order) {
            // Сначала заполнили код используемой MODBUS-функции запроса
            order.NumberModbusFunction = mbusDescr[support_type].actualCode;
            order.IdModbusServer = m_config->modbus_specific().slave_idx;
            // TODO: доработать возможность диффузии данных от клиента серверу
            order.Direction = MBUS_FLOW_ACQUISITION;
            // order.StartAddress: Начальный адрес регистра текущего запроса
            // previous_address: Инициализировали предыдущий адрес текущего запроса, он пока равен текущему адресу
            order.StartAddress = previous_address = current_address;
            order_state = BEGIN;
          }

          // Проверить, заполнился ли уже полностью один запрос?
          if ((current_address - order.StartAddress) < mbusDescr[support_type].maxZaprosLength) {
            // Пока нет, еще есть свободное место
            useful_registers_in_order++;
            previous_address = current_address;
#if (VERBOSE >=9)
            LOG(INFO) << "insert new register " << current_address << " into request " << m_actual_orders_description.size()+1;
#endif
          }
          else {
            // Заполнили текущий запрос, сохраним его, и создадим очередной
            // NB: current_address должен принадлежать следующему запросу, т.к. он превысил допустимый размер пакета
            polish_order(support_type, previous_address, order);

            LOG(INFO) << "Order " << m_actual_orders_description.size()+1 << " type=" << support_type << " is full,"
                      << " start=" << order.StartAddress
                      << " stop=" << previous_address
                      << " #registers=" << order.QuantityRegisters
                      << " (actual:" << useful_registers_in_order << ")"
                      << " wait_bytes=" << order.SizeRequestedPayload
                      << " limit=" << mbusDescr[support_type].maxZaprosLength;

            m_actual_orders_description.push_back(order);
            useful_registers_in_order = 0;
            order_state = END;
            memset(&order, '\0', sizeof(ModbusOrderDescription));
          }
      }

      // Проверить возможность неполного заполнения запроса - данные в SQL-выборке закончились,
      // а ордер не внесен в список активных, поскольку пакет ещё не заполнен.
      if (order_state != END) {
          // Обнаружен не до конца сформированный запрос
          polish_order(support_type, current_address, order);

          LOG(INFO) << "Close order " << m_actual_orders_description.size()+1 << ", OT:" << order_state
                    << " start=" << order.StartAddress
                    << " stop=" << current_address
                    << " #registers=" << order.QuantityRegisters
                    << " (actual:" << useful_registers_in_order << ")"
                    << " wait_bytes=" << order.SizeRequestedPayload
                    << " limit=" << mbusDescr[support_type].maxZaprosLength;

          m_actual_orders_description.push_back(order);
          // Завершить
          order_state = END;
      }

    } // конец обработки текущего используемого типа

  } // Конец поочерёдной проверки всех типов обработки

  int idx = 0;
  for (std::vector<ModbusOrderDescription>::iterator it = m_actual_orders_description.begin();
       it != m_actual_orders_description.end();
       it++)
  {
      LOG(INFO) << "#" << ++idx << "/" << m_actual_orders_description.size()
                << " order, function:" << (unsigned int) (*it).NumberModbusFunction
                << " slave:" << (unsigned int) (*it).IdModbusServer
                << " direction:" << (unsigned int) (*it).Direction
                << " start:" << (unsigned int) (*it).StartAddress
                << " quantity:" << (unsigned int) (*it).QuantityRegisters;
  }

  return m_actual_orders_description.size();
}

// ==========================================================================================
int main(int argc, char* argv[])
{
  RTDBUS_Modbus_client *mbus_client = NULL;
  // Название конфигурационного файла локальной тестовой СС
  char config_filename[2000 + 1] = "BI4500.json";
  int  opt;

#ifndef VERBOSE
  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();
#endif

  LOG(INFO) << "RTDBUS MODBUS client " << argv[0] << " START";

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

  mbus_client = new RTDBUS_Modbus_client(config_filename);

  if (0 == mbus_client->run())
    LOG(INFO) << "DONE";
  else
    LOG(ERROR) << "FAIL";

  delete mbus_client;

  LOG(INFO) << "RTDBUS MODBUS client " << argv[0] << " FINISH";

#ifndef VERBOSE
  ::google::ShutdownGoogleLogging();
#endif

  return 0;
}

