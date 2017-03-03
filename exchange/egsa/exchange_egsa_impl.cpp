#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>     // for 'usleep'

#include <iostream>
#include <functional>
#include <queue>
#include <chrono>
#include <thread>
#include <sys/time.h>   // for 'time_t' and 'struct timeval'

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "tool_events.hpp"
#include "mdp_zmsg.hpp"
#include "mdp_worker_api.hpp"
#include "msg_common.h"     // константы запросов SIG_D_MSG_*
// сообщения общего порядка
#include "msg_adm.hpp"
// сообщения по работе с БДРВ
#include "msg_sinf.hpp"
#include "exchange_config.hpp"
#include "exchange_config_egsa.hpp"
#include "exchange_smad_int.hpp"
#include "exchange_smad_ext.hpp"
#include "exchange_egsa_sa.hpp"
#include "exchange_egsa_site.hpp"
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_request_cyclic.hpp"
#include "xdb_common.hpp"

extern volatile int interrupt_worker;

// ==========================================================================================================
EGSA::EGSA(const std::string& _broker, const std::string& _service)
  : mdp::mdwrk(_broker, _service, 1 /* num threads */, true /* use  direct connection */),
    m_signal_socket(m_context, ZMQ_ROUTER),  // Сообщения от таймеров циклов
    m_message_factory(new msg::MessageFactory(EXCHANGE_NAME)),
    m_backend_socket(m_context, ZMQ_DEALER), // Сокет обмена сообщениями с Интерфейсом второго уровня
    m_ext_smad(NULL),
    m_egsa_config(NULL),
    m_socket(-1) // TODO: удалить, планируется использовать events вместо таймеров
{
/*  m_socket = open("egsa_timers.pipe", S_IFIFO|0666, 0);
  if (-1 == m_socket) {
    LOG(ERROR) << "Unable to open socket: " << strerror(errno);
  }
  else {
    LOG(INFO) << "Timers pipe " << (unsigned int)m_socket << " is ready";
  }*/
}

// ==========================================================================================================
EGSA::~EGSA()
{
  if (-1 != m_socket) {
    LOG(INFO) << "Timers pipe " << (unsigned int)m_socket << " is closed"; 
  }

  detach();

  delete m_ext_smad;
  delete m_egsa_config;
  delete m_message_factory;

  for (std::vector<Cycle*>::iterator it = m_ega_ega_odm_ar_Cycles.begin();
       it != m_ega_ega_odm_ar_Cycles.end();
       ++it)
  {
    LOG(INFO) << "free cycle " << (*it)->name();
    delete (*it);
  }
}

// ==========================================================================================================
int EGSA::init()
{
  const char* fname = "EGSA::init"; 
  int status = NOK;
  int linger = 0;
  int hwm = 100;
  int send_timeout_msec = SEND_TIMEOUT_MSEC; // 1 sec
  int recv_timeout_msec = RECV_TIMEOUT_MSEC; // 3 sec
  smad_connection_state_t ext_state;

  // Инициализировать массивы Сайтов и Циклов
  init_sites();

  // Подключиться к своей внутренней памяти SMAD
  m_ext_smad = new ExternalSMAD(m_egsa_config->smad_name().c_str());

  if (STATE_OK == (ext_state = m_ext_smad->connect())) {
    // Активировать группу подписки
    status = activateSBS();

    if (OK == status) {
      try {
#warning "GEV inproc socket, what first: connect() or bind()"
        m_backend_socket.connect(ENDPOINT_EXCHG_FRONTEND);
        m_backend_socket.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
        m_backend_socket.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
        m_backend_socket.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));
        LOG(INFO) << fname << ": connect to EGSA backend " << ENDPOINT_EXCHG_FRONTEND;
      }
      catch(zmq::error_t error)
      {
        LOG(ERROR) << fname << ": catch: " << error.what();
        status = NOK;
      }
      catch (std::exception &e)
      {
        LOG(ERROR) << fname << ": catch the signal: " << e.what();
        status = NOK;
      }
    } // end if SBS activated normally
  } // end if external smad connected normally
  else {
    LOG(ERROR) << "Connecting to internal EGSA SMAD, code=" << ext_state;
  }

  return status;
}

