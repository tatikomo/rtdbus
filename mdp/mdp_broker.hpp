#if !defined GEV_MDP_BROKER_HPP
#define GEV_MDP_BROKER_HPP
#pragma once

#include <map>
#include <list>
#include <string>

#include <zmq.hpp>
#include "zmsg.hpp"

#include "mdp_broker.hpp"

//#include "xdb_database_broker.hpp"
class XDBDatabaseBroker;
class Service;
class Worker;
class Letter;

//
//  This defines a single broker
class Broker {
 public:

   //  ---------------------------------------------------------------------
   //  Constructor for broker object
   Broker (int verbose);

   //  ---------------------------------------------------------------------
   //  Destructor for broker object
   virtual
   ~Broker ();

   bool
   Init();
   //  ---------------------------------------------------------------------
   //  Bind broker to endpoint, can call this multiple times
   //  We use a single socket for both clients and workers.
   bool
   bind (const std::string& endpoint);

   //  ---------------------------------------------------------------------
   //  Delete any idle workers that haven't pinged us in a while.
   void
   purge_workers ();

   // Регистрировать новый экземпляр Обработчика для Сервиса
   //  ---------------------------------------------------------------------
   Worker *
   worker_register(const std::string&, const std::string&);

   //  ---------------------------------------------------------------------
   //  Locate or create new service entry
   Service *
   service_require (const std::string& name);

   zmq::socket_t&
   get_socket() { return *m_socket; };

   //  ---------------------------------------------------------------------
   //  Dispatch requests to waiting workers as possible
   void
   service_dispatch (Service *srv/*, zmsg *msg*/);

   //  ---------------------------------------------------------------------
   //  Handle internal service according to 8/MMI specification
   void
   service_internal (const std::string& service_name, zmsg *msg);

   //  ---------------------------------------------------------------------
   //  Creates worker if necessary
   Worker *
   worker_require (const std::string& sender/*, char* identity*/);

   //  ---------------------------------------------------------------------
   //  Deletes worker from all data structures, and destroys worker
   void
   worker_delete (Worker *&wrk, int disconnect);


   //  ---------------------------------------------------------------------
   //  Processes one READY, REPORT, HEARTBEAT or  DISCONNECT message 
   //  sent to the broker by a worker
   void
   worker_msg (const std::string& sender, zmsg *msg);

   //  ---------------------------------------------------------------------
   //  Send message to worker
   //  If pointer to message is provided, sends that message
   void
   worker_send (Worker*, const char*, const std::string&, zmsg *msg);
   void
   worker_send (Worker*, const char*, const std::string&, Letter*);


   //  ---------------------------------------------------------------------
   //  This worker is now waiting for work
   void
   worker_waiting (Worker *worker);


   //  ---------------------------------------------------------------------
   //  Process a request coming from a client
   void
   client_msg (const std::string& sender, zmsg *msg);

   //  Get and process messages forever or until interrupted
   void
   start_brokering();

   void
   database_snapshot(const char*);

 private:
    zmq::context_t  * m_context;               //  0MQ context
    zmq::socket_t   * m_socket;                //  Socket for clients & workers
    int               m_verbose;               //  Print activity to stdout
    std::string       m_endpoint;              //  Broker binds to this endpoint
    int64_t           m_heartbeat_at;          //  When to send HEARTBEAT
//    std::map<std::string, Service*> m_services;//  Hash of known services
//    std::map<std::string, Worker*>  m_workers; //  Hash of known workers
//    std::vector<Worker*>            m_waiting; //  List of waiting workers

    /* 
     * Назначение: Локальная база данных в разделяемой памяти.
     * Доступ: монопольное использование экземпляром Брокера
     * Содержание: список Сервисов и их Обработчиков
     */
    XDBDatabaseBroker *m_database;

};

#endif
