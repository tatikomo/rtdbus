/*  =====================================================================
    mdp_client_async_api.cpp

    Majordomo Protocol Client API (async version)
    Implements the MDP/Worker spec at http://rfc.zeromq.org/spec:7.

    ---------------------------------------------------------------------
    Copyright (c) 1991-2011 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of the ZeroMQ Guide: http://zguide.zeromq.org

    This is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or (at
    your option) any later version.

    This software is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program. If not, see
    <http://www.gnu.org/licenses/>.
    =====================================================================

        Andreas Hoelzlwimmer <andreas.hoelzlwimmer@fh-hagenberg.at>
*/

#ifndef __MDP_CLIENT_ASYNC_API_HPP_INCLUDED__
#define __MDP_CLIENT_ASYNC_API_HPP_INCLUDED__

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

} //namespace mdp

#endif
