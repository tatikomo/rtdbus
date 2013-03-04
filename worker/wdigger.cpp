#include "zmq.hpp"
#include "mdp_common.h"
#include "mdp_worker_api.hpp"
#include "wdigger.hpp"

int Digger::handle_request(zmsg* request, std::string* reply_to)
{
  assert (request->parts () >= 3);
  std::cout << "Process new request with " << request->parts ()
            << "parts and reply to " 
            << reply_to->c_str() << std::endl;
  request->dump();
  return 0;
}

int main(int argc, char **argv)
{
  int verbose = (argc > 1 && (strcmp (argv [1], "-v") == 0));
  try
  {
    Digger *engine = new Digger("tcp://localhost:5555", "NYSE", verbose);
    while (1) 
    {
       std::string * reply_to = NULL;
       zmsg        * request  = NULL;

       request = engine->recv (reply_to);
       if (request == NULL)
           break;          // Worker has been interrupted
       engine->handle_request (request, reply_to);
    }
    delete engine;
  }
  catch(zmq::error_t err)
  {
    std::cout << "E: " << err.what() << std::endl;
  }

  return 0;
}

