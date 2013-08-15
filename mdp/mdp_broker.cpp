#include <glog/logging.h>

#include "mdp_broker.hpp"
#include "zmsg.hpp"
#include "xdb_database_broker.hpp"
#include "xdb_database_service.hpp"
#include "xdb_database_worker.hpp"
#include "xdb_database_letter.hpp"

#include "mdp_common.h"

//  ---------------------------------------------------------------------
//  Signal handling
//
//  Call s_catch_signals() in your application at startup, and then exit
//  your main loop if s_interrupted is ever 1. Works especially well with
//  zmq_poll.
int s_interrupted = 0;
void s_signal_handler (int signal_value)
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
Broker::Broker (int verbose)
{
    //  Initialize broker state
    m_context = new zmq::context_t(1);
    m_socket = new zmq::socket_t(*m_context, ZMQ_ROUTER);
    m_verbose = verbose;
    m_heartbeat_at = s_clock () + HEARTBEAT_INTERVAL;
    m_database = new XDBDatabaseBroker();
    assert(m_database);
}

bool Broker::Init()
{
  assert(m_database);
  return m_database->Connect();
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
Worker* 
Broker::worker_register(const std::string& service_name, const std::string& worker_identity)
{
  return m_database->PushWorkerForService(service_name, worker_identity);
//  m_workers.insert(std::make_pair(instance->GetIDENTITY(), instance));
}

void
Broker::database_snapshot(const char* msg)
{
  m_database->MakeSnapshot(msg);
}

//  ---------------------------------------------------------------------
//  Delete any idle workers that haven't pinged us in a while.
void
Broker::purge_workers ()
{
#warning "Убери return!"
   return;
    Worker  *wrk = NULL;
    Service *service = NULL;
    /* TODO Пройти по списку Сервисов */
    service = NULL; // GEV временно для компиляции, убрать!
    while (NULL != (wrk = m_database->PopWorker(service)))
    {
        if (!wrk->Expired ()) {
        /* TODO continue так же завершит текущий цикл, 
         * но позволит проверить всех Обработчиков. 
         * Это бесполезно, если Обработчики хранятся в 
         * отсортированном по времени ожидания списке. */
            break;              //  Worker is alive, we're done here
        }
        if (m_verbose) {
            LOG(INFO) << "Deleting expired worker: " << wrk->GetIDENTITY();
        }
        worker_delete (wrk, 0);
    }
}

//  ---------------------------------------------------------------------
//  Locate or create new service entry
Service *
Broker::service_require (const std::string& service_name)
{
    Service *acquired_service = NULL;

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
Broker::service_dispatch (Service *srv/*, zmsg *processing_msg*/)
{
//    zmsg   *msg = NULL;
    Worker *wrk = NULL;
    Letter *letter = NULL;
//    bool    enabled = false;

    assert (srv);
    /* Очистить список Обработчиков Сервиса от зомби */
    purge_workers ();
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
      while (NULL != (letter = m_database->GetWaitingLetter(srv, wrk)))
      {
        /* передать ожидающую обслуживания команду выбранному 
         * Обработчику
         * TODO удалить команду в worker_send() из ожидания 
         * только после успешной передачи
         */
        worker_send (wrk, (char*)MDPW_REQUEST, EMPTY_FRAME, letter);
        //delete msg; // NB: удаляется в worker_send()
      }
      delete wrk;
    }
    delete srv;
}

//  ---------------------------------------------------------------------
//  Handle internal service according to 8/MMI specification
void
Broker::service_internal (const std::string& service_name, zmsg *msg)
{
    if (service_name.compare("mmi.service") == 0) 
    {
        Service * srv = m_database->GetServiceByName(service_name);
        // Если у Сервиса есть активные Обработчики
        if (m_database->GetServiceState(srv) == Service::ACTIVATED)
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
        ustring operation_frame = msg->pop_front();
        // GEV : как так?
        // ведь service_name уже прочитан и передан в качестве параметра? +++
        ustring service_frame   = msg->pop_front();
        ustring command_frame   = msg->pop_front();

        if (operation_frame.compare ("enable") == 0) {
//                Service *service = service_require (service_frame);
//                m_database->EnableServiceCommand (service, command_frame);
                m_database->EnableServiceCommand (service_frame, command_frame);
                msg->body_set("200");
        }
        else
        if (operation_frame.compare ("disable") == 0) {
//                Service *service = service_require (service_frame);
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
    delete client;
}

//  ---------------------------------------------------------------------
//  Here is the implementation of the methods that work on a worker.
//  Lazy constructor that locates a worker by identity, or creates a new
//  worker if there is no worker already with that identity
Worker *
Broker::worker_require (const std::string& identity)
{
    Worker * instance = NULL;

    if (NULL != (instance = m_database->GetWorkerByIdent(identity)))
    {
       LOG(INFO) << "Worker '" << identity << "' is registered";
    }
    else
    {
       LOG(WARNING) << "Unable to find worker " << identity;
/*       if (m_verbose)
       {
          LOG(INFO) << "Registering new worker instance:" << identity;
       }*/
    }

    return instance;
}

//  ---------------------------------------------------------------------
//  Deletes worker from all data structures, and destroys worker
void
Broker::worker_delete (Worker *&wrk, int disconnect)
{
    assert (wrk);
    if (disconnect) {
        worker_send (wrk, (char*)MDPW_DISCONNECT, EMPTY_FRAME, (Letter*)NULL);
    }

    if (true == m_database->RemoveWorker(wrk))
      delete wrk;
}

//  ---------------------------------------------------------------------
//  Message processing sent to us by a worker
//  NB: Удаляет обрабатываемое сообщение zmsg
void
Broker::worker_msg (const std::string& sender_identity, zmsg *msg)
{
    bool status = false;
    Worker  *wrk = NULL;
    Service *service = NULL;

    assert (msg && msg->parts() >= 1);     //  At least, command
    assert (sender_identity.size() > 0);

    std::string command = msg->pop_front();
    /* Зарегистрирован ли Обработчик с данным identity? */
    wrk = m_database->GetWorkerByIdent(sender_identity);
    bool worker_ready = (NULL != wrk);

    /*
     * TODO нужно заменить функцию worker_require()
     * Нельзя допускать неизвестных обработчиков, они все должны 
     * принаждлежать своему Сервису. Значит, нужно узнать, к какому
     * Сервису принадлежит сообщение от данного Обработчика.
     */
    if (command.compare (MDPW_READY) == 0) {
        if (worker_ready)  {              //  Not first command in session
            worker_delete (wrk, 1);
        }
        else {
            if (sender_identity.size() >= 4  //  Reserved service name
             && sender_identity.find_first_of("mmi.") == 0) {
                worker_delete (wrk, 1);
            }
            else {
                // Attach worker to service and mark as idle
                // Получить название сервиса
                std::string service_name = msg->pop_front();

                // найти сервис по имени или создать новый
// GEV: "Service creation is only possible on first workers call"                
#if 1
                service = service_require/*m_database->RequireServiceByName*/ (service_name);
                assert (service);
#endif
                if (!worker_ready)
                {
                  // Создать экземпляр Обработчика
                  wrk = new Worker(sender_identity.c_str(), service->GetID());
                }

                // Привязать нового Обработчика к обслуживаемому им Сервису
                worker_waiting (wrk);
                service_dispatch(service);

                LOG(INFO) << "Created worker " << sender_identity;
            }
        }
    }
    else {
       if (command.compare (MDPW_REPORT) == 0) {
           LOG(INFO) << "Get REPORT from '" << sender_identity << "' wr=" << worker_ready;
           if (worker_ready) {
               service = m_database->GetServiceForWorker(wrk);
               //  Remove & save client return envelope and insert the
               //  protocol header and service name, then rewrap envelope.
               char* client = msg->unwrap ();
               msg->push_front((char*)service->GetNAME());
               msg->push_front((char*)MDPC_REPORT);
               msg->push_front((char*)MDPC_CLIENT);
               msg->wrap (client, EMPTY_FRAME);
               msg->send (*m_socket);
 //             delete client; // ай-яй-яй! +++
//+++++               worker_waiting (wrk);

               delete service;
           }
           else {
               worker_delete (wrk, 1);
           }
       }
       else {
          if (command.compare (MDPW_HEARTBEAT) == 0) {
              LOG(INFO) << "Get HEARTBEAT from '" << sender_identity << "' wr=" << worker_ready;
              if (worker_ready) {
                // wrk содержит идентификатор своего Сервиса
                  if (false == (status = m_database->PushWorker(wrk)))
                  {
                    LOG(ERROR) << "Unable to register worker " << wrk->GetIDENTITY();
                  }
                  m_database->MakeSnapshot("HEARTBEAT");
                  worker_waiting(wrk);
              }
              else {
//                  worker_delete (wrk, 1);
//                  GEV: 13/08/2013 - в этом месте wrk = NULL,
//                  и в worker_delete() происходит assertion
              }
          }
          else {
             if (command.compare (MDPW_DISCONNECT) == 0) {
                 LOG(INFO) << "Get DISCONNECT from " 
                           << sender_identity;
                 worker_delete (wrk, 0);
             }
             else {
                 LOG(ERROR) << "Invalid input message " 
                            << mdpw_commands [(int) *command.c_str()];
                 msg->dump ();
             }
          }
       }
    }
    delete msg;
}

//  ---------------------------------------------------------------------
//  Send message to worker
//  If pointer to message is provided, sends that message
void
Broker::worker_send (Worker *worker,
    const char *command, const std::string& option, Letter *letter)
{
  // TODO: удалить сообщение из базы только после успешной передачи
  // NB: это может привести к параллельному исполнению 
  // команды двумя Обработчиками. Как бороться? И стоит ли?
}

//  ---------------------------------------------------------------------
//  Send message to worker
//  If pointer to message is provided, sends that message
void
Broker::worker_send (Worker *worker,
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
Broker::worker_waiting (Worker *worker)
{
    Service *service = NULL;
    assert (worker);
#if 0
    m_waiting.push_back(worker);
    worker->m_service->m_waiting.push_back(worker);
    worker->m_expiry = s_clock () + HEARTBEAT_EXPIRY;
#else
    //  Queue to broker and service waiting lists
    m_database->PushWorker(worker);
#endif
    // +++ послать ответ на HEARTBEAT
//    NB: В версии zguide/C/mdbroker не вызывается worker_send
//  Версия worker_send(), работающая с Letter, на 13/08/2013 еще не реализована
    worker_send (worker, (char*)MDPW_HEARTBEAT, EMPTY_FRAME, /*(Letter*)*/(zmsg*)NULL);
    service = m_database->GetServiceById(worker->GetSERVICE_ID());
    service_dispatch (service);
    //delete service; - он удаляется в service_dispatch()
}


//  ---------------------------------------------------------------------
//  Process a request coming from a client
//  NB: Удаляет обрабатываемое сообщение zmsg
void
Broker::client_msg (const std::string& sender, zmsg *msg)
{
    bool enabled = false;

    assert (msg && msg->parts () >= 2);     //  Service name + body
//  LOG(INFO) << "Client_msg " << sender; // GEV delete me
    std::string service_frame = msg->pop_front();
    Service *srv = service_require (service_frame);
    if (!srv) /* сервис неизвестен - выводим предупреждение */
    {
        LOG(INFO) << "Unknown service " << service_frame;
    }
    else /* Сервис известен */
    {
      /* является служебным (mmi.*) или одним из внешних */
      /*  Установить обратный адрес */
      msg->wrap (sender.c_str(), EMPTY_FRAME);
      if (service_frame.length() >= 4
      &&  service_frame.find_first_of("mmi.") == 0) {
          /* Запрос к служебному сервису */
          service_internal (service_frame, msg);
      }
      else /* запрос к внешнему сервису */
      {
        /* как минимум, содержит название команды */
        if (msg && msg->parts() >= 1)
        {
            /* Если команда разрешена, исполнить её */
            /* NB: Только здесь читается не команда, а Получатель сообщения! */
            ustring cmd_frame = msg->front ();
            enabled = m_database->IsServiceCommandEnabled (srv, cmd_frame);

            if (true == enabled) {
              /* внести команду в очередь и обработать её */
              LOG(INFO) << "Command '" << cmd_frame << "' is enabled";
#warning "Make PushRequestToService() implementation"
//GEV              m_database->PushRequestToService(srv, msg);
              
              service_dispatch (srv/*, msg*/);
            }
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
       zmq::poll (items, 1, HEARTBEAT_INTERVAL);

       //  Process next input message, if any
       if (items [0].revents & ZMQ_POLLIN) {
           zmsg *msg = new zmsg(*m_socket);
           if (m_verbose) {
               LOG(INFO) <<  "Received message:";
               msg->dump ();
           }

           assert (msg->parts () >= 3);

           std::string sender = msg->pop_front ();
           assert(sender.size ());

           std::string empty = msg->pop_front (); //empty message
           assert(empty.size () == 0);

           std::string header = msg->pop_front ();
           assert(header.size ());

           if (header.compare(MDPC_CLIENT) == 0) {
               client_msg (sender, msg);
           }
           else if (header.compare(MDPW_WORKER) == 0) {
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
           m_heartbeat_at = s_clock () + HEARTBEAT_INTERVAL;
#endif
       }
   }
}