// ==========================================================================================================
#warning "Зачем дублировать в m_ega_ega_odm_ar_AcqSites информацию из m_egsa_config->sites() ?"
int EGSA::init_sites()
{
  int rc = OK;
  AcqSiteEntry* entry = NULL;

  // Открыть конфигурацию
  m_egsa_config = new EgsaConfig("egsa.json");

  // Прочитать информацию по сайтам и циклам
  m_egsa_config->load();

  // По списку известных нам систем сбора создать интерфейсы к их SMAD
  for (egsa_config_sites_t::const_iterator it = m_egsa_config->sites().begin();
       it != m_egsa_config->sites().end();
       ++it)
  {
    /*
    int raw_nature = (*it).second->nature;
    gof_t_SacNature sa_nature = GOF_D_SAC_NATURE_EUNK;
    if ((GOF_D_SAC_NATURE_DIR >= raw_nature) && (raw_nature <= GOF_D_SAC_NATURE_EUNK)) {
      sa_nature = static_cast<gof_t_SacNature>(raw_nature);
    }

    int raw_level = (*it).second->level;
    sa_object_level_t sa_level = LEVEL_UNKNOWN;
    if ((LEVEL_UNKNOWN >= raw_level) && (raw_level <= LEVEL_UPPER)) {
      sa_level = static_cast<sa_object_level_t>(raw_level);
    }*/

    entry = new AcqSiteEntry((*it).second
    /*
                             (*it).second->name,
                             (*it).second->nature,
                             (*it).second->auto_init,
                             (*it).second->auto_gencontrol */
                            );
#if 0
    strncpy(entry.s_IdAcqSite, (*it).second->name.c_str(), TAG_NAME_MAXLEN);
    entry.i_NatureAcqSite = sa_nature;
    entry.b_AutomaticalInit = (*it).second->auto_init;
    entry.b_AutomaticalGenCtrl = (*it).second->auto_gencontrol;

    // TODO: дать вменяемые значения
    entry.i_FunctionalState = 0;
    entry.b_InterfaceComponentActive = false;
    entry.p_ProgList = NULL;
    entry.p_AcquiredData = NULL;
#endif

    m_ega_ega_odm_ar_AcqSites.insert(entry);
  }

  return rc;
}


// ==========================================================================================================
// Подключиться к SMAD систем сбора
// TEST - подключиться к SMAD для каждой подчиненной системы
int EGSA::attach_to_sites_smad()
{
  // По списку известных нам систем сбора создать интерфейсы к их SMAD
  for (egsa_config_sites_t::const_iterator it = m_egsa_config->sites().begin();
       it != m_egsa_config->sites().end();
       ++it)
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

#warning "ACHTUNG! тут нужно предусмотреть возможность работы системы сбора и EGSA на разных хостах"
    // TODO: СС и EGSA могут работать на разных хостах, в этом случае подключение EGSA к smad СС
    // не получит доступа к реальным данным от СС. Их придется EGSA туда заносить самостоятельно.
    SystemAcquisition *sa_instance = new SystemAcquisition(
            this,
            sa_level,
            sa_nature,
            sa_name);
    if (SA_STATE_UNKNOWN != sa_instance->state()) {
      m_sa_list.insert(std::pair<std::string, SystemAcquisition*>(sa_name, sa_instance));
    }
    else {
      LOG(WARNING) << "Skip SA " << sa_name << ", SMAD state=" << sa_instance->state();
    }
  }

  return OK;
}

// ==========================================================================================================
// Изменение состояния подключенных систем сбора и отключение от их внутренней SMAD 
int EGSA::detach()
{
  int rc = NOK;

  // TODO: Для всех подчиненных систем сбора:
  // 1. Изменить их состояние SYNTHSTATE на "ОТКЛЮЧЕНО"
  // 2. Отключиться от их внутренней SMAD
  for (system_acquisition_list_t::const_iterator it = m_sa_list.begin();
       it != m_sa_list.end();
       ++it)
  {
    LOG(INFO) << "TODO: set " << (*it).first << "." << RTDB_ATT_SYNTHSTATE << " = 0";
    LOG(INFO) << "TODO: detach " << (*it).first << " SMAD";
    rc = OK;
  }

  return rc;
}

