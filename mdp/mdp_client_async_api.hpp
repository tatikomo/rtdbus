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

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "mdp_zmsg.hpp"
#include "mdp_common.h"
#include "mdp_helpers.hpp"

namespace mdp {

typedef enum {
  PERSISTENT = 1, // Обмен через Брокера
  DIRECT     = 2, // Обмен со Службой напрямую
} ChannelType;

//  Structure of our class
//  We access these properties only via class methods
class mdcli {
  public:
   // Индексы сокетов:
   // 0 = Брокера, 1 = Получателя обновления подписок, 2 = Прямого сообщения
   enum { BROKER_ITEM       = 0,
          SUBSCRIBER_ITEM   = 1,
          SERVICE_ITEM      = 2,
          SOCKET_MAX_COUNT  = SERVICE_ITEM + 1 };

   enum { RECEIVE_FATAL     = -1,
          RECEIVE_OK = 0,
          RECEIVE_ERROR = 1 };

   //  ---------------------------------------------------------------------
   //  Constructor
   mdcli (std::string& broker, int verbose);

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
   int ask_service_info(const char* service_name,   // in: имя Службы
                        char* service_endpoint,     // out: значение точки подключения
                        int buf_size /* размер буфера service_endpoint */);

   //  ---------------------------------------------------------------------
   //  Активация подписки с указанным названием для указанной Службы
   int subscript(const std::string&, const std::string&);

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
   //  Fill the reply message or NULL if there was no reply. Does not
   //  attempt to recover from a broker failure, this is not possible
   //  without storing all unanswered requests and resending them all...
   int recv (zmsg*&);

  private:
   DISALLOW_COPY_AND_ASSIGN(mdcli);
   std::string     m_broker;
   zmq::context_t *m_context;
   zmq::socket_t  *m_client;    //  Socket to broker
   // Сокет обновлений подписки
   zmq::socket_t  *m_subscriber;
   // Сокет для прямого взаимодействия со Службами
   zmq::socket_t  *m_peer;
   // Опрос входных сокетов - от Брокера, Подписки, и прямой от Служб
   zmq::pollitem_t m_socket_items[SOCKET_MAX_COUNT];
   int m_active_socket_num;         // количество зарегистрированных в пуле сокетов
   int m_subscriber_socket_index;   // индекс сокета подписки
   int m_peer_socket_index;         // индекс сокета обмена сообщениями с Клиентами

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
   // Добавить в пул сокетов m_socket_items для опроса zmq::pool новый сокет
   void add_socket_to_pool(zmq::socket_t*, int);
};

} //namespace mdp

#endif
