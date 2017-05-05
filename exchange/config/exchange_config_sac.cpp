#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

// Служебные файлы RTDBUS
#include "exchange_config_sac.hpp"
#include "exchange_config_nature.hpp"

using namespace rapidjson;

// ==========================================================================================
AcquisitionSystemConfig::AcquisitionSystemConfig(const std::string& config)
 : m_config_filename(config),
   m_config_has_good_format(false),
   m_sa_commands(),
   m_sa_common({0, 0, 0, SA_BYTEORDER_ABCD, GOF_D_SAC_NATURE_EUNK, 0, "", "", SA_MODE_UNKNOWN, 0}),
   m_protocol()
{
  const char* fname = "AcquisitionSystemConfig()";
  FILE* f_params = NULL;
  struct stat configfile_info;

  m_sa_common.timeout = 3;      // Таймаут ожидания ответа в секундах
  m_sa_common.repeat_nb = 1;    // Количество последовательных попыток передачи данных при сбое
  m_sa_common.error_nb = 2;     // Количество последовательных ошибок связи до диагностики разрыва связи
  m_sa_common.byte_order = SA_BYTEORDER_ABCD;   // Порядок байт СС
  m_sa_common.nature = GOF_D_SAC_NATURE_EUNK;   // Неизвестный тип СС
  m_sa_common.subtract = 0;     // Признак необходимости вычитания единицы из адреса
  m_sa_common.channel = SA_MODE_UNKNOWN;      // Тип канала доступа к серверу, MODBUS-{TCP|RTU}, OPC-UA
  m_sa_common.trace_level = 1;

  /*
  m_rtu_info.dev_name[0] = '\0';
  m_rtu_info.parity = 'N';
  m_rtu_info.speed = 9600;
  m_rtu_info.flow_control = 1;
  m_rtu_info.nbit = 8;
  m_rtu_info.start_stop = 1;
  */

  m_sa_parameters_input.clear();
  m_sa_parameters_output.clear();

  // Выделить буфер readBuffer размером с читаемый файл
  if (-1 == stat(m_config_filename.c_str(), &configfile_info)) {
    LOG(ERROR) << fname << ": Unable to stat() '" << m_config_filename << "': " << strerror(errno);
  }
  else {
    if (NULL != (f_params = fopen(m_config_filename.c_str(), "r"))) {
      // Файл открылся успешно, прочитаем содержимое
      LOG(INFO) << fname << ": size of '" << m_config_filename << "' is " << configfile_info.st_size;
      char *readBuffer = new char[configfile_info.st_size + 1];

      FileReadStream is(f_params, readBuffer, configfile_info.st_size + 1);
      m_document.ParseStream(is);
      delete []readBuffer;

      assert(m_document.IsObject());

      if (!m_document.HasParseError())
      {
        // Конфиг-файл имеет корректную структуру
        m_config_has_good_format = true;
      }
      else {
        LOG(ERROR) << fname << ": Parsing " << m_config_filename;
      }

      fclose (f_params);
    } // Конец успешного чтения содержимого файла
    else {
      LOG(FATAL) << fname << ": Locating config file " << m_config_filename
                 << " (" << strerror(errno) << ")";
    }
  } // Конец успешной проверки размера файла
}

// ==========================================================================================
AcquisitionSystemConfig::~AcquisitionSystemConfig()
{
}

// ==========================================================================================
// загрузка всех параметров обмена
int AcquisitionSystemConfig::load()
{
  const char* fname = "load";
  int status = NOK;

  do {

    if (false == m_config_has_good_format) {
      LOG(WARNING) << fname << ": bad or missing config file " << m_config_filename;
      break;
    }

    // загрузка основной конфигурации: разделы COMMON, CONFIG, SERVERS
    status = load_common(m_sa_common);
    if (NOK == status) {
      LOG(ERROR) << fname << ": Get control configuration";
      break;
    }

    switch(channel()) {
      case SA_MODE_MODBUS_TCP:
      case SA_MODE_MODBUS_RTU:
        status = load_modbus(m_protocol);
        break;
      case SA_MODE_OPC_UA:
        status = load_opc(m_protocol);
        break;
      case SA_MODE_UNKNOWN:
        status = NOK;
        LOG(ERROR) << "Unsupported channel type: " << channel();
    }

    // загрузка параметров обмена: разделы PARAMETERS:ACQUISITION, PARAMETERS:TRANSMISSION
    status = load_parameters(&m_sa_parameters_input, &m_sa_parameters_output);
    if (NOK == status) {
      LOG(ERROR) << fname << ": Get in/out parameters";
      break;
    }

    // загрузка команд управления: раздел PARAMETERS:COMMAND
    status = load_commands(m_sa_commands);
    if (NOK == status) {
      LOG(ERROR) << fname << ": Get command parameters";
      break;
    }

    status = OK;

  } while(false);

  return status;
}

