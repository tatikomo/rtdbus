#include "mdp_worker_api.hpp"

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
    m_heartbeat = 2500;     //  msecs
    m_reconnect = 2500;     //  msecs
    m_worker = 0;
    m_expect_reply = false;

    s_catch_signals ();
    connect_to_broker ();
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
//  OPTION
//  COMMAND
//  WORKER
void mdwrk::send_to_broker(char *command, std::string option, zmsg *_msg)
{
    zmsg *msg = _msg? new zmsg(*_msg): new zmsg ();

    //  Stack protocol envelope to start of message
    if (option.length() != 0) {
        msg->push_front ((char*)option.c_str());
    }
    msg->push_front (command);
    msg->push_front ((char*)MDPW_WORKER);
    msg->push_front ((char*)"");
    /*
     * Следующее поле д.б. "sender"
     *  идентификатор отправителя - буквенно-цифровой код
     *  Например:
     *   [005] 006B8B4567
     *   [000] 
     *   [006] MDPW01
     *   [001] 01
     *   [004] echo
     */ 

    if (m_verbose) {
        s_console ("I: sending %s to broker",
            mdpw_commands [(int) *command]);
        msg->dump ();
    }
    msg->send (*m_worker);
}

//  ---------------------------------------------------------------------
//  Connect or reconnect to broker
void mdwrk::connect_to_broker ()
{
    if (m_worker) {
        delete m_worker;
    }
    m_worker = new zmq::socket_t (*m_context, ZMQ_DEALER);
#if defined GEV
    int linger = 0;
    m_worker->setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
#endif
    m_worker->connect (m_broker.c_str());
    if (m_verbose)
        s_console ("I: connecting to broker at %s...", m_broker.c_str());

    //  Register service with broker
    send_to_broker ((char*)MDPW_READY, m_service, NULL);

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
//  If reply_p is not NULL, a pointer to client's address is filled in.
zmsg *
mdwrk::recv (std::string *&reply_p)
{
    //  Format and send the reply if we were provided one
    std::string *reply = reply_p;
    assert (reply || !m_expect_reply);
#if defined GEV
    if (reply) {
        assert (m_reply_to.size()!=0);
        reply->wrap (m_reply_to.c_str(), "");
        m_reply_to = "";
        send_to_broker ((char*)MDPW_REPLY, "", reply);
        delete reply_p;
        reply_p = 0;
    }
#endif
    m_expect_reply = true;

    while (!s_interrupted) {
        zmq::pollitem_t items [] = {
            { *m_worker,  0, ZMQ_POLLIN, 0 } };
        zmq::poll (items, 1, m_heartbeat * 1000);

        if (items [0].revents & ZMQ_POLLIN) {
            zmsg *msg = new zmsg(*m_worker);
            if (m_verbose) {
                s_console ("I: received message from broker:");
                msg->dump ();
            }
            m_liveness = HEARTBEAT_LIVENESS;

            //  Don't try to handle errors, just assert noisily
            assert (msg->parts () >= 3);

            ustring empty = msg->pop_front ();
            assert (empty.compare("") == 0);
            //free (empty);

            ustring header = msg->pop_front ();
            assert (header.compare(MDPW_WORKER) == 0);
            //free (header);

            ustring command = msg->pop_front ();
            if (command.compare (MDPW_REQUEST) == 0) {
                //  We should pop and save as many addresses as there are
                //  up to a null part, but for now, just save one...
                *reply_p = msg->unwrap ();
//                m_reply_to = reply_p; // +++ GEV добавил на всякий случай
                return msg;     //  We have a request to process
            }
            else if (command.compare (MDPW_HEARTBEAT) == 0) {
                //  Do nothing for heartbeats
            }
            else if (command.compare (MDPW_DISCONNECT) == 0) {
                connect_to_broker ();
            }
            else {
                s_console ("E: invalid input message (%d)",
                      (int) *(command.c_str()));
                msg->dump ();
            }
            delete msg;
        }
        else
        if (--m_liveness == 0) {
            if (m_verbose) {
                s_console ("W: disconnected from broker - retrying...");
            }
            s_sleep (m_reconnect);
            connect_to_broker ();
        }
        //  Send HEARTBEAT if it's time
        if (s_clock () > m_heartbeat_at) {
            send_to_broker ((char*)MDPW_HEARTBEAT, "", NULL);
            m_heartbeat_at = s_clock () + m_heartbeat;
        }
    }
    if (s_interrupted)
        printf ("W: interrupt received, killing worker...\n");
    return NULL;
}