// ==========================================================================================================
// Интерфейс реализации второго уровня
// Отвечает за взаимодействие с интерфейсными модулями систем сбора и смежными объектами
int EGSA::implementation()
{
  const char* fname = "EGSA::implementation";
  int status = OK;

  try {

    while(!interrupt_worker) {
      LOG(INFO) << fname;
      sleep (1);
    }

  }
  catch(zmq::error_t error)
  {
    LOG(ERROR) << fname << ": catch: " << error.what();
    status = NOK;
  }
  catch (std::exception &e)
  {
    LOG(ERROR) << fname << ": catch the signal: " << e.what();
    status = NOK;
  }

  m_backend_socket.close();

  return status;
}

// ==========================================================================================================
// 1. Дождаться сообщения ADDL о запросе готовности к работе
// 2. Прочитать конфигурационный файл
// 3. Инициализация подписки на атрибуты состояний Систем Сбора
// 4. Ответить на запрос готовности к работе
//    Если инициализация прошла неуспешно - выйти из программы
// 5. Дождаться сообщения ADDL о начале штатной работы
// 6. Цикл до получения сообщения об останове:
// 6.1. Проверять наступление очередного цикла опроса
// 6.2. Опрашивать пробуждения (?)
// 6.3. Опрашивать состояния СС
//
int EGSA::run()
{
  const char* fname = "run";
  int status = NOK;
//  int mandatory = 1;
//  int linger = 0;
//  int hwm = 100;
//  int send_timeout_msec = SEND_TIMEOUT_MSEC; // 1 sec
//  int recv_timeout_msec = RECV_TIMEOUT_MSEC; // 3 sec
  bool recv_timeout = false;
  std::string *reply_to = new std::string;
  // Устанавливается флаг в случае получения команды на "Останов" из-вне
  bool get_order_to_stop = false;
//1  mdp::zmsg *request = NULL;

  interrupt_worker = true;

  LOG(INFO) << fname << ": START";

  // Загрузка конфигурации EGSA
  if (init()) {
    // Загрузка основной части кофигурации систем сбора и создание экземпляров
    // InternalSMAD без фактического подключения
    if (OK == (status = attach_to_sites_smad()))
      interrupt_worker = false;
  }

  try
  {

    // Запустить нить интерфейса
    m_servant_thread = new std::thread(std::bind(&EGSA::implementation, this));

#if 0
    if (OK == status) {
      // Сокет для получения запросов, прямого подключения, выполняется в отдельной нити 
      // ZMQ_ROUTER_MANDATORY может привести zmq_proxy_steerable к аномальному завершению: rc=-1, errno=113
      // Наблюдалось в случаях интенсивного обмена с клиентом, если тот аномально завершался.
      m_frontend.setsockopt(ZMQ_ROUTER_MANDATORY, &mandatory, sizeof (mandatory));
      m_frontend.bind("tcp://lo:5559" /*ENDPOINT_EXCHG_FRONTEND*/);
      m_frontend.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
      m_frontend.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
      m_frontend.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
      m_frontend.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
      m_frontend.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));
      m_socket_items[0].socket = (void*)m_frontend; // NB: Оператор (void*) необходим
      m_socket_items[0].fd = 0;
      m_socket_items[0].events = ZMQ_POLLIN;
      m_socket_items[0].revents = 0;

      // Сокет для получения сообщений от таймеров
      m_signal_socket.bind("inproc://egsa_timers");
      m_signal_socket.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
      m_signal_socket.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
      m_signal_socket.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
      m_signal_socket.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
      m_signal_socket.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));
      m_socket_items[1].socket = (void*)m_signal_socket; // NB: Оператор (void*) необходим
      m_socket_items[1].fd = 0;
      m_socket_items[1].events = ZMQ_POLLIN;
      m_socket_items[1].revents = 0;
    }
