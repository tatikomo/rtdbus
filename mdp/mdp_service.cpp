#include <string>
#include "mdp_common.h"
#include "mdp_broker.hpp"
#include "mdp_service.hpp"

//  ---------------------------------------------------------------------
//  Destroy worker object, called when worker is removed from
//  broker->workers.
Worker::~Worker ()
{
}

//  ---------------------------------------------------------------------
//  Return 1 if worker has expired and must be deleted
bool
Worker::expired ()
{
   return m_expiry < s_clock ();
}

//  ---------------------------------------------------------------------
//  Constructor is private, only used from broker
Worker::Worker(const char* identity, Service * service, int64_t expiry)
{
   assert(identity != 0);
   m_identity = identity;
   m_service = service;
   m_expiry = expiry;
}

Worker::Worker(const char* identity, Broker* broker, std::string& address)
{
   assert(identity != 0);
   m_broker = broker;
   m_address = address;
   // скопировать identity, поскольку он однозначно идентифицирует экземпляр
   m_identity = address; // TODO: может удалить вообще address?
}

//  ---------------------------------------------------------------------
//  Destroy service object, called when service is removed from
//  broker->services.
Service::~Service ()
{
   for(size_t i = 0; i < m_requests.size(); i++) {
        delete m_requests[i];
   }
   m_requests.clear();

   for(size_t i = 0; i < m_waiting.size(); i++) {
        delete m_waiting[i];
   }
   m_waiting.clear();

   // Очистить список блокированных команд
   //  Free memory keeping  blacklisted commands.
   for(size_t i = 0; i < m_blacklist.size(); i++) {
        delete &m_blacklist[i];
   }
   m_blacklist.clear();
}

Service::Service(std::string& name)
{
   m_name = name;
}


//  The dispatch method sends request to the worker.
void
Service::dispatch ()
{
    //m_broker->purge_workers ();

    if (m_waiting.size() == 0)
      return;

    while (m_requests.size() > 0) {
        //worker_t *worker = (worker_t*)zlist_pop (self->waiting);
        Worker * wrk = m_waiting.size()>0 ? m_waiting.front() : 0;

        //zlist_remove (self->waiting, worker);
        m_broker->worker_delete(wrk, 0);

        //zmsg_t *msg = (zmsg_t*)zlist_pop (self->requests);
        zmsg *msg = m_requests.front();

        m_broker->worker_send (wrk, (char*)MDPW_REQUEST, NULL, msg);

        //  Workers are scheduled in the round-robin fashion
        //zlist_append (self->waiting, worker);
        m_waiting.push_back(wrk);
        delete msg;
    }
}

void
Service::enable_command (std::string& /*const char **/command)
{
    // попробуем найти полученную команду в списке заблокированных
    for(std::vector<std::string>::iterator it = m_blacklist.begin();
        it != m_blacklist.end();
        it++) {
           if (*it == command) {
               // удалим элемент, тем самым разрешив ранее заблокированную команду
               it = m_blacklist.erase(it)-1;
               break;
           }
    }
}

void
Service::disable_command (std::string& /*const char **/command)
{
    bool exist = false;

    // попробуем найти полученную команду в списке заблокированных
    for(std::vector<std::string>::iterator it = m_blacklist.begin();
        it != m_blacklist.end();
        it++) {
           if (*it == command) {
               exist = true;
               break;
           }
    }

    // в списке отсуствует -> добавим
    if (false == exist) {
        // GEV: сделать копию строки?
        m_blacklist.push_back(command);
    }
}

bool
Service::is_command_enabled (std::string& /*const char **/command)
{
    bool exist = false;

    // попробуем найти полученную команду в списке заблокированных
    for(std::vector<std::string>::iterator it = m_blacklist.begin();
        it != m_blacklist.end();
        it++) {
           if (*it == command) {
               exist = true;
               break;
           }
    }

    return !exist;
}

