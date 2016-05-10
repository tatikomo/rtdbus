///////////////////////////////////////////////////////////////////////////////////////////
// Класс представления системы сбора (СС) для EGSA
// 2016/04/14
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_config_sac.hpp"
#include "exchange_smad_int.hpp"
#include "exchange_egsa_sa.hpp"

// -----------------------------------------------------------------------------------
// sa_object_level_t - Место в иерархии
// gof_t_SacNature - Тип объекта
// name - код системы сбора
SystemAcquisition::SystemAcquisition(sa_object_level_t level, gof_t_SacNature nature, const char* name)
  : m_level(level),
    m_nature(nature),
    m_smad(NULL),
    m_state(SA_STATE_UNKNOWN),
    m_smad_state(STATE_DISCONNECTED)
{
  std::string sa_config_filename = name;
  sa_common_t sa_common;
  AcquisitionSystemConfig* sa_config = NULL;

  m_name = strdup(name);
  sa_config_filename += ".json";

  // Определить для указанной СС название файла-снимка SMAD
  sa_config = new AcquisitionSystemConfig(sa_config_filename.c_str());
  if (NOK == sa_config->load_common(sa_common)) {
     LOG(ERROR) << "Бяка SA " << m_name << " common config";
  }
  else {
    m_smad = new InternalSMAD(sa_common.smad.c_str());

    if (STATE_OK != (m_smad_state = m_smad->attach(m_name, m_nature))) {
      LOG(ERROR) << "FAIL attach to '" << m_name << "', file=" << sa_common.smad;
    }
    else {
      LOG(INFO) << "OK attach to  '" << m_name << "', file=" << sa_common.smad;
    }
  }
}

// -----------------------------------------------------------------------------------
SystemAcquisition::~SystemAcquisition()
{
  LOG(INFO) << "Destructor SA " << m_name;
  free(m_name);
  delete m_smad;
}

// -----------------------------------------------------------------------------------
sa_state_t SystemAcquisition::state()
{
  // TODO: выбрать состояние СС из внутренней SMAD, никакого кеша
  // Во внутренней SMAD интерфейсным модулем заполняются поля:
  // 1. Состояние подключения
  // 2. Время последнего опроса
  // 3. ...
  return m_state;
}

// -----------------------------------------------------------------------------------
int SystemAcquisition::control(int code)
{
  int rc = NOK;

  // TODO: Передать интерфейсному модулю указанную команду
  LOG(INFO) << "Control SA '" << m_name << "' with " << code << " code";
  return rc;
}

// Прочитать изменившиеся данные
int SystemAcquisition::pop(sa_parameters_t&)
{
  int rc = NOK;
  LOG(INFO) << "Pop changed data from SA '" << m_name << "'";
  return rc;
}

// Передать в СС указанные значения
int SystemAcquisition::push(sa_parameters_t&)
{
  int rc = NOK;
  LOG(INFO) << "Push data to SA '" << m_name << "'";
  return rc;
}

