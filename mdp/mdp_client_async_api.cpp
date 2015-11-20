#include <glog/logging.h>

#include "config.h"
#include "mdp_zmsg.hpp"
#include "mdp_client_async_api.hpp"

using namespace mdp;

//  ---------------------------------------------------------------------
//  Signal handling
//
//  Call s_catch_signals() in your application at startup, and then exit
//  your main loop if interrupt_client is ever 1. Works especially well with
//  zmq_poll.
int interrupt_client = 0;
void s_signal_handler (int signal_value)
{
    interrupt_client = 1;
    LOG(INFO) << "Got signal "<<signal_value;
}

void s_catch_signals ()
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}


/*
 * Клиент может иметь представление о Службах, с которыми ему
 * предстоит работать. В этом случае можно, помимо подключения 
 * к Брокеру, создавать подключения к общему сокету Служб.
 * Тип у общих сокетов Служб (m_welcome) => ROUTER
 */
mdcli::mdcli (std::string broker, int verbose) :
   m_broker(broker),
   m_context(NULL),
   m_client(NULL),
   m_subscriber(NULL),
   m_peer(NULL),
   m_active_socket_num(0),
   m_subscriber_socket_index(0),
   m_peer_socket_index(0),
   m_verbose(verbose),
   m_timeout(2500)       //  msecs
{
   s_version_assert (3, 2);
   m_context = new zmq::context_t (1);
   s_catch_signals ();

   connect_to_broker ();
   connect_to_services ();
}


//  ---------------------------------------------------------------------
//  Destructor
mdcli::~mdcli ()
{
  for(ServicesHashIterator_t it = m_services_info.begin();
      (it != m_services_info.end());
      it++)
  {
     // Освободить занятую под ServiceInfo память
     delete it->second;
  }
  
  try
  {
    if (m_subscriber)
      m_subscriber->close();

    if (m_peer)
      m_peer->close();

    m_client->close();

    delete m_subscriber;
    delete m_peer;
    delete m_client;

    // [08.10.2015] Проверить - проблема решена путем установки таймаута на передачу данных
    //
    // ISSUE 25.05.2015: если Клиент попытался отправить сообщение несуществующему Сервису,
    // а затем завершает свою работу, то он "зависает" на удалении контекста:
    // #0  0xb7fdd424 in __kernel_vsyscall ()
    // #1  0xb7c0ac8b in poll () at ../sysdeps/unix/syscall-template.S:81
    // #2  0xb7f329d5 in zmq::signaler_t::wait (this=0x8053dd0, timeout_=-1) at src/signaler.cpp:205
    // #3  0xb7f13da2 in zmq::mailbox_t::recv (this=0x8053d9c, cmd_=0xbfffee00, timeout_=-1) at src/mailbox.cpp:70
    // #4  0xb7f02867 in zmq::ctx_t::terminate (this=0x8053d48) at src/ctx.cpp:158
    // #5  0xb7f4fbdc in zmq_ctx_term (ctx_=0x8053d48) at src/zmq.cpp:157
    // #6  0xb7f4fdbd in zmq_ctx_destroy (ctx_=0x8053d48) at src/zmq.cpp:227
    // #7  0xb7fcf214 in zmq::context_t::close (this=0x80549c8) at /home/gev/ITG/sandbox/rtdbus/cmake/../../cppzmq/zmq.hpp:309
    // #8  0xb7fcf1e3 in zmq::context_t::~context_t (this=0x80549c8, __in_chrg=<optimized out>)
    //     at /home/gev/ITG/sandbox/rtdbus/cmake/../../cppzmq/zmq.hpp:302
    // #9  0xb7fcd518 in mdp::mdcli::~mdcli (this=0x8053b50, __in_chrg=<optimized out>)
    //
    delete m_context;
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
  }
}


