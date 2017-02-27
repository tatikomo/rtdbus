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
#include "msg_common.h"
#include "tool_timer.hpp"
#include "exchange_config.hpp"
#include "exchange_config_sac.hpp"
#include "exchange_smad_int.hpp"
#include "exchange_egsa_cycle.hpp"
#include "exchange_egsa_sa.hpp"
#include "exchange_egsa_impl.hpp"

// -----------------------------------------------------------------------------------
// egsa - доступ к функциям отправки и приёма сообщений
// sa_object_level_t - Место в иерархии
// gof_t_SacNature - Тип объекта
// tag - код системы сбора
SystemAcquisition::SystemAcquisition(EGSA* egsa,
                                     sa_object_level_t level,
                                     gof_t_SacNature nature,
                                     const std::string& tag)
  : m_egsa(egsa),
    m_name(tag),
    m_level(level),
    m_nature(nature),
    m_smad(NULL),
    m_state(SA_STATE_UNKNOWN),
    m_smad_state(STATE_DISCONNECTED),
    m_cycles(NULL),
    m_timer_CONNECT(NULL),
    m_timer_RESPONSE(NULL),
    m_timer_INIT(NULL),
    m_timer_ENDALLINIT(NULL),
    m_timer_GENCONTROL(NULL),
    m_timer_DIFFUSION(NULL),
    m_timer_TELEREGULATION(NULL)
{
  std::string sa_config_filename;
  sa_common_t sa_common;
  AcquisitionSystemConfig* sa_config = NULL;

  // Пропустим первый символ "/" тега
  assert(m_name.find("/") == 0);
  m_name.erase(0, 1);

  sa_config_filename.assign(m_name);

  sa_config_filename += ".json";

  // Определить для указанной СС название файла-снимка SMAD
  sa_config = new AcquisitionSystemConfig(sa_config_filename.c_str());
  if (NOK == sa_config->load_common(sa_common)) {
     LOG(ERROR) << "Unable to parse SA " << m_name << " common config";
  }
  else {
    m_smad = new InternalSMAD(sa_common.name.c_str(), sa_common.nature, sa_common.smad.c_str());

    // TODO: подключаться к InternalSMAD только после успешной инициализации модуля данной СС
    m_state = SA_STATE_DISCONNECTED;
#if 0
    if (STATE_OK != (m_smad_state = m_smad->attach(m_name, m_nature))) {
      LOG(ERROR) << "FAIL attach to '" << m_name << "', file=" << sa_common.smad
                 << ", rc=" << m_smad_state;
    }
    else {
      LOG(INFO) << "OK attach to  '" << m_name << "', file=" << sa_common.smad;
    }
#endif
  }

  m_cycles = look_my_cycles();

  delete sa_config;
}

// -----------------------------------------------------------------------------------
SystemAcquisition::~SystemAcquisition()
{
  LOG(INFO) << "Destructor SA " << m_name;
  delete m_smad;
  delete m_cycles;

  delete m_timer_CONNECT;
  delete m_timer_RESPONSE;
  delete m_timer_INIT;
  delete m_timer_ENDALLINIT;
  delete m_timer_GENCONTROL;
  delete m_timer_DIFFUSION;
  delete m_timer_TELEREGULATION;
}

// -----------------------------------------------------------------------------------
sa_state_t SystemAcquisition::state()
{
  // NB: Напрямую полагаться на чтение состояния СС из внутренней SMAD нельзя, т.к.
  // допустима конфигурация системы, в которой EGSA и СС работают на разных хостах.
  // Во внутренней SMAD интерфейсным модулем заполняются поля:
  // 1. Состояние подключения
  // 2. Время последнего опроса
  // 3. ...
  return m_state;
}