#endif

    if (OK == status) {
      // Отправить подчиненным системам сообщение о запросе готовности
      fire_ENDALLINIT();
    }

    // Ожидание завершения работы Прокси
    while (!interrupt_worker)
    {
      mdp::zmsg   *request  = NULL;

      LOG(INFO) << "EGSA::recv() ready";
      request = mdwrk::recv (reply_to, 1000, &recv_timeout);
      if (request)
      {
        LOG(INFO) << "EGSA::recv() got a message";

        processing(request, *reply_to, get_order_to_stop);

        // Обработка очередного цикла из очереди
        events::timer();

        if (get_order_to_stop) {
          LOG(INFO) << fname << " got a shutdown order";
          interrupt_worker = true; // order to shutting down
        }

        delete request;
      }
      else
      {
        if (!recv_timeout) {
          LOG(INFO) << "EGSA::recv() got a NULL";
          interrupt_worker = true; // Worker has been interrupted
        }
        else {
          // Все в порядке, за период опроса не было сообщений
        }
      }
    }

    m_servant_thread->join();
    delete m_servant_thread;

    delete reply_to;
//    cleanup();

    LOG(INFO) << "Digger's message processing cycle is finished";

/*
    while (!interrupt_worker && (OK == status))
    {
      // TODO: пробежаться по всем зарегистрированным системам сбора.
      // Если они в активном состоянии, получить от них даннные
      for (system_acquisition_list_t::const_iterator it = m_sa_list.begin();
          it != m_sa_list.end();
          ++it)
      {
        switch((*it).second->state())
        {
          case SA_STATE_OPER:
            LOG(INFO) << (*it).first << "state is OPERATIVE";
#if 0

            if (OK == ((*it).second->pop(m_list))) {
              for (sa_parameters_t::const_iterator it = m_list.begin();
                   it != m_list.end();
                   ++it)
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
#endif
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





      usleep (100000);
    }
*/
  }
  catch(zmq::error_t error)
  {
    LOG(ERROR) << fname << ": transport error catch: " << error.what();
    interrupt_worker = true;
    status = NOK;
  }
  catch (std::exception &e)
  {
    LOG(ERROR) << fname << ": runtime signal catch: " << e.what();
    interrupt_worker = true;
    status = NOK;
  }

//  m_frontend.close();
  m_signal_socket.close();

  // Ресурсы очищаются в локальной нити, деструктору ничего не достанется...
  LOG(INFO) << fname << ": STOP";

  return status;
}

void EGSA::tick_tack()
{
  events::timer();
}

// ==========================================================================================================
// Отправить всем подчиненным системам запрос готовности, и ожидать ответа
void EGSA::fire_ENDALLINIT()
{
#if (VERBOSE > 5)
  const char *fname = "fire_ENDALLINIT";
#endif
  bool stop_status;
  msg::SimpleRequest *simple_request = NULL;
//  msg::SimpleReply   *simple_reply = NULL;
  mdp::zmsg          *send_msg  = NULL;
  mdp::zmsg          *recv_msg  = NULL;
  std::string        *reply_to = new std::string;
//1  auto now = std::chrono::system_clock::now();

  LOG(INFO) << "Start sending ENDALLINIT to all SA";

  simple_request = static_cast<msg::SimpleRequest*>(m_message_factory->create(ADG_D_MSG_ENDALLINIT));

  for (system_acquisition_list_t::const_iterator sa = m_sa_list.begin();
       sa != m_sa_list.end();
       ++sa)
  {
    LOG(INFO) << "Send ENDALLINIT to " << (*sa).first;

    simple_request->header()->set_proc_dest((*sa).first);
    send_msg = simple_request->get_zmsg();
    send_msg->wrap((*sa).first.c_str(), "");

    send_to_broker((char*) MDPW_REPORT, NULL, send_msg);
  }

#if 0
    switch((*sa).second->state())
    {
      case SA_STATE_OPER:
        LOG(WARNING) << "SA " << (*sa).first << " state is OPERATIONAL before ENDALLINIT";
        break;

      case SA_STATE_PRE_OPER:
        LOG(WARNING) << (*sa).first << "state is PRE OPERATIONAL";
        break;

      case SA_STATE_DISCONNECTED:
      case SA_STATE_UNKNOWN:
      case SA_STATE_UNREACH:
        LOG(INFO) << (*sa).first << " state is UNREACHABLE";
        if ((*sa).second->send(ADG_D_MSG_ENDALLINIT)) {
//1          events::add(std::bind(&SystemAcquisition::check_ENDALLINIT, (*sa).second), now + std::chrono::seconds(2));
        }
        break;

/*      case SA_STATE_UNKNOWN:
        LOG(ERROR) << (*sa).first << " state is UNKNOWN";
        break;*/
    }
  }