//  ---------------------------------------------------------------------
//  Connect or reconnect to broker
void mdcli::connect_to_broker ()
{
  int linger = 0;
  int send_timeout_msec = SEND_TIMEOUT_MSEC; // def: 1 sec
  int recv_timeout_msec = RECV_TIMEOUT_MSEC; // def: 3 sec

  try
  {
    if (m_client) {
      delete m_client;
    }
  
    m_client = new zmq::socket_t (*m_context, ZMQ_DEALER);

    m_client->setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    m_client->setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    m_client->setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

    m_client->connect (m_broker.c_str());

    // Заполним структуру для работы recv с помощью zmq::poll
    m_socket_items[BROKER_ITEM].socket = (void*)*m_client; // NB: оператор (void*) обязателен!
    // zmq::socket_t::operator (void*) предоставляет доступ к внутреннему представлению zmq_socket_t
    m_socket_items[BROKER_ITEM].fd = 0;
    m_socket_items[BROKER_ITEM].events = ZMQ_POLLIN;
    m_socket_items[BROKER_ITEM].revents = 0;

    // Есть пока одно подключение, к Брокеру
    m_active_socket_num = 1;

    LOG(INFO) << "Connecting to broker at " << m_broker;
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
  }
}

//  ---------------------------------------------------------------------
//  Connect or reconnect to Services
void mdcli::connect_to_services ()
{
  LOG(INFO) << "Connecting to services and publishers";
}

//  ---------------------------------------------------------------------
//  Set request timeout
void
mdcli::set_timeout (int timeout)
{
   m_timeout = timeout;
}

//  ---------------------------------------------------------------------
bool mdcli::service_info_by_name(const char* serv_name, ServiceInfo *&serv_info)
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

//  ---------------------------------------------------------------------
// NB: Фунция insert_service_info сейчас (2015/02/18) вызывается после проверки
// отсутствия дупликатов в хеше. Возможно, повторная проверка на уникальность излишняя.
bool mdcli::insert_service_info(const char* serv_name, ServiceInfo *&serv_info)
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
//  Get the endpoint connecton string for specified service name
int mdcli::ask_service_info(const char* service_name, char* service_endpoint, int buf_size)
{
  mdp::zmsg *report  = NULL;
  mdp::zmsg *request = new mdp::zmsg ();
  const char *mmi_service_get_name = "mmi.service";
  int service_status_code;
  int status;
  ServiceInfo *service_info;

  // Хеш соответствий:
  // 1. Имя Службы
  // 2. Был ли connect() к точке подключения основного сокета Клиента
  // Сокет Служб m_peer, для каждой новой Службы выполняется новый connect()
  //-----------------------------------------------------------------

  assert(service_endpoint);
  LOG(INFO) << "Ask BROKER about '"<< service_name <<"' endpoint";
  service_endpoint[0] = '\0';

  // Брокеру - именно для этого Сервиса дай точку входа
  request->push_front(const_cast<char*>(service_name));
  // второй фрейм запроса - идентификатор ображения к внутренней службе Брокера
  send (mmi_service_get_name, request);

  status = recv(report);

  if (RECEIVE_OK != status)
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
    std::string client_code = report->pop_front();
    // mmi.service
    std::string service_request  = report->pop_front();
    assert(service_request.compare(mmi_service_get_name) == 0);
    // 200|404|501
    std::string existance_code = report->pop_front();
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
        service_info = new ServiceInfo; // Удаление в деструкторе
        memset(service_info, '\0', sizeof(ServiceInfo));
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

    LOG(INFO) << "'" << service_name << "' status: " << existance_code
              << " endpoint: " << service_endpoint;
  }

  delete request;
  delete report;

  return service_status_code;
}

//  ---------------------------------------------------------------------
//  Активация подписки с указанным названием для указанной Службы
int mdcli::subscript(const std::string& service_name, const std::string& group_name)
{
  int status_code = 0;
  int linger = 0;
  int hwm = 100;

  m_subscriber = new zmq::socket_t (*m_context, ZMQ_SUB);

  // TODO: Получить точку подключения для подписок этой Службы
  // NB: пока считается, что есть только одна Служба с единственным адресом публикации

  m_subscriber->setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
  m_subscriber->setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));

