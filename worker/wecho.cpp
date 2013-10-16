#include <glog/logging.h>

#include "zmq.hpp"
#include "mdp_zmsg.hpp"
#include "mdp_worker_api.hpp"
#include "wecho.hpp"

extern int s_interrupted;

int Echo::handle_request(mdp::zmsg* request, std::string*& reply_to)
{
  assert (request->parts () >= 1);
  LOG(INFO) << "Process new request with " << request->parts() 
    << " parts and reply to " << reply_to;

  request->dump();

  request->unwrap();
  request->wrap(reply_to->c_str(), EMPTY_FRAME);

  request->dump();
  send_to_broker((char*) MDPW_REPORT, NULL, request);

  return 0;
}

int main(int argc, char **argv)
{
  int verbose = (argc > 1 && (0 == strcmp (argv [1], "-v")));
  google::InitGoogleLogging(argv[0]);

  try
  {
    Echo *engine = new Echo("tcp://localhost:5555", "echo", verbose);
    while (!s_interrupted) 
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
  }

  return 0;
}

