//
// реализация Службы
//

// Общесистемные заголовочные файлы
#include <vector>
#include <thread>
#include <memory>
#include <functional>

// Служебные заголовочные файлы сторонних утилит
#include "zmq.hpp"
#include "glog/logging.h"

// Служебные файлы RTDBUS
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "mdp_helpers.hpp"
#include "mdp_zmsg.hpp"
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
  {BROKER_NAME,     ENDPOINT_BROKER /* tcp://localhost:5555 */, ""}, // Сам Брокер (BROKER_ENDPOINT_IDX)
  {RTDB_NAME,       ENDPOINT_RTDB_FRONTEND  /* tcp://localhost:5556 */, ""}, // Информационный сервер БДРВ
  {HMI_NAME,        ENDPOINT_HMI_FRONTEND   /* tcp://localhost:5558 */, ""}, // Сервер отображения
  {EXCHANGE_NAME,   ENDPOINT_EXCHG_FRONTEND /* tcp://localhost:5559 */, ""}, // Сервер обменов
  {HISTORIAN_NAME,  ENDPOINT_HIST_FRONTEND  /* tcp://localhost:5561 */, ""}, // Сервер архивирования и накопления предыстории
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
  m_service(service),
  m_broker_endpoint(broker_endpoint),
  m_direct_endpoint(NULL),
  m_worker(NULL),
  m_peer(NULL),
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

    // Получить ссылку на статически выделенную строку с параметрами подключения для bind
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
  delete[] m_direct_endpoint;

  for(ServicesHash_t::const_iterator it = m_services_info.begin();
      it != m_services_info.end();
      ++it)
  {
     // Освободить занятую под ServiceInfo_t память
     delete it->second;
  }

  try
  {
      // NB: При удалении экземпляров zmq::socket_t их соединение
      // закрывается автоматически
      LOG(INFO) << "mdwrk destroy tunnel " << m_worker << " to Broker";
      delete m_worker;
      LOG(INFO) << "mdwrk destroy tunnel " << m_peer << " to other Services";
      delete m_peer;
      LOG(INFO) << "mdwrk destroy direct tunnel " << m_direct;
      delete m_direct;

      // NB: Контекст m_context закрывается сам при уничтожении экземпляра mdwrk
  }
  catch(zmq::error_t error)
  {
      LOG(ERROR) << "mdwrk destructor: " << error.what();
  }

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

    assert((0 <= command[0]) && (command[0] <= 5));
    LOG(INFO) << "Sending " << mdpw_commands [static_cast<int>(*command)] << " to broker";
#if (VERBOSE > 5)
    msg->dump ();
