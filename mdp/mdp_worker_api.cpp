/*
 * реализация Службы
 */

// Общесистемные заголовочные файлы
#include <vector>
#include <thread>
#include <memory>
#include <functional>

// Служебные заголовочные файлы сторонних утилит
#include "zmq.hpp"
#include <glog/logging.h>

// Служебные файлы RTDBUS
#include "mdp_helpers.hpp"
#include "config.h"
#include "mdp_common.h"
#include "mdp_worker_api.hpp"

using namespace mdp;

// NB: Это дубликат структуры, хранящийся в xdb_impl_db_broker.cpp
// Структура записи соответствия между названием Сервиса и точкой подключения к нему
typedef struct {
  // Название Службы
  const char name[SERVICE_NAME_MAXLEN + 1];
  // Значение по-умолчанию
  const char endpoint_default[ENDPOINT_MAXLEN + 1];
  // Значение, прочитанное из снимка БД
  char endpoint_given[ENDPOINT_MAXLEN + 1];
} ServiceEndpoint_t;

// Таблица соответствия названия службы и ее точки входа Endpoints
// Формат endpoint серверный, предназначен для передачи в bind()
//
// NB: Это дубликат структуры, хранящийся в xdb_impl_db_broker.cpp
// Для серверного подключения с помощью bind по указанному endpoint 
// нужно заменить "localhost" на "*" или "lo", поскольку в таблице
// синтаксис endpoint приведен для вызова connect()
//
// TODO: вывести соответствия между Службой и её точкой входа в общую конфигурацию
//
ServiceEndpoint_t Endpoints[] = { // NB: Копия структуры в файле xdb_impl_db_broker.cpp
  {BROKER_NAME, ENDPOINT_BROKER /* tcp://localhost:5555 */, ""}, // Сам Брокер (BROKER_ENDPOINT_IDX)
  {RTDB_NAME,   ENDPOINT_RTDB_FRONTEND  /* tcp://localhost:5556 */, ""}, // Информационный сервер БДРВ
  {HMI_NAME,    ENDPOINT_HMI_FRONTEND   /* tcp://localhost:5558 */, ""}, // Сервер отображения
  {EXCHANGE_NAME,  ENDPOINT_EXCHG_FRONTEND /* tcp://localhost:5559 */, ""}, // Сервер обменов
  {HISTORIAN_NAME, ENDPOINT_HIST_FRONTEND  /* tcp://localhost:5561 */, ""}, // Сервер архивирования и накопления предыстории
  {"", "", ""}  // Последняя запись
};

//  ---------------------------------------------------------------------
//  Signal handling
//
//  Call s_catch_signals() in your application at startup, and then exit
//  your main loop if interrupt_worker is ever 1. Works especially well with
//  zmq_poll.
volatile int interrupt_worker = 0;

static void signal_handler (int signal_value)
{
    interrupt_worker = 1;
    LOG(INFO) << "Got signal "<<signal_value;
    // TODO: активировать нить-сторожа для принудительного удаления "зависших" процессов и ресурсов
}

static void catch_signals ()
{
    struct sigaction action;
    action.sa_handler = signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT,  &action, NULL);
    sigaction (SIGTERM, &action, NULL);
    sigaction (SIGUSR1, &action, NULL);
}

