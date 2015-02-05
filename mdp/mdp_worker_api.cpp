#include <glog/logging.h>
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
// NB: Это дубликат структуры, хранящийся в xdb_impl_db_broker.cpp
// TODO: вывести соответствия между Службой и её точкой входа в общую конфигурацию
ServiceEndpoint_t Endpoints[] = {
    {"BROKER","tcp://lo:5555", ""}, // Сам Брокер (BROKER_ENDPOINT_IDX)
    {"SINF",  "tcp://lo:5556", ""}, // Информационный сервер БДРВ
    {"IHM",   "tcp://lo:5557", ""}, // Сервер отображения
    {"ECH",   "tcp://lo:5558", ""}, // Сервер обменов
    {"", "", ""}  // Последняя запись
};

//  ---------------------------------------------------------------------
//  Signal handling
//
//  Call s_catch_signals() in your application at startup, and then exit
//  your main loop if interrupt_worker is ever 1. Works especially well with
//  zmq_poll.
int interrupt_worker = 0;

static void s_signal_handler (int signal_value)
{
    interrupt_worker = 1;
    LOG(INFO) << "Got signal "<<signal_value;
}

static void s_catch_signals ()
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}

mdwrk::mdwrk (std::string broker, std::string service, int verbose) :
  m_broker(broker),
  m_service(service),
  m_welcome_endpoint(NULL),
  m_context(NULL),
  m_worker(0),
  m_welcome(0),
  m_verbose(verbose),
  m_heartbeat(HeartbeatInterval), //  msecs
  m_reconnect(HeartbeatInterval), //  msecs
  m_expect_reply(false)
{
    /* NB
     * Отличия в версиях 2.1       3.2
     * ZMQ_RECVMORE:     int64 ->  int
     *
     * смотри https://github.com/zeromq/CZMQ/blob/master/sockopts.xml
     */
    s_version_assert (3, 2);

    m_context = new zmq::context_t (1);
    s_catch_signals ();

    // Обнулим хранище данных сокетов для zmq::poll
    // Заполняется хранилище в функциях connect_to_*
    memset (static_cast<void*>(m_socket_items), '\0', sizeof(zmq::pollitem_t) * SOCKET_COUNT);

    // Получить ссылку на статически выделенную строку с параметрами подключения
    m_welcome_endpoint = getEndpoint();

    connect_to_broker ();
    connect_to_world ();
}

//  ---------------------------------------------------------------------
//  Destructor
mdwrk::~mdwrk ()
{
    LOG(INFO) << "Worker destructor";

    send_to_broker (MDPW_DISCONNECT, NULL, NULL);

    // NB: удалять m_welcome_endpoint не нужно

    delete m_worker;
    delete m_welcome;
    delete m_context;
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

    if (m_verbose) {
        LOG(INFO) << "Sending " << mdpw_commands [(int) *command] << " to broker";
        msg->dump ();
    }
    msg->send (*m_worker);
    delete msg;
}

//  ---------------------------------------------------------------------
//  Connect or reconnect to broker
void mdwrk::connect_to_broker ()
{
    int linger = 0;

    if (m_worker) {
        // TODO: Пересоздание сокета влечет за собой ошибку zmq-рантайма 
        // "Socket operation on non-socket" при выполнении zmq::poll
        LOG(INFO) << "connect_to_broker() => delete old m_worker";
        delete m_worker;
    }
    m_worker = new zmq::socket_t (*m_context, ZMQ_DEALER);
    m_worker->setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
    // GEV 22/01/2015: ZMQ_IDENTITY добавлено для теста
    m_worker->setsockopt (ZMQ_IDENTITY, m_service.c_str(), m_service.size());
    m_worker->connect (m_broker.c_str());
    // Инициализация записей для zmq::poll для Брокера
    m_socket_items[BROKER_ITEM].socket = *m_worker;
    m_socket_items[BROKER_ITEM].fd = 0;
    m_socket_items[BROKER_ITEM].events = ZMQ_POLLIN;
    m_socket_items[BROKER_ITEM].revents = 0;

    if (m_verbose)
        LOG(INFO) << "Connecting to broker " << m_broker;

    // Register service with broker
    // Внесены изменения из-за необходимости передачи значения точки подключения 
    zmsg *msg = new zmsg ();
    msg->push_front ((char*)m_welcome_endpoint);
    msg->push_front ((char*)m_service.c_str());
    send_to_broker((char*)MDPW_READY, NULL, msg);

    delete msg;

    // If liveness hits zero, queue is considered disconnected
    update_heartbeat_sign();
    LOG(INFO) << "Next HEARTBEAT will be in " << m_heartbeat << " msec, m_liveness=" << m_liveness;
}

