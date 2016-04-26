#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_config_egsa.hpp"
#include "exchange_smad_int.hpp"
#include "exchange_smad_ext.hpp"
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_request_cyclic.hpp"
#include "xdb_common.hpp"

// ==========================================================================================================
EGSA::EGSA()
  : m_ext_smad(NULL),
    m_config(NULL)
{
  m_socket = open("egsa_timers.pipe", S_IFIFO|0666, 0);
  if (-1 == m_socket) {
    LOG(ERROR) << "Unable to open socket: " << strerror(errno);
  }
  else {
    LOG(INFO) << "Timers pipe " << (unsigned int)m_socket << " is ready";
  }
}

// ==========================================================================================================
EGSA::~EGSA()
{
  if (-1 != m_socket) {
    LOG(INFO) << "Timers pipe " << (unsigned int)m_socket << " is closed"; 
  }

  detach();

  delete m_ext_smad;
  delete m_config;

  for (std::vector<Cycle*>::iterator it = ega_ega_odm_ar_Cycles.begin();
       it != ega_ega_odm_ar_Cycles.end();
       it++)
  {
    LOG(INFO) << "free cycle " << (*it)->name();
    delete (*it);
  }
}

// ==========================================================================================================
int EGSA::init()
{
  int rc = NOK;

  // Открыть конфигурацию
  m_config = new EgsaConfig("egsa.json");
  // Прочитать информацию по сайтам и циклам
  m_config->load();

  // Подключиться к своей внутренней памяти SMAD
  m_ext_smad = new ExternalSMAD(m_config->smad_name().c_str());
  smad_connection_state_t ext_state = m_ext_smad->connect();
  LOG(INFO) << "Connect to internal EGSA SMAD code=" << ext_state;
  rc = (STATE_OK == ext_state)? OK : NOK;

  return rc;
}

// ==========================================================================================================
// Подключиться к SMAD систем сбора
int EGSA::attach_to_sites_smad()
{
  // TODO: должен быть список подчиненных систем сбора, взять оттуда
  const char* sa_name = "BI4500";
  int rc = NOK;
  // TEST - подключиться к SMAD для каждой подчиненной системы
  // 2016/04/13 Пока у нас одна система, для опытов
#if 1

  SystemAcquisition *sa_instance = new SystemAcquisition(TYPE_LOCAL_SA, GOF_D_SAC_NATURE_EELE, sa_name);
  m_sa_list.insert(std::pair<std::string, SystemAcquisition*>(sa_name, sa_instance));
  rc = OK;

#else
  InternalSMAD* int_smad = new InternalSMAD(smad_name);
  smad_connection_state_t int_state = int_smad->attach(sa_name, sa_type);
  if (STATE_OK == int_state) {
    m_internal_smad_list.insert(std::pair<std::string, InternalSMAD*>(sa_name, int_smad))
    LOG(INFO) << "Attach to '" << sa_name << "'";
  }
  else {
    LOG(ERROR) << "Fail attach to '" << sa_name << "'";
  }
#endif

  return rc;
}

// ==========================================================================================================
// Изменение состояния подключенных систем сбора и отключение от их внутренней SMAD 
int EGSA::detach()
{
  int rc = NOK;

  // TODO: Для всех подчиненных систем сбора:
  // 1. Изменить их состояние SYNTHSTATE на "ОТКЛЮЧЕНО"
  // 2. Отключиться от их внутренней SMAD
  for (system_acquisition_list_t::iterator it = m_sa_list.begin();
       it != m_sa_list.end();
       it++)
  {
    LOG(INFO) << "TODO: set " << (*it).first << "." << RTDB_ATT_SYNTHSTATE << " = 0";
    LOG(INFO) << "TODO: datach " << (*it).first << " SMAD";
    rc = OK;
  }

  return rc;
}

