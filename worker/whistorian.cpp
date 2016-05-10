/*
 * ============================================================================
 * Процесс обработки и накопления архивных данных
 * Аналоговые значения выбираются из БДРВ раз в минуту и заносятся в таблицу
 * минутных семплов.
 *
 * В минутные семплы попадают текущие значения и достоверность атрибута.
 * В пятиминутные семплы попадают средние арифметические значений и максимальное
 * значение достоверности из минутных семплов.
 *
 * Дискретные значения поступают в виде сообщений от DiggerWorker при изменении
 * значений соответствующих атрибутов VALIDCHANGE.
 *
 * eugeni.gorovoi@gmail.com
 * 01/11/2015
 * ============================================================================
 */

#include <string>
#include <vector>
#include <thread>
//#include <memory>
//#include <functional>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "sqlite3.h"

#include "helper.hpp"
#include "xdb_common.hpp"
#include "xdb_impl_error.hpp"
// общий интерфейс сообщений
#include "msg_common.h"
#include "msg_message.hpp"
// сообщения общего порядка
#include "msg_adm.hpp"
// сообщения по работе с БДРВ
#include "msg_sinf.hpp"
// Работа с историей
#include "hdb_impl_processor.hpp"
#include "mdp_zmsg.hpp"

#include "whistorian.hpp"

using namespace mdp;

// ---------------------------------------------------------------------
// Признаки необходимости завершения работы группы процессов генератора предыстории
extern int interrupt_worker;
volatile bool HistorianProducer::m_interrupt = false;
volatile bool HistorianResponder::m_interrupt = false;
// Таймаут в миллисекундах (мсек) ожидания получения данных
const int HistorianResponder::PollingTimeout = 1000;

// Массив переходов от текущего типа собирателя к следующему
const state_pair_t m_stages[xdb::PERIOD_LAST + 1] = {
  // текущая стадия (current)
  // |                     предыдущая стадия (prev)
  // |                     |                    следующая стадия (next)
  // |                     |                    |                       анализируемых отсчетов
  // |                     |                    |                       | предыдущей стадии (num_prev_samples)
  // |                     |                    |                       |   длительность стадии в секундах
  // |                     |                    |                       |   | (duration)
  // |                     |                    |                       |   |
  { xdb::PERIOD_NONE,      xdb::PERIOD_NONE,     xdb::PERIOD_NONE,      0,  0},
  { xdb::PERIOD_1_MINUTE,  xdb::PERIOD_NONE,     xdb::PERIOD_5_MINUTES, 1,  60},
  { xdb::PERIOD_5_MINUTES, xdb::PERIOD_1_MINUTE, xdb::PERIOD_HOUR,      5,  300},
  { xdb::PERIOD_HOUR,      xdb::PERIOD_5_MINUTES,xdb::PERIOD_DAY,      12,  3600},
  { xdb::PERIOD_DAY,       xdb::PERIOD_HOUR,     xdb::PERIOD_MONTH,    24,  86400},
  { xdb::PERIOD_MONTH,     xdb::PERIOD_DAY,      xdb::PERIOD_LAST,     30,  2592000}, // для 30 дней
  { xdb::PERIOD_LAST,      xdb::PERIOD_MONTH,    xdb::PERIOD_LAST,      0,  0}
};

// ---------------------------------------------------------------------
HistorianResponder::HistorianResponder(zmq::context_t& ctx) :
  m_context(ctx),
  m_frontend(ctx, ZMQ_ROUTER),
  m_time_before({0, 0}),
  m_time_after({0, 0}),
  m_message_factory(NULL),
  m_historic(NULL)
{
  LOG(INFO) << "Constructor HistorianResponder";
}

// ---------------------------------------------------------------------
HistorianResponder::~HistorianResponder()
{
}

// ---------------------------------------------------------------------
void HistorianResponder::stop()
{
  LOG(INFO) << "Set HistorianResponder's STOP sign";
  m_interrupt = true;
}