#endif

  // TODO: Проверить, кто из СС успел передать сообщение о завершении инициализации
  int steps = 6;
  while(--steps) {

    // NB: Функция recv возвращает только PERSISTENT-сообщения
    recv_msg = mdwrk::recv (reply_to, 1000);
    if (recv_msg)
    {
#if (VERBOSE > 5)
      LOG(INFO) << fname << ": got a message from " << reply_to;
#endif

      processing (recv_msg, *reply_to, stop_status);

      delete recv_msg;
    }

    events::timer();
  }
}

// ==========================================================================================================
// Ввести в оборот новый Цикл сбора, вернуть новый размер очереди циклов
int EGSA::push_cycle(Cycle* _cycle)
{
  assert(_cycle);
  m_ega_ega_odm_ar_Cycles.push_back(_cycle);

  return m_ega_ega_odm_ar_Cycles.size();
}

// ==========================================================================================================
// Активировать циклы
// Активация заключается во взведении таймера Timer с требуемой периодичностью, который по достижению срока
// активации пишет название таймера в ранее объявленный сокет.
// Основной цикл работы приложения включает select из этого сокета, и в случае появления в нем данных
// читается название активированного цикла.
int EGSA::activate_cycles()
{
  auto now = std::chrono::system_clock::now();
  int cycle_idx = 0;

  for (std::vector<Cycle*>::iterator it = m_ega_ega_odm_ar_Cycles.begin();
       it != m_ega_ega_odm_ar_Cycles.end();
       ++it)
  {
    LOG(INFO) << "Activate cycle id:" << cycle_idx
              << " period: " << (*it)->period()
              << " (sec), name: " << (*it)->name();

    // Связать триггер с объектом, идентификаторами Цикла и Системы Сбора
    events::add(std::bind(&EGSA::trigger, this, cycle_idx++, 100),
                now /*+ std::chrono::seconds((*it)->period())*/); // Время активации: сейчас (+ период цикла?)
  }

  return OK;
}

// ==========================================================================================================
// Деактивировать циклы
int EGSA::deactivate_cycles()
{
  int cycle_idx = 0;

  for (std::vector<Cycle*>::iterator it = m_ega_ega_odm_ar_Cycles.begin();
       it != m_ega_ega_odm_ar_Cycles.end();
       ++it)
  {
    LOG(INFO) << "Deactivate cycle id " << cycle_idx++ << ": " << (*it)->name();
  }

  return OK;
}

