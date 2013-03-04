#if !defined GEV_MDP_SERVICE_HPP
#define GEV_MDP_SERVICE_HPP
#pragma once

//  We'd normally pull these from config data

#define HEARTBEAT_LIVENESS  3       //  3-5 is reasonable
#define HEARTBEAT_INTERVAL  2500    //  msecs
#define HEARTBEAT_EXPIRY    HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS

#include <vector>
#include <string>
#include <stdint.h>

#include "zmsg.hpp"

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

 private:
    Broker      * m_broker;     //  Broker instance
    std::string   m_identity;   //  Identity of worker
    std::string   m_address;    //  Address to route to
    Service     * m_service;    //  Owning service, if known
    int64_t       m_expiry;     //  Expires at unless heartbeat

    //  ---------------------------------------------------------------------
    //  Constructor is private, only used from broker
    Worker(std::string identity, Service * service = 0, int64_t expiry = 0);
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

#if 0
   void
    dispatch (Service *service);
#endif

   void
    enable_command (std::string& /*const char **/command);

   void
    disable_command (std::string& /*const char **/command);
    
   bool
    is_command_enabled (std::string& /*const char **/command);

 private:
    Broker              * m_broker;   //  Broker instance
    std::string           m_name;     //  Service name
    std::vector<zmsg*>    m_requests; //  List of client requests
    std::vector<Worker*>  m_waiting;  //  List of waiting workers
    //  List of blocked commands after [enable|disable]_command()
    std::vector<std::string>  m_blacklist;
    size_t                m_workers;  //  How many workers we have

    Service(std::string name);
};


#endif