#warning "Создать механизм получения Клиентом от Сервиса адреса публикации поддерживаемых ей Групп подписок"
  m_subscriber->connect(ENDPOINT_SBS_PUBLISHER);

  // Первым фреймом должно идти название группы
  m_subscriber->setsockopt(ZMQ_SUBSCRIBE, group_name.c_str(), group_name.size());

  add_socket_to_pool(m_subscriber, SUBSCRIBER_ITEM);
 
  LOG(INFO) << "Subscript to ["<<service_name<<":"<<group_name<<"]";

  return status_code;
}

//  ---------------------------------------------------------------------
//  Send request to broker
//  Takes ownership of request message and destroys it when sent.
//  The send method now just sends one message, without waiting for a
//  reply. Since we're using a DEALER socket we have to send an empty
//  frame at the start, to create the same envelope that the REQ socket
//  would normally make for us
//  TODO: deprecated, use send (std::string, zmsg*&, ChannelType) instead
int
mdcli::send (std::string service, zmsg *&request_p)
{
  int stat = 0;
  assert (request_p);
  zmsg *request = request_p;

  try
  {
    //  Prefix request with protocol frames
    //  Frame 0: empty (REQ emulation)
    //  Frame 1: "MDPCxy" (six bytes, MDP/Client x.y)
    //  Frame 2: Service name (printable string)
    request->push_front (const_cast<char*>(service.c_str()));
    request->push_front (const_cast<char*>(MDPC_CLIENT));
    request->push_front (const_cast<char*>(""));

#ifdef VERBOSE
    if (m_verbose) {
        LOG(INFO) << "Send request to service " << service;
        request->dump ();
    }
#endif

    request->send (*m_client);
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
    stat = err.num();
  }

  return stat;
}


//  ---------------------------------------------------------------------
//  Send request to broker
//  Takes ownership of request message and destroys it when sent.
//  The send method now just sends one message, without waiting for a
//  reply. Since we're using a DEALER socket we have to send an empty
//  frame at the start, to create the same envelope that the REQ socket
//  would normally make for us
int
mdcli::send (std::string& service, zmsg *&request_p, ChannelType chan)
{
   bool status = true;

   assert (request_p);
   zmsg *request = request_p;

   switch(chan)
   {
     case PERSISTENT:
       //  Prefix request with protocol frames
       //  Frame 0: empty (REQ emulation)
       //  Frame 1: "MDPCxy" (six bytes, MDP/Client x.y)
       //  Frame 2: Service name (printable string)
       request->push_front (const_cast<char*>(service.c_str())); // frame 2
       request->push_front (const_cast<char*>(MDPC_CLIENT));     // frame 1
       request->push_front (const_cast<char*>(""));              // frame 0

       LOG(INFO) << "client-api: before calling send";
       request->send (*m_client);
       LOG(INFO) << "client-api: after calling send";
       break;

     case DIRECT:
       // Если Служба известна, и у нее есть точка подключения, проверить -
       //   было ли подключение к прямому соединению с данной Службой.
       //   Если было - отправить сообщение на ранее сохраненный прямой сокет.
       //   Если нет - подключить клиентский сокет к точке подключения данной Службы
       request->push_front (const_cast<char*>(""));
       LOG(INFO) << "client-api: before calling direct send";
       status = send_direct (service, request);
       LOG(INFO) << "client-api: after calling direct send";
       break;

     default:
       LOG(ERROR) << "Unsupported sending type: " << chan;
       status = false;
   }

   // TODO: что возвращать после отправки сообщения?
   return status;
}


