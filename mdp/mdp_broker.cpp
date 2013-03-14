#include "mdp_common.h"
#include "mdp_service.hpp"
#include "mdp_broker.hpp"
#include "zmsg.hpp"

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
}

//  ---------------------------------------------------------------------
//  Destructor for broker object
Broker::~Broker ()
{
    m_services.clear();
    m_workers.clear();
    m_waiting.clear();
    delete m_socket;
    delete m_context;
}

//  ---------------------------------------------------------------------
//  Bind broker to endpoint, can call this multiple times
//  We use a single socket for both clients and workers.
void
Broker::bind (std::string endpoint)
{
    m_endpoint = endpoint;
    m_socket->bind(m_endpoint.c_str());
    s_console ("I: MDP Broker/0.2.0 is active at %s", endpoint.c_str());
}

//  ---------------------------------------------------------------------
//  Delete any idle workers that haven't pinged us in a while.
void
Broker::purge_workers ()
{
    Worker * wrk = m_waiting.size()>0 ? m_waiting.front() : 0;
    while (wrk) {
        if (!wrk->expired ()) {
            break;              //  Worker is alive, we're done here
        }
//        if (m_verbose) {
            s_console ("I: deleting expired worker: %s",
                  wrk->m_identity.c_str());
//        }
        worker_delete (wrk, 0);
        wrk = m_waiting.size()>0 ? m_waiting.front() : 0;
    }
}

//  ---------------------------------------------------------------------
//  Locate or create new service entry
Service *
Broker::service_require (std::string& name)
{
    assert (name.size() > 0);

    if (m_services.count(name)) {
        s_console("I: service '%s' is registered", name.c_str());
        return m_services.at(name);
    } else {
        s_console("W: service '%s' is not registered", name.c_str());
        Service * srv = new Service(name);
        m_services.insert(std::make_pair(name, srv));
        if (m_verbose) {
            s_console ("I: received message:");
        }
        return srv;
    }
}

//  ---------------------------------------------------------------------
//  Dispatch requests to waiting workers as possible
void
Broker::service_dispatch (Service *srv, zmsg *msg)
{
    assert (srv);
    if (msg) {                    //  Queue message if any
        srv->m_requests.push_back(msg);
    }

    purge_workers ();
    while (srv->m_waiting.size()>0 && srv->m_requests.size()>0)
    {
        Worker *wrk = srv->m_waiting.size() ? srv->m_waiting.front() : 0;
        srv->m_waiting.erase(srv->m_waiting.begin());
        zmsg *msg = srv->m_requests.size() ? srv->m_requests.front() : 0;
        srv->m_requests.erase(srv->m_requests.begin());
        worker_send (wrk, (char*)MDPW_REQUEST, "", msg);
        for(std::vector<Worker*>::iterator it = m_waiting.begin();
            it != m_waiting.end();
            it++) {
           if (*it == wrk) {
              it = m_waiting.erase(it)-1;
           }
        }

        delete msg;
    }
}

