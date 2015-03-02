/*  =====================================================================
    mdp_client_async_api.cpp

    Majordomo Protocol Client API (async version)
    Implements the MDP/Worker spec at http://rfc.zeromq.org/spec:7.
    ---------------------------------------------------------------------
*/

#ifndef MDP_CLIENT_ASYNC_API_HPP
#define MDP_CLIENT_ASYNC_API_HPP

#include <map>
#include <string>

#include "config.h"
#include "mdp_zmsg.hpp"
#include "mdp_common.h"

namespace mdp {

typedef enum {
  PERSISTENT = 1, // Обмен через Брокера
  DIRECT     = 2, // Обмен со Службой напрямую
} ChannelType;

typedef char ServiceName[SERVICE_NAME_MAXLEN + 1];

typedef struct {
  // Строка подключения в формате connect 
  char endpoint[ENDPOINT_MAXLEN + 1];
  // Было ли подключение к данной Службе?
  bool  connected;
} ServiceInfo;

#ifdef __SUNPRO_CC
#else
typedef std::pair <const std::string, ServiceInfo*> ServicesHashPair_t;
typedef std::map  <const std::string, ServiceInfo*> ServicesHash_t;
#endif
typedef ServicesHash_t::iterator ServicesHashIterator_t;

//  Structure of our class
//  We access these properties only via class methods
class mdcli {
  public:
   enum { BROKER_ITEM = 0, SERVICE_ITEM = 1};
   enum { SOCKET_COUNT = 2 };

   //  ---------------------------------------------------------------------
   //  Constructor
   mdcli (std::string broker, int verbose);

   //  ---------------------------------------------------------------------
   //  Destructor
   virtual ~mdcli ();

   //  ---------------------------------------------------------------------
   //  Connect or reconnect to broker
   void connect_to_broker ();

   //  ---------------------------------------------------------------------
   //  Connect or reconnect to Services
   void connect_to_services ();

   //  ---------------------------------------------------------------------
   //  Set request timeout
   void set_timeout (int timeout);

   //  ---------------------------------------------------------------------
   //  Get the endpoint connecton string for specified service name
   int ask_service_info(const char* service_name, char* service_endpoint, int size);

   //  ---------------------------------------------------------------------
   //  Send request to broker
   //  Takes ownership of request message and destroys it when sent.
   //  The send method now just sends one message, without waiting for a
   //  reply. Since we're using a DEALER socket we have to send an empty
   //  frame at the start, to create the same envelope that the REQ socket
   //  would normally make for us
   //  TODO: deprecated, use send (std::string&, zmsg*&, ChannelType)
   int send (std::string  service, zmsg *&request_p);
   int send (std::string& service, zmsg *&request_p, ChannelType);

   //  ---------------------------------------------------------------------
   //  Returns the reply message or NULL if there was no reply. Does not
   //  attempt to recover from a broker failure, this is not possible
   //  without storing all unanswered requests and resending them all...
   zmsg * recv ();

  private:
   DISALLOW_COPY_AND_ASSIGN(mdcli);
   std::string     m_broker;
   zmq::context_t *m_context;
   zmq::socket_t  *m_client;    //  Socket to broker
   // Сокет для прямого взаимодействия со Службами
   zmq::socket_t  *m_peer;
   // Опрос двух сокетов - от Брокера и прямой от Служб
   zmq::pollitem_t m_socket_items[2];
   int m_verbose;               //  Print activity to stdout
   int m_timeout;               //  Request timeout

   // Список Служб получать анализом данных от ask_service_info
   // Связка "название Службы" <=> "информация о Службе", ключ хранится копией значения
   ServicesHash_t   m_services_info;
   // Получить информацию по Службе с заданным именем.
   bool service_info_by_name(const char*, ServiceInfo*&);
   bool insert_service_info(const char*, ServiceInfo*&);
   // Передать DIRECT-сообщение
   int  send_direct(std::string&, zmsg *&);
};

} //namespace mdp

#endif
