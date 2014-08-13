#pragma once
#ifndef MDP_BROKER_HPP
#define MDP_BROKER_HPP

#include <map>
#include <list>
#include <string>

#include <zmq.hpp>

#include "config.h"
#include "mdp_zmsg.hpp"
#include "mdp_broker.hpp"
#include "xdb_broker.hpp"
#include "xdb_broker_service.hpp"
#include "xdb_broker_worker.hpp"
#include "xdb_broker_letter.hpp"

namespace mdp {

//
//  This defines a single broker
class Broker {
 public:
   static const int HeartbeatInterval = HEARTBEAT_PERIOD_MSEC;
   static const int HeartbeatExpiration = HEARTBEAT_PERIOD_MSEC * HEARTBEAT_LIVENESS;
   //  ---------------------------------------------------------------------
   //  Constructor for broker object
   Broker (bool verbose);

   //  ---------------------------------------------------------------------
   //  Destructor for broker object
   virtual
   ~Broker ();

/* 
 * Ненормальное состояние, вызывается из напрямую тестов, 
 * когда нельзя явно запускать start_brokering() 
 */
#if defined _FUNCTIONAL_TEST
   /* Инициализация внутренних структур, обычно вызывается из start_brokering() */
   bool Init();
   /* Доступ для тестирования к интерфейсу базы данных */
   xdb::DatabaseBroker* get_internal_db_api() { return m_database; }
#endif

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
   xdb::Worker *
   worker_register(const std::string&, const std::string&);

   //  ---------------------------------------------------------------------
   //  Locate or create new service entry
   xdb::Service *
   service_require (const std::string& name);

   zmq::socket_t&
   get_socket() { return *m_socket; };

   //  ---------------------------------------------------------------------
   //  Dispatch requests to waiting workers as possible
   void
   service_dispatch (xdb::Service *srv, zmsg *msg);

   //  ---------------------------------------------------------------------
   //  Handle internal service according to 8/MMI specification
   void
   service_internal (const std::string& service_name, zmsg *msg);

   //  ---------------------------------------------------------------------
   //  Creates worker if necessary
   xdb::Worker *
   worker_require (const std::string& sender/*, char* identity*/);

   //  ---------------------------------------------------------------------
   //  Deletes worker from all data structures, and destroys worker
   bool
   //worker_delete (xdb::Worker *&wrk, int disconnect); - старое название
   release (xdb::Worker *&wrk, int disconnect);


   //  ---------------------------------------------------------------------
   //  Processes one READY, REPORT, HEARTBEAT or  DISCONNECT message 
   //  sent to the broker by a worker
   void
   worker_msg (const std::string& sender, zmsg *msg);

   //  ---------------------------------------------------------------------
   //  Send message to worker
   //  If pointer to message is provided, sends that message
   void
   worker_send (xdb::Worker*, const char*, const std::string&, zmsg *msg);
   void
   worker_send (xdb::Worker*, const char*, const std::string&, xdb::Letter*);
   void
   worker_send (xdb::Worker*, const char*, const std::string&, std::string&, std::string&);


   //  ---------------------------------------------------------------------
   //  This worker is now waiting for work
   void
   worker_waiting (xdb::Worker *worker);


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
    DISALLOW_COPY_AND_ASSIGN(Broker);

    zmq::context_t  * m_context;               //  0MQ context
    zmq::socket_t   * m_socket;                //  Socket for clients & workers
    bool              m_verbose;               //  Print activity to stdout
    std::string       m_endpoint;              //  Broker binds to this endpoint
    int64_t           m_heartbeat_at;          //  When to send HEARTBEAT
//    std::map<std::string, xdb::Service*> m_services;//  Hash of known services
//    std::map<std::string, xdb::Worker*>  m_workers; //  Hash of known workers
//    std::vector<xdb::Worker*>            m_waiting; //  List of waiting workers

    /* 
     * Назначение: Локальная база данных в разделяемой памяти.
     * Доступ: монопольное использование экземпляром Брокера
     * Содержание: список Сервисов и их Обработчиков
     */
    xdb::DatabaseBroker *m_database;

    /* Обработка соответствующих команд, полученных от Обработчиков */
    bool worker_process_READY(xdb::Worker*&, const std::string&, zmsg*);
    bool worker_process_REPORT(xdb::Worker*&, const std::string&, zmsg*);
    bool worker_process_HEARTBEAT(xdb::Worker*&, const std::string&, zmsg*);
    bool worker_process_DISCONNECT(xdb::Worker*&, const std::string&, zmsg*);

#if !defined _FUNCTIONAL_TEST
   /* Инициализация внутренних структур, вызывается из start_brokering() */
   bool Init();
#endif
};

} //namespace mdp

#endif
