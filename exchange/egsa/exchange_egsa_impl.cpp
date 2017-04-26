#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <sys/time.h>   // for 'time_t' and 'struct timeval'
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>     // for 'usleep'
#include <iostream>
#include <functional>
#include <queue>
#include <chrono>
#include <thread>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "tool_events.hpp"
#include "mdp_common.h"
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
#include "exchange_egsa_request.hpp"
//#include "exchange_egsa_request_cyclic.hpp"
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

  // Отключиться от SMAD Сайтов
  detach();
  // Удалить сведения о Сайтах из памяти
  m_ega_ega_odm_ar_AcqSites.release();
  delete m_ext_smad;
  delete m_egsa_config;
  delete m_message_factory;
}

// ==========================================================================================================
int EGSA::init()
{
  const char* fname = "EGSA::init"; 
  int status = NOK;
  int linger = 0;
  //int hwm = 100;
  int send_timeout_msec = SEND_TIMEOUT_MSEC; // 1 sec
  int recv_timeout_msec = RECV_TIMEOUT_MSEC; // 3 sec
  smad_connection_state_t ext_state;

  // Инициализировать массивы Сайтов и Циклов
  load_config();

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
// На входе информация из конфигурационного файла. Она может содержать неотсортированные данные.
// То есть m_egsa_config->load() выполняет первый проход по обработке информации - проверяет ее на
// корректность и загружает все в символьном виде (названия Циклов, Сайтов и типов Запросов)
// Второй проход обработки заключается в переходе от символьного представления НСИ к идентификаторам
// и ссылкам, для ускорения работы
int EGSA::load_config()
{
  ech_t_ReqId request_id;
  int rc = OK;

  // Была ли ранее уже загружена конфигурация?
  if (m_egsa_config) {
    // Была - очистим старые данные и перечитаем её
    LOG(INFO) << "Reload configuration";

    delete m_egsa_config;
    m_ega_ega_odm_ar_AcqSites.release();
    // Sites.release();
    // Requests.release();
  }

  m_ega_ega_odm_ar_AcqSites.set_egsa(this);

  // Открыть конфигурацию
  m_egsa_config = new EgsaConfig("egsa.json");

  // Прочитать информацию по сайтам и циклам
  m_egsa_config->load();

  // Сейчас загружены списки актуальных Циклов, Запросов, Сайтов:
  // m_egsa_config->sites()
  // m_egsa_config->cycles()
  // m_egsa_config->requests()
  //
  // TODO: сформировать БД, содержащую все используемые Циклы, каждый из
  // которых содержит ссылки на Сайты, которые в свою очередь содержат
  // очередь актуальных Запросов.

  // 1) Создать объект-список Сайтов
  for (egsa_config_sites_t::const_iterator sit = config()->sites().begin();
       sit != config()->sites().end();
       ++sit)
  {
    AcqSiteEntry* site = new AcqSiteEntry(this, (*sit).second);
#warning "Проверь, нужен ли доступ к EGSA внутри AcqSiteEntry"
    m_ega_ega_odm_ar_AcqSites.insert(site);
  }

  // 2) Создать список Циклов
  for(egsa_config_cycles_t::const_iterator cit = config()->cycles().begin();
      cit != config()->cycles().end();
      ++cit)
  {
    // request_id получить по имени запроса
    if (ECH_D_NOT_EXISTENT != (request_id = m_egsa_config->get_request_id((*cit).second->request_name))) {

      // Создадим экземпляр Цикла, удалится он в деструкторе EGSA
      Cycle *cycle = new Cycle((*cit).first.c_str(),
                               (*cit).second->period,
                               (*cit).second->id,
                               request_id,
                               CYCLE_NORMAL);

      // Для данного цикла получить все использующие его сайты
      for(std::vector <std::string>::const_iterator sit = (*cit).second->sites.begin();
          sit != (*cit).second->sites.end();
          ++sit)
      {
        // Найти AcqSiteEntry для текущей SA в m_ega_ega_odm_ar_AcqSites
        AcqSiteEntry* site = m_ega_ega_odm_ar_AcqSites[(*sit)];
        cycle->link(site);
      }
      //
      cycle->dump();

      // Ввести в оборот новый Цикл сбора, вернуть новый размер очереди циклов
      m_ega_ega_odm_ar_Cycles.insert(cycle);
    }
    else {
      LOG(ERROR) << "Cycle " << (*cit).first << ": unknown included request " << (*cit).second->request_name;
    }
  }

  // 3) Создать список Запросов
  for(egsa_config_requests_t::const_iterator rit = config()->requests().begin();
      rit != config()->requests().end();
      ++rit)
  {
#if VERBOSE>6
    LOG(INFO) << "Req "
              << (*rit).second->s_RequestName << " "
              << (*rit).second->i_RequestPriority << " "
              << (unsigned int)(*rit).second->e_RequestObject << " "
              << (unsigned int)(*rit).second->e_RequestMode
              << " (irs " << (*rit).second->r_IncludingRequests[0]
              << " " << (*rit).second->r_IncludingRequests[1]
              << " " << (*rit).second->r_IncludingRequests[2]
              << " " << (*rit).second->r_IncludingRequests[3]
              << " " << (*rit).second->r_IncludingRequests[4]
              << " " << (*rit).second->r_IncludingRequests[5]
              << " " << (*rit).second->r_IncludingRequests[6]
              << " " << (*rit).second->r_IncludingRequests[7]
              << " " << (*rit).second->r_IncludingRequests[8]
              << " " << (*rit).second->r_IncludingRequests[9]
              << " " << (*rit).second->r_IncludingRequests[10]
              << " " << (*rit).second->r_IncludingRequests[11]
              << " " << (*rit).second->r_IncludingRequests[12]
              << " " << (*rit).second->r_IncludingRequests[13]
              << " " << (*rit).second->r_IncludingRequests[14]
              << " " << (*rit).second->r_IncludingRequests[15]
              << " " << (*rit).second->r_IncludingRequests[16]
              << " " << (*rit).second->r_IncludingRequests[17]
              << " " << (*rit).second->r_IncludingRequests[18]
              << " " << (*rit).second->r_IncludingRequests[19]
              << " " << (*rit).second->r_IncludingRequests[20]
              << " " << (*rit).second->r_IncludingRequests[21]
              << ")";
#endif
    Request* rq = new Request((*rit).second);

    m_ega_ega_odm_ar_Requests.add(rq);
  }

  return rc;
}

// ==========================================================================================================
// Доступ к конфигурации
EgsaConfig* EGSA::config()
{
  return m_egsa_config;
}

// ==========================================================================================================
// Доступ к Сайтам
AcqSiteList& EGSA::sites()
{
  return m_ega_ega_odm_ar_AcqSites;
}

// ==========================================================================================================
// Доступ к Циклам
CycleList& EGSA::cycles() {
  return m_ega_ega_odm_ar_Cycles;
}

// ==========================================================================================================
// Доступ к Запросам
RequestDictionary& EGSA::dictionary_requests()
{
  return m_ega_ega_odm_ar_Requests;
}

// ==========================================================================================================
// Подключиться к SMAD систем сбора
// TEST - подключиться к SMAD для каждой подчиненной системы
int EGSA::attach_to_sites_smad()
{
  int rc = OK;

  // По списку известных нам систем сбора создать интерфейсы к их SMAD
  for (size_t i=0; i < m_ega_ega_odm_ar_AcqSites.size(); i++) {

#warning "Нужно предусмотреть возможность работы системы сбора и EGSA на разных хостах"
    // TODO: СС и EGSA могут работать на разных хостах, в этом случае подключение EGSA к smad СС
    // не получит доступа к реальным данным от СС. Их придется EGSA туда заносить самостоятельно.
    rc |= m_ega_ega_odm_ar_AcqSites[i]->attach_smad();
  }

  return OK;
}

// ==========================================================================================================
// Изменение состояния подключенных систем сбора и отключение от их внутренней SMAD 
int EGSA::detach()
{
  int rc = OK;

  // TODO: Для всех подчиненных систем сбора:
  // 1. Изменить их состояние SYNTHSTATE на "ОТКЛЮЧЕНО"
  // 2. Отключиться от их внутренней SMAD
  for (size_t i=0; i < m_ega_ega_odm_ar_AcqSites.size(); i++) {
    LOG(INFO) << "TODO: set " << m_ega_ega_odm_ar_AcqSites[i]->name() << "." << RTDB_ATT_SYNTHSTATE << " = 0";
    LOG(INFO) << "TODO: detach " << m_ega_ega_odm_ar_AcqSites[i]->name() << " SMAD";
    rc |= m_ega_ega_odm_ar_AcqSites[i]->detach_smad();
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

#if 0
    if (OK == status) {
      // Отправить подчиненным системам сообщение о запросе готовности
      fire_ENDALLINIT();
    }
#endif

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
        cycles::timer();

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
      for (idx = 0; idx < m_ega_ega_odm_ar_AcqSites.size(); idx++)
      {
        sa = AcqSiteList[idx];

        switch(sa->state())
        {
          case SA_STATE_OPER:
            LOG(INFO) << sa->name() << "state is OPERATIVE";
#if 0

            if (OK == (sa->pop(m_list))) {
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
            LOG(INFO) << sa->name() << "state is PRE OPERATIVE";
            break;

          case SA_STATE_UNREACH:
            LOG(INFO) << sa->name() << " state is UNREACHABLE";
            break;

          case SA_STATE_UNKNOWN:
            LOG(INFO) << sa->name() << " state is UNKNOWN";
            break;

          default:
            LOG(ERROR) << sa->name() << " state unsupported: " << sa->state();
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

// ==========================================================================================================
void EGSA::tick_tack()
{
  cycles::timer();
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
  AcqSiteEntry       *sa;
//1  auto now = std::chrono::system_clock::now();

  LOG(INFO) << "Start sending ENDALLINIT to all SA";

  simple_request = static_cast<msg::SimpleRequest*>(m_message_factory->create(ADG_D_MSG_ENDALLINIT));

  for (size_t idx = 0; idx < m_ega_ega_odm_ar_AcqSites.size(); idx++)
  {
    sa = m_ega_ega_odm_ar_AcqSites[idx];
    assert(sa);

    switch(sa->state())
    {
      case SA_STATE_UNREACH: // Недоступна
        LOG(INFO) << sa->name() << " state is UNREACHABLE";
        break;

      case SA_STATE_OPER:    // Оперативная работа
        LOG(WARNING) << sa->name() << " state is OPERATIONAL before ENDALLINIT";
        break;

      case SA_STATE_PRE_OPER:// В процессе инициализации
        LOG(WARNING) << sa->name() << " state is PRE OPERATIONAL";
        break;

      case SA_STATE_INHIBITED:      // СС в состоянии запрета работы
      case SA_STATE_FAULT:          // СС в сбойном состоянии
      case SA_STATE_DISCONNECTED:   // СС не подключена
        break;


      case SA_STATE_UNKNOWN: // Неопределенное состояние
      default:
        LOG(ERROR) << sa->name() << " is in unsupported state:" << sa->state();
        break;
    }

    LOG(INFO) << "Send ENDALLINIT to " << sa->name();

    simple_request->header()->set_proc_dest(sa->name());
    send_msg = simple_request->get_zmsg();
    send_msg->wrap(sa->name(), "");

    send_to_broker((char*) MDPW_REPORT, NULL, send_msg);

#if 0
    if (sa->send(ADG_D_MSG_ENDALLINIT)) {
      cycles::add(std::bind(&SystemAcquisition::check_ENDALLINIT, sa), now + std::chrono::seconds(2));
    }
#endif
  }

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

    cycles::timer();
  }
}

// ==========================================================================================================
// Активировать циклы
int EGSA::activate_cycles()
{
  auto now = std::chrono::system_clock::now();
  //int site_idx; // Порядковый номер Сайта в списке для данного Цикла

  // Пройти по всем возможным Циклам
  for (size_t idx = ID_CYCLE_GENCONTROL; idx < ID_CYCLE_UNKNOWN; idx++)
  {
    // Вернет экземпляр, если таковой Цикл используется
    Cycle* cycle = m_ega_ega_odm_ar_Cycles[idx];

    if (cycle) {
      // Для каждого Цикла активировать столько экземпляров, сколько в нем объявлено Сайтов
      for (size_t sid = 0, site_idx = 0; sid < cycle->sites()->size(); sid++, site_idx++)
      {
        LOG(INFO) << "Activate cycle: " << cycle->name()
                  << " id=" << cycle->id()
                  << " period=" << cycle->period()
                  << " SA: " << (*cycle->sites())[sid]->name() << " id=" << site_idx;

        // Связать триггер с объектом, идентификаторами Цикла и Системы Сбора
        cycles::add(std::bind(&EGSA::cycle_trigger, this, cycle->id(), site_idx),
                    now /*+ std::chrono::seconds((*it)->period())*/); // Время активации: сейчас (+ период цикла?)
      }
    } /*
    else {
      LOG(ERROR) << "Skip missing cycle id=" << idx;
    } */
  }

  return OK;
}

// ==========================================================================================================
// Деактивировать циклы
int EGSA::deactivate_cycles()
{
//  int cycle_idx = 0;

  for (size_t idx = ID_CYCLE_GENCONTROL; idx < ID_CYCLE_UNKNOWN; idx++) {
    if (m_ega_ega_odm_ar_Cycles[idx])
      LOG(INFO) << "Deactivate cycle id=" << idx << ": " << m_ega_ega_odm_ar_Cycles[idx]->name();
  }
  cycles::clear();

  return OK;
}

// ==========================================================================================================
// Функция срабатывания при наступлении времени очередного таймера
// На входе
// cycle_id : идентификатор номера Цикла из m_ega_ega_odm_ar_Cycles
// sa_id    : локальный идентификатор СС для этого Цикла (NB: они не совпадают с id из m_ega_ega_odm_ar_Sites)
// Для каждой СС периоды Циклов одинаковы, но время начала индивидуально.
// sa_id == -1 означает, что Цикл общий для всех его СС.
void EGSA::cycle_trigger(size_t cycle_id, size_t sa_id)
{
  int rc = OK;
#if VERBOSE>6
  int delta_sec;
#endif

  Cycle *cycle = m_ega_ega_odm_ar_Cycles[cycle_id];
  AcqSiteEntry *site = (*cycle->sites())[sa_id];
  
  assert(site);
  assert(cycle);

  LOG(INFO) << "Trigger callback for cycle id=" << cycle_id << " (" << cycle->name() << ") "
            << " SA id=" << sa_id << " (" << site->name() << ")";

  if (cycle_id < ID_CYCLE_UNKNOWN) {
#if VERBOSE>6
    delta_sec = cycle->period();
#endif

    if (OK == rc) {
#if VERBOSE>6
      LOG(INFO) << "Reactivate cycle " << cycle->name() << " (" << cycle_id << "," << sa_id << ")";
#endif
      auto now = std::chrono::system_clock::now();
      cycles::add(std::bind(&EGSA::cycle_trigger, this, cycle_id, sa_id),
                  now + std::chrono::seconds(cycle->period()));

#if VERBOSE>6
      auto next = now + std::chrono::seconds(delta_sec);
      time_t tt_now    = std::chrono::system_clock::to_time_t ( now );
      time_t tt_future = std::chrono::system_clock::to_time_t ( next );
      LOG(INFO) << "now is: " << ctime(&tt_now);
      LOG(INFO) << "future is: " << ctime(&tt_future);
#endif
    }
  }
  else {
      LOG(ERROR) << "Unsupported cycle_id=" << cycle_id << " for SA id=" << sa_id;
      rc = NOK;
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
        cycles::timer();
        std::cout << "." << std::flush;
        break;

      default:
        cycles::timer();
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
  if (letter && letter->valid())
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
// Источник: сторожевая схема
// Условия получения: завершение инициализации Обменов
// Результат: старт нормальной работы Обменов
//
// Порядок выполнения процедуры:
// 1) Для каждой системы сбора - активация циклов
// 2) Отправка ответа EXEC_RESULT по завершению этапа
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

    // TODO: обновить состояние Сайтов m_ega_ega_odm_ar_AcqSites
    if (std::string::npos != attr_val.tag().find(RTDB_ATT_SYNTHSTATE)) {

      // Вырезать из attr_val.tag() часть от первого символа '/' до '.'
      const size_t point_pos = attr_val.tag().find(".");
      assert(std::string::npos != point_pos);
      assert(point_pos > 1);

      const std::string sa_name = attr_val.tag().substr(1, point_pos-1);
      AcqSiteEntry* sa_entry = m_ega_ega_odm_ar_AcqSites[sa_name];

      // Нашли в своей конфигурации упоминание этой СС
      if (sa_entry) {
        // Преобразовали числовой код состояния SYNTHSTATE из БДРВ
        const sa_state_t new_state = int_to_sa_state(attr_val.raw().fixed.val_uint8);
        // Зарядили необходимые Запросы в Циклы, где участвует эта СС

        // Поменяли состояние СС на нужное
        sa_entry->change_state_to(new_state);
        LOG(INFO) << "Update SA " << sa_name << " state to " << sa_entry->state();
      }
      else {
        LOG(WARNING) << "Skip processing SA " << sa_name << " because of absense it's configuration";
      }
    }

    //attr_val.tag()
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

#if 0
// ==========================================================================================================
// Получить набор циклов, в которых участвует заданная СС
std::vector<Cycle*> *EGSA::get_Cycles_for_SA(const std::string& sa_name)
{
  std::vector<Cycle*> *search_result = NULL;

  // СС может быть заявлена в нескольких циклах, проверить их все
  for (size_t idx = 0; idx < m_ega_ega_odm_ar_Cycles.size(); idx++)
  {
    //LOG(INFO) << sa_name << ":" << (*it)->name();
    Cycle* cycle = m_ega_ega_odm_ar_Cycles[idx];
    // Если указанная СС заявлена в данном цикле
    if (cycle && cycle->exist_for_SA(sa_name)) {

      //LOG(INFO) << sa_name << ": found cycle " << (*it)->name();

      // Создать вектор с циклами, если это не было сделано ранее
      if (!search_result) search_result = new std::vector<Cycle*>;
      search_result->push_back(cycle);
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

#endif
// ==========================================================================================================
