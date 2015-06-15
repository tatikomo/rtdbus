#include <glog/logging.h>

#include "zmq.hpp"
#include "helper.hpp"
#include "mdp_zmsg.hpp"
#include "mdp_worker_api.hpp"
#include "msg_message.hpp"
#include "proto/common.pb.h"
#include "wtester.hpp"

extern int interrupt_worker;

Tester::Tester(const std::string& broker, const std::string& service, int verbose) 
  : mdwrk(broker, service, verbose),
    m_message_factory(new msg::MessageFactory(service.c_str()))
{
}

Tester::~Tester()
{
  delete m_message_factory;
}

int Tester::handle_request(mdp::zmsg* request, std::string*& reply_to)
{
  assert (request->parts () >= 2);
  LOG(INFO) << "Process new request with " << request->parts() << " parts and reply to " << *reply_to;

#if 0
  std::string message = request->pop_front ();
  std::string header  = request->pop_front ();

  if (operation.compare("SELL") == 0)
        handle_sell_request (price, volume, reply_to);
  else
  if (operation.compare("BUY") == 0)
        handle_buy_request (price, volume, reply_to);
  else {
        zclock_log ("E: invalid message: ");
        request->dump();
  }
#else
  msg::Letter *letter = m_message_factory->create(request);
  if (letter->valid())
  {
    handle_rtdbus_message(letter, reply_to);
  }
  else
  {
    LOG(ERROR) << "Readed letter "<<letter->header()->exchange_id()<<" not valid";
  }

  delete letter;
#endif

  return 0;
}

int Tester::handle_rtdbus_message(msg::Letter* letter, 
                                std::string *reply_to)
{
    assert(reply_to != NULL);
    mdp::zmsg * msg = new mdp::zmsg();

    /* TODO: поменять местами в заголовке значения полей "Отправитель" и "Получатель" */
    std::string origin = letter->header()->proc_origin();
    std::string dest = letter->header()->proc_dest();

    letter->set_origin(dest.c_str());
    letter->set_destination(origin.c_str());

    msg->push_front(const_cast<std::string&>(letter->data()->get_serialized()));
    msg->push_front(const_cast<std::string&>(letter->header()->get_serialized()));
    msg->wrap(reply_to->c_str(), "");

    send_to_broker((char*) MDPW_REPORT, NULL, msg);
    delete msg;
    return 0;
}


int Tester::handle_sell_request(std::string &price, 
                                std::string &volume,
                                std::string *reply_to)
{
    LOG(INFO) << "SELL to '" << reply_to << "' price=" << price << " volume=" << volume;

    assert(reply_to != NULL);
    // в ответе д.б. два поля: REPORT_TYPE и VOLUME
    mdp::zmsg * msg = new mdp::zmsg();
    msg->push_front((char*)price.c_str());
    msg->push_front((char*)"SOLD");
    msg->wrap(reply_to->c_str(), "");
    send_to_broker((char*) MDPW_REPORT, NULL, msg);
    delete msg;
    return 0;
}

int Tester::handle_buy_request(std::string &price, 
                               std::string &volume,
                               std::string *reply_to)
{
    LOG(INFO) << "BUY from '" << reply_to << "' price=" << price << " volume=" << volume;
    assert(reply_to != NULL);
    // в ответе д.б. два поля: REPORT_TYPE и VOLUME
    mdp::zmsg * msg = new mdp::zmsg();
    msg->push_front((char*)price.c_str());
    msg->push_front((char*)"BYED");
    msg->wrap(reply_to->c_str(), "");
    send_to_broker((char*) MDPW_REPORT, NULL, msg);
    delete msg;
    return 0;
}

/*
 * Если задан параметр FUNCTIONAL_TEST, значит объект находится
 * под тестированием, следует заблокировать main() 
 */
#if !defined _FUNCTIONAL_TEST
int main(int argc, char **argv)
{
  int  verbose = (argc > 1 && (0 == strcmp (argv [1], "-v")));
  char service_name[SERVICE_NAME_MAXLEN + 1];
  bool is_service_name_given = false;
  int  opt;

  ::google::InstallFailureSignalHandler();
  ::google::InitGoogleLogging(argv[0]);
//  Letter *letter = NULL;

  while ((opt = getopt (argc, argv, "vs:")) != -1)
  {
     switch (opt)
     {
       case 'v':
         verbose = 1;
         break;

       case 's':
         strncpy(service_name, optarg, SERVICE_NAME_MAXLEN);
         service_name[SERVICE_NAME_MAXLEN] = '\0';
         is_service_name_given = true;
         break;

       case '?':
         if (optopt == 'n')
           fprintf (stderr, "Option -%c requires an argument.\n", optopt);
         else if (isprint (optopt))
           fprintf (stderr, "Unknown option `-%c'.\n", optopt);
         else
           fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
         return 1;
       default:
         abort ();
     }
  }

  if (!is_service_name_given)
  {
    std::cout << "Service name not given.\nUse '-s <service>' option.\n";
    return(1);
  }

  try
  {
    Tester *engine = new Tester("tcp://localhost:5555", service_name, verbose);
    while (!interrupt_worker)
    {
       std::string *reply_to = new std::string;
       mdp::zmsg   *request  = NULL;

       request = engine->recv (reply_to);
       if (request)
       {
//         letter = new (request);
         engine->handle_request (request/*letter*/, reply_to);
         delete request;
       }
       else
       {
         interrupt_worker = true; // Worker has been interrupted
       }
       delete reply_to;
    }
    delete engine;
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
  }

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();
  return 0;
}
#endif