// ==========================================================================================
// загрузка параметров обмена
// in - параметры к приему от внешней системы
// out - параметры к передаче в адрес внешней системы
int AcquisitionSystemConfig::load_parameters(sa_parameters_t* in, sa_parameters_t* out)
{
  static const char *fname = "load_parameters";
  bool test_constrains_correct;
  int status = OK;
  // Значения атрибутов прочитанных из конфигурации параметров
  int include;              // Признак необходимости передачи параметра в БДРВ
  std::string s_type;       // Символьное значения типа параметра
  std::string name;         // Тег БДРВ
  std::string type_support; // Тип обработки параметра
  sa_parameter_info_t info;   // Описание текущего параметра

  if (false == m_config_has_good_format) {
    LOG(WARNING) << fname << ": bad or missing config file " << m_config_filename;
    return NOK;
  }

  assert(m_document.HasMember(s_PARAMETERS));
  if (in) in->clear();
  if (out) out->clear();

  // Получение списка опрашиваемых параметров
  Value& acquisition = m_document[s_PARAMETERS][s_ACQUISITION];
  assert(acquisition.IsArray());

  LOG(INFO) << "Number of acquisition points is " << acquisition.Size();

  // Если объект для хранения результата не пуст - заполним его
  if (NULL != in)
  {
    int i = 0;
    for (Value::ValueIterator itr = acquisition.Begin(); itr != acquisition.End(); ++itr)
    {
      LOG(INFO) << "Acquisition item " << ++i;
      // Обнулим читаемые переменные
      memset(&info, '\0', sizeof(info));
      type_support.clear();
      name.clear();
      s_type.clear();

      // Получили доступ к очередному элементу acquisition
      const Value::Object& acq_item = itr->GetObject();

      // Проверили присутствие обязательных полей
      assert(acq_item.HasMember(s_PARAM_NAME));
      assert(acq_item.HasMember(s_PARAM_INCLUDE));
      assert(acq_item.HasMember(s_PARAM_TYPE));
      // Получили значения обязательных полей
      name = acq_item[s_PARAM_NAME].GetString();
      include = acq_item[s_PARAM_INCLUDE].GetInt();
      s_type = acq_item[s_PARAM_TYPE].GetString();

      LOG(INFO) << "name:" << name << " include:" << include << " type:" << s_type;

      strncpy(info.name, name.c_str(), sizeof(wchar_t)*TAG_NAME_MAXLEN);
      info.name[sizeof(wchar_t)*TAG_NAME_MAXLEN] = '\0';

      if (0 == s_type.compare(s_TM)) {
        info.type = SA_PARAM_TYPE_TM;
        // NB: LOW_FAN и HIGH_FAN востребованы только для типа обработки HR и IR
        if (acq_item.HasMember(s_PARAM_LOW_FAN)) {
          info.low_fan = acq_item[s_PARAM_LOW_FAN].GetInt();
        }
        if (acq_item.HasMember(s_PARAM_HIGH_FAN)) {
          info.high_fan = acq_item[s_PARAM_HIGH_FAN].GetInt();
        }
        if (acq_item.HasMember(s_PARAM_LOW_FIS)) {
          info.low_fis = acq_item[s_PARAM_LOW_FIS].GetDouble();
        }
        if (acq_item.HasMember(s_PARAM_HIGH_FIS)) {
          info.high_fis = acq_item[s_PARAM_HIGH_FIS].GetDouble();
        }
        LOG(INFO) << s_PARAM_LOW_FAN << ":" << info.low_fan
                  << " " << s_PARAM_HIGH_FAN << ":" << info.high_fan
                  << " " << s_PARAM_LOW_FIS << ":" << info.low_fis
                  << " " << s_PARAM_HIGH_FIS << ":" << info.high_fis;

        // Прочитать коэффициент пропорциональности
        if (acq_item.HasMember(s_PARAM_FACTOR)) {
          info.factor = acq_item[s_PARAM_FACTOR].GetDouble();
        }
      }
      else if (0 == s_type.compare(s_TS)) {
        info.type = SA_PARAM_TYPE_TS;
      }
      else if (0 == s_type.compare(s_TSA)) {
        info.type = SA_PARAM_TYPE_TSA;
      }
      else if (0 == s_type.compare(s_AL)) {
        info.type = SA_PARAM_TYPE_AL;
      }
      else {
        info.type = SA_PARAM_TYPE_UNK;
        LOG(ERROR) << fname << ": Unsupported " << name << " type: " << info.type;
      }

      if (info.type != SA_PARAM_TYPE_UNK) {
        // Для параметров данных типов может присутствовать параметр ADDR

        if (include) { // Этот параметр попадает в БДРВ

            assert(acq_item.HasMember(s_PARAM_TYPESUP));
            // Прочтём обязательный параметр "способ обработки"
            if (acq_item.HasMember(s_PARAM_TYPESUP)) {
              type_support = acq_item[s_PARAM_TYPESUP].GetString();
            }

            assert(acq_item.HasMember(s_PARAM_ADDR));
            // Прочтём обязательный MODBUS-адрес параметра
            if (acq_item.HasMember(s_PARAM_ADDR)) {
              info.address = acq_item[s_PARAM_ADDR].GetInt();
            }

            if (0 == type_support.compare(s_MBUS_TYPE_SUPPORT_HC)) {
                info.support = MBUS_TYPE_SUPPORT_HC;
            }
            else if (0 == type_support.compare(s_MBUS_TYPE_SUPPORT_IC)) {
                info.support = MBUS_TYPE_SUPPORT_IC;
            }
            else if (0 == type_support.compare(s_MBUS_TYPE_SUPPORT_HR)) {
                info.support = MBUS_TYPE_SUPPORT_HR;
            }
            else if (0 == type_support.compare(s_MBUS_TYPE_SUPPORT_IR)) {
                info.support = MBUS_TYPE_SUPPORT_IR;
            }
            else if (0 == type_support.compare(s_MBUS_TYPE_SUPPORT_FHR)) {
                info.support = MBUS_TYPE_SUPPORT_FHR;
            }
            else if (0 == type_support.compare(s_MBUS_TYPE_SUPPORT_FP)) {
                info.support = MBUS_TYPE_SUPPORT_FP;
            }
            else if (0 == type_support.compare(s_MBUS_TYPE_SUPPORT_DP)) {
                info.support = MBUS_TYPE_SUPPORT_DP;
            }
            else {
                LOG(ERROR) << fname << ": unknown support type: " << type_support
                           << " for parameter " << name;
            }

            LOG(INFO) << "address: " << info.address << ", support type: "
                      << type_support << " (" << info.support << ")";

            // Выполнить проверки возможности указанных ограничений для параметра.
            // Если проверки не прошли, игнорируем этот параметр.
            test_constrains_correct = check_constrains(info);

            if (true == test_constrains_correct) {
              // Поместить входной параметр в соответствующий список
              in->push_back(info);
            }
            else {
              LOG(WARNING) << "skip parameter " << info.name;
            }

        } // Конец блока "параметр попадает в SMAD"
      } // Конец блока "тип параметра успешно определился"

    } // Конец перебора всех собираемых от системы сбора параметров
  } // Конец проверки, необходимости сбора информации по входным параметрам

  if (out) {
    // Получение списка параметров для передачи серверу
    Value& transmission = m_document[s_PARAMETERS][s_TRANSMISSION];
    assert(transmission.IsArray());
  }

  return status;
}