//  ---------------------------------------------------------------------
//  Handle internal service according to 8/MMI specification
void
Broker::service_internal (std::string service_name, zmsg *msg)
{
    if (service_name.compare("mmi.service") == 0) {
        Service * srv = m_services.at(msg->body());
        if (srv && srv->m_workers) {
            msg->body_set("200");
        } else {
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
                Service *service = service_require (service_frame);
                service->enable_command (command_frame);
                msg->body_set("200");
        }
        else
        if (operation_frame.compare ("disable") == 0) {
                Service *service = service_require (service_frame);
                service->disable_command (command_frame);
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
Broker::worker_require (std::string& sender/*, char *identity*/)
{
//    assert (identity != 0);
    Worker * instance = NULL;

    if (m_workers.count(sender/*identity*/)) {
       s_console("I: worker '%s' is registered", sender.c_str());
       instance = m_workers.at(sender/*identity*/);
    }
    else {
       instance = new Worker(""/*identity*/, this, sender);
       m_workers.insert(std::make_pair(sender/*identity*/, instance));
       if (m_verbose) {
          s_console ("I: registering new worker: %s", sender.c_str()/*identity*/);
       }
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
        worker_send (wrk, (char*)MDPW_DISCONNECT, "", NULL);
    }

    if (wrk->m_service)
    {
        for(std::vector<Worker*>::iterator it = wrk->m_service->m_waiting.begin();
            it != wrk->m_service->m_waiting.end();
            it++)
        {
           if (*it == wrk)
           {
              it = wrk->m_service->m_waiting.erase(it)-1;
           }
        }
        wrk->m_service->m_workers--;
    }

    for(std::vector<Worker*>::iterator it = m_waiting.begin();
        it != m_waiting.end();
        it++)
    {
       if (*it == wrk)
       {
          it = m_waiting.erase(it)-1;
       }
    }
    //  This implicitly calls the worker destructor
    m_workers.erase(wrk->m_identity);
    delete wrk;
}

//  ---------------------------------------------------------------------
//  Process message sent to us by a worker
void
Broker::worker_msg (std::string& sender, zmsg *msg)
{
    assert (msg && msg->parts() >= 1);     //  At least, command
    assert (sender.size() > 0);

    std::string command = msg->pop_front();
//    char *identity = msg->strhex (sender.c_str());
    bool worker_ready = m_workers.count(sender)>0;
    Worker *wrk = worker_require (sender/*, identity*/);
//    delete identity;

    if (command.compare (MDPW_READY) == 0) {
        if (worker_ready)  {              //  Not first command in session
            worker_delete (wrk, 1);
        }
        else {
            if (sender.size() >= 4  //  Reserved service name
             && sender.find_first_of("mmi.") == 0) {
                worker_delete (wrk, 1);
            } else {
                //  Attach worker to service and mark as idle
                std::string service_name = msg->pop_front();
                wrk->m_service = service_require (service_name);
                wrk->m_service->m_workers++;
                worker_waiting (wrk);
                service_dispatch(wrk->m_service, 0); // GEV недавно добавил
                zclock_log ("worker '%s' created", sender.c_str());
            }
        }
    } else {
       if (command.compare (MDPW_REPORT) == 0) {
           s_console("D: get REPORT from '%s' wr=%d",
                     sender.c_str(), worker_ready);
           if (worker_ready) {
               //  Remove & save client return envelope and insert the
               //  protocol header and service name, then rewrap envelope.
               char* client = msg->unwrap ();
               msg->push_front((char*)wrk->m_service->m_name.c_str());
               msg->push_front((char*)MDPC_REPORT);
               msg->push_front((char*)MDPC_CLIENT);
               msg->wrap (client, EMPTY_FRAME);
               msg->send (*m_socket);
 //             delete client; // ай-яй-яй! +++
               worker_waiting (wrk);
           }
           else {
               worker_delete (wrk, 1);
           }
       } else {
          if (command.compare (MDPW_HEARTBEAT) == 0) {
              s_console("D: get HEARTBEAT from '%s' wr=%d",
                        sender.c_str(), worker_ready);
              if (worker_ready) {
                  worker_waiting(wrk);
              } else {
                  worker_delete (wrk, 1);
              }
          } else {
             if (command.compare (MDPW_DISCONNECT) == 0) {
                 s_console("D: get DISCONNECT from '%s'",
                           sender.c_str());
                 worker_delete (wrk, 0);
             } else {
                 s_console ("E: invalid input message (%d)", 
                            (int) *command.c_str());
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
    char *command, std::string option, zmsg *msg)
{
    msg = (msg ? new zmsg(*msg) : new zmsg ());

    //  Stack protocol envelope to start of message
    if (option.size()>0) {                 //  Optional frame after command
        msg->push_front ((char*)option.c_str());
    }
    msg->push_front (command);
    msg->push_front ((char*)MDPW_WORKER);
    //  Stack routing envelope to start of message
    msg->wrap(worker->m_address.c_str(), EMPTY_FRAME);

    if (m_verbose) {
        s_console ("I: sending %s to worker (%s)",
            mdpw_commands [(int) *command], worker->m_address.c_str());
        msg->dump ();
    }
    msg->send (*m_socket);
}

//  ---------------------------------------------------------------------
//  This worker is now waiting for work
// TODO: Move worker to the end of the waiting queue,
// so purge_workers() will only check old worker(s) in m_waiting vector.
void
Broker::worker_waiting (Worker *worker)
{
    assert (worker);
    //  Queue to broker and service waiting lists
    m_waiting.push_back(worker);
    worker->m_service->m_waiting.push_back(worker);
    worker->m_expiry = s_clock () + HEARTBEAT_EXPIRY;
    // +++ послать ответ на HEARTBEAT
    worker_send (worker, (char*)MDPW_HEARTBEAT, EMPTY_FRAME, NULL);
    service_dispatch (worker->m_service, 0);
}


//  ---------------------------------------------------------------------
//  Process a request coming from a client
void
Broker::client_msg (std::string& sender, zmsg *msg)
{
    assert (msg && msg->parts () >= 2);     //  Service name + body
s_console("D: client_msg %s", sender.c_str()); // GEV delete me
    std::string service_frame = msg->pop_front();
    Service *srv = service_require (service_frame);
    if (!srv)
        s_console("W: service %s is not known", service_frame.c_str());
    //  Set reply return address to client sender
    msg->wrap (sender.c_str(), EMPTY_FRAME);
    if (service_frame.length() >= 4
    &&  service_frame.find_first_of("mmi.") == 0) {
        service_internal (service_frame, msg);
    } else {
        service_dispatch (srv, msg);
    }
}

//  Get and process messages forever or until interrupted
void
Broker::start_brokering()
{
   while (!s_interrupted)
   {
       zmq::pollitem_t items [] = {
           { *m_socket,  0, ZMQ_POLLIN, 0 } };
       zmq::poll (items, 1, HEARTBEAT_INTERVAL);

       //  Process next input message, if any
       if (items [0].revents & ZMQ_POLLIN) {
           zmsg *msg = new zmsg(*m_socket);
           if (m_verbose) {
               s_console ("I: received message:");
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
               s_console ("E: invalid message:");
               msg->dump ();
               delete msg;
           }
       }
       //  Disconnect and delete any expired workers
       //  Send heartbeats to idle workers if needed
       if (s_clock () > m_heartbeat_at) {
           purge_workers ();
           // TODO: можно не посылать HEARTBEAT если от службы 
           // было получено любое сообщение, датированное в пределах 
           // интервала опроса HEARTBEAT_INTERVAL
           for (std::vector<Worker*>::iterator it = m_waiting.begin();
                 it != m_waiting.end() && (*it)!=0; it++) {
               worker_send (*it, (char*)MDPW_HEARTBEAT, "", NULL);
           }
           m_heartbeat_at = s_clock () + HEARTBEAT_INTERVAL;
       }
   }
}

