/*  =====================================================================
    mdp_worker_api.hpp

    Majordomo Protocol Worker API
    Implements the MDP/Worker spec at http://rfc.zeromq.org/spec:7.

    =====================================================================
*/

#ifndef __MDP_WORKER_API_HPP_INCLUDED__
#define __MDP_WORKER_API_HPP_INCLUDED__
#pragma once

#include "zmsg.hpp"
#include "mdp_common.h"

//  Structure of our class
//  We access these properties only via class methods
class mdwrk {
  public:

    //  ---------------------------------------------------------------------
    //  Constructor
    mdwrk (std::string broker, std::string service, int verbose);

    //  ---------------------------------------------------------------------
    //  Destructor
    virtual
    ~mdwrk ();

    //  ---------------------------------------------------------------------
    //  Send message to broker
    //  If no _msg is provided, creates one internally
    void send_to_broker(char *command, std::string option, zmsg *_msg);

    //  ---------------------------------------------------------------------
    //  Connect or reconnect to broker
    void connect_to_broker ();


    //  ---------------------------------------------------------------------
    //  Set heartbeat delay
    void
    set_heartbeat (int heartbeat);


    //  ---------------------------------------------------------------------
    //  Set reconnect delay
    void
    set_reconnect (int reconnect);

    //  ---------------------------------------------------------------------
    //  wait for next request and get the address for reply.
    zmsg *
    recv (std::string *&reply);

  private:
    std::string      m_broker;
    std::string      m_service;
    zmq::context_t * m_context;
    zmq::socket_t  * m_worker;      //  Socket to broker
    int              m_verbose;     //  Print activity to stdout

    //  Heartbeat management
    int64_t          m_heartbeat_at;//  When to send HEARTBEAT
    size_t           m_liveness;    //  How many attempts left
    int              m_heartbeat;   //  Heartbeat delay, msecs
    int              m_reconnect;   //  Reconnect delay, msecs

    //  Internal state
    bool             m_expect_reply;//  Zero only at start

    //  Return address, if any
    //std::string      m_reply_to;
};

#endif