// ---------------------------------------------------------------------
// Запуск собирателя предыстории Historian
// TODO: циклически проверять получение сообщения от входного сокета
// Если получено новое сообщение:
//  1) сообщение о запросе истории?
//  2) сообщение об изменении состояния?
//  Возможно ли создание новой нити для обработки нового запроса истории? Можно обойтись без proxy?
void HistorianResponder::run()
{
  const char *fname = "HistorianResponder::run";
//  int rc = 1;
  int mandatory = 1;
  int linger = 0;
  int hwm = 100;
  int send_timeout_msec = SEND_TIMEOUT_MSEC; // 1 sec
  int recv_timeout_msec = RECV_TIMEOUT_MSEC; // 3 sec
  zmq::pollitem_t  socket_items[2];
  mdp::zmsg *request = NULL;

  LOG(INFO) << fname << ": START";
  try
  {
    // Сокет прямого подключения, выполняется в отдельной нити 
    // ZMQ_ROUTER_MANDATORY может привести zmq_proxy_steerable к аномальному завершению: rc=-1, errno=113
    // Наблюдалось в случаях интенсивного обмена с клиентом, если тот аномально завершался.
    m_frontend.setsockopt(ZMQ_ROUTER_MANDATORY, &mandatory, sizeof (mandatory));
    m_frontend.bind("tcp://lo:5561" /*ENDPOINT_HIST_FRONTEND*/);
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

    m_message_factory = new msg::MessageFactory(HISTORIAN_NAME);

    // Первый аргумент - среда БДРВ = NULL - подключения к БДРВ не нужно
    // Второй аргумент - название снимка Исторической БД (HDB) - по умолчанию
    m_historic = new HistoricImpl(NULL, NULL);

    if (!m_historic->Connect()) {
        LOG(FATAL) << fname << ": Historian initializing FAILURE";
        m_interrupt = true;
    }
    else LOG(INFO) << fname << ": Historian connection OK";

    while (!m_interrupt)
    {
      zmq::poll (socket_items, 1, PollingTimeout);

      if (socket_items[0].revents & ZMQ_POLLIN) { // Получен запрос

          request = new zmsg (m_frontend);
          assert (request);

          // replay address
          std::string identity  = request->pop_front();

          std::string empty = request->pop_front();

          processing(request, identity);

          delete request;
      } // если получен запрос

      LOG(INFO) << ":)";
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
  delete m_historic;

  LOG(INFO) << fname << ": STOP";
}

// --------------------------------------------------------------------------------
// Функция первично обрабатывает полученный запрос на получение предыстории.
// NB: Запросы к HDB обрабатываются последовательно.
int HistorianResponder::processing(mdp::zmsg* request, std::string &identity)
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
      // Запрос на чтение истории
      case SIG_D_MSG_REQ_HISTORY:
        rc = handle_query_history(letter, &identity);
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

// ---------------------------------------------------------------------
// Обработчик запросов истории от Клиентов
int HistorianResponder::handle_query_history(msg::Letter* letter, std::string* reply_to)
{
  int rc = OK;
  historized_attributes_t info[100];
  bool existance;
  msg::HistoryRequest *msg_req_history = static_cast<msg::HistoryRequest*>(letter);
  mdp::zmsg *response = new mdp::zmsg();

  LOG(INFO) << "Processing history request from " << *reply_to
            << " sid:" << msg_req_history->header()->exchange_id()
            << " iid:" << msg_req_history->header()->interest_id()
            << " dest:" << msg_req_history->header()->proc_dest()
            << " origin:" << msg_req_history->header()->proc_origin();

  msg_req_history->set_destination(reply_to->c_str());

  // Получить значения из истории
  int loaded = m_historic->load_samples_period_per_tag(
            msg_req_history->tag().c_str(),
            existance,
            msg_req_history->history_type(),
            msg_req_history->start_time(),
            info,
            msg_req_history->num_required_samples());

  if (!loaded) {
    // Установить признак существования запрошенного тега в HDB
    msg_req_history->set_existance(existance);

    if (false == existance) {
      // Нет такой точки в БДРВ
      LOG(ERROR) << "Request history of unexistent point \"" << msg_req_history->tag() << "\"";
      rc = NOK;
    }
    else LOG(ERROR) << "None selected for point \"" << msg_req_history->tag() << "\""
                    << " started at " << msg_req_history->start_time()
                    << " " << msg_req_history->num_required_samples() << " samples";
  }
  else
  {
     LOG(INFO) << "Load " << loaded << " of " << msg_req_history->num_required_samples()
               << " history items for " << msg_req_history->tag()
               << " from " << msg_req_history->start_time() << " sec";

     for (int idx=0; idx < loaded; idx++) {
#if (VERBOSE > 6)
        LOG(INFO) << idx << "/" << loaded << info[idx].datehourm
                  << " VAL=" << info[idx].val
                  << " VALID=" << (unsigned int)info[idx].valid;
#endif
        // Заполнить ответное сообщение полученными данными
        msg_req_history->add(info[idx].datehourm, info[idx].val, info[idx].valid);
     }
  }

  response->push_front(const_cast<std::string&>(msg_req_history->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_req_history->header()->get_serialized()));
  response->wrap(reply_to->c_str(), EMPTY_FRAME);

  //response->dump(); //1
  response->send (m_frontend); // Отправить ответ на запрос истории

  delete response;

  return rc;
}

// ---------------------------------------------------------------------
HistorianProducer::HistorianProducer(zmq::context_t& ctx, xdb::RtEnvironment* env) :
  m_context(ctx),
  m_env(env),
  m_time_before({0, 0}),
  m_time_after({0, 0})
{
  LOG(INFO) << "Constructor HistorianProducer, env: " << m_env;
}

// ---------------------------------------------------------------------
HistorianProducer::~HistorianProducer()
{
}

// ---------------------------------------------------------------------
void HistorianProducer::stop()
{
  LOG(INFO) << "Set HistorianProducer's STOP sign";
  m_interrupt = true;
}

// ---------------------------------------------------------------------
// Запуск собирателя предыстории Historian
void HistorianProducer::run()
{
  const char* fname = "HistorianProducer::run";
  char time_as_string[20];
  // Время обработки запроса
  timer_mark_t processing_duration;
  // Секунд до начала новой минуты
  long awaiting;
  time_t now;
  // первый аргумент - среда БДРВ
  // второй аргумент - название снимка Исторической БД (HDB)
  HistoricImpl *historic = new HistoricImpl(m_env, NULL);

  if (!historic->Connect()) {
      LOG(FATAL) << fname << ": Historian initializing FAILURE";
      m_interrupt = true;
  }
  else LOG(INFO) << fname << ": Historian connection OK";

  while (!m_interrupt)
  {
    // TODO: проверять работоспособность нитей Собирателей
    LOG(INFO) << "HistorianProducer iteration";

    // Дождаться начала интервала новой минуты
    while(!m_interrupt && ((awaiting = get_seconds_to_minute_edge()) > 0))
    {
#if (VERBOSE > 2)
      std::cout << "wait " << awaiting << " sec" << std::endl;
#endif
      // Разбить возможно большое время ожидания начала минуты на секундные интервалы.
      // Задержка чуть меньше секунды сделана специально, чтобы компенсировать задержки
      // внутри функции get_seconds_to_minute_edge() и др. накладные расходы на while
      usleep(700000);
    }

    // За время ожидания могла прийти команда завершения работы
    if (m_interrupt) break;

    now = time(0);
    // Начало рабочего цикла
    // ---------------------------------------------------
    historic->Make(now);
 
    // Запомнить время начала исполнения
    m_time_before = getTime();
    sprintf(time_as_string, "%ld", m_time_before.tv_sec);

    // Запомнить время завершения
    m_time_after = getTime();

    // Вычислить длительность исполнения, для формирования правильной задержки
    processing_duration = timeDiff(m_time_before, m_time_after);

#if (VERBOSE > 2)
    std::cout << "Sampling duration = " << processing_duration.tv_sec << " sec" << std::endl;
#endif
    // Конец рабочего цикла
    // NB: Если длительность цикла составила меньше секунды, есть вероятность
    // повторения цикла сразу же после завершения предыдущего.
    // ---------------------------------------------------
    if (0 == processing_duration.tv_sec)
    {
      LOG(INFO) << "Sampling cycle took less than one second, sleep one second";
      sleep(1); // Подождем секунду, чтобы не повторить цикл два (или больше) раза подряд
    }
  } // Конец рабочего цикла

  delete historic;

  LOG(INFO) << "STOP HistorianProducer::run()";
}

// ---------------------------------------------------------------------
Historian::Historian(const std::string& endpoint, const std::string& name) :
  mdp::mdwrk(endpoint, name, 1 /* num zmq io threads (default = 1) */),
  m_broker_endpoint(endpoint),
  m_server_name(name),
  m_producer(NULL),
  m_producer_thread(NULL),
  m_responder(NULL),
  m_responder_thread(NULL),
  m_message_factory(new msg::MessageFactory(HISTORIAN_NAME)),
  m_appli(NULL),
  m_environment(NULL),
  m_db_connection(NULL)
{
  LOG(INFO) << "Constructor Historian " << m_server_name
            << ", connect to " << m_broker_endpoint;

  m_appli = new xdb::RtApplication(HISTORIAN_NAME);
  // TODO: к этому моменту БДРВ должна быть открыта Диггером, завершиться если это не так
  m_appli->setOption("OF_READONLY",1);      // Открыть БД только для чтения
}

// ---------------------------------------------------------------------
Historian::~Historian()
{
  LOG(INFO) << "Destructor Historian";

  delete m_message_factory;
  delete m_db_connection;
  // NB: m_environment удаляется в RtApplication
  delete m_appli;
}

// --------------------------------------------------------------------------------
// Функция обрабатывает полученные служебные сообщения, не требующих подключения к БДРВ.
// Сейчас (2015.07.06) эта функция принимает запросы на доступ к БД, но не обрабатывает их.
int Historian::handle_request(mdp::zmsg* request, std::string* reply_to, bool& need_to_stop)
{
  int status = 0;
  rtdbMsgType msgType;

  assert (request->parts () >= 2);
  LOG(INFO) << "Process new request with " << request->parts() 
            << " parts and reply to " << *reply_to;

  msg::Letter *letter = m_message_factory->create(request);
  if (letter->valid())
  {
    msgType = letter->header()->usr_msg_type();

    switch(msgType)
    {
      case ADG_D_MSG_ASKLIFE:
        status = handle_asklife(letter, reply_to);
        break;

      case ADG_D_MSG_STOP:
        status = handle_stop(letter, reply_to);
        need_to_stop = true;
        break;

      default:
        LOG(ERROR) << "Unsupported request type: " << msgType;
    }
  }
  else
  {
    LOG(ERROR) << "Readed letter "<<letter->header()->exchange_id()<<" not valid";
  }

  delete letter;
  return status;
}

// --------------------------------------------------------------------------------
int Historian::handle_stop(msg::Letter* letter, std::string* reply_to)
{
  LOG(WARNING) << "Received message not yet supported received: ADG_D_MSG_STOP";
  return OK;
}

// --------------------------------------------------------------------------------
int Historian::handle_asklife(msg::Letter* letter, std::string* reply_to)
{
  msg::AskLife   *msg_ask_life = static_cast<msg::AskLife*>(letter);
  mdp::zmsg      *response = new mdp::zmsg();
  int exec_val = 1;

  msg_ask_life->set_exec_result(exec_val);

  response->push_front(const_cast<std::string&>(msg_ask_life->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_ask_life->header()->get_serialized()));
  response->wrap(reply_to->c_str(), EMPTY_FRAME);

  LOG(INFO) << "Processing asklife from " << *reply_to
            << " has status:" << msg_ask_life->exec_result(exec_val)
            << " sid:" << msg_ask_life->header()->exchange_id()
            << " iid:" << msg_ask_life->header()->interest_id()
            << " dest:" << msg_ask_life->header()->proc_dest()
            << " origin:" << msg_ask_life->header()->proc_origin();

  send_to_broker((char*) MDPW_REPORT, NULL, response);
  delete response;

  return OK;
}

// ---------------------------------------------------------------------
bool Historian::init()
{
  bool status = false;

  m_appli->initialize();

  if (NULL != (m_environment = m_appli->attach_to_env(RTDB_NAME)))
  {
    // Каждая нить процесса, желающая работать с БДРВ, должна получить свой экземпляр
    m_db_connection = m_environment->getConnection();

    if (m_db_connection && (m_db_connection->state() == xdb::CONNECTION_VALID))
      status = true;

    LOG(INFO) << "RTDB status: " << m_environment->getLastError().what()
              << ", connection status: " << status;
  }
  else
  {
    LOG(ERROR) << "Couldn't attach to '" << RTDB_NAME << "'";
  }

  // TODO: проверить подключение к Redis

  return status;
}

// ---------------------------------------------------------------------
void Historian::run()
{
  std::string *reply_to = NULL;
  // Устанавливается флаг в случае получения команды на "Останов" из-вне
  bool get_order_to_stop = false;
  const char* fname = "Historian::run()";

  LOG(INFO) << "start " << fname;

  interrupt_worker = false;

  try
  {
    // Создать экземпляр Накопителя
    m_producer = new HistorianProducer(m_context, m_environment);
    // Запустить Накопителя предыстории
    m_producer_thread = new std::thread(std::bind(&HistorianProducer::run, m_producer));

#warning "Проверить конкуренцию между открытием HistoricImpl в HistorianProducer и HistorianResponder"
    usleep(500000);

    // Запустить обработчик запросов от Клиентов на получение истории
    m_responder = new HistorianResponder(m_context);
    // Запустить Ответчика
    m_responder_thread = new std::thread(std::bind(&HistorianResponder::run, m_responder));

    while(!interrupt_worker)
    {
        reply_to = new std::string;

        LOG(INFO) << fname << " ready";
        // NB: Функция recv возвращает только PERSISTENT-сообщения
        mdp::zmsg *request = recv (reply_to);

        if (request)
        {
          LOG(INFO) << fname << " got a message";

          handle_request (request, reply_to, get_order_to_stop);

          if (get_order_to_stop) {
            LOG(INFO) << fname << " got a shutdown order";
            interrupt_worker = 1; // order to shutting down
          }

          delete request;
        }
        else
        {
          LOG(INFO) << fname << " got a NULL";
          interrupt_worker = 1; // has been interrupted
        }

        delete reply_to;
    }
  }
  catch(zmq::error_t error)
  {
    interrupt_worker = true;
    LOG(ERROR) << error.what();
  }
  catch(std::exception &e)
  {
    interrupt_worker = true;
    LOG(ERROR) << e.what();
  }

  // Послать в HistorianProducer сигнал завершения работы
  m_producer->stop();
  m_producer_thread->join();
  delete m_producer_thread;
  delete m_producer;

  // Послать в HistorianResponder сигнал завершения работы
  m_responder->stop();
  m_responder_thread->join();
  delete m_responder_thread;
  delete m_responder;

  LOG(INFO) << "finish " << fname;

}

// ---------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int status = EXIT_SUCCESS;
  Historian* historian = NULL;
  int  opt;
  char service_name[SERVICE_NAME_MAXLEN + 1];
  char broker_endpoint[ENDPOINT_MAXLEN + 1];

  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();

  // Значения по-умолчанию
  strcpy(broker_endpoint, ENDPOINT_BROKER);
  strcpy(service_name, HISTORIAN_NAME);

  while ((opt = getopt (argc, argv, "b:s:")) != -1)
  {
     switch (opt)
     {
       case 'b': // точка подключения к Брокеру
         strncpy(broker_endpoint, optarg, ENDPOINT_MAXLEN);
         broker_endpoint[ENDPOINT_MAXLEN] = '\0';
         break;

       case 's': // название собственной Службы
         strncpy(service_name, optarg, SERVICE_NAME_MAXLEN);
         service_name[SERVICE_NAME_MAXLEN] = '\0';
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

  std::string bro_endpoint(broker_endpoint);
  std::string srv_name(service_name);
  historian = new Historian(bro_endpoint, srv_name);

  if (true == historian->init())
  {
    LOG(INFO) << "Initialization success";
    historian->run();
    status = EXIT_SUCCESS;
  }
  else
  {
    LOG(ERROR) << "Initialization failure";
    status = EXIT_FAILURE;
  }

  delete historian;

//  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();

  return status;
}