// ==========================================================================================================
// Функция срабатывания при наступлении времени очередного таймера
// На входе получет порядковые номера Цикла и СС из m_ega_ega_odm_ar_Cycles
// Для каждой СС периоды Циклов одинаковы, но свое время начала.
// sa_id == -1 означает, что Цикл общий для всех его СС.
void EGSA::trigger(size_t cycle_id, size_t sa_id)
{
  int rc = OK;
  int delta_sec;

  assert(cycle_id < m_ega_ega_odm_ar_Cycles.size());
  LOG(INFO) << "Trigger callback for cycle id: " << cycle_id
            << ": " << m_ega_ega_odm_ar_Cycles.at(cycle_id)->name()
            << " SA id: " << sa_id;

  switch(cycle_id)
  {
    case 0:
      delta_sec = 1;
      break;

    case 1:
      delta_sec = 2;
      break;

    case 2:
      delta_sec = 3;
      break;

    case 3:
      delta_sec = 4;
      break;


    default:
      LOG(ERROR) << "Unsupported cycle_id: " << cycle_id;
      rc = NOK;
  }

  if (OK == rc) {
    LOG(INFO) << "Reactivate cycle " << m_ega_ega_odm_ar_Cycles.at(cycle_id)->name();

    auto now = std::chrono::system_clock::now();
    events::add(std::bind(&EGSA::trigger, this, cycle_id, 100),
                now + std::chrono::seconds(m_ega_ega_odm_ar_Cycles.at(cycle_id)->period()));

#if 0
    auto next = now + std::chrono::seconds(delta_sec);
    time_t tt_now    = std::chrono::system_clock::to_time_t ( now );
    time_t tt_future = std::chrono::system_clock::to_time_t ( next );
    LOG(INFO) << "now is: " << ctime(&tt_now);
    LOG(INFO) << "future is: " << ctime(&tt_future);
#endif
  }
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
    tv.tv_sec = 1;
    tv.tv_usec = 1;

    retval = select(m_socket+1, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

    switch(retval) {
      case -1:
        perror("select()");
        break;

      case 0:
        events::timer();
        std::cout << "." << std::flush;
        break;

      default:
        events::timer();
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
int EGSA::processing(mdp::zmsg* request, const std::string &identity, bool& need_to_stop)
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
      case ADG_D_MSG_ASKLIFE:
        rc = handle_asklife(letter, identity);
        break;

      // Завершение инициализации
      case ADG_D_MSG_ENDALLINIT:
        rc = handle_end_all_init(letter, identity);
        break;

      // Чтение данных
      case SIG_D_MSG_READ_MULTI:
        rc = handle_read_multiple(letter, identity);
        break;

      // Обновление группы подписки
      case SIG_D_MSG_GRPSBS:
        rc = handle_sbs_update(letter, identity);
        break;

      // Запрос на телерегулирование
      case SIG_D_MSG_ECHCTLPRESS:
      case SIG_D_MSG_ECHDIRCTLPRESS:
        rc = handle_teleregulation(letter, identity);
        break;

      case ADG_D_MSG_STOP:
        rc = handle_stop(letter, identity);
        need_to_stop = true;
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
  int rc = OK;

  LOG(INFO) << "Get request to stop component";
  interrupt_worker = true;

  return rc;
}

// ==========================================================================================================
int EGSA::handle_end_all_init(msg::Letter*, const std::string& origin)
{
  const char* fname = "EGSA::handle_end_all_init";

  LOG(INFO) << fname << ": BEGIN (from " << origin << ")";
  activate_cycles();
  LOG(INFO) << fname << ": END";

  return OK;
}

// ==========================================================================================================
int EGSA::handle_teleregulation(msg::Letter*, const std::string& origin)
{
  LOG(ERROR) << "Not yet realized: handle_teleregulation from " << origin;
  return NOK;
}

// ==========================================================================================================
int EGSA::handle_read_multiple(msg::Letter*, const std::string& origin)
{
  LOG(ERROR) << "Not yet realized: handle_read_multiple from " << origin;
  return NOK;
}

// ==========================================================================================================
int EGSA::handle_sbs_update(msg::Letter*, const std::string& origin)
{
  LOG(ERROR) << "Not yet realized: handle_sbs_update from " << origin;
  return NOK;
}

// ==========================================================================================================
// Активация группы подписки точек систем сбора 
int EGSA::activateSBS()
{
  std::string sbs_name = EXCHANGE_NAME;
  std::string rtdb_service_name = RTDB_NAME;
  int status = NOK;
  msg::Letter *request = NULL;
  mdp::zmsg* transport_cell;
  // Управление Группами Подписки
  msg::SubscriptionControl *sbs_ctrl_msg = NULL;

  try {
      // Инициируем создание сокета получения сообщений от сервера подписки
      // NB: (2016-05-24) mdp::mdwrk может только регистрировать одну подписку от БДРВ
      if (subscribe_to(RTDB_NAME, sbs_name)) {
        // Сообщим Службе о готовности к приему данных
        request = m_message_factory->create(SIG_D_MSG_GRPSBS_CTRL);
        sbs_ctrl_msg = dynamic_cast<msg::SubscriptionControl*>(request);
        sbs_ctrl_msg->set_name(sbs_name);
        // TODO: ввести перечисление возможных кодов
        // Сейчас (2015-10-01) 0 = SUSPEND, 1 = ENABLE
        sbs_ctrl_msg->set_ctrl(1);

        request->set_destination(rtdb_service_name);
        transport_cell = request->get_zmsg();

        if (send_to(RTDB_NAME, transport_cell, DIRECT)) {
          // От сервера подписки получить и разобрать ответ, содержащий
          // начальные значения атрибутов из группы подписки "EGSA"
          status = waitSBS();
        }
        else {
          LOG(ERROR) << "Subscribing failure";
        }

        delete transport_cell;
        delete request;
      }
      else {
        LOG(ERROR) << "Can't activate SA states group";
      }
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
  }
  catch (std::exception &e)
  {
    LOG(ERROR) << "catch the signal: " << e.what();
  }

  return status;
}

// ==========================================================================================================
int EGSA::recv(msg::Letter* &result, int timeout_msec)
{
  int status = OK;
  mdp::zmsg* report = NULL;
  std::string *reply_to = NULL;
  bool timeout_sign = false;

  assert(&result);
  result = NULL;
  reply_to = new std::string;

  if (NULL == (report = mdwrk::recv(reply_to, timeout_msec, &timeout_sign)))
  {
    if (!timeout_sign) {
      LOG(ERROR) << "Receiving failure";
      // Возможно, Брокер не запущен
      status = NOK;
    }
    else {
      // Всё в порядке, но за указанное время новых сообщений не поступало
      LOG(INFO) << "Receiving timeout";
    }
  }
  else
  {
    result = m_message_factory->create(report);
    delete report;
  }

  return status;
}

// ==========================================================================================================
// Ожидать получение сообщения SIG_D_MSG_READ_MULTI с первоначальными данными по группе подписки EGSA
int EGSA::waitSBS()
{
  int status = NOK;
  std::string cause_text = "";
  //msg::ExecResult *resp = NULL;
  msg::Letter *report = NULL;
  bool stop_receiving = false;

  try
  {
    //  Wait for report
    while (!stop_receiving)
    {
      LOG(INFO) << "Wait receiving SBS answer";
      // Ждать 2 секунды ответа с начальными значениями группы подписки
      status = recv(report, 2000);
      // Что-то получили?
      if (report) {
        // И полученное сообщение прочиталось?
        if (report->valid()) {

          if (SIG_D_MSG_READ_MULTI == report->header()->usr_msg_type()) {
            // Для режима подписки это сообщение приходят однократно,
            // для инициализации начальных значений атрибутов из группы
            LOG(INFO) << "Got SIG_D_MSG_READ_MULTI response: " << report->header()->usr_msg_type();

            status = process_read_response(report);
          }
          else {
            LOG(ERROR) << "Get unwilling message type #" << report->header()->usr_msg_type()
                       << " while waiting #" << SIG_D_MSG_READ_MULTI;
#if (VERBOSE > 5)
            report->dump();
#endif
          }
          stop_receiving = true;
          delete report;
        }
        else {
         stop_receiving = true;
         LOG(INFO) << "Receive broken SBS message";
        }
      }
      else {
        LOG(ERROR) << "None SBS received, status=" << status << ", exiting";
        stop_receiving = true;
      }
    }
  }
  catch (zmq::error_t err)
  {
    LOG(ERROR) << err.what();
  }

  return status;
}

// ==========================================================================================================
// Обработка полученных по подписке от БДРВ данных
int EGSA::process_read_response(msg::Letter* report)
{
  int status = OK;
  msg::ReadMulti* response = dynamic_cast<msg::ReadMulti*>(report);
 
  for (std::size_t idx = 0; idx < response->num_items(); idx++)
  {
     const msg::Value& attr_val = response->item(idx);
     LOG(INFO) << attr_val.tag() << " = " << attr_val.as_string() << std::endl;
  }
 
  return status;
}

// ==========================================================================================================
int EGSA::handle_asklife(msg::Letter* letter, const std::string& identity)
{
  msg::AskLife   *msg_ask_life = static_cast<msg::AskLife*>(letter);
  mdp::zmsg      *response = new mdp::zmsg();
  int exec_val = 1;

  msg_ask_life->set_exec_result(exec_val);

  response->push_front(const_cast<std::string&>(msg_ask_life->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_ask_life->header()->get_serialized()));
  response->wrap(identity.c_str(), EMPTY_FRAME);

  LOG(INFO) << "Processing asklife from " << identity
            << " has status:" << msg_ask_life->exec_result(exec_val)
            << " sid:" << msg_ask_life->header()->exchange_id()
            << " iid:" << msg_ask_life->header()->interest_id()
            << " dest:" << msg_ask_life->header()->proc_dest()
            << " origin:" << msg_ask_life->header()->proc_origin();

  send_to_broker((char*) MDPW_REPORT, NULL, response);
  delete response;

  return OK;
}

// ==========================================================================================================
int EGSA::handle_stop(msg::Letter* letter, const std::string& identity)
{
  int rc = OK;
  const char* fname = "handle_stop";
  msg::ExecResult *exec_reply = static_cast<msg::ExecResult*>(m_message_factory->create(ADG_D_MSG_EXECRESULT));

  LOG(INFO) << fname << ": BEGIN ADG_D_MSG_STOP from " << identity;
  interrupt_worker = true;

  try
  {
    // Отправить сообщение в EGSA о принудительном останове
    LOG(INFO) << "Send TERMINATE to EGSA";
    m_backend_socket.send("TERMINATE", 9, 0);

    deactivate_cycles();

    // TODO: Проверить, что все ресурсы освобождены, а нити остановлены
    exec_reply->header()->set_exchange_id(letter->header()->exchange_id());
    // TODO: Возможно ли переполнение?
    exec_reply->header()->set_interest_id(letter->header()->interest_id() + 1);
    exec_reply->header()->set_proc_dest(identity);
    if (OK == rc) 
      exec_reply->set_exec_result(1);
    else
      exec_reply->set_failure_cause(0, "GEV");

    mdp::zmsg *response = exec_reply->get_zmsg();
    response->wrap(identity.c_str(), "");

    // Послать отправителю сообщение EXEC_RESULT
    LOG(INFO) << "Send EXEC_RESULT to " << identity;
    send_to_broker((char*) MDPW_REPORT, NULL, response);

    delete response;
  }
  catch(zmq::error_t error)
  {
    LOG(ERROR) << fname << ": catch: " << error.what();
    rc = NOK;
  }
  catch(std::exception &e)
  {
    LOG(ERROR) << e.what();
    rc = NOK;
  }

  delete exec_reply;

  LOG(INFO) << fname << ": END ADG_D_MSG_STOP";

  return rc;
}

// ==========================================================================================================
// Получить набор циклов, в которых участвует заданная СС
std::vector<Cycle*> *EGSA::get_Cycles_for_SA(const std::string& sa_name)
{
  std::vector<Cycle*> *search_result = NULL;

  // СС может быть заявлена в нескольких циклах, проверить их все
  for (std::vector<Cycle*>::const_iterator it = m_ega_ega_odm_ar_Cycles.begin();
       it != m_ega_ega_odm_ar_Cycles.end();
       ++it)
  {
    LOG(INFO) << sa_name << ":" << (*it)->name();
    // Если указанная СС заявлена в данном цикле
    if ((*it)->exist_for_SA(sa_name)) {

      LOG(INFO) << sa_name << ": found cycle " << (*it)->name();

      // Создать вектор с циклами, если это не было сделано ранее
      if (!search_result) search_result = new std::vector<Cycle*>;
      search_result->push_back(*it);
    }

  }

  return search_result;
}

// ==========================================================================================================
// Получить набор запросов, зарегистрированных за данной СС
std::vector<Request*> *EGSA::get_Request_for_SA(const std::string& sa_name)
{
  return NULL;
}

// ==========================================================================================================
