#include "glog/logging.h"

#include "mdp_common.h"
#include "mdp_broker.hpp"
#include "mdp_zmsg.hpp"
#include "xdb_broker.hpp"
#include "xdb_broker_service.hpp"
#include "xdb_broker_worker.hpp"
#include "xdb_broker_letter.hpp"
#include "xdb_impl_db_broker.hpp"

using namespace mdp;

//  ---------------------------------------------------------------------
//  Signal handling
//
//  Call s_catch_signals() in your application at startup, and then exit
//  your main loop if s_interrupted is ever 1. Works especially well with
//  zmq_poll.
int s_interrupted = 0;
void s_signal_handler (int /*signal_value*/)
{
    s_interrupted = 1;
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

//  ---------------------------------------------------------------------
//  Constructor for broker object
Broker::Broker (bool verbose) :
  m_context(NULL),
  m_socket(NULL),
  m_verbose(verbose),
  m_database(NULL)
{
    //  Initialize broker state
    m_context = new zmq::context_t(1);
    m_socket = new zmq::socket_t(*m_context, ZMQ_ROUTER);
    // TODO: объединить значения интервалов Брокера и Обработчика
    m_heartbeat_at = s_clock () + Broker::HeartbeatInterval;
    m_database = new xdb::DatabaseBroker();
}

bool Broker::Init()
{
  assert(m_database);
  return (m_database->Connect());
}

//  ---------------------------------------------------------------------
//  Destructor for broker object
Broker::~Broker ()
{
    m_database->ClearServices();
    //m_workers.clear();
    //m_waiting.clear();
    delete m_database;
    delete m_socket;
    delete m_context;
}

//  ---------------------------------------------------------------------
//  Bind broker to endpoint, can call this multiple times
//  We use a single socket for both clients and workers.
bool
Broker::bind (const std::string& endpoint)
{
    bool status = false;

    m_endpoint = endpoint;
    try
    {
      m_socket->bind(m_endpoint.c_str());
      LOG(INFO) << "MDP Broker/0.2.0 is active at " << endpoint;
      status = true;
    }
    catch(zmq::error_t err)
    {
      LOG(ERROR) << "MDP Broker/0.2.0 unable bind to " << endpoint 
        << " [" <<  err.what() << "]";
      status = false;
    }

    return status;
}

// Регистрировать новый экземпляр Обработчика для Сервиса
//  ---------------------------------------------------------------------
xdb::Worker* 
Broker::worker_register(const std::string& service_name, const std::string& worker_identity)
{
  return m_database->PushWorkerForService(service_name, worker_identity);
//  m_workers.insert(std::make_pair(instance->GetIDENTITY(), instance));
}

void
Broker::database_snapshot(const char* msg)
{
  if (m_verbose)
    m_database->MakeSnapshot(msg);
}

//  ---------------------------------------------------------------------
//  Delete any idle workers that haven't pinged us in a while.
//  
//  TODO: необходимо единовременно получить список Обработчиков 
//  в состоянии ARMED для заданного Сервиса. PopWorker всегда будет 
//  возвращать один и тот же экземпляр Обработчика до тех пор, пока 
//  у него не сменится состояние (к примеру, после окончания срока годности)
//
void
Broker::purge_workers ()
{
  xdb::ServiceList   *sl = m_database->GetServiceList();
  xdb::Service       *service = sl->first();
  xdb::Worker        *wrk;
  int           srv_count;
  int           wrk_count;

  srv_count = 0;
  /* Пройти по списку Сервисов */
  while(NULL != service)
  {
    wrk_count = 0;
    /* NB: Пока список Обработчиков не реализован - удаляем по одному за раз */
/*    while (NULL != (*/wrk = m_database->PopWorker(service);/*))*/
    if (wrk)
    {
//        LOG(INFO) <<"Purge workers for "<<srv_count<<":"<<service->GetNAME()
//                  <<" => "<<wrk_count<<":"<<wrk->GetIDENTITY();
        if (wrk->Expired ()) 
        {
          if (m_verbose) 
          {
            LOG(INFO) << "Deleting expired worker: " << wrk->GetIDENTITY();
          }
          release (wrk, 0); // Обработчик не подавал признаков жизни
        }
        delete wrk;
        wrk_count++;
    }
    service = sl->next();
    srv_count++;
  }
}

//  ---------------------------------------------------------------------
//  Locate or create new service entry
xdb::Service *
Broker::service_require (const std::string& service_name)
{
    xdb::Service *acquired_service = NULL;

    assert (service_name.size() > 0);
    if (m_verbose) {
        LOG(INFO) << "Require service '" << service_name << "'";
    }

    if (NULL == (acquired_service = m_database->GetServiceByName(service_name.c_str())))
    {
        LOG(WARNING) << "Service '" << service_name << "' is not registered";
        acquired_service = m_database->AddService(service_name.c_str());
    }
    else if (m_verbose)
    {
        LOG(INFO) << "Service '" << service_name << "' already registered";
    }

    return acquired_service;
}

//  ---------------------------------------------------------------------
//  Dispatch requests to waiting workers as possible
void
Broker::service_dispatch (xdb::Service *srv, zmsg *processing_msg = NULL)
{
    xdb::Worker  *wrk = NULL;
    xdb::Letter  *letter = NULL;
    bool     status = false;
    // всегда проверять, есть ли в очереди ранее необслужанные запросы
    std::string header;
    std::string message_body;

    assert (srv);

    database_snapshot("SERVICE_DISPATCH.START");
    // Сообщение может отсутствовать
    if (processing_msg)
    {
      letter = new xdb::Letter(processing_msg);
      status = m_database->PushRequestToService(srv, letter);
      if (!status) 
        LOG(ERROR) << "Unable to push new letter "<<letter->GetID()
                   <<" for service '"<<srv->GetNAME()<<"'";
      delete letter;
    }

//    database_snapshot("PURGE_SERVICE_DISPATCH.START");
    /* Очистить список Обработчиков Сервиса от зомби */
    purge_workers ();
//    database_snapshot("PURGE_SERVICE_DISPATCH.STOP");

    /*
     * Продолжать обработку, пока
     *  Для Сервиса есть свободные Обработчики
     *  Для Сервиса есть необработанные разрешенные Сообщения
     *
     *   1. Выбрать Сообщение из базы, не удаляя его
     *   2. Послать Сообщение указанному Обработчику
     *   3. Удалить Сообщение только после успешной отправки
     */
    while (NULL != (wrk = m_database->PopWorker(srv)))
    {
      if (m_verbose)
        LOG(INFO) << "Pop worker '"<<wrk->GetIDENTITY()<<"'";

      if (NULL != (letter = m_database->GetWaitingLetter(srv)))
      {
//        database_snapshot("SEND_SERVICE_DISPATCH.START");
        if (m_verbose)
        {
          LOG(INFO) << "Pop letter id="<<letter->GetID()<<" reply '"
                    <<letter->GetREPLY_TO()<<"' state="<<letter->GetSTATE();
          letter->Dump();
        }
        // Передать ожидающую обслуживания команду выбранному Обработчику
        worker_send (wrk, (char*)MDPW_REQUEST, EMPTY_FRAME, letter);
        delete letter;
        // После отсылки Сообщения этот экземпляр Обработчика перешел 
        // в состояние OCCUPIED, нужно выбрать нового Обработчика.
//        database_snapshot("SEND_SERVICE_DISPATCH.STOP");
      }
      delete wrk;
      break;
    }
    database_snapshot("SERVICE_DISPATCH.STOP");
}

//  ---------------------------------------------------------------------
//  Handle internal service according to 8/MMI specification
void
Broker::service_internal (const std::string& service_name, zmsg *msg)
{
    if (service_name.compare("mmi.service") == 0) 
    {
        xdb::Service * srv = m_database->GetServiceByName(service_name);
        // Если у Сервиса есть активные Обработчики
        if (srv->GetSTATE() == xdb::Service::ACTIVATED)
        {
            msg->body_set("200");
        }
        else
        {
            msg->body_set("404");
        }
    }
    else
    // The filter service that can be used to manipulate
    // the command filter table.
    if (service_name.compare("mmi.filter") == 0) {
        std::string operation_frame = msg->pop_front();
        // GEV : как так?
        // ведь service_name уже прочитан и передан в качестве параметра? +++
        std::string service_frame   = msg->pop_front();
        std::string command_frame   = msg->pop_front();

        if (operation_frame.compare ("enable") == 0) {
//                xdb::Service *service = service_require (service_frame);
//                m_database->EnableServiceCommand (service, command_frame);
                m_database->EnableServiceCommand (service_frame, command_frame);
                msg->body_set("200");
        }
        else
        if (operation_frame.compare ("disable") == 0) {
//                xdb::Service *service = service_require (service_frame);
//                service->disable_command (command_frame);
                m_database->DisableServiceCommand (service_frame, command_frame);
                msg->body_set("200");
        }
        else
                msg->body_set("400");

        //  Add an empty frame; it will be replaced by the return code.
        msg->push_front((char*)EMPTY_FRAME);
    }
        else {
        msg->body_set("501");
    }

    //  Remove & save client return envelope and insert the
    //  protocol header and service name, then rewrap envelope.
    char* client = msg->unwrap();
    msg->wrap(MDPC_CLIENT, service_name.c_str());
    msg->push_front((char*)MDPC_REPORT);
    msg->push_front((char*)MDPC_CLIENT);
    msg->wrap (client, EMPTY_FRAME);
    msg->send (*m_socket);
    delete[] client;
}

//  ---------------------------------------------------------------------
//  Here is the implementation of the methods that work on a worker.
//  Lazy constructor that locates a worker by identity, or creates a new
//  worker if there is no worker already with that identity
xdb::Worker *
Broker::worker_require (const std::string& identity)
{
    xdb::Worker * instance = NULL;

    instance = m_database->GetWorkerByIdent(identity);

    if (NULL == instance && m_verbose)
    {
       LOG(WARNING) << "Unable to find worker " << identity;
    }

    return instance;
}

//  ---------------------------------------------------------------------
//  Deletes worker from all data structures, but do not destroy worker class
bool
Broker::release (xdb::Worker *&wrk, int disconnect)
{
  bool status;

  assert (wrk);
  if (disconnect) 
  {
    worker_send (wrk, (char*)MDPW_DISCONNECT, EMPTY_FRAME, (xdb::Letter*)NULL);
  }

  if (false == (status = m_database->RemoveWorker(wrk)))
    LOG(ERROR) << "Unable to remove worker '"<<wrk->GetIDENTITY()<<"'";

  if (xdb::Worker::DISARMED != wrk->GetSTATE())
  {
        LOG(ERROR) << "Worker "<<wrk->GetIDENTITY()
        <<" state should be DISARMED ("<<xdb::Worker::DISARMED
        <<", but it is ("<<wrk->GetSTATE()<<")";
  }

  // Экземпляр должен быть удален из памяти в вызывающем контексте
  //delete wrk;

  return status;
}

//  ---------------------------------------------------------------------
//  MDPW_READY processing sent to us by a worker
bool 
Broker::worker_process_READY(xdb::Worker*& worker,
                             const std::string& sender_identity,
                             zmsg *msg)
{
  bool status = false;
  xdb::Service *service = NULL;

  if (worker) /* Указанный обработчик уже известен */
  {
    /*  Not first command in session */
    // TODO обновить ему время регистрации
    status = release (worker, 1);
  }
  else /* Указанный Обработчик не зарегистрирован */
  {
    /* Проверка на служебную */
    if (sender_identity.size() >= 4  //  Reserved service name
     && sender_identity.find("mmi.") != std::string::npos)
    {
      status = release (worker, 1);
    }
    else /* команда не служебная, зарегистрировать НОВЫЙ ОБРАБОТЧИК */
    {
      // Attach worker to service and mark as idle
      // Получить название сервиса
      std::string service_name = msg->pop_front();

      // найти сервис по имени или создать новый
      // NB: Service creation is only possible on first workers call
      service = service_require(service_name);
      assert (service);
      if (!worker)
      {
        // Создать экземпляр Обработчика
        worker = new xdb::Worker(service->GetID(), sender_identity.c_str());
      }

      // Привязать нового Обработчика к обслуживаемому им Сервису
      worker_waiting(worker);
      service_dispatch(service);
      delete service;
      status = true; // TODO присвоить корректное значение
      if (m_verbose)
        LOG(INFO) << "Worker '" << sender_identity << "' created";
    }
  }

  return status;
}

bool 
Broker::worker_process_REPORT(xdb::Worker*& worker,
                              const std::string& sender_identity,
                              zmsg *msg)
{
  bool status = false;
  xdb::Service *service = NULL;
//  xdb::Letter  *letter = NULL;
//  xdb::Letter::State    letter_state;

  if (m_verbose)
    LOG(INFO) << "Get REPORT from '" << sender_identity << "' worker=" << worker;

  if (worker) 
  {
     service = m_database->GetServiceForWorker(worker);
     assert(service->GetID() == worker->GetSERVICE_ID());
     /*
      * Установить корректные значения для Сообщения и Обработчика:
      * 1. Удалить Letter из БД
      * 2. Установить для Worker-а состояние в ARMED
      */
//     letter_state = xdb::Letter::DONE_OK; /* или DONE_FAIL */
     if (true == (status = m_database->ReleaseLetterFromWorker(worker)))
     {
       if (m_verbose) LOG(INFO) << "Letter released from worker '"<<worker->GetIDENTITY()<<"'";
     }
     else
     {
       LOG(ERROR) << "Report received about fantom message processing"
        <<" [srv:"<<service->GetNAME()<<", wrk:"<<worker->GetIDENTITY()<<"]";
     }

     //  Remove & save client return envelope and insert the
     //  protocol header and service name, then rewrap envelope.
     char* client = msg->unwrap ();
     msg->push_front((char*)service->GetNAME());
     msg->push_front((char*)MDPC_REPORT);
     msg->push_front((char*)MDPC_CLIENT);
     msg->wrap (client, EMPTY_FRAME);
     msg->send (*m_socket);
     delete[] client;

     worker_waiting (worker);
     delete service;
  }
  else 
  {
     LOG(ERROR) << "Got report from NULL worker";
     status = false;
     //status = release (worker, 1);
  }

  return status;
}

bool 
Broker::worker_process_HEARTBEAT(xdb::Worker*& worker,
                                 const std::string& sender_identity,
                                 zmsg*)
{
  bool status = false;

  LOG(INFO) << "Get HEARTBEAT from '" << sender_identity << "'";
  if (worker)
  {
    // worker содержит идентификатор своего Сервиса
    if (false == (status = m_database->PushWorker(worker)))
    {
      LOG(ERROR) << "Unable to register worker " << worker->GetIDENTITY();
    }
    database_snapshot("HEARTBEAT");
    worker_waiting(worker);
  }
  else
  {
    LOG(ERROR) << "Get HEARTBEAT from unknown worker "<<sender_identity;
  }

  return status;
}

bool 
Broker::worker_process_DISCONNECT(xdb::Worker*& worker, const std::string& sender_identity, zmsg*)
{
  if (m_verbose) LOG(INFO) << "Get DISCONNECT from worker " << sender_identity;
  return release (worker, 0);
}

//  ---------------------------------------------------------------------
//  Message processing sent to us by a worker
//  NB: Удаляет обрабатываемое сообщение zmsg
void
Broker::worker_msg (const std::string& sender_identity, zmsg *msg)
{
  bool status = false;
  xdb::Worker  *wrk = NULL;
//  xdb::Service *service = NULL;

  assert (msg && msg->parts() >= 1);     //  At least, command
  assert (sender_identity.size() > 0);

  std::string command = msg->pop_front();
  /* Зарегистрирован ли Обработчик с данным identity? */
  wrk = m_database->GetWorkerByIdent(sender_identity);

  /*
   * TODO нужно заменить функцию worker_require()
   * Нельзя допускать неизвестных обработчиков, они все должны 
   * принадлежать своему Сервису. Значит, нужно узнать, к какому
   * Сервису принадлежит сообщение от данного Обработчика.
   */
  if (command.compare (MDPW_READY) == 0)
  {
    status = worker_process_READY(wrk, sender_identity, msg);
  }
  else
  {
    if (command.compare (MDPW_REPORT) == 0)
    {
      status = worker_process_REPORT(wrk, sender_identity, msg);
    }
    else 
    {
      if (command.compare (MDPW_HEARTBEAT) == 0)
      {
         status = worker_process_HEARTBEAT(wrk, sender_identity, msg);
      }
      else 
      {
        if (command.compare (MDPW_DISCONNECT) == 0)
        {
          status = worker_process_DISCONNECT(wrk, sender_identity, msg);
        }
        else
        {
          LOG(ERROR) << "Invalid input message "
                     << mdpw_commands [(int) *command.c_str()];
          msg->dump ();
        }
      }
    }
  }

  if (false == status)
  {
    if (wrk)
      LOG(ERROR) << "Processing message from worker " << wrk->GetIDENTITY();
    else
      LOG(ERROR) << "Processing message from unknown worker";
  }

  delete wrk;
  delete msg;
}

//  ---------------------------------------------------------------------
//  Send message to worker
//  If pointer to message is provided, sends that message
void
Broker::worker_send (xdb::Worker *worker,
    const char *command, const std::string& option, xdb::Letter *letter)
{
  // TODO: удалить сообщение из базы только после успешной передачи
  // NB: это может привести к параллельному исполнению 
  // команды двумя Обработчиками. Как бороться? И стоит ли?
  assert(worker);

  // Назначить это сообщение данному Обработчику:
  // 1. установить статус OCCUPIED данному Обработчику
  // 2. установить статус ASSIGNED данному Сообщению
  if (false == m_database->AssignLetterToWorker(worker, letter))
  {
    LOG(ERROR) << "Unable to assign message id="<<letter->GetID()
               <<" to worker '"<<worker->GetIDENTITY()<<"'";
  }
  else
  {
    try
    {
      zmsg *msg = new zmsg();
      //  Stack protocol envelope to start of message
      if (option.size()>0) {                 //  Optional frame after command
        msg->push_front ((char*)option.c_str());
      }

      msg->push_front(const_cast<std::string&>(letter->GetDATA()));
      msg->push_front(const_cast<std::string&>(letter->GetHEADER()));
      msg->push_front(const_cast<char*>(letter->GetREPLY_TO()));
      msg->push_front(const_cast<char*>(command));
      msg->push_front((char*)MDPW_WORKER); 
      //  Stack routing envelope to start of message
      msg->wrap(worker->GetIDENTITY(), EMPTY_FRAME);

      if (m_verbose)
      {
        LOG(INFO) << "Sending '" << mdpw_commands [(int) *command]
                << "' from '"<<letter->GetREPLY_TO()<<"' to worker '"
                << worker->GetIDENTITY() << "'";
        msg->dump ();
      }
      msg->send (*m_socket);
      delete msg;
    }
    catch(zmq::error_t err)
    {
      LOG(ERROR) << err.what();
    }
  }
}

void
Broker::worker_send (
    /* IN     */ xdb::Worker* worker, 
    /* IN     */ const char* command,
    /* IN     */ const std::string& option, 
    /* IN-OUT */ std::string& header,
    /* IN     */ std::string& body)
{
  if (m_verbose)
    LOG(INFO) << "Send message #"<<(int)command<<" to worker '"
              <<worker->GetIDENTITY()<<"'";
  zmsg *msg = new zmsg();
  //  Stack protocol envelope to start of message
  if (option.size()>0) {                 //  Optional frame after command
      msg->push_front ((char*)option.c_str());
  }

  msg->push_front(body);
  msg->push_front(header);
  msg->push_front (const_cast<char*>(command));
  msg->push_front ((char*)MDPW_WORKER); 
  //  Stack routing envelope to start of message
  msg->wrap(worker->GetIDENTITY(), EMPTY_FRAME);

  if (m_verbose) {
    LOG(INFO) << "Sending '" << mdpw_commands [(int) *command]
              << "' to worker '" << worker->GetIDENTITY() << "'";
    msg->dump ();
  }
  msg->send (*m_socket);

  // TODO стоит ли менять состояние Обработчика, ведь экземпляра Letter нет?
  delete msg;
}

//  ---------------------------------------------------------------------
//  Send message to worker
//  If pointer to message is provided, sends that message
void
Broker::worker_send (xdb::Worker *worker,
    const char *command, const std::string& option, zmsg *msg)
{
    msg = (msg ? new zmsg(*msg) : new zmsg ());

    //  Stack protocol envelope to start of message
    if (option.size()>0) {                 //  Optional frame after command
        msg->push_front ((char*)option.c_str());
    }
    msg->push_front (const_cast<char*>(command));
    msg->push_front ((char*)MDPW_WORKER);
    //  Stack routing envelope to start of message
    msg->wrap(worker->GetIDENTITY(), EMPTY_FRAME);

    if (m_verbose) {
        LOG(INFO) << "Sending '" << mdpw_commands [(int) *command]
            << "' to worker '" << worker->GetIDENTITY() << "'";
        msg->dump ();
    }
    msg->send (*m_socket);
    delete msg;
}

//  ---------------------------------------------------------------------
//  This worker is now waiting for work
// TODO: Move worker to the end of the waiting queue,
// so purge_workers() will only check old worker(s).
void
Broker::worker_waiting (xdb::Worker *worker)
{
    xdb::Service *service = NULL;
    assert (worker);
    //  Queue to broker and service waiting lists
    m_database->PushWorker(worker);
    service = m_database->GetServiceById(worker->GetSERVICE_ID());
    service_dispatch (service);
    delete service;
}


//  ---------------------------------------------------------------------
//  Process a request coming from a client
//  NB: Удаляет обрабатываемое сообщение zmsg
void
Broker::client_msg (const std::string& sender, zmsg *msg)
{
//    bool enabled = false;

    assert (msg && msg->parts () >= 2);     //  Service name + body
    std::string service_frame = msg->pop_front();
    xdb::Service *srv = service_require (service_frame);
    if (!srv) /* сервис неизвестен - выводим предупреждение */
    {
        LOG(ERROR) << "Request from client '"<<sender
            <<"' to unknown service '"<<service_frame<<"'";
    }
    else /* Сервис известен */
    {
      /* является служебным (mmi.*) или одним из внешних */
      /* Установить обратный адрес */
      msg->wrap (sender.c_str(), EMPTY_FRAME);
      if (service_frame.length() >= 4
      &&  service_frame.find("mmi.") != std::string::npos) {
          /* Запрос к служебному сервису */
          service_internal (service_frame, msg);
      }
      else /* запрос к внешнему сервису */
      {
        /* как минимум, содержит идентификатор Клиента */
        if (msg && msg->parts() >= 1)
        {
            std::string client_frame = msg->front ();
            // TODO: проверить доступность Службы, которой адресуется сообщение
            //[011]@006B8B4568                      - автоматически подставляется Брокером при приёме!
            //[000]                                 - заполняется клиентом
            //[006]MDPC0X                           - заполняется клиентом
            //[XXX]сериализованный RTDBM::Header    - заполняется клиентом
            //[XXX]сериализованное сообщение Клиента- заполняется клиентом

            /* внести команду в очередь и обработать её */
            service_dispatch (srv, msg);
            delete srv;
#if 0
#error "continue here!"

            else /*  Send a NAK message back to the client. */
            {
              LOG(INFO) << "Disabled command " << cmd_frame;
              msg->clear();
              msg->push_front ((char*) service_frame.c_str());
              msg->push_front ((char*) MDPC_NAK);
              msg->push_front ((char*) MDPC_CLIENT);
              msg->wrap(sender.c_str(), EMPTY_FRAME);
              msg->send (*m_socket);
            }
#endif
        }
      } /* запрос к внешнему сервису */
    }   /* запрос к известному сервису */
    delete msg;
}

//  Get and process messages forever or until interrupted
void
Broker::start_brokering()
{
   // Если инициализация не прошла, завершить работу
   s_interrupted = (Init() == true)? 0 : 1;

   while (!s_interrupted)
   {
       zmq::pollitem_t items [] = {
           { *m_socket,  0, ZMQ_POLLIN, 0 } };
       zmq::poll (items, 1, Broker::HeartbeatInterval);

       //  Process next input message, if any
       if (items [0].revents & ZMQ_POLLIN) {
           zmsg *msg = new zmsg(*m_socket);
           if (m_verbose) {
               LOG(INFO) <<  "Received message:";
               msg->dump ();
           }

           assert (msg->parts () >= 3);

           std::string sender = msg->pop_front ();
           assert(sender.empty () == 0);

           std::string empty = msg->pop_front (); //empty message
           assert(empty.empty () == 1);

           std::string header = msg->pop_front ();
           assert(header.empty () == 0);

           if (header.compare(MDPC_CLIENT) == 0)
           {
             client_msg (sender, msg);
           }
           else if (header.compare(MDPW_WORKER) == 0)
           {
             worker_msg (sender, msg);
           }
           else {
               LOG(ERROR) << "Invalid message:";
               msg->dump ();
               delete msg;
           }
       }
       //  Disconnect and delete any expired workers
           //  Send heartbeats to idle workers if needed
       if (s_clock () > m_heartbeat_at) {
           purge_workers ();
#if 0
           // TODO: можно не посылать HEARTBEAT если от службы 
           // было получено любое сообщение, датированное в пределах 
           // интервала опроса HEARTBEAT_INTERVAL
           for (std::vector<Worker*>::iterator it = m_waiting.begin();
                 it != m_waiting.end() && (*it)!=0; it++) {
               worker_send (*it, (char*)MDPW_HEARTBEAT, EMPTY_FRAME, NULL);
           }
           m_heartbeat_at = s_clock () + Broker::HeartbeatInterval;
#endif
       }
   }
}