// ==========================================================================================================
// 1. Дождаться сообщения ADDL о запросе готовности к работе
// 2. Прочитать конфигурационный файл
// 3. Инициализация подписки на атрибуты состояний Систем Сбора
// 4. Ответить на запрос готовности к работе
//    Если инициализация прошла неуспешно - выйти из программы
// 5. Дождаться сообщения ADDL о начале штатной работы
// 6. Цикл до получения сообщения об останове:
// 6.1. Проверять наступление очередного периода цикла опроса
// 6.2. Опрашивать пробуждения (?)
// 6.3. Опрашивать состояния СС
//
int EGSA::run()
{
  volatile int status;

  if (OK == init()) {
    status = attach_to_sites_smad();
  }

  while (OK == status)
  {
    // TODO: пробежаться по всем зарегистрированным системам сбора.
    // Если они в активном состоянии, получить от них даннные
    for (system_acquisition_list_t::iterator it = m_sa_list.begin();
        it != m_sa_list.end();
        it++)
    {
      switch((*it).second->state())
      {
        case SA_STATE_OPER:
          LOG(INFO) << (*it).first << "state is OPERATIVE";

          if (OK == ((*it).second->pop(m_list))) {
            for (sa_parameters_t::iterator it = m_list.begin();
                 it != m_list.end();
                 it++)
            {
              switch ((*it).type)
              {
                case SA_PARAM_TYPE_TM:
                  LOG(INFO) << "name:" << (*it).name << " type:" << (*it).type << " g_val:" << (*it).g_value;
                  break;

                case SA_PARAM_TYPE_TS:
                case SA_PARAM_TYPE_TSA:
                case SA_PARAM_TYPE_AL:
                  LOG(INFO) << "name:" << (*it).name << " type:" << (*it).type << " i_val:" << (*it).i_value;
                  break;

                default:
                  LOG(ERROR) << "name: " << (*it).name << " unsupported type:" << (*it).type;
              }
            }
          }
          else {
            LOG(ERROR) << "Failure pop changed values";
          }
          break;

        case SA_STATE_PRE_OPER:
          LOG(INFO) << (*it).first << "state is PRE OPERATIVE";
          break;

        case SA_STATE_UNREACH:
          LOG(INFO) << (*it).first << " state is UNREACHABLE";
          break;

        case SA_STATE_UNKNOWN:
          LOG(INFO) << (*it).first << " state is UNKNOWN";
          break;

        default:
          LOG(ERROR) << (*it).first << " state unsupported: " << (*it).second->state();
          break;
      }
    }
    sleep (1);
  }

  return status;
}

// ==========================================================================================================
// Ввести в оборот новый Цикл сбора, вернуть новый размер очереди циклов
int EGSA::push_cycle(Cycle* _cycle)
{
  assert(_cycle);
  ega_ega_odm_ar_Cycles.push_back(_cycle);

  return ega_ega_odm_ar_Cycles.size();
}

// ==========================================================================================================
// Активировать циклы
// Активация заключается во взведении таймера Timer с требуемой периодичностью, который по достижению срока
// активации пишет название таймера в ранее объявленный сокет.
// Основной цикл работы приложения включает select из этого сокета, и в случае появления в нем данных
// читается название активированного цикла.
int EGSA::activate_cycles()
{
  int rc = 0;

  for (std::vector<Cycle*>::iterator it = ega_ega_odm_ar_Cycles.begin();
       it != ega_ega_odm_ar_Cycles.end();
       it++)
  {
    rc |= (*it)->activate(m_socket);

    LOG(INFO) << "Activate cycle " << (*it)->name() << ", rc=" << rc;
  }

  return rc;
}

// ==========================================================================================================
// Деактивировать циклы
int EGSA::deactivate_cycles()
{
  int rc = 0;

  for (std::vector<Cycle*>::iterator it = ega_ega_odm_ar_Cycles.begin();
       it != ega_ega_odm_ar_Cycles.end();
       it++)
  {
    rc |= (*it)->deactivate();
    LOG(INFO) << "Deactivate cycle " << (*it)->name() << ", rc=" << rc;
  }

  return rc;
}


// ==========================================================================================================
// Функция срабатывания при наступлении времени очередного таймера
int EGSA::trigger()
{
  LOG(INFO) << "Trigger callback for cycle";
  return OK;
}

// ==========================================================================================================
// Тестовая функция ожидания срабатывания таймеров в течении заданного времени
int EGSA::wait(int max_wait_sec)
{
  fd_set rfds;
  struct timeval tv;
  int retval;
  char buffer[100 + 1];
  ssize_t read_sz;
  ssize_t buffer_sz = 100;

  /* Watch fd[1] to see when it has input. */
  FD_ZERO(&rfds);
  FD_SET(m_socket, &rfds);


  for (int i=0; i<max_wait_sec; i++) {
    /* Wait up to one seconds. */
    tv.tv_sec = 2;
    tv.tv_usec = 1;

    retval = select(m_socket+1, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

    switch(retval) {
      case -1:
        perror("select()");
        break;

      case 0:
        std::cout << "." << std::flush;
        break;

      default:
        read_sz = read(m_socket, buffer, buffer_sz);
        if (read_sz >= 0) buffer[read_sz] = '\0';
        else buffer[0] = '\0';
        LOG(INFO) << "Read " << read_sz << " bytes: " << buffer;
        /* FD_ISSET(0, &rfds) will be true. */
    }
  }
  std::cout << std::endl;

  return OK;
}
// ==========================================================================================================