// Базовый экземпляр Службы
mdwrk::mdwrk (const std::string& broker_endpoint, const std::string& service, int num_threads, bool use_direct) :
  m_context(num_threads),
  m_broker_endpoint(broker_endpoint),
  m_service(service),
  m_direct_endpoint(NULL),
  m_worker(NULL),
  m_direct(NULL),
  m_subscriber(NULL),
  m_heartbeat_at_msec(0),
  m_liveness(HEARTBEAT_LIVENESS),
  m_heartbeat_msec(HeartbeatInterval), //  msecs
  m_reconnect_msec(HeartbeatInterval), //  msecs
  m_send_timeout_msec(SEND_TIMEOUT_MSEC),
  m_recv_timeout_msec(RECV_TIMEOUT_MSEC),
  m_expect_reply(false)
{
    s_version_assert (3, 2);

    catch_signals ();

    LOG(INFO) << "Create mdwrk, context " << &m_context;

    // Обнулим хранище данных сокетов для zmq::poll
    // Заполняется хранилище в функциях connect_to_*
    memset (static_cast<void*>(m_socket_items), '\0', sizeof(zmq::pollitem_t) * SOCKETS_COUNT);

    // Получить ссылку на динамически выделенную строку с параметрами подключения для bind
    m_direct_endpoint = getEndpoint(true);

    connect_to_broker ();

    // Для предотвращение повторного создания серверного сокета здесь и в DiggerProxy::run()
    if (use_direct)
      connect_to_world ();
}

//  ---------------------------------------------------------------------
//  Destructor
mdwrk::~mdwrk ()
{
  LOG(INFO) << "start mdwrk destructor";

  send_to_broker (MDPW_DISCONNECT, NULL, NULL);

  for(ServicesHashIterator_t it = m_services_info.begin();
      it != m_services_info.end();
      it++)
  {
     // Освободить занятую под ServiceInfo_t память
     delete it->second;
  }

  try
  {
      LOG(INFO) << "mdwrk destroy m_worker " << m_worker;
      if (m_worker)
      {
        m_worker->close();
        delete m_worker;
      }

      LOG(INFO) << "mdwrk destroy m_direct " << m_direct;
      if (m_direct)
      {
        m_direct->close();
        delete m_direct;
      }
      // NB: Контекст m_context закрывается сам при уничтожении экземпляра mdwrk
  }
  catch(zmq::error_t error)
  {
      LOG(ERROR) << "mdwrk destructor: " << error.what();
  }

  // Или == NULL, или память выделялась в getEndpoint()
  delete []m_direct_endpoint;
  LOG(INFO) << "finish mdwrk destructor";
}

//  ---------------------------------------------------------------------
//  Send message to broker
//  If no _msg is provided, creates one internally
void mdwrk::send_to_broker(const char *command, const char* option, zmsg *_msg)
{
    zmsg *msg = _msg? new zmsg(*_msg): new zmsg ();

    //  Stack protocol envelope to start of message
    if (option) {
        msg->push_front ((char*)option);
    }
    msg->push_front ((char*)command);
    msg->push_front ((char*)MDPW_WORKER);
    msg->push_front ((char*)"");

    LOG(INFO) << "Sending " << mdpw_commands [(int) *command] << " to broker";
    msg->dump ();

    if (m_worker)
    {
      msg->send (*m_worker);
    }
    else LOG(ERROR) << "send_to_broker() to uninitialized socket";

    delete msg;
}

