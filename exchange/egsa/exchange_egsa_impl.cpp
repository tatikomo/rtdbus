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
#include <sys/time.h>   // for 'time_t' and 'struct timeval'

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
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
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_request_cyclic.hpp"
#include "xdb_common.hpp"

volatile bool EGSA::m_interrupt = false;

namespace events
{
    struct event
    {
        typedef std::function<void()> callback_type;
        typedef std::chrono::time_point<std::chrono::system_clock> time_type;

        event(const callback_type &cb, const time_type &when)
            : callback_(cb), when_(when)
            { }

        void operator()() const
            { callback_(); }

        callback_type callback_;
        time_type     when_;
    };

    struct event_less : public std::less<event>
    {
            bool operator()(const event &e1, const event &e2) const
                {
                    return (e2.when_ < e1.when_);
                }
    };

    std::priority_queue<event, std::vector<event>, event_less> event_queue;

    void add(const event::callback_type &cb, const time_t &when)
    {
        auto real_when = std::chrono::system_clock::from_time_t(when);

        event_queue.emplace(cb, real_when);
    }

    void add(const event::callback_type &cb, const timeval &when)
    {
        auto real_when = std::chrono::system_clock::from_time_t(when.tv_sec) +
                         std::chrono::microseconds(when.tv_usec);

        event_queue.emplace(cb, real_when);
    }

    void add(const event::callback_type &cb,
             const std::chrono::time_point<std::chrono::system_clock> &when)
    {
        event_queue.emplace(cb, when);
    }

    void timer()
    {
        event::time_type now = std::chrono::system_clock::now();

        while (!event_queue.empty() &&
               (event_queue.top().when_ < now))
        {
            event_queue.top()();
            event_queue.pop();
        }
    }
}

// ==========================================================================================================
EGSA::EGSA(std::string& _broker, zmq::context_t& _ctx, msg::MessageFactory* _factory)
  : mdp::mdwrk(_broker, EXCHANGE_NAME, 1 /* num threads */, true /* use  direct connection */),
    m_context(_ctx),
    m_frontend(_ctx, ZMQ_ROUTER),       // Входящие соединения
    m_signal_socket(_ctx, ZMQ_ROUTER),  // Сообщения от таймеров циклов
    m_subscriber(_ctx, ZMQ_SUB),        // Подписка от БДРВ на атрибуты точек систем сбора
    m_message_factory(_factory),
    m_ext_smad(NULL),
    m_socket(-1),
    m_egsa_config(NULL)
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
  smad_connection_state_t ext_state;

  // Открыть конфигурацию
  m_egsa_config = new EgsaConfig("egsa.json");
  // Прочитать информацию по сайтам и циклам
  m_egsa_config->load();

  // Подключиться к своей внутренней памяти SMAD
  m_ext_smad = new ExternalSMAD(m_egsa_config->smad_name().c_str());

  if (STATE_OK == (ext_state = m_ext_smad->connect())) {
    // Активировать группу подписки
    if (OK != (rc = activateSBS())) {
      LOG(ERROR) << "Activating EGSA SBS, code=" << rc;
    }
  }
  else {
    LOG(INFO) << "Connecting to internal EGSA SMAD, code=" << ext_state;
  }

  return rc;
}

// ==========================================================================================================
// Подключиться к SMAD систем сбора
int EGSA::attach_to_sites_smad()
{
  // Список подчиненных систем сбора
  egsa_config_sites_t& sites = m_egsa_config->sites();
  // TEST - подключиться к SMAD для каждой подчиненной системы

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

    SystemAcquisition *sa_instance = new SystemAcquisition(
            m_signal_socket, // сюда идут сигналы от таймеров
            ega_ega_odm_ar_Cycles,
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
  for (system_acquisition_list_t::iterator it = m_sa_list.begin();
       it != m_sa_list.end();
       it++)
  {
    LOG(INFO) << "TODO: set " << (*it).first << "." << RTDB_ATT_SYNTHSTATE << " = 0";
    LOG(INFO) << "TODO: detach " << (*it).first << " SMAD";
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

  // Загрузка конфигурации EGSA
  if (OK == init()) {
    // Загрузка основной части кофигурации систем сбора
    // и создание экземпляров InternalSMAD без фактического
    // подключения, поскольку 
    status = attach_to_sites_smad();
  }

  LOG(INFO) << fname << ": START, status=" << status;

  try
  {
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
      socket_items[0].socket = (void*)m_frontend; // NB: Оператор (void*) необходим
      socket_items[0].fd = 0;
      socket_items[0].events = ZMQ_POLLIN;
      socket_items[0].revents = 0;

      // Сокет для получения сообщений от таймеров
      m_signal_socket.bind("inproc://egsa_timers");
      m_signal_socket.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
      m_signal_socket.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
      m_signal_socket.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
      m_signal_socket.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
      m_signal_socket.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));
      socket_items[1].socket = (void*)m_signal_socket; // NB: Оператор (void*) необходим
      socket_items[1].fd = 0;
      socket_items[1].events = ZMQ_POLLIN;
      socket_items[1].revents = 0;
    }

    // TODO: отправить подчиненным системам сообщение о запросе готовности
    fire_ENDALLINIT();


    while (!m_interrupt && (OK == status))
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
#if 0

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
  m_signal_socket.close();

  // Ресурсы очищаются в локальной нити, деструктору ничего не достанется...
  LOG(INFO) << fname << ": STOP";

  return status;
}

