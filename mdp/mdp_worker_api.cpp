#include <glog/logging.h>
#include "mdp_common.h"
#include "mdp_worker_api.hpp"

using namespace mdp;

//  ---------------------------------------------------------------------
//  Signal handling
//
//  Call s_catch_signals() in your application at startup, and then exit
//  your main loop if s_interrupted is ever 1. Works especially well with
//  zmq_poll.
static int s_interrupted = 0;
static void s_signal_handler (int signal_value)
{
    s_interrupted = 1;
}

static void s_catch_signals ()
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}

mdwrk::mdwrk (std::string broker, std::string service, int verbose)
{
    /* NB
     * Отличия в версиях 2.1       3.2
     * ZMQ_RECVMORE:     int64 ->  int
     *
     * смотри https://github.com/zeromq/CZMQ/blob/master/sockopts.xml
     */
    s_version_assert (3, 2);

    m_context = new zmq::context_t (1);
    m_broker = broker;
    m_service = service;
    m_verbose = verbose;
    m_heartbeat = HeartbeatInterval;     //  msecs
    m_reconnect = HeartbeatInterval;     //  msecs
    m_worker = 0;
    m_expect_reply = false;

    s_catch_signals ();
    connect_to_broker ();
//    set_heartbeat(m_heartbeat);
}

//  ---------------------------------------------------------------------
//  Destructor
mdwrk::~mdwrk ()
{
    delete m_worker;
    delete m_context;
}

//  ---------------------------------------------------------------------
//  Send message to broker
//  If no _msg is provided, creates one internally
void mdwrk::send_to_broker(const char *command, const char* option, zmsg *_msg)
{
    zmsg *msg = _msg? new zmsg(*_msg): new zmsg ();

    //  Stack protocol envelope to start of message
    if (option) {
        msg->push_front ((char*)option);
    }
    msg->push_front ((char*)command);
    msg->push_front ((char*)MDPW_WORKER);
    msg->push_front ((char*)"");

    if (m_verbose) {
        LOG(INFO) << "Sending " << mdpw_commands [(int) *command] << " to broker";
        msg->dump ();
    }
    msg->send (*m_worker);
    delete msg;
}

//  ---------------------------------------------------------------------
//  Connect or reconnect to broker
void mdwrk::connect_to_broker ()
{
    int linger = 0;

    if (m_worker) {
        delete m_worker;
    }
    m_worker = new zmq::socket_t (*m_context, ZMQ_DEALER);
    m_worker->setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
    m_worker->connect (m_broker.c_str());
    if (m_verbose)
        LOG(INFO) << "Connecting to broker " << m_broker;

    //  Register service with broker
    send_to_broker ((char*)MDPW_READY, m_service.c_str(), NULL);

    //  If liveness hits zero, queue is considered disconnected
    m_liveness = HEARTBEAT_LIVENESS;
    m_heartbeat_at = s_clock () + m_heartbeat;
}


//  ---------------------------------------------------------------------
//  Set heartbeat delay
void
mdwrk::set_heartbeat (int heartbeat)
{
    m_heartbeat = heartbeat;
}


//  ---------------------------------------------------------------------
//  Set reconnect delay
void
mdwrk::set_reconnect (int reconnect)
{
    m_reconnect = reconnect;
}

//  ---------------------------------------------------------------------
//  Send reply, if any, to broker and wait for next request.
//  If reply is not NULL, a pointer to client's address is filled in.
zmsg *
mdwrk::recv (std::string *&reply)
{
    //  Format and send the reply if we were provided one
    assert (reply || !m_expect_reply);

    m_expect_reply = true;
    while (!s_interrupted) {
        zmq::pollitem_t items [] = {
            { *m_worker,  0, ZMQ_POLLIN, 0 } };
        zmq::poll (items, 1, m_heartbeat);

        if (items [0].revents & ZMQ_POLLIN) {
            zmsg *msg = new zmsg(*m_worker);
            if (m_verbose) {
                LOG(INFO) << "New message from broker:";
                msg->dump ();
            }
            m_liveness = HEARTBEAT_LIVENESS;

            //  Don't try to handle errors, just assert noisily
            assert (msg->parts () >= 3);

            /* 
             * NB: если в zmsg [GEV:генерация GUID] закомментирована проверка 
             * на количество фреймов в сообщении (=5),
             * то в этом случае empty будет равен
             * [011] @0000000000
             * и assert сработает на непустую строку.
             *
             * Во случае, если принятое сообщение не первое от данного Обработчика,
             * empty будет действительно пустым.
             */
            std::string empty = msg->pop_front ();
            //assert (empty.compare("") == 0);
            //free (empty);

            std::string header = msg->pop_front ();
            assert (header.compare(MDPW_WORKER) == 0);
            //free (header);

            std::string command = msg->pop_front ();
            if (command.compare (MDPW_REQUEST) == 0) {
                //  We should pop and save as many addresses as there are
                //  up to a null part, but for now, just save one...
                *reply = msg->unwrap ();
                return msg;     //  We have a request to process
            }
            else if (command.compare (MDPW_HEARTBEAT) == 0) {
                LOG(INFO) << "HEARTBEAT from broker";
                //  Do nothing for heartbeats
            }
            else if (command.compare (MDPW_DISCONNECT) == 0) {
                connect_to_broker ();
            }
            else {
                LOG(ERROR) << "Receive invalid message " << (int) *(command.c_str());
                msg->dump ();
            }
            delete msg;
        }
        else /* Ожидание нового запроса завершено по таймауту */
        if (--m_liveness == 0) {
            if (m_verbose) {
                LOG(WARNING) << "Disconnected from broker - retrying...";
            }
            s_sleep (m_reconnect);
            connect_to_broker ();
        }

        if (s_clock () > m_heartbeat_at) {
            send_to_broker ((char*)MDPW_HEARTBEAT, NULL, NULL);
            m_heartbeat_at = s_clock () + m_heartbeat;
        }
    }
    if (s_interrupted)
        LOG(WARNING) << "Interrupt received, killing worker...";
    return NULL;
}