// ==========================================================================================
// Выполнить проверки возможности указанных ограничений для параметра.
bool AcquisitionSystemConfig::check_constrains(const sa_parameter_info_t& info)
{
  bool status = true;

  // Проверка 1. для HR и IR границы диапазонов не должны совпадать
  if ((MBUS_TYPE_SUPPORT_HR == info.support) || (MBUS_TYPE_SUPPORT_IR == info.support)) {
     if ((info.low_fis == info.high_fis) || (info.low_fan == info.high_fan)) {
       status = false;
       LOG(WARNING) << "HR-IR: edges of intervals must not be equivalent for " << info.name;
     }
  }

  return status;
}

// ==========================================================================================
// загрузка команд управления: раздел PARAMETERS:COMMAND
int AcquisitionSystemConfig::load_commands(sa_commands_t&)
{
//  static const char *fname = "load_commands";
  int status = OK;

  return status;
}

// ==========================================================================================
// Загрузка основной конфигурации из разделов COMMON, CONFIG, SERVERS
int AcquisitionSystemConfig::load_modbus(sa_protocol_t& protocol)
{
  const char* fname = "load_modbus";
  int status = NOK;

  if (false == m_config_has_good_format) {
    LOG(WARNING) << fname << ": bad or missing config file " << m_config_filename;
    return NOK;
  }

  do {
    if (!m_document.HasMember(s_MODBUS)) {
      LOG(ERROR) << "Unable to find MODBUS-specific part in " << m_config_filename;
      break;
    }

    // Получение общих данных по конфигурации протокола MODBUS
    Value& modbus_part = m_document[s_MODBUS];

    if (!modbus_part.IsObject()) {
      LOG(ERROR) << "Unable to load MODBUS-specific part from " << m_config_filename;
      break;
    }

    // Начальные значения
    protocol.mbus.dubious_value = 0x7FFFF;
    protocol.mbus.actual_HC_FUNCTION = code_MBUS_TYPE_SUPPORT_HC;
    protocol.mbus.actual_IC_FUNCTION = code_MBUS_TYPE_SUPPORT_IC;
    protocol.mbus.actual_HR_FUNCTION = code_MBUS_TYPE_SUPPORT_HR;
    protocol.mbus.actual_IR_FUNCTION = code_MBUS_TYPE_SUPPORT_IR;
    protocol.mbus.actual_FSC_FUNCTION = code_MBUS_TYPE_SUPPORT_FSC;
    protocol.mbus.actual_PSR_FUNCTION = code_MBUS_TYPE_SUPPORT_PSR;
    protocol.mbus.actual_FHR_FUNCTION = code_MBUS_TYPE_SUPPORT_FHR;
    protocol.mbus.actual_FP_FUNCTION = code_MBUS_TYPE_SUPPORT_FP;
    protocol.mbus.actual_DP_FUNCTION = code_MBUS_TYPE_SUPPORT_DP;

    // Действительные значения
    if (modbus_part.HasMember(s_MODBUS_HC_FUNCTION))
      protocol.mbus.actual_HC_FUNCTION = modbus_part[s_MODBUS_HC_FUNCTION].GetInt();
    if (modbus_part.HasMember(s_MODBUS_IC_FUNCTION))
      protocol.mbus.actual_IC_FUNCTION = modbus_part[s_MODBUS_IC_FUNCTION].GetInt();
    if (modbus_part.HasMember(s_MODBUS_HR_FUNCTION))
      protocol.mbus.actual_HR_FUNCTION = modbus_part[s_MODBUS_HR_FUNCTION].GetInt();
    if (modbus_part.HasMember(s_MODBUS_IR_FUNCTION))
      protocol.mbus.actual_IR_FUNCTION = modbus_part[s_MODBUS_IR_FUNCTION].GetInt();
    if (modbus_part.HasMember(s_MODBUS_FSC_FUNCTION))
      protocol.mbus.actual_FSC_FUNCTION = modbus_part[s_MODBUS_FSC_FUNCTION].GetInt();
    if (modbus_part.HasMember(s_MODBUS_PSR_FUNCTION))
      protocol.mbus.actual_PSR_FUNCTION = modbus_part[s_MODBUS_PSR_FUNCTION].GetInt();
    if (modbus_part.HasMember(s_MODBUS_FHR_FUNCTION))
      protocol.mbus.actual_FHR_FUNCTION = modbus_part[s_MODBUS_FHR_FUNCTION].GetInt();
    if (modbus_part.HasMember(s_MODBUS_FP_FUNCTION))
      protocol.mbus.actual_FP_FUNCTION = modbus_part[s_MODBUS_FP_FUNCTION].GetInt();
    if (modbus_part.HasMember(s_MODBUS_DP_FUNCTION))
      protocol.mbus.actual_DP_FUNCTION = modbus_part[s_MODBUS_DP_FUNCTION].GetInt();
    if (modbus_part.HasMember(s_MODBUS_DELEGATION_COIL_ADDRESS))
      protocol.mbus.delegation_coil_address = modbus_part[s_MODBUS_DELEGATION_COIL_ADDRESS].GetInt();
    if (modbus_part.HasMember(s_MODBUS_DELEGATION_2DIPL_WRITE))
      protocol.mbus.delegation_2dipl_write = modbus_part[s_MODBUS_DELEGATION_2DIPL_WRITE].GetInt();
    if (modbus_part.HasMember(s_MODBUS_DELEGATION_2SCC_WRITE))
      protocol.mbus.delegation_2scc_write = modbus_part[s_MODBUS_DELEGATION_2SCC_WRITE].GetInt();
    if (modbus_part.HasMember(s_MODBUS_TELECOMAND_LATCH_ADDRESS))
      protocol.mbus.telecomand_latch_address = modbus_part[s_MODBUS_TELECOMAND_LATCH_ADDRESS].GetInt();
    if (modbus_part.HasMember(s_MODBUS_VALIDITY_OFFSET))
      protocol.mbus.validity_offset = modbus_part[s_MODBUS_VALIDITY_OFFSET].GetInt();
    if (modbus_part.HasMember(s_MODBUS_DUBIOUS_VALUE))
      protocol.mbus.dubious_value = modbus_part[s_MODBUS_DUBIOUS_VALUE].GetDouble();
    if (modbus_part.HasMember(s_MODBUS_SLAVE_IDX))
      protocol.mbus.slave_idx = modbus_part[s_MODBUS_SLAVE_IDX].GetInt();
    else protocol.mbus.slave_idx = MODBUS_TCP_SLAVE;

    LOG(INFO) << "MBUS: SLAVE=" << protocol.mbus.slave_idx
            << " HC=" << protocol.mbus.actual_HC_FUNCTION
            << " IC=" << protocol.mbus.actual_IC_FUNCTION
            << " HR=" << protocol.mbus.actual_HR_FUNCTION
            << " IR=" << protocol.mbus.actual_IR_FUNCTION
            << " FSC="<< protocol.mbus.actual_FSC_FUNCTION
            << " PSR="<< protocol.mbus.actual_PSR_FUNCTION
            << " FHR=" << protocol.mbus.actual_FHR_FUNCTION
            << " FP=" << protocol.mbus.actual_FP_FUNCTION
            << " DP=" << protocol.mbus.actual_DP_FUNCTION
            << " D_C_A=" << protocol.mbus.delegation_coil_address
            << " D_2D_W=" << protocol.mbus.delegation_2dipl_write
            << " D_2S_W=" << protocol.mbus.delegation_2scc_write
            << " T_L_A=" << protocol.mbus.telecomand_latch_address
            << " V_O=" << protocol.mbus.validity_offset
            << " D_V=" << protocol.mbus.dubious_value;

    status = OK;

  } while (false);

  return status;
}