// -----------------------------------------------------------------------------------
int SystemAcquisition::send(int msg_id)
{
  int rc = OK;

  switch(msg_id) {
    case ADG_D_MSG_ENDALLINIT:  process_end_all_init(); break;
    case ADG_D_MSG_ENDINITACK:  process_end_init_acq(); break;
    case ADG_D_MSG_INIT:        process_init();         break;
    case ADG_D_MSG_DIFINIT:     process_dif_init();     break;
    default:
      LOG(ERROR) << "Unsupported message (" << msg_id << ") to send to " << m_name;
      assert(0 == 1);
      rc = NOK;
  }

  return rc;
}

// -----------------------------------------------------------------------------------
// Найти циклы, в которых участвует данная система сбора
std::vector<Cycle*>* SystemAcquisition::look_my_cycles()
{
  std::vector<Cycle*> *cycles = NULL;

  cycles = m_egsa->get_Cycles_for_SA(m_name);

  if (!cycles) {
    LOG(WARNING) << "SA " << m_name << " hasn't any cycles";
  }

  for (std::vector<Cycle*>::const_iterator it = cycles->begin();
       it != cycles->end();
       ++it)
  {
    LOG(INFO) << "local cycle: " << (*it)->name();
  }

  return cycles;
}

// -----------------------------------------------------------------------------------
// TODO: послать сообщение об инициализации каждой системе
void SystemAcquisition::init()
{
  std::string stage_name = "INIT";

//1  m_timer_INIT = new Timer(stage_name);
  LOG(INFO) << "Fire " << stage_name << " for " << m_name;
}

// -----------------------------------------------------------------------------------
// TODO: послать сообщение об успешном завершении инициализации связи
// сразу после того, как последняя из CC успешно ответила на сообщение об инициализации
void SystemAcquisition::process_end_all_init()
{
//1  m_timer_ENDALLINIT = new Timer("ENDALLINIT");
//  m_egsa->send_to(m_name, ADG_D_MSG_ENDALLINIT);
  LOG(INFO) << "Process ENDALLINIT for " << m_name;
}

// -----------------------------------------------------------------------------------
// Запрос состояния завершения инициализации
void SystemAcquisition::process_end_init_acq()
{
  LOG(INFO) << "Process ENDINITACQ for " << m_name;
}

// -----------------------------------------------------------------------------------
// Конец инициализации
void SystemAcquisition::process_init()
{
  LOG(INFO) << "Process INIT for " << m_name;
}

// -----------------------------------------------------------------------------------
// Запрос завершения инициализации после аварийного завершения
void SystemAcquisition::process_dif_init()
{
  LOG(INFO) << "Process DIFINIT for " << m_name;
}

// -----------------------------------------------------------------------------------
// TODO: послать сообщение об завершении связи
void SystemAcquisition::shutdown()
{
//1  m_timer_INIT = new Timer("SHUTDOWN");
  LOG(INFO) << "Process SHUTDOWN for " << m_name;
}

// -----------------------------------------------------------------------------------
int SystemAcquisition::control(int code)
{
  int rc = NOK;

  // TODO: Передать интерфейсному модулю указанную команду
  LOG(INFO) << "Control SA '" << m_name << "' with " << code << " code";
  return rc;
}

// -----------------------------------------------------------------------------------
// Прочитать изменившиеся данные
int SystemAcquisition::pop(sa_parameters_t&)
{
  int rc = NOK;
  LOG(INFO) << "Pop changed data from SA '" << m_name << "'";
  return rc;
}

// -----------------------------------------------------------------------------------
// Передать в СС указанные значения
int SystemAcquisition::push(sa_parameters_t&)
{
  int rc = NOK;
  LOG(INFO) << "Push data to SA '" << m_name << "'";
  return rc;
}

// -----------------------------------------------------------------------------------
int SystemAcquisition::ask_ENDALLINIT()
{
  LOG(INFO) << "CALL SystemAcquisition::ask_ENDALLINIT() for " << m_name;
  return NOK;
}
// -----------------------------------------------------------------------------------
void SystemAcquisition::check_ENDALLINIT()
{
  LOG(INFO) << "CALL SystemAcquisition::check_ENDALLINIT() for " << m_name;
}