//  ---------------------------------------------------------------------
//  Connect or reconnect to broker
void mdwrk::connect_to_broker ()
{
    int linger = 0;

    if (m_worker) {
        // Пересоздание сокета без обновления таблицы m_socket_items
        // влечет за собой ошибку zmq-рантайма "Socket operation on non-socket"
        // при выполнении zmq::poll
        LOG(INFO) << "connect_to_broker() => delete old m_worker";
        delete m_worker;
    }
    m_worker = new zmq::socket_t (m_context, ZMQ_DEALER);
    m_worker->setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    m_worker->setsockopt(ZMQ_SNDTIMEO, &m_send_timeout_msec, sizeof(m_send_timeout_msec));
    m_worker->setsockopt(ZMQ_RCVTIMEO, &m_recv_timeout_msec, sizeof(m_recv_timeout_msec));
    // GEV 22/01/2015: ZMQ_IDENTITY добавлено для теста
    m_worker->setsockopt (ZMQ_IDENTITY, m_service.c_str(), m_service.size());
    m_worker->connect (m_broker_endpoint.c_str());
    // Инициализация записей для zmq::poll для Брокера
    // NB: необходимо использовать оператор void* для получения доступа
    // к внутреннему представлению ptr сокета
    m_socket_items[BROKER_ITEM].socket = (void*)*m_worker;
    m_socket_items[BROKER_ITEM].fd = 0;
    m_socket_items[BROKER_ITEM].events = ZMQ_POLLIN;
    m_socket_items[BROKER_ITEM].revents = 0;

    LOG(INFO) << "Connecting to broker " << m_broker_endpoint;

    // Register service with broker
    // Внесены изменения из-за необходимости передачи значения точки подключения 
    zmsg *msg = new zmsg ();
    const char* endp = getEndpoint(false);
    if (endp)
    {
      msg->push_front (const_cast<char*>(endp));
    }
    else
    {
      LOG(WARNING) << "Service '" << m_service << "' has no information about external endpoint";
      msg->push_front (const_cast<char*>(EMPTY_FRAME));
    }

    msg->push_front (const_cast<char*>(m_service.c_str()));
    send_to_broker((char*)MDPW_READY, NULL, msg);

    delete msg;

    // If liveness hits zero, queue is considered disconnected
    update_heartbeat_sign();
    LOG(INFO) << "Next HEARTBEAT will be in " << m_heartbeat_msec
              << " msec, m_liveness=" << m_liveness;
}

//  ---------------------------------------------------------------------
//  Connect or reconnect to world
void mdwrk::connect_to_world ()
{
  int linger = 0;

  try
  {
    if (m_direct) {
        m_direct->close();
        delete m_direct;
    }

    // Сокет типа "Ответ", процесс играет роль сервера, получая запросы
    m_direct = new zmq::socket_t (m_context, ZMQ_REP);
    m_direct->setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
    // ===================================================================================
    // TODO: Каждый Сервис должен иметь уникальную строку подключения
    // Предложение: таблицу связей между Сервисами и их строками подключения читает Брокер,
    // после чего каждый Обработчик в ответе на свое первое сообщение Брокеру (READY) 
    // получает нужную строку. Эту же строку получают все Клиенты, заинтересованные 
    // в прямой передаче данных между ними и Обработчиками
    // ===================================================================================
    // 
    if(m_direct_endpoint)
    {
      m_direct->bind (m_direct_endpoint);
      // Инициализация записей для zmq::poll для общей точки входа
      m_socket_items[DIRECT_ITEM].socket = (void*)*m_direct;
      m_socket_items[DIRECT_ITEM].fd = 0;
      m_socket_items[DIRECT_ITEM].events = ZMQ_POLLIN;
      m_socket_items[DIRECT_ITEM].revents = 0;

      LOG(INFO) << "Connecting to world at " << m_direct_endpoint;
    }
    else
    {
      LOG(ERROR) << "Can't find direct endpoint for " << m_service;
    }
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << "Opening world socket: " << err.what();
  }
}

//  ---------------------------------------------------------------------
//  Set heartbeat delay
void
mdwrk::set_heartbeat (int heartbeat)
{
    m_heartbeat_msec = heartbeat;
}

//  ---------------------------------------------------------------------
//  Установить таймаут приема значений в милисекундах
void mdwrk::set_recv_timeout(int msec)
{
  LOG(INFO) << "Set new recv timeout to " << msec << " msec";
  m_recv_timeout_msec = msec;
}

//  ---------------------------------------------------------------------
//  Set reconnect delay
void
mdwrk::set_reconnect (int reconnect)
{
    m_reconnect_msec = reconnect;
}

//  ---------------------------------------------------------------------
//  Подключиться к указанной службе
int mdwrk::subscribe_to(const std::string& service, const std::string& sbs)
{
  int status_code = 0;
  int linger = 0;
  int hwm = 100;

  try {
    LOG(ERROR) << "Subscribing to " << service << ":" << sbs;

    if (!m_subscriber) {
      m_subscriber = new zmq::socket_t (m_context, ZMQ_SUB);
      m_subscriber->setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
      m_subscriber->setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
    }

#warning "TODO: сейчас возможна только одна подписка, и только на RTDB"
    m_subscriber->connect(ENDPOINT_SBS_PUBLISHER);

    // Первым фреймом должно идти название группы
    m_subscriber->setsockopt(ZMQ_SUBSCRIBE, sbs.c_str(), sbs.size());
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
    status_code = err.num();
  }

  return status_code;
}