//  ---------------------------------------------------------------------
//  Fill the reply message or NULL if there was no reply. Does not
//  attempt to recover from a broker failure, this is not possible
//  without storing all unanswered requests and resending them all...
//  Возвращает код ошибки - завершение по таймауту, фатальная ошибка, или т.п.
//   0 - ничего не получено за время таймаута
//   1 - получено сообщение
//  -1 - завершить работу
int 
mdcli::recv (zmsg* &msg)
{
  int status = RECEIVE_OK; // все в порядке
  int mdpc_command;

  assert(&msg);
  msg = NULL;

  try
  {
    zmq::poll (m_socket_items, m_active_socket_num, m_timeout); // 2500 msec => 2.5 sec

/*    LOG(INFO) << "GEV: mdcli::recv #"<<m_active_socket_num
              << ": brok[0]=" <<m_client     << ": " << m_socket_items [BROKER_ITEM].revents
                                                  << " " << m_socket_items [BROKER_ITEM].fd
              << ", subs["<<m_subscriber_socket_index<<"]=" <<m_subscriber << ": " << m_socket_items [m_subscriber_socket_index].revents
                                                  << " " << m_socket_items [m_subscriber_socket_index].fd
              << ", peer["<<m_peer_socket_index<<"]="       <<m_peer       << ": " << m_socket_items [m_peer_socket_index].revents
                                                  << " " << m_socket_items [m_peer_socket_index].fd;*/

    // PERSISTENT-сообщение (через Брокер)
    if (m_socket_items[BROKER_ITEM].revents & ZMQ_POLLIN) {
        msg = new zmsg (*m_client);
#ifdef VERBOSE
        if (m_verbose) {
            LOG(INFO) << "received broker reply:";
            msg->dump ();
        }
#endif
        //  Don't try to handle errors, just assert noisily
        assert (msg->parts () >= 4);
        std::string empty = msg->pop_front ();
        assert (empty.length() == 0);  // empty message

        std::string header = msg->pop_front();
        assert (header.compare(MDPC_CLIENT) == 0);

        std::string command = msg->pop_front();
        // Возможные ответы: [1]REQUEST, [2]REPORT, [3]NAK
        mdpc_command = *command.c_str();
        if (mdpc_command && (mdpc_command <= (int)*MDPC_NAK))
        {
          LOG(INFO) << "Receive " << mdpc_commands[(int)*command.c_str()];
        }
        else
        {
          LOG(ERROR) << "Receive unknown command code: " << (int)mdpc_command;
        }
        // TODO: добавить фрейм КОМАНДА

        return status;     //  Success
    }

    // Проверить получение сообщения от публикатора подписки
    if (m_subscriber && (m_socket_items[m_subscriber_socket_index].revents & ZMQ_POLLIN)) {
        msg = new zmsg (*m_subscriber);
#ifdef VERBOSE
        if (m_verbose) {
            LOG(INFO) << "received subscriber message:";
            msg->dump ();
        }
#endif
        return status;     //  Success
    }

    // Проверить получение DIRECT-сообщения
    if (m_peer && (m_socket_items[m_peer_socket_index].revents & ZMQ_POLLIN)) {
        msg = new zmsg (*m_peer);
#ifdef VERBOSE
        if (m_verbose) {
            LOG(INFO) << "received direct reply:";
            msg->dump ();
        }
#endif
        return status;     //  Success
    }

  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
    status = err.num(); // коды ошибок от 1 и далее, повторяют errno
  }

  if (interrupt_client)
  {
    LOG(WARNING) << "Interrupt received, killing client...";
    status = RECEIVE_FATAL;
  }
  else
  {
#ifdef VERBOSE
    if (m_verbose)
      LOG(WARNING) << "Permanent error, abandoning request";
#endif
    status = RECEIVE_ERROR;
  }

  return status;
}

