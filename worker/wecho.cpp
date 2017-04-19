#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "zmq.hpp"
#include "mdp_common.h"
#include "mdp_zmsg.hpp"
#include "mdp_worker_api.hpp"
#include "wecho.hpp"

extern int interrupt_worker;

int Echo::handle_request(mdp::zmsg* request, std::string*& reply_to)
{
  int rc = OK;
  assert (request->parts () >= 1);
  LOG(INFO) << "Process new request with " << request->parts() 
    << " parts and reply to " << reply_to;

  request->dump();

  request->unwrap();
  request->wrap(reply_to->c_str(), EMPTY_FRAME);

  request->dump();
  send_to_broker((char*) MDPW_REPORT, NULL, request);

  return rc;
}

int main(int argc, char **argv)
{
  int rc = OK;
  int verbose = (argc > 1 && (0 == strcmp (argv [1], "-v")));

  google::InitGoogleLogging(argv[0]);

  try
  {
    Echo *engine = new Echo("tcp://localhost:5555", "echo", verbose);
    while (!interrupt_worker) 
    {
       std::string *reply_to = new std::string;
       mdp::zmsg   *request  = NULL;

       request = engine->recv (reply_to);
       if (request)
         engine->handle_request (request, reply_to);
       else
         break;          // Worker has been interrupted
    }
    delete engine;
  }
  catch(zmq::error_t err)
  {
    std::cout << "E: " << err.what() << std::endl;
    rc = NOK;
  }

  return (OK == rc)? EXIT_SUCCESS : EXIT_FAILURE;
}