// ==========================================================================================================
// Отправить всем подчиненным системам запрос готовности, и жидать ответа
void EGSA::fire_ENDALLINIT()
{
  auto now = std::chrono::system_clock::now();

  for (system_acquisition_list_t::iterator sa = m_sa_list.begin();
       sa != m_sa_list.end();
       sa++)
  {
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

  // TODO: Проверить, кто из СС успел передать сообщение о завершении инициализации
  int timeout_endallinit = 6;
  while(--timeout_endallinit) {
      events::timer();
      sleep(1);
  }

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
    if (m_interrupt) {
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
  int rc = OK;

  LOG(INFO) << "Get request to stop component";
  m_interrupt = true;

  return rc;
}

// ==========================================================================================================
int EGSA::handle_teleregulation(msg::Letter*, std::string*)
{
  return NOK;
}

// ==========================================================================================================
// Активация группы подписки точек систем сбора 
int EGSA::activateSBS()
{
  std::string sbs_name = EXCHANGE_NAME;
  std::string service_name = RTDB_NAME;
  int rc = NOK;
  msg::Letter *request = NULL;
  char service_endpoint[ENDPOINT_MAXLEN + 1] = "";
  mdp::zmsg* transport_cell;
  int service_status;   // 200|400|404|501
  int status_code;
  // Управление Группами Подписки
  msg::SubscriptionControl *sbs_ctrl_msg = NULL;

  try {
      // Инициируем создание сокета получения сообщений от сервера подписки
      // NB: (2016-05-24) mdp::mdwrk может только регистрировать одну подписку от БДРВ
      if (0 == (status_code = subscribe_to(RTDB_NAME, sbs_name))) {
        // Сообщим Службе о готовности к приему данных
        request = m_message_factory->create(SIG_D_MSG_GRPSBS_CTRL);
        assert(request);
        sbs_ctrl_msg = dynamic_cast<msg::SubscriptionControl*>(request);
        sbs_ctrl_msg->set_name(sbs_name);
        // TODO: ввести перечисление возможных кодов
        // Сейчас (2015-10-01) 0 = SUSPEND, 1 = ENABLE
        sbs_ctrl_msg->set_ctrl(1); // TODO: ввести перечисление возможных кодов

        service_status = ask_service_info(RTDB_NAME, service_endpoint, ENDPOINT_MAXLEN);
        switch(service_status)
        {
          case 200:
            LOG(INFO) << RTDB_NAME << " status OK, endpoint=" << service_endpoint;

            request->set_destination(service_name);
            transport_cell = new mdp::zmsg ();
            transport_cell->push_front(const_cast<std::string&>(request->data()->get_serialized()));
            transport_cell->push_front(const_cast<std::string&>(request->header()->get_serialized()));

            // Отправить сообщение
            send (service_name, transport_cell);

            delete transport_cell;
            delete request;

            // От сервера подписки получить ответ, содержащий начальные значения атрибутов
            rc = waitSBS();
            break;

          case 400:
            LOG(INFO) << service_name << " status BAD_REQUEST : " << service_status;
            break;
          case 404:
            LOG(INFO) << service_name << " status NOT_FOUND : " << service_status;
            break;
          case 501:
            LOG(INFO) << service_name << " status NOT_SUPPORTED : " << service_status;
            break;
          default:
            LOG(INFO) << service_name << " status UNKNOWN : " << service_status;
        }
      }
      else {
        LOG(ERROR) << "Subscribing failure, status=" << status_code;
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

  return rc;
}

// ==========================================================================================================
int EGSA::recv(msg::Letter* &result)
{
  int status;
  mdp::zmsg* req = NULL;

  assert(&result);
  result = NULL;

//1  status = recv(req);

//1  if (RECEIVE_OK == status)
  {
    result = m_message_factory->create(req);
    delete req;
  }

  return status;
}

// ==========================================================================================================
int EGSA::waitSBS()
{
  int rc = NOK;
  int status;
  std::string cause_text = "";
  msg::ExecResult *resp = NULL;
  msg::Letter *report = NULL;
  bool stop_receiving = false;

  try
  {
    //  Wait for report
    while (!stop_receiving)
    {
      LOG(INFO) << "Try to receive SBS answer";

      status = recv(report);
      if (report) {

        switch (report->valid())
        {
          case false:
            stop_receiving = true;
            LOG(INFO) << "Receive broken message";
            break;

          case true:
            assert(report);
            LOG(INFO) << "Receive message:";
            report->dump();

            if (SIG_D_MSG_READ_MULTI == report->header()->usr_msg_type())
            {
              LOG(INFO) << "Got SIG_D_MSG_READ_MULTI response: " << report->header()->usr_msg_type();
              rc = OK;
              process_read_response(report);
              // Для режима подписки это сообщение приходят однократно,
              // для инициализации начальных значений атрибутов из группы
            }
            stop_receiving = true;
            delete report;
            break;

          default:
            LOG(ERROR) << "Бяка в статусе ответа на сообщение о подписке на EGSA";
        }
      }
      else {
        LOG(ERROR) << "Receive null message, exiting";
        stop_receiving = true;
      }
    }
  }
  catch (zmq::error_t err)
  {
    LOG(ERROR) << err.what();
  }

  return rc;
}

// ==========================================================================================================
bool EGSA::process_read_response(msg::Letter* report)
{
  bool status = false;
  msg::ReadMulti* response = dynamic_cast<msg::ReadMulti*>(report);
 
  for (std::size_t idx = 0; idx < response->num_items(); idx++)
  {
     const msg::Value& attr_val = response->item(idx);
     LOG(INFO) << attr_val.tag() << " = " << attr_val.as_string() << std::endl;
  }
 
  return status;
}

// ==========================================================================================================