int mdcli::send_direct(std::string& service_name, zmsg *&request)
{
  int status = 0;
  int hwm = 100;
  int send_timeout_msec = SEND_TIMEOUT_MSEC; // 1 sec
  int recv_timeout_msec = RECV_TIMEOUT_MSEC; // 3 sec
  ServiceInfo* info;

  if (!service_info_by_name(service_name.c_str(), info))
  {
    // Информацию по такой Службе не запрашивали
    LOG(ERROR) << "Service " << service_name << " not associated";
    // NB: Возможно, стоит сделать это сейчас?
    return status;
  }

  // Информация по Службе найдена
  LOG(INFO) << "Service " << service_name
            << " located, connected: " << info->connected
            << ", endpoint: " << info->endpoint_external;

  try
  {
    if (!info->connected)
    {
      // Подключиться к Службе сейчас
      // TODO: Проверить возможность использования уже существующего сокета
      // m_client типа DEALER, используемого для связи с Брокером.
      // Клиент - ZMQ_DEALER, Служба - ZMQ_ROUTER.
      if (!m_peer)
      {
        // Сокета для прямых подключений еще не было, создадим его один раз
        m_peer = new zmq::socket_t(*m_context, ZMQ_DEALER);

        // generate random identity
        srandom((unsigned) time(0));
        char identity[10] = {};
        sprintf(identity, "%04X-%04X", within(0x10000), within(0x10000));
        LOG(INFO) << "Created DIRECT messaging socket with identity " << identity;
        //printf("new DIRECT messaging socket %s\n", identity);

        m_peer->setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
        m_peer->setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
        m_peer->setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
        m_peer->setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
        m_peer->setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));
      }
      m_peer->connect(info->endpoint_external);

      // Добавить в пул сокетов для опроса zmq::pool новый direct-сокет
      add_socket_to_pool(m_peer, SERVICE_ITEM);

      // все в порядке => выставим признак "сокет подключен"
      info->connected = 1;
      LOG(INFO) << "Associating with service " << service_name << " is successful";
    }

#ifdef VERBOSE
    if (m_verbose) {
        LOG(INFO) << "Send direct request to service " << service_name;
        request->dump ();
    }
#endif
    request->send (*m_peer);

    status = 1;
  }
  catch(zmq::error_t& err)
  {
    LOG(ERROR) << "Direct connecting to " << service_name << ": " << err.what();
  }

  return status;
}

void mdcli::add_socket_to_pool(zmq::socket_t* socket, int socket_type)
{
  // по умолчанию ссылаться на первую свободную ячейку в пуле
  int use_index = m_active_socket_num;
  // Корректен ли переданный в функцию тип сокета
  bool type_is_correct = true;

  switch(socket_type)
  {
    case SUBSCRIBER_ITEM:
      // Был ли уже создан сокет подписки?
      if (m_subscriber_socket_index)
      { // Да, его индекс не нулевой, используем ранее определенный индекс
        use_index = m_subscriber_socket_index;
        LOG(INFO) << "Update SUBSCRIBER "<<socket<<" at " << m_subscriber_socket_index;
      }
      else
      {
        m_subscriber_socket_index = m_active_socket_num++;
        LOG(INFO) << "Register new SUBSCRIBER "<<socket<<" at " << m_subscriber_socket_index;
      }
      break;

    case SERVICE_ITEM:
      // Был ли уже создан сокет связи с Клиентами?
      if (m_peer_socket_index)
      { // Да, его индекс не нулевой, используем ранее определенный индекс
        use_index = m_peer_socket_index;
        LOG(INFO) << "Update SERVICE "<<socket<<" at " << m_peer_socket_index;
      }
      else
      {
        m_peer_socket_index = m_active_socket_num++; // Запомним новый индекс
        LOG(INFO) << "Register new SERVICE "<<socket<<" at " << m_peer_socket_index;
      }
      break;

    default:
      LOG(ERROR) << "Unsupported socket type: " << socket_type;
      type_is_correct = false;
  }

  if (type_is_correct)
  {
    // заполним ячейку пула сокетов
    m_socket_items[use_index].socket = (void*)*socket;// NB: оператор (void*) обязателен!
    // zmq::socket_t::operator (void*) предоставляет доступ к внутреннему представлению zmq_socket_t
    m_socket_items[use_index].fd = 0;
    m_socket_items[use_index].events = ZMQ_POLLIN;
    m_socket_items[use_index].revents = 0;
  }

  if (m_active_socket_num > SOCKET_MAX_COUNT)
    LOG(ERROR) << "Socket pool is too small: " << m_active_socket_num
               << ">" << SOCKET_MAX_COUNT;
}