#endif

    if (m_worker) {
      msg->send (*m_worker);
    }
    else {
      LOG(ERROR) << "send_to_broker() to uninitialized socket";
    }

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
    m_worker->setsockopt(ZMQ_LINGER, &linger, sizeof (int));
    m_worker->setsockopt(ZMQ_SNDTIMEO, &m_send_timeout_msec, sizeof(int));
    m_worker->setsockopt(ZMQ_RCVTIMEO, &m_recv_timeout_msec, sizeof(int));
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
    const char* endp = getEndpoint();
    msg->push_front (const_cast<char*>(endp));

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
        // При удалении экземпляра zmq::socket_t соединение закрывается автоматически
        //A m_direct->close();
        delete m_direct;
    }

    // Сокет типа "Ответ", процесс играет роль сервера, получая запросы
    m_direct = new zmq::socket_t (m_context, ZMQ_REP);
    m_direct->setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
    // ===================================================================================
    // Каждый Сервис, который допускает к себе прямое подключение Клиента, должен иметь
    // заранее определенную уникальную строку подключения. Подобные строки объединены в
    // таблицу связей между Сервисами и их строками подключения.
    // Таблицу читает Брокер, после чего каждый Обработчик в ответе на свое первое
    // сообщение Брокеру (READY) получает нужную строку. Эту же строку получают все
    // Клиенты, заинтересованные в прямой передаче данных между ними и Обработчиками.
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
mdwrk::set_heartbeat (int msec)
{
  LOG(INFO) << "Set heartbeat interval to " << msec << " msec";
  m_heartbeat_msec = msec;
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
mdwrk::set_reconnect (int msec)
{
  LOG(INFO) << "Set reconnect interval to " << msec << " msec";
  m_reconnect_msec = msec;
}

//  ---------------------------------------------------------------------
//  Подключиться к указанной службе
bool mdwrk::subscribe_to(const std::string& service, const std::string& sbs)
{
  bool status = true;
  int linger = 0;
  int hwm = 100;

  try {

    if (!m_subscriber) {
      m_subscriber = new zmq::socket_t (m_context, ZMQ_SUB);
      m_subscriber->setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
      m_subscriber->setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
    }

#warning "TODO: сейчас возможна только одна подписка, и только на RTDB"
    m_subscriber->connect(ENDPOINT_SBS_PUBLISHER);

    // Первым фреймом должно идти название группы
    m_subscriber->setsockopt(ZMQ_SUBSCRIBE, sbs.c_str(), sbs.size());

    LOG(INFO) << "Subscribe to " << service << ":" << sbs;
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << "Subscribing to " << service << ":" << sbs << " - " << err.what();
    status = false;
  }

  return status;
}

// ==========================================================================================================
// Отправить сообщение в адрес сторонней Службы
// Коды возврата
// OK - Служба известна, сообщение отправлено в очередь на пересылку
// NOK - Служба неизвестна, отправка сообщения невозможна
bool mdwrk::send_to(const std::string& service_name, mdp::zmsg *&request, ChannelType channel)
{
  ServiceInfo_t *service_info = NULL;
  bool status = false;

  switch (channel) {
    case PERSISTENT: // ----------------------------------------------------------------------
       //  Prefix request with protocol frames
       //  Frame 0: empty (REQ emulation)
       //  Frame 1: "MDPCxy" (six bytes, MDP/Client x.y)
       //  Frame 2: Service name (printable string)
       request->push_front (const_cast<char*>(service_name.c_str())); // frame 2
       request->push_front (const_cast<char*>(MDPC_CLIENT));       // frame 1
       request->push_front (const_cast<char*>(""));                // frame 0
       request->send (*m_worker);
       break;

     case DIRECT: // ----------------------------------------------------------------------
       // Если Служба известна, и у нее есть точка подключения, проверить -
       //   было ли подключение к прямому соединению с данной Службой.
       //   Если было - отправить сообщение на ранее сохраненный прямой сокет.
       //   Если нет - подключить клиентский сокет к точке подключения данной Службы

       // Проверить, есть ли информация по данному Сервису в кеше
       if (false == service_info_by_name(service_name, service_info)) {
         // Кеш не знает такой службы, запросим её у Брокера
         if ((ask_service_info(service_name, service_info)
         /* && (SERVICE_SUCCESS_OK == service_info->status_code)*/)) {
           LOG(INFO) << "GEV: " << service_name << " status=" << service_info->status_code;
           // Успешно получили от Брокера информацию по подключению к указанной Службе.
           // Она зарегистрирована в Брокере и находится в активном состоянии.

           // -----------------------------------------------------------
           // Подключить m_peer к конечной точке указанного Сервиса
           // Создадим, если ранее не было, сокет для связи с другими Службами
           if (!m_peer) {
             create_peering_to_services();
           }
           LOG(INFO) << "Connects first time to " << service_info->endpoint_external;
           m_peer->connect(service_info->endpoint_external);
           service_info->connected = true;

           // -----------------------------------------------------------
           // Обернули сообщение в конверт с обратным адресом
           request->wrap(m_service.c_str(), EMPTY_FRAME);
           request->send(*m_peer);

           LOG(INFO) << "Send first message to " << service_name
                     << " at " << service_info->endpoint_external;
           status = true;
         }
         else {
           LOG(ERROR) << "Can't get service '" << service_name << "' endpoint";
         }
       }
       else {
         // Кеш знает о данной Службе
         if (false == service_info->connected) { // Но мы еще не были подключены к ней
           
           // -----------------------------------------------------------
           // (2) - Подключить m_peer к конечной точке указанного Сервиса
           // Создадим, если ранее не было, сокет для связи с другими Службами
           if (!m_peer) {
             create_peering_to_services();
           }
           LOG(INFO) << "Connects time to " << service_info->endpoint_external;
           m_peer->connect(service_info->endpoint_external);
           service_info->connected = true;
         }
         // -----------------------------------------------------------
         // (3) Обернули сообщение в конверт с обратным адресом
         request->wrap(m_service.c_str(), EMPTY_FRAME);
         request->send(*m_peer);

         LOG(INFO) << "Send message to service " << service_name
                   << " at " << service_info->endpoint_external;
         status = true;
       }
       break;

     default: // ----------------------------------------------------------------------
       LOG(ERROR) << "Unsupported message sending channel: " << channel;
       assert(0 == 1);
  }

  return status;
}

// ==========================================================================================================
// Создать структуры для обмена с другими Службами
void mdwrk::create_peering_to_services()
{
//  int mandatory = 1;
//  int linger = 0;
  int hwm = 100;
  int send_timeout_msec = SEND_TIMEOUT_MSEC; // 1 sec
  int recv_timeout_msec = RECV_TIMEOUT_MSEC; // 3 sec

  // Сокет не должен быть создан ранее
  assert(!m_peer);

  // Типы сокетов: ZMQ_DEALER - Клиент, ZMQ_DEALER - Сервер
  m_peer = new zmq::socket_t(m_context, ZMQ_DEALER);

  LOG(INFO) << "Created DIRECT messaging socket with identity " << m_service;

  m_peer->setsockopt(ZMQ_IDENTITY, m_service.c_str(), m_service.size());
  m_peer->setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
  m_peer->setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
  m_peer->setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
  m_peer->setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

  // Инициализация записей для zmq::poll для точки входа ответов других Служб
  m_socket_items[PEER_ITEM].socket = (void*)*m_peer;
  m_socket_items[PEER_ITEM].fd = 0;
  m_socket_items[PEER_ITEM].events = ZMQ_POLLIN;
  m_socket_items[PEER_ITEM].revents = 0;
}

// ==========================================================================================================
//  Get the endpoint connecton string for specified service name
//  Если Служба найдена, по адресу ServiceInfo_t* передается информация о ней
//  Код возврата:
//  true - Служба известна
//  false - Нет информации о Службе с указанным названием
bool mdwrk::ask_service_info(const std::string& service_name, ServiceInfo_t*& service_info)
{
  mdp::zmsg *report  = NULL;
  mdp::zmsg *request = new mdp::zmsg ();
  const char *mmi_service_get_name = "mmi.service";
//1  std::string *reply_to = NULL;
  std::string reply_to;
  std::string* p_reply_to = &reply_to;
  bool exists = false;

  // Хеш соответствий:
  // 1. Имя Службы
  // 2. Был ли connect() к точке подключения основного сокета
  // Для каждой новой Службы выполняется новый connect()
  //-----------------------------------------------------------------

  LOG(INFO) << "Ask BROKER about '"<< service_name <<"' endpoint";

  // Брокеру - именно для этого Сервиса дай точку входа
  request->push_front(const_cast<char*>(service_name.c_str()));
  request->push_front(const_cast<char*>(mmi_service_get_name));
  // второй фрейм запроса - идентификатор обращения к внутренней службе Брокера
  send_to_broker (MDPW_REQUEST, NULL, request);
  // ============================================================
  // NB: вся дальнейшая обработка происходит внутри mdwrk::recv()
  // ============================================================

//1  reply_to = new std::string;
  // NB: Ответ разбирается внутри recv(), и mdwrk::recv ничего не возвращает
  report = recv(p_reply_to, 1000);

  // Проверить, пропала ли информация о точке подключения уже в справочник
  exists = service_info_by_name(service_name, service_info);

//1  delete reply_to;
  delete request;
  delete report;

  return exists;
}

// ==========================================================================================================
bool mdwrk::service_info_by_name(const std::string& serv_name, ServiceInfo_t *&serv_info)
{
  bool status = false;

  const ServicesHash_t::const_iterator it = m_services_info.find(serv_name);
  if (it != m_services_info.end()) {
    serv_info = it->second;
    status = true;
  }

  return status;
}

// ==========================================================================================================
// Запомнить соответствие "Название Службы" <=> "Строка подключения"
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
    LOG(WARNING) << "Try to duplicate service '" << serv_name << "' info";
    // TODO: Обновить информацию?
  }
  return status;
}

