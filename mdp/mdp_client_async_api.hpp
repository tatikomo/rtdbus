/*  =====================================================================
    mdp_client_async_api.cpp

    Majordomo Protocol Client API (async version)
    Implements the MDP/Worker spec at http://rfc.zeromq.org/spec:7.
    ---------------------------------------------------------------------
*/

#ifndef MDP_CLIENT_ASYNC_API_HPP
#define MDP_CLIENT_ASYNC_API_HPP

#include "config.h"
#include "mdp_zmsg.hpp"
#include "mdp_common.h"

namespace mdp {

//  Structure of our class
//  We access these properties only via class methods
class mdcli {
  public:

   //  ---------------------------------------------------------------------
   //  Constructor
   mdcli (std::string broker, int verbose);

   //  ---------------------------------------------------------------------
   //  Destructor
   virtual
   ~mdcli ();

   //  ---------------------------------------------------------------------
   //  Connect or reconnect to broker
   void connect_to_broker ();

   //  ---------------------------------------------------------------------
   //  Set request timeout
   void
   set_timeout (int timeout);

   //  ---------------------------------------------------------------------
   //  Send request to broker
   //  Takes ownership of request message and destroys it when sent.
   //  The send method now just sends one message, without waiting for a
   //  reply. Since we're using a DEALER socket we have to send an empty
   //  frame at the start, to create the same envelope that the REQ socket
   //  would normally make for us
   int
   send (std::string service, zmsg *&request_p);

   //  ---------------------------------------------------------------------
   //  Returns the reply message or NULL if there was no reply. Does not
   //  attempt to recover from a broker failure, this is not possible
   //  without storing all unanswered requests and resending them all...
   zmsg *
   recv ();

  private:
   DISALLOW_COPY_AND_ASSIGN(mdcli);
   std::string     m_broker;
   zmq::context_t *m_context;
   zmq::socket_t  *m_client;    //  Socket to broker
   int m_verbose;                //  Print activity to stdout
   int m_timeout;                //  Request timeout
};

}; //namespace mdp

#endif
