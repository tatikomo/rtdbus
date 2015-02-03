/*  =====================================================================
    mdp_worker_api.hpp

    Majordomo Protocol Worker API
    Implements the MDP/Worker spec at http://rfc.zeromq.org/spec:7.

    =====================================================================
*/

#pragma once
#ifndef MDP_WORKER_API_HPP_INCLUDED
#define MDP_WORKER_API_HPP_INCLUDED

#include "config.h"
#include "mdp_zmsg.hpp"
#include "mdp_common.h"

namespace mdp {

//  Structure of our class
//  We access these properties only via class methods
class mdwrk {
  public:
    static const int HeartbeatInterval = HEARTBEAT_PERIOD_MSEC;
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
    void send_to_broker(const char *command, const char* option, zmsg *_msg);

    //  ---------------------------------------------------------------------
    //  Connect or reconnect to broker
    void connect_to_broker ();

    //  ---------------------------------------------------------------------
    //  Connect or reconnect to everyone
    //  Этот сокет используется для обмена "быстрыми" сообщениями, минуя Брокера
    void connect_to_world ();

    //  ---------------------------------------------------------------------
    //  Get service endpoint
    //  Получить строку подключения для своего Сервиса
    void ask_endpoint();

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
    DISALLOW_COPY_AND_ASSIGN(mdwrk);
    const char* getEndpoint() const;

    std::string      m_broker;
    std::string      m_service;
    // Точка подключения 
    const char     * m_welcome_endpoint;
    zmq::context_t * m_context;
    zmq::socket_t  * m_worker;      //  Socket to broker
    zmq::socket_t  * m_welcome;     //  Socket to subscribe on brokerless messages
    int              m_verbose;     //  Print activity to stdout
    //  Heartbeat management
    int64_t          m_heartbeat_at;//  When to send HEARTBEAT
    size_t           m_liveness;    //  How many attempts left
    int              m_heartbeat;   //  Heartbeat delay, msecs
    int              m_reconnect;   //  Reconnect delay, msecs
    //  Internal state
    bool             m_expect_reply;//  Zero only at start
};

} //namespace mdp

#endif