// ==========================================================================================================
//  Send reply, if any, to broker and wait for next request.
//  If reply is not NULL, a pointer to client's address is filled in.
//  Второй параметр - количество милисекунд ожидания получения сообщения,
//  превышение данного значения приводит к выходу из функции.
//
//  NB: Ненулевое msec_timeout должно завершать mdwrk::recv по истечению
//  этого значения времени с ответом NULL.
zmsg *
mdwrk::recv (std::string *&reply, int msec_timeout, bool* timeout_sign)
{
  int poll_socket_number = 1;
  // Интервал опроса сокетов
  int poll_interval = m_heartbeat_msec;
  int time_to_finish = 0;
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

    // Если используется сокет обмена со сторонними Службами, на него могут приходить
    // ответы на запросы текущей Службы
    if (m_peer)
      poll_socket_number++;
  
    // Проверить четыре возможных диапазона таймаута
    if (0 == msec_timeout) { // 1. Равен нулю - немедленный выход в случае отсутствия принимаемых данных
      poll_interval = 0;
      // Закончить recv() немедленно после проверки превышения текущего времени
      time_to_finish = 0;
    }
    else if (0 < msec_timeout) { // 2. Положительное значение - количество милисекунд ожидания
      poll_interval = std::min(msec_timeout, m_heartbeat_msec);
      // Отметка времени, по достижении которой выйти из recv()
      time_to_finish = s_clock() + poll_interval;
      if (msec_timeout > m_heartbeat_msec) {
        LOG(WARNING) << "Given poll timeout (" << msec_timeout
                     << ") exceeds heartbeat (" << m_heartbeat_msec
                     << ") and will be reduced to it";
      }
    }
    else {  // 4. Любое другое отрицательное значение - ошибка
      LOG(ERROR) << "Unsupported value for recv timeout: " << msec_timeout;
      assert(0 == 1);
    }

    interrupt_worker = 0;
    m_expect_reply = true;
    while (!interrupt_worker)
    {
      // NB: Нельзя для zmq::poll использовать значение '-1', поскольку в этом случае
      // перестанут выдаваться сообщения HEARTBEAT, т.к. операционная система не
      // вернет управления из poll() до поступления данных.
      zmq::poll (m_socket_items, poll_socket_number, poll_interval);

	  if (m_socket_items[BROKER_ITEM].revents & ZMQ_POLLIN) {
        zmsg *msg = new zmsg(*m_worker);

        // Отметить сообщение как "получено от Брокера"
        msg->set_source(zmsg::BROKER);
        LOG(INFO) << "New message from broker";
#if (VERBOSE > 5)
        msg->dump ();
#endif
        update_heartbeat_sign();

        //  Don't try to handle errors, just assert noisily
        assert (msg->parts () >= 3);

        // NB: если в zmsg [GEV:генерация GUID] закомментирована проверка 
        // на количество фреймов в сообщении (=5),
        // то в этом случае empty будет равен
        // [011] @0000000000
        // и assert сработает на непустую строку.
        //
        // Во случае, если принятое сообщение не первое от данного Обработчика,
        // empty будет действительно пустым.
        std::string empty = msg->pop_front ();
        assert (empty.empty() == 1);

        std::string header = msg->pop_front ();
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
        else if (command.compare (MDPW_REPORT) == 0) {
            LOG(INFO) << "REPORT from broker";
            // TODO: Обработать возможные oтветы от Брокера:
            // 1) Значение точки подключения другой Службы
            // 2) Состояние кластера?
            // 3) ...
            std::string report_type = msg->pop_front ();
            if (report_type.find("mmi.") != std::string::npos) {
               process_internal_report(report_type, msg);
            }
            else {
              LOG(ERROR) << "Unsupported report type: " << report_type;
            }
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

      } // Конец проверки активности на сокете Брокера
      else if (m_subscriber && (m_socket_items[SUBSCRIBER_ITEM].revents & ZMQ_POLLIN)) { // Событие подписки
        zmsg *msg = new zmsg(*m_subscriber);

        msg->set_source(zmsg::SUBSCRIBER);
        LOG(INFO) << "New subscription event";
#if (VERBOSE > 5)
        msg->dump ();
#endif
        return msg;
      }
      else if (m_direct && (m_socket_items[DIRECT_ITEM].revents & ZMQ_POLLIN)) { // Событие на общем сокете
        zmsg *msg = new zmsg(*m_direct);

        // Отметить сообщение как "получено напрямую от Клиента"
        msg->set_source(zmsg::DIRECT);
        LOG(INFO) << "New message from world";
#if (VERBOSE > 5)
        msg->dump ();
#endif
        return msg;
      }
      else if (m_peer && (m_socket_items[PEER_ITEM].revents & ZMQ_POLLIN)) { // Событие на сокете для ответов от других Служб
        zmsg *msg = new zmsg(*m_peer);

        // Отметить сообщение как "получено от других Служб"
        msg->set_source(zmsg::PEER);
        LOG(INFO) << "New response from other services";
#if (VERBOSE > 5)
        msg->dump ();
#endif
        return msg;
      }
      else if (s_clock() > (m_heartbeat_at_msec + (m_heartbeat_msec * m_liveness))) {
        // Последний обмен с Брокером был более чем m_liveness (три) периода HEARTBEAT назад - переустановить связь
        LOG(INFO) << "timeout, last HEARTBEAT was planned "
                  << s_clock() - m_heartbeat_at_msec << " msec ago, m_liveness=" << m_liveness;

        LOG(WARNING) << "Disconnected from broker - try to reconnect after " << m_reconnect_msec << " msec...";

        // NB: в этот период (интервал HEARTBEAT) Служба недоступна!
        // см. запись в README от 05.02.2015
        s_sleep (m_reconnect_msec);
        // После подключения к Брокеру сокет связи с ним был пересоздан
        connect_to_broker ();
      }

      if (s_clock () > m_heartbeat_at_msec) {
        send_to_broker ((char*)MDPW_HEARTBEAT, NULL, NULL);
        update_heartbeat_sign();
        LOG(INFO) << "update next HEARTBEAT event time, m_liveness=" << m_liveness;
      }

      // Ну всё уже проверили, осталось проверить - выходить из цикла или нет
      // Для нулевого интервала опроса - только одна итерация цикла
      // Для ненулевого значения - выход по превышению маркера времени
      //A LOG(INFO) << "GEV time_to_finish=" << time_to_finish << ", s_clock=" << s_clock();
      if (s_clock() > time_to_finish) {
        // Если нам передали адрес флага, вернем его значение
        if (timeout_sign)
          *timeout_sign = true; // Ничего не получено, возвратить признак таймаута
        break;
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

// ==========================================================================================================
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

// ==========================================================================================================
// Точка входа в разбор ответов Брокера
void mdwrk::process_internal_report(const std::string& report_type, zmsg* msg)
{
  // [2016-06-24] Пока только единственная обработка сообщения о другой Службе
  if (report_type.compare("mmi.service") == 0) {
    // Пришла информация по другой Службе
    process_endpoint_info(msg);
  }
  else {
    LOG(ERROR) << "Unsupported report type for internal service: " << report_type;
  }
}

// ==========================================================================================================
// Обработка ответа от Брокера с информацией о другой Службе
void mdwrk::process_endpoint_info(zmsg *msg)
{
  // Пример:
  // [006] <Имя Службы>
  // [003] {200|404|501}
  // Если предыдущее поле = 200, то:
  // [0XX] <строка с адресом подключения>
  const std::string service_name = msg->pop_front();
  const std::string existance_code = msg->pop_front();
  int service_status_code = atoi(existance_code.c_str()); // 200|404|501
  std::string endpoint;
  ServiceInfo_t *service_info = NULL;
  bool registered = service_info_by_name(service_name, service_info);

  switch(service_status_code) {
    case SERVICE_SUCCESS_OK:
      // Точка подключения - пока только при получении кода 200
      endpoint = msg->pop_front();
      LOG(INFO) << "Got '" << service_name << "' endpoint " << endpoint;

      // Проверить информацию о такой Службе
      if (!registered)
      {
            // Служба неизвестна, запомнить информацию
            service_info = new ServiceInfo_t; // Удаление в деструкторе
            memset(service_info, '\0', sizeof(ServiceInfo_t));
            strncpy(service_info->endpoint_external, endpoint.c_str(), ENDPOINT_MAXLEN);
            service_info->endpoint_external[ENDPOINT_MAXLEN] = '\0';
            service_info->connected = false;
            service_info->status_code = SERVICE_SUCCESS_OK;
            if (!insert_service_info(service_name, service_info))
              delete service_info; // Не удалось поместить информацию в хранилище
      }
      else
      {
            // Служба известна => проверить, обновился ли endpoint?
            if (strncmp(service_info->endpoint_external, endpoint.c_str(), ENDPOINT_MAXLEN))
            {
              // Есть изменения
              // TODO: что делать с новым значением точки подключения к Службе? Переподключить m_peer?
              LOG(WARNING) << "Endpoint for service '" << service_name
                           << "' was changed from " << service_info->endpoint_external
                           << " to " << endpoint;
            }
      }
      break;

    case SERVICE_ERROR_NOT_FOUND:
      LOG(ERROR) << "Requested service '" << service_name << "' not registered";
      if (registered) {
        service_info->status_code = SERVICE_ERROR_NOT_FOUND;
        // TODO: Удалить из словаря такие записи, занимающие место в ОЗУ?
      }
      break;

    default:
      LOG(ERROR) << "Unexpected state " << service_status_code
                 << " of requested service '" << service_name << "'";
  }
}

// ==========================================================================================================
// Получить точку подключения для нашего Сервиса
// NB: если запрашивается конвертация строки подключения, возвращаемая строка выделяется
// в куче динамически, и память под её нужно будет освободить на вызвавшей стороне.
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

  if (!endpoint) {
    LOG(WARNING) << "Service '" << m_service << "' has no information about external endpoint";
    // Сервис не имеет заранее известной строки подключения,
    // entry_idx ссылается на пустую строку
    endpoint = Endpoints[entry_idx].endpoint_default; // Нет
  }

  return endpoint;
}

// ==========================================================================================================
