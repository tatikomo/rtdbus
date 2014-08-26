#pragma once
#ifndef MDP_SERVICE_HPP
#define MDP_SERVICE_HPP

#include <vector>
#include <string>
#include <stdint.h>

#include "zmsg.hpp"

namespace mdp {

class Service;
class Broker;

//  This defines one worker, idle or active
class Worker
{
 public:
   friend class Broker;
   //  ---------------------------------------------------------------------
   //  Destroy worker object, called when worker is removed from
   //  broker->workers.
   virtual
   ~Worker ();

   //  ---------------------------------------------------------------------
   //  Return 1 if worker has expired and must be deleted
   bool
   expired ();
   
   const std::string& 
   get_identity();

 private:
   Broker      * m_broker;     //  Broker instance
   std::string   m_identity;   //  Identity of worker
//   std::string   m_address;    //  Address to route to
   Service     * m_service;    //  Owning service, if known
   int64_t       m_expiry;     //  Expires at unless heartbeat

   //  ---------------------------------------------------------------------
   //  Constructor is private, only used from broker
   Worker(const char* identity, Service * service = 0, int64_t expiry = 0);
   Worker(Broker* broker, std::string& identity);
};

//  This defines a single Service
class Service
{
 public:
   friend class Broker;

   //  ---------------------------------------------------------------------
   //  Destroy Service object, called when Service is removed from
   //  broker->services.
   virtual
   ~Service ();

   void
    destroy (void *argument);

   //  The dispatch method sends request to the worker.
   void
    dispatch ();

   void
    attach_worker(Worker*);

   void
    enable_command (const std::string& command);

   void
    disable_command (const std::string& command);
    
   bool
    is_command_enabled (const std::string& command);

 private:
    Broker              * m_broker;   //  Broker instance
    std::string           m_name;     //  Service name
    std::vector<zmsg*>    m_requests; //  List of client requests
    std::vector<Worker*>  m_waiting;  //  List of waiting workers
    //  List of blocked commands after [enable|disable]_command()
    std::vector<std::string>  m_blacklist;
    size_t                m_workers;  //  How many workers we have

    Service(std::string& name);
};

}; //namespace mdp

#endif