// ==========================================================================================================
// Отправить сообщение в адрес сторонней Службы
void mdwrk::send(const std::string& serv_name, zmsg *&request)
{
  ServiceInfo_t *serv_info = NULL;
  int rc = 0;
  char service_endpoint[SERVICE_NAME_MAXLEN + 1];

  LOG(INFO) << "Send message to service " << serv_name;

  // Проверить, есть ли информация по данному Сервису в кеше
  if (false == service_info_by_name(serv_name, serv_info)) {
    // Кеш не знает такой службы, запросим её
    if (0 == (rc = ask_service_info(serv_name, service_endpoint, SERVICE_NAME_MAXLEN))) {
      // Успешно получили от Брокера информацию по подключению к указанной Службе
    }
    else {
      LOG(ERROR) << "Can't get service '" << serv_name << "' endpoint, rc=" << rc;
    }
  }
  else {
    // Кеш знает о данной Службе (serv_info)
#warning "2016-05-24 Продолжить отсюда"
    request->send(*m_direct);
  }

}

// ==========================================================================================================
//  Get the endpoint connecton string for specified service name
int mdwrk::ask_service_info(const std::string& service_name, char* service_endpoint, int buf_size)
{
  mdp::zmsg *report  = NULL;
  mdp::zmsg *request = new mdp::zmsg ();
  const char *mmi_service_get_name = "mmi.service";
  int service_status_code = 404;
  ServiceInfo_t *service_info;
  std::string *reply_to = NULL;

  // Хеш соответствий:
  // 1. Имя Службы
  // 2. Был ли connect() к точке подключения основного сокета
  // Для каждой новой Службы выполняется новый connect()
  //-----------------------------------------------------------------

  assert(service_endpoint);
  LOG(INFO) << "Ask BROKER about '"<< service_name <<"' endpoint";
  service_endpoint[0] = '\0';

  // Брокеру - именно для этого Сервиса дай точку входа
  request->push_front(const_cast<char*>(service_name.c_str()));
  // второй фрейм запроса - идентификатор обращения к внутренней службе Брокера
  send_to_broker (mmi_service_get_name, NULL, request);

  reply_to = new std::string;
  if (NULL == (report = mdwrk::recv(reply_to)))
  {
    LOG(ERROR) << "Receiving enpoint failure";
    // Возможно, Брокер не запущен
    service_status_code = 0;
  }
  else
  {
    LOG(INFO) << "Receive enpoint's response";
    //report->dump();

    // MDPC0X
//1    const std::string client_code = report->pop_front();
    // mmi.service
    const std::string service_request  = report->pop_front();
    assert(service_request.compare(mmi_service_get_name) == 0);
    const std::string service_name = report->pop_front();
    // 200|404|501
    const std::string existance_code = report->pop_front();
    service_status_code = atoi(existance_code.c_str());
    // Точка подключения - пока только при получении кода 200
    if (200 == service_status_code)
    {
      std::string endpoint = report->pop_front();
      strncpy(service_endpoint, endpoint.c_str(), buf_size);
      service_endpoint[buf_size] = '\0';

      // Проверить информацию о такой Службе
      if (false == service_info_by_name(service_name, service_info))
      {
        // Служба неизвестна, запомнить информацию
        service_info = new ServiceInfo_t; // Удаление в деструкторе
        memset(service_info, '\0', sizeof(ServiceInfo_t));
        strncpy(service_info->endpoint_external, service_endpoint, ENDPOINT_MAXLEN);
        service_info->connected = false;
        insert_service_info(service_name, service_info);
      }
      else
      {
        // Служба известна => проверить, обновился ли endpoint?
        if (strncmp(service_info->endpoint_external, service_endpoint, ENDPOINT_MAXLEN))
        {
          // Есть изменения
          // TODO: что делать со значением точки подключения к Службе?
          // Нормальная ли это ситуация?
          LOG(WARNING) << "Endpoint for service '" << service_name
                       << "' was changed from " << service_info->endpoint_external
                       << " to " << service_endpoint;
        }
      }
    }

    LOG(INFO) << "'" << service_name << "' status=" << existance_code
              << ", endpoint: " << service_endpoint;
  }

  delete reply_to;
  delete request;
  delete report;

  return service_status_code;
}