// ==========================================================================================
// Загрузка основной конфигурации из разделов COMMON, CONFIG, SERVERS
int AcquisitionSystemConfig::load_opc(sa_protocol_t& protocol)
{
  const char* fname = "load_opc";
  int status = NOK;

  if (false == m_config_has_good_format) {
    LOG(WARNING) << fname << ": bad or missing config file " << m_config_filename;
    return NOK;
  }

  do {
    if (!m_document.HasMember(s_OPC)) {
      LOG(ERROR) << "Unable to find OPC-specific part in " << m_config_filename;
      break;
    }

    // Получение общих данных по конфигурации протокола MODBUS
    Value& opc_part = m_document[s_OPC];

    if (!opc_part.IsObject()) {
      LOG(ERROR) << "Unable to load OPC-specific part from " << m_config_filename;
      break;
    }

    protocol.opc.lala = 0;

    status = OK;

  } while (false);

  return status;
}

// ==========================================================================================
// Загрузка основной конфигурации из разделов COMMON, CONFIG, SERVERS
int AcquisitionSystemConfig::load_common(sa_common_t& common)
{
  const char* fname = "load_common";
  sa_network_address_t server_addr;
  sa_rtu_info_t rtu_info;
  std::string buffer;
  int status = OK;
  int server_idx = 0;

  if (false == m_config_has_good_format) {
    LOG(WARNING) << fname << ": bad or missing config file " << m_config_filename;
    return NOK;
  }

  assert(m_document.HasMember(s_COMMON));
  assert(m_document.HasMember(s_SERVICE));

  // Получение общих данных по конфигурации интерфейсного модуля RTDBUS Системы Сбора
  Value& section = m_document[s_COMMON];
  assert(section.IsObject());
  common.timeout = section[s_COMMON_TIMEOUT].GetInt();
  common.repeat_nb = section[s_COMMON_REPEAT_NB].GetInt();
  common.error_nb = section[s_COMMON_ERROR_NB].GetInt();
  buffer = section[s_COMMON_BYTE_ORDER].GetString();
  if (0 == buffer.compare(s_COMMON_BYTE_ORDER_DCBA))
    common.byte_order = SA_BYTEORDER_DCBA;
  else if (0 == buffer.compare(s_COMMON_BYTE_ORDER_ABCD))
    common.byte_order = SA_BYTEORDER_ABCD;
  else {
    LOG(WARNING) << "Unsupported endianess value: " << buffer << ", will use ABCD";
    common.byte_order = SA_BYTEORDER_ABCD;
  }

  if (section.HasMember(s_COMMON_NATURE)) {
    // TODO: process SA NATURE
    const std::string& config_nature = section[s_COMMON_NATURE].GetString();
    common.nature = SacNature::get_natureid_by_name(config_nature);
    LOG(INFO) << "GEV: Process SA nature " << config_nature << ", id=" << common.nature;
  }

  common.subtract = section[s_COMMON_SUB].GetInt();

  common.name.assign(section[s_COMMON_NAME].GetString());
  if (!common.name.empty() && common.name[0] == '/') {
    LOG(WARNING) << "Unsupported SA code: '" << common.name << "', trim leading symbol";
    // Удалить лидирующий символ '/'
    common.name.erase(0, 1);
  }
  else {
    LOG(INFO) << "SA code " << common.name << " is OK";
  }

  common.smad.assign(section[s_COMMON_SMAD].GetString());

  buffer = section[s_COMMON_CHANNEL].GetString();
  if (0 == buffer.compare(s_SA_MODE_MODBUS_TCP))
    common.channel = SA_MODE_MODBUS_TCP;
  else if (0 == buffer.compare(s_SA_MODE_MODBUS_RTU))
    common.channel = SA_MODE_MODBUS_RTU;
  else if (0 == buffer.compare(s_SA_MODE_OPC_UA))
    common.channel = SA_MODE_OPC_UA;
  else {
    common.channel = SA_MODE_UNKNOWN;
    LOG(FATAL) << "Server mode is unknown: " << buffer;
    status = NOK;
  }

  if (section.HasMember(s_COMMON_TRACE))
    common.trace_level = section[s_COMMON_TRACE].GetInt();

  // Получение общих данных по конфигурации сервера Системы Сбора
  Value& service = m_document[s_SERVICE];
  if (service.IsArray()) {
    for (Value::ValueIterator itr = service.Begin(); itr != service.End(); ++itr)
    {
      // Получили доступ к очередному элементу service
      const Value::Object& srv_item = itr->GetObject();

      switch(common.channel) {
        case SA_MODE_MODBUS_TCP:
          // Режим работы - по сети
          if (srv_item.HasMember(s_SERVICE_HOST)) {
            const std::string srv_name = srv_item[s_SERVICE_HOST].GetString();

            if (srv_item.HasMember(s_SERVICE_PORT)) {
              const std::string srv_port = srv_item[s_SERVICE_PORT].GetString();
              LOG(INFO) << (unsigned int)server_idx+1 << "/" << service.Size()
                        << " host:" << srv_name << " port:" << srv_port;

              strncpy(server_addr.host_name, srv_name.c_str(), 20);
              strncpy(server_addr.port_name, srv_port.c_str(), 20);

              m_server_address.push_back(server_addr);
            }

            server_idx++;
          }
          else {
              LOG(ERROR) << (unsigned int)server_idx+1 << "/" << service.Size()
                         << "Missing server name field:" << s_SERVICE_HOST;
          }
          break;

        case SA_MODE_MODBUS_RTU:
          // Режим работы - асинхронный канал связи
          // Установим значения по умолчанию
          strcpy(rtu_info.dev_name, "/dev/ttyS0");
          rtu_info.parity = 'N';
          rtu_info.speed = 9600;
          rtu_info.flow_control = SOFTWARE;
          rtu_info.nbit = 8;
          rtu_info.start_stop = 1;

          if (srv_item.HasMember(s_SERVICE_DEVICE)) {
            strncpy(rtu_info.dev_name, srv_item[s_SERVICE_DEVICE].GetString(), MAX_DEVICE_FILENAME_LENGTH);
            rtu_info.dev_name[MAX_DEVICE_FILENAME_LENGTH] = '\0';
          }
          if (srv_item.HasMember(s_SERVICE_SPEED)) {
            rtu_info.speed = srv_item[s_SERVICE_SPEED].GetInt();
          }
          if (srv_item.HasMember(s_SERVICE_NBIT)) {
            rtu_info.nbit = srv_item[s_SERVICE_NBIT].GetInt();
          }
          if (srv_item.HasMember(s_SERVICE_FLOW)) {
            const std::string f_c = srv_item[s_SERVICE_FLOW].GetString();
            if (0 == f_c.compare(s_SERVICE_FLOW_SOFTWARE))
              rtu_info.flow_control = SOFTWARE;
            else if (0 == f_c.compare(s_SERVICE_FLOW_HARDWARE))
              rtu_info.flow_control = HARDWARE;
            else {
              LOG(ERROR) << "Unsupported '" << s_SERVICE_FLOW << "' value: " << f_c
                         << ", will use " << s_SERVICE_FLOW_SOFTWARE;
            }
          }
          if (srv_item.HasMember(s_SERVICE_PARITY)) {
            const std::string parity = srv_item[s_SERVICE_PARITY].GetString();
            if (0 == parity.compare(s_SERVICE_PARITY_N))
              rtu_info.parity = 'N';
            else if (0 == parity.compare(s_SERVICE_PARITY_E))
              rtu_info.parity = 'E';
            else if (0 == parity.compare(s_SERVICE_PARITY_O))
              rtu_info.parity = 'O';
            else {
              LOG(ERROR) << "Unsupported '" << s_SERVICE_PARITY << "' value: '" << parity
                         << "', will use '" << s_SERVICE_PARITY_N << "'";
            }
          }
          m_rtu_list.push_back(rtu_info);
          break;

        default:
          LOG(ERROR) << "Section '" << s_SERVICE << "': unsupported channel " << common.channel;
      }
    }
  }
  else {
    LOG(ERROR) << "Unsupported SERVICE section format";
    status = NOK;
  }

  LOG(INFO) << "COMMON " << common.name
            << ": channel=" << common.channel
            << " timeout=" << common.timeout
            << " #repeats=" << common.repeat_nb
            << " #errors=" << common.error_nb
            << " nature=" << common.nature
            << " byte_order=" << common.byte_order
            << " channel=" << common.channel
            << " subtract=" <<  common.subtract
            << " trace=" << common.trace_level;

  return status;
}