void mdwrk::ask_endpoint ()
{
  LOG(INFO) << "Ask endpoint for '" << m_service << "'";
}

//  ---------------------------------------------------------------------
//  Connect or reconnect to world
void mdwrk::connect_to_world ()
{
  int linger = 0;

  try
  {
    if (m_welcome) {
        delete m_welcome;
    }

    // Сокет типа "Ответ", процесс играет роль сервера, получая запросы
    m_welcome = new zmq::socket_t (*m_context, ZMQ_REP);
    m_welcome->setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
    // ===================================================================================
    // TODO: Каждый Сервис должен иметь уникальную строку подключения
    // Предложение: таблицу связей между Сервисами и их строками подключения читает Брокер,
    // после чего каждый Обработчик в ответе на свое первое сообщение Брокеру (READY) 
    // получает нужную строку. Эту же строку получают все Клиенты, заинтересованные 
    // в прямой передаче данных между ними и Обработчиками
    // ===================================================================================
    // 
    if(m_welcome_endpoint)
    {
      m_welcome->bind (m_welcome_endpoint);
      // Инициализация записей для zmq::poll для общей точки входа
      m_socket_items[WORLD_ITEM].socket = *m_welcome;
      m_socket_items[WORLD_ITEM].fd = 0;
      m_socket_items[WORLD_ITEM].events = ZMQ_POLLIN;
      m_socket_items[WORLD_ITEM].revents = 0;

      LOG(INFO) << "Connecting to world at " << m_welcome_endpoint;
    }
    else
    {
      LOG(ERROR) << "Can't find endpoint for unknown service " << m_service;
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
    m_heartbeat = heartbeat;
}


//  ---------------------------------------------------------------------
//  Set reconnect delay
void
mdwrk::set_reconnect (int reconnect)
{
    m_reconnect = reconnect;
}

//  ---------------------------------------------------------------------
//  Send reply, if any, to broker and wait for next request.
//  If reply is not NULL, a pointer to client's address is filled in.
zmsg *
mdwrk::recv (std::string *&reply)
{
  //  Format and send the reply if we were provided one
  assert (reply || !m_expect_reply);

  try
  {
    m_expect_reply = true;
    while (!interrupt_worker)
    {
        // NB: До реализации m_welcome пока будет 1 сокет
        zmq::poll (m_socket_items, SOCKET_COUNT, m_heartbeat);

        if (m_socket_items[BROKER_ITEM].revents & ZMQ_POLLIN) {
            zmsg *msg = new zmsg(*m_worker);
            if (m_verbose) {
                LOG(INFO) << "New message from broker:";
                msg->dump ();
            }
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
        }
        else if (m_socket_items[WORLD_ITEM].revents & ZMQ_POLLIN) // Событие на общем сокете
        {
            zmsg *msg = new zmsg(*m_worker);
//            if (m_verbose) {
                LOG(INFO) << "New message from world:";
                msg->dump ();
//            }
            delete msg;
        }
        else // Ожидание нового запроса завершено по таймауту
        if (--m_liveness == 0) {
            LOG(INFO) << "timeout, last HEARTBEAT was planned "
                      << s_clock() - m_heartbeat_at << " sec ago, m_liveness=" << m_liveness;
            if (m_verbose) {
                LOG(WARNING) << "Disconnected from broker - retrying...";
            }
            // TODO: в этот период Служба недоступна. Уточнить таймауты!
            // см. запись в README от 05.02.2015
            s_sleep (m_reconnect);
            // После подключения к Брокеру сокет связи с ним был пересоздан
            connect_to_broker ();
        }

        if (s_clock () > m_heartbeat_at) {
            send_to_broker ((char*)MDPW_HEARTBEAT, NULL, NULL);
            LOG(INFO) << "update m_heartbeat_at, new HEARTBEAT will be in  "
                      << s_clock () - m_heartbeat_at << ", m_liveness = " << m_liveness;
            update_heartbeat_sign();
        }
    }
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
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
  m_heartbeat_at = s_clock () + m_heartbeat;
  // Сбросить счетчик таймаутов
  // При достижении им 0 соединение с Брокером пересоздается
  m_liveness = HEARTBEAT_LIVENESS;
}

// Получить точку подключения для указанного Сервиса
const char* mdwrk::getEndpoint() const
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
          endpoint = Endpoints[entry_idx].endpoint_given;   // Да
        else
          endpoint = Endpoints[entry_idx].endpoint_default; // Нет

        break; // Завершаем поиск, т.к. названия Служб уникальны
    }
  }

  return endpoint;
}