// ==========================================================================================================
bool mdwrk::service_info_by_name(const std::string& serv_name, ServiceInfo_t *&serv_info)
{
  bool status = false;

  ServicesHashIterator_t it;
  it = m_services_info.find(serv_name);
  if (it != m_services_info.end())
  {
    serv_info = it->second;
    status = true;
  }

  return status;
}

// ==========================================================================================================
// NB: Фунция insert_service_info сейчас (2015/02/18) вызывается после проверки
// отсутствия дубликатов в хеше. Возможно, повторная проверка на уникальность излишняя.
bool mdwrk::insert_service_info(const std::string& serv_name, ServiceInfo_t *&serv_info)
{
  bool status = false;

  if (0 == m_services_info.count(serv_name))
  {
    // Ранее такой Службы не было известно
    m_services_info.insert(ServicesHashPair_t(serv_name, serv_info));
    LOG(INFO) << "Store endpoint '" << serv_info->endpoint_external
              << "' for service '" << serv_name << "'";
    status = true;
  }
  else
  {
    LOG(ERROR) << "Try to duplicate service '" << serv_name << "' info";
  }
  return status;
}

//  ---------------------------------------------------------------------
//  Send reply, if any, to broker and wait for next request.
//  If reply is not NULL, a pointer to client's address is filled in.
//  Второй параметр - количество милисекунд ожидания получения сообщения,
//  превышение данного значения приводит к выходу из функции.
zmsg *
mdwrk::recv (std::string *&reply, int msec_timeout)
{
  int poll_socket_number = 1;
  // Флаг, было ли превышение таймаута
  bool timeout_exceeded = false;
  // Интервал опроса сокетов
  int poll_interval = m_heartbeat_msec;
  //  Format and send the reply if we were provided one
  assert (reply || !m_expect_reply);

  try
  {
    // Если активирована хотя бы одна подписка, пытаемся читать события с сокета подписки
    if (m_subscriber)
      poll_socket_number++;

    // Если используется сокет прямого подключения, пытаемся читать с него данные
    if (m_direct)
      poll_socket_number++;
  
    // Проверить четыре возможных диапазона таймаута
    if (0 == msec_timeout) { // 1. Равен нулю - немедленный выход в случае отсутствия принимаемых данных
      poll_interval = 0;
      timeout_exceeded = true;
    }
    else if (0 < msec_timeout) { // 2. Положительное значение - количество милисекунд ожидания
      poll_interval = std::min(msec_timeout, m_heartbeat_msec);
      if (msec_timeout > m_heartbeat_msec) {
        LOG(WARNING) << "Given poll timeout (" << msec_timeout
                     << ") exceeds heartbeat (" << m_heartbeat_msec
                     << ") and will be reduced to it";
      }
      timeout_exceeded = false;
    }
    else if (-1 == msec_timeout) {  // 3. Константа '-1' - неограниченно ожидать приёма сообщения  
      poll_interval = -1;
      timeout_exceeded = false;
    }
    else {  // 4. Любое другое отрицательное значение - ошибка
      LOG(ERROR) << "Unsupported value for recv timeout: " << msec_timeout;
      assert(0 == 1);
    }

    m_expect_reply = true;
    while (!interrupt_worker)
    {
      // NB: m_direct также создается и в DiggerProxy, что может привести к ошибке 'Address already in use'
      zmq::poll (m_socket_items, poll_socket_number, poll_interval /*m_heartbeat_msec*/);

	  if (m_socket_items[BROKER_ITEM].revents & ZMQ_POLLIN) {
        zmsg *msg = new zmsg(*m_worker);

        LOG(INFO) << "New message from broker:";
        msg->dump ();

        update_heartbeat_sign();

        //  Don't try to handle errors, just assert noisily
        assert (msg->parts () >= 3);

        /* 
         * NB: если в zmsg [GEV:генерация GUID] закомментирована проверка 
         * на количество фреймов в сообщении (=5),
         * то в этом случае empty будет равен
         * [011] @0000000000
         * и assert сработает на непустую строку.
         *
         * Во случае, если принятое сообщение не первое от данного Обработчика,
         * empty будет действительно пустым.
         */
        std::string empty = msg->pop_front ();
        assert (empty.empty() == 1);

        std::string header = msg->pop_front ();
        // Если Служба запрашивала информацию по точке подключения другой Службы,
        // здесь будет клиентский код.
        if (header.compare(MDPC_CLIENT) == 0) {
          // Да, это клиентский код, проверить на запрос точки подключения
          // Новый формат:
          // [000]
          // [006] MDPC0X
          // [001] 02
          // [006] MDPC0X
          // [011] mmi.service
          // [006] <Имя Службы>
          // [003] {200|404|501}
		  // Если предыдущее поле = 200, то:
		  // [0XX] <строка с адресом подключения>
          const char *mmi_service_get_name = "mmi.service";
          char service_endpoint[ENDPOINT_MAXLEN + 1] = "<none>";
          const std::string empty = msg->pop_front();
          const std::string client_code = msg->pop_front();
          const std::string service_request  = msg->pop_front();
          const std::string service_name = msg->pop_front();
          assert(service_request.compare(mmi_service_get_name) == 0);
          // 200|404|501
          const std::string existance_code = msg->pop_front();
          int service_status_code = atoi(existance_code.c_str());
          // Точка подключения - пока только при получении кода 200
          if (200 == service_status_code) {
            std::string endpoint = msg->pop_front();
            strncpy(service_endpoint, endpoint.c_str(), ENDPOINT_MAXLEN);
            service_endpoint[ENDPOINT_MAXLEN] = '\0';
            LOG(INFO) << "Reply " << service_name << " endpoint " << service_endpoint;
          }
          else {
            LOG(ERROR) << "Requested service not known";
          }
        } // Конец проверки сообщения от Клиента
        else { // Сообщение от Брокера
          assert (header.compare(MDPW_WORKER) == 0);

          std::string command = msg->pop_front ();
          if (command.compare (MDPW_REQUEST) == 0) {
              //  We should pop and save as many addresses as there are
              //  up to a null part, but for now, just save one...
              char *frame_reply = msg->unwrap ();
              (*reply).assign(frame_reply);
              delete[] frame_reply;
              return msg;     //  We have a request to process
          }
          else if (command.compare (MDPW_HEARTBEAT) == 0) {
              LOG(INFO) << "HEARTBEAT from broker, m_liveness=" << m_liveness;
              //  Do nothing for heartbeats
          }
          else if (command.compare (MDPW_DISCONNECT) == 0) {
              // После подключения к Брокеру сокет связи с ним был пересоздан
              connect_to_broker ();
          }
          else {
              LOG(ERROR) << "Receive invalid message " << (int) *(command.c_str());
              msg->dump ();
          }
          delete msg;
        } // Конец обработки сообщения от Брокера
      } // Конец проверки активности на сокете Брокера
      else if (m_subscriber && (m_socket_items[SUBSCRIBER_ITEM].revents & ZMQ_POLLIN)) { // Событие подписки
        zmsg *msg = new zmsg(*m_subscriber);

        LOG(INFO) << "New subscription event: ";
        msg->dump ();

        return msg;
      }
      else if (m_direct && (m_socket_items[DIRECT_ITEM].revents & ZMQ_POLLIN)) { // Событие на общем сокете
        zmsg *msg = new zmsg(*m_direct);

        LOG(INFO) << "New message from world:";
        msg->dump ();

        return msg;
      }
      else if (poll_interval && (0 == --m_liveness)) { // Ожидание нового запроса завершено по таймауту
        LOG(INFO) << "timeout, last HEARTBEAT was planned "
                  << s_clock() - m_heartbeat_at_msec << " msec ago, m_liveness=" << m_liveness;

        LOG(WARNING) << "Disconnected from broker - retrying...";

        // TODO: в этот период Служба недоступна. Уточнить таймауты!
        // см. запись в README от 05.02.2015
        s_sleep (m_reconnect_msec);
        // После подключения к Брокеру сокет связи с ним был пересоздан
        connect_to_broker ();
      }
      else if (!poll_interval) {
        LOG(INFO) << "inside recv: " << s_clock() << ", m_liveness="<<m_liveness << " poll_interval=" << poll_interval;
        break;
      }

      if (s_clock () > m_heartbeat_at_msec) {
        send_to_broker ((char*)MDPW_HEARTBEAT, NULL, NULL);
        update_heartbeat_sign();
        LOG(INFO) << "update next HEARTBEAT event time, m_liveness=" << m_liveness;
      }
    } // Конец цикла "пока не получен сигнал/команда на прерывание работы
  } // Конец блока try
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
    interrupt_worker = 1;
  }
  catch(std::exception &e)
  {
    LOG(ERROR) << e.what();
    interrupt_worker = 1;
  }

  if (interrupt_worker)
    LOG(WARNING) << "Interrupt received, killing worker...";

  return NULL;
}

