#include <glog/logging.h>
#include "config.h"
#include "zmsg.hpp"
#include "mdp_client_async_api.hpp"

using namespace mdp;

//  ---------------------------------------------------------------------
//  Signal handling
//
//  Call s_catch_signals() in your application at startup, and then exit
//  your main loop if s_interrupted is ever 1. Works especially well with
//  zmq_poll.
int s_interrupted = 0;
void s_signal_handler (int signal_value)
{
    s_interrupted = 1;
}

void s_catch_signals ()
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}


mdcli::mdcli (std::string broker, int verbose)
{
   s_version_assert (3, 2);

   m_broker = broker;
   m_context = new zmq::context_t (1);
   m_verbose = verbose;
   m_timeout = 2500;           //  msecs
   m_client = 0;

   s_catch_signals ();
   connect_to_broker ();
}


//  ---------------------------------------------------------------------
//  Destructor
mdcli::~mdcli ()
{
   delete m_client;
   delete m_context;
}


//  ---------------------------------------------------------------------
//  Connect or reconnect to broker
void mdcli::connect_to_broker ()
{
   if (m_client) {
        delete m_client;
   }

   m_client = new zmq::socket_t (*m_context, ZMQ_DEALER);
   int linger = 0;
   m_client->setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
   m_client->connect (m_broker.c_str());
   if (m_verbose)
        LOG(INFO) << "Connecting to broker " << m_broker;
}


//  ---------------------------------------------------------------------
//  Set request timeout
void
mdcli::set_timeout (int timeout)
{
   m_timeout = timeout;
}


//  ---------------------------------------------------------------------
//  Send request to broker
//  Takes ownership of request message and destroys it when sent.
//  The send method now just sends one message, without waiting for a
//  reply. Since we're using a DEALER socket we have to send an empty
//  frame at the start, to create the same envelope that the REQ socket
//  would normally make for us
int
mdcli::send (std::string service, zmsg *&request_p)
{
   assert (request_p);
   zmsg *request = request_p;

   //  Prefix request with protocol frames
   //  Frame 0: empty (REQ emulation)
   //  Frame 1: "MDPCxy" (six bytes, MDP/Client x.y)
   //  Frame 2: Service name (printable string)
   request->push_front ((char*)service.c_str());
   request->push_front ((char*)MDPC_CLIENT);
   request->push_front ((char*)"");

   if (m_verbose) {
        LOG(INFO) << "Send request to service " << service;
        request->dump ();
   }

   request->send (*m_client);
   return 0;
}


//  ---------------------------------------------------------------------
//  Returns the reply message or NULL if there was no reply. Does not
//  attempt to recover from a broker failure, this is not possible
//  without storing all unanswered requests and resending them all...
zmsg *
mdcli::recv ()
{
   //  Poll socket for a reply, with timeout
   zmq::pollitem_t items [] = { { *m_client, 0, ZMQ_POLLIN, 0 } };
   zmq::poll (items, 1, m_timeout /** 1000*/); // 1000 -> msec

   //  If we got a reply, process it
   if (items [0].revents & ZMQ_POLLIN) {
        zmsg *msg = new zmsg (*m_client);
        if (m_verbose) {
            LOG(INFO) << "received reply:";
            msg->dump ();
        }
        //  Don't try to handle errors, just assert noisily
        assert (msg->parts () >= 4);
        std::string empty = msg->pop_front ();
//GEV        assert (empty.length() == 0);  // empty message

        std::string header = msg->pop_front();
        assert (header.compare(MDPC_CLIENT) == 0);

        std::string service = msg->pop_front();
        // TODO следующая функция всегда вернет 0
        assert (service.compare(service) == 0);

        // TODO: добавить фрейм КОМАНДА

        return msg;     //  Success
   }

   if (s_interrupted)
     LOG(WARNING) << "Interrupt received, killing client...";
   else if (m_verbose)
     LOG(WARNING) << "Permanent error, abandoning request";

   return 0;
}

