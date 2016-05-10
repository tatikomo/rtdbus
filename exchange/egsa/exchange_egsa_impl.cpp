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
#include "mdp_zmsg.hpp"
#include "mdp_worker_api.hpp"
#include "msg_common.h"     // константы запросов SIG_D_MSG_*
#include "exchange_config.hpp"
#include "exchange_config_egsa.hpp"
#include "exchange_smad_int.hpp"
#include "exchange_smad_ext.hpp"
#include "exchange_egsa_sa.hpp"
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_request_cyclic.hpp"
#include "xdb_common.hpp"

extern int interrupt_worker;

volatile bool EGSA::m_interrupt  = false;

// ==========================================================================================================
EGSA::EGSA(zmq::context_t& _ctx)
  : m_context(_ctx),
    m_frontend(_ctx, ZMQ_ROUTER),
    m_message_factory(NULL),
    m_ext_smad(NULL),
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

  // Активировать группу подписки
  rc = (STATE_OK == ext_state)? OK : NOK;

  return rc;
}

// ==========================================================================================================
// Подключиться к SMAD систем сбора
int EGSA::attach_to_sites_smad()
{
  // Список подчиненных систем сбора
  egsa_config_sites_t& sites = m_config->sites();
  int rc = NOK;
  // TEST - подключиться к SMAD для каждой подчиненной системы
#if 1

  // По списку известных нам систем сбора создать интерфейсы к их SMAD
  for (egsa_config_sites_t::iterator it = sites.begin();
       it != sites.end();
       it++)
  {
    const std::string& sa_name = (*it).second->name;
    int raw_nature = (*it).second->nature;
    gof_t_SacNature sa_nature = GOF_D_SAC_NATURE_EUNK;
    if ((GOF_D_SAC_NATURE_DIR >= raw_nature) && (raw_nature <= GOF_D_SAC_NATURE_EUNK)) {
      sa_nature = static_cast<gof_t_SacNature>(raw_nature);
    }

    int raw_level = (*it).second->level;
    sa_object_level_t sa_level = LEVEL_UNKNOWN;
    if ((LEVEL_UNKNOWN >= raw_level) && (raw_level <= LEVEL_UPPER)) {
      sa_level = static_cast<sa_object_level_t>(raw_level);
    }

    SystemAcquisition *sa_instance = new SystemAcquisition(sa_level, sa_nature, sa_name.c_str());
    m_sa_list.insert(std::pair<std::string, SystemAcquisition*>(sa_name, sa_instance));
  }

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
  const char* fname = "run()";
  volatile int status;
  int mandatory = 1;
  int linger = 0;
  int hwm = 100;
  int send_timeout_msec = SEND_TIMEOUT_MSEC; // 1 sec
  int recv_timeout_msec = RECV_TIMEOUT_MSEC; // 3 sec
  zmq::pollitem_t  socket_items[2];
  mdp::zmsg *request = NULL;

  if (OK == init()) {
    status = attach_to_sites_smad();
  }

  LOG(INFO) << fname << ": START";
  try
  {
    // Сокет прямого подключения, выполняется в отдельной нити 
    // ZMQ_ROUTER_MANDATORY может привести zmq_proxy_steerable к аномальному завершению: rc=-1, errno=113
    // Наблюдалось в случаях интенсивного обмена с клиентом, если тот аномально завершался.
    m_frontend.setsockopt(ZMQ_ROUTER_MANDATORY, &mandatory, sizeof (mandatory));
    m_frontend.bind("tcp://lo:5559" /*ENDPOINT_EXCHG_FRONTEND*/);
    m_frontend.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    m_frontend.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
    m_frontend.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
    m_frontend.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    m_frontend.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

    // Сокет для получения запросов по истории
    socket_items[0].socket = (void*)m_frontend;
    socket_items[0].fd = 0;
    socket_items[0].events = ZMQ_POLLIN;
    socket_items[0].revents = 0;

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
  }
  catch(zmq::error_t error)
  {
      LOG(ERROR) << fname << ": transport error catch: " << error.what();
  }
  catch (std::exception &e)
  {
      LOG(ERROR) << fname << ": runtime signal catch: " << e.what();
  }

  m_frontend.close();

  // Ресурсы очищаются в локальной нити, деструктору ничего не достанется...
  delete m_message_factory;

  LOG(INFO) << fname << ": STOP";

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
    if (interrupt_worker) {
      LOG(INFO) << "EGSA::wait breaks";
      break;
    }

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
        buffer[(read_sz >= 0)? read_sz : 0] = '\0';
        LOG(INFO) << "Read " << read_sz << " bytes: " << buffer;
        /* FD_ISSET(0, &rfds) will be true. */
    }
  }
  std::cout << std::endl;

  return OK;
}

// ==========================================================================================================
// Функция первично обрабатывает полученный запрос.
// Обслуживаемые запросы перечислены в egsa.mkd
//
// NB: Запросы обрабатываются последовательно.
int EGSA::processing(mdp::zmsg* request, std::string &identity)
{
  rtdbMsgType msgType;
  int rc = OK;

//  LOG(INFO) << "Process new request with " << request->parts() 
//            << " parts and reply to " << identity;

  // Получить отметку времени начала обработки запроса
  // m_metric_center->before();

  msg::Letter *letter = m_message_factory->create(request);
  if (letter->valid())
  {
    msgType = letter->header()->usr_msg_type();

    switch(msgType)
    {
      // Запрос на телерегулирование
      case SIG_D_MSG_ECHCTLPRESS:
      case SIG_D_MSG_ECHDIRCTLPRESS:
        rc = handle_teleregulation(letter, &identity);
        break;

      default:
        LOG(ERROR) << "Unsupported request type: " << msgType;
    }
  }
  else
  {
    LOG(ERROR) << "Received letter "<<letter->header()->exchange_id()<<" not valid";
  }

  if (NOK == rc) {
    LOG(ERROR) << "Failure processing request: " << msgType;
  }

  delete letter;

  // Получить отметку времени завершения обработки запроса
  // m_metric_center->after();

  return rc;
}

// ==========================================================================================================
// Останов экземпляра
int EGSA::stop()
{
  LOG(INFO) << "Get request to stop component";
  interrupt_worker = 1;
}

// ==========================================================================================================
int EGSA::handle_teleregulation(msg::Letter*, std::string*)
{
  return NOK;
}

// ==========================================================================================================
