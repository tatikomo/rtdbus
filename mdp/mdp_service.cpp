#include <string>
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
Worker::Worker(std::string& identity, Service * service, int64_t expiry)
{
   m_identity = identity;
   m_service = service;
   m_expiry = expiry;
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
   for(size_t i = 0; i < m_blacklist.size(); i++) {
        delete &m_blacklist[i];
   }
   m_blacklist.clear();
}

Service::Service(std::string& name)
{
   m_name = name;
}


#if 0
//  Service destructor is called automatically whenever the service is
//  removed from broker->services.
void
Service::destroy (void *argument)
{
    Service *service = (Service *) argument;
    while (zlist_size (service->requests)) {
        zmsg_t *msg = (zmsg_t*)zlist_pop (service->requests);
        zmsg_destroy (&msg);
    }
    //  Free memory keeping  blacklisted commands.
    char *command = (char *) zlist_first (service->blacklist);
    while (command) {
        zlist_remove (service->blacklist, command);
        free (command);
    }
    zlist_destroy (&service->requests);
    zlist_destroy (&service->waiting);
    zlist_destroy (&service->blacklist);
    free (service->name);
    free (service);
}

//  The dispatch method sends request to the worker.
void
Service::dispatch ()
{
    m_broker->purge ();
    if (zlist_size (self->waiting) == 0)
        return;

    while (zlist_size (self->requests) > 0) {
        worker_t *worker = (worker_t*)zlist_pop (self->waiting);
        zlist_remove (self->waiting, worker);
        zmsg_t *msg = (zmsg_t*)zlist_pop (self->requests);
        s_worker_send (worker, MDPW_REQUEST, NULL, msg);
        //  Workers are scheduled in the round-robin fashion
        zlist_append (self->waiting, worker);
        zmsg_destroy (&msg);
    }
}

#endif
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