// Обновить значения атрибутов, отвечающих за время выдачи HEARBEAT Брокеру
// Вызывается после успешной явной отправки HEARTBEAT, или в процессе нормального 
// обмена сообщениями между Службой и Брокером, т.к. успешность этого процесса 
// так же говорит о хорошем состоянии процесса Службы.
void mdwrk::update_heartbeat_sign()
{
  // Назначить время следующей подачи HEARTBEAT
  m_heartbeat_at_msec = s_clock () + m_heartbeat_msec;
  // Сбросить счетчик таймаутов
  // При достижении им 0 соединение с Брокером пересоздается
  m_liveness = HEARTBEAT_LIVENESS;
}

// Получить точку подключения для нашего Сервиса
const char* mdwrk::getEndpoint(bool convertation_asked) const
{
  int entry_idx = 0; // =0 вместо -1, чтобы пропустить нулевой индекс (Брокера)
  const char* endpoint = NULL;

  while (Endpoints[++entry_idx].name[0])  // До пустой записи в конце массива
  {
    if (0 == m_service.compare(Endpoints[entry_idx].name))
    {
        // Нашли нашу Службу
        // Есть ли для нее значение из БД?
        if (Endpoints[entry_idx].endpoint_given[0])
        {
          endpoint = Endpoints[entry_idx].endpoint_given;   // Да
        }
        else
        {
          endpoint = Endpoints[entry_idx].endpoint_default; // Нет
        }
        
        // Для bind() нужно переделать строку подключения, т.к. она в формате connect()
        // Заменить "tcp://адрес:порт" на "tcp://*:порт" или "tcp://lo:порт"
        if (convertation_asked)
        {
          char *endp = new char[strlen(endpoint) + 1];
          strcpy(endp, endpoint);
          char *pos_slash = strrchr(endp, '/');
          char *pos_colon = strrchr(endp, ':');

          strcpy(pos_slash + 1, "*"); // "tcp://*"
          strcat(endp, pos_colon);    // "tcp://*:5555"

          endpoint = endp;
        }

        break; // Завершаем поиск, т.к. названия Служб уникальны
    }
  }

  return endpoint;
}

