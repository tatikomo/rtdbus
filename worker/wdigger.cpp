#include "zmq.hpp"
#include "mdp_common.h"
#include "mdp_worker_api.hpp"
#include "wdigger.hpp"

int Digger::handle_request(zmsg* request, std::string*& reply_to)
{
  assert (request->parts () >= 3);
  s_console("Process new request with %d parts and reply to '%s'",
            request->parts(), reply_to->c_str());


  std::string operation = request->pop_front ();
  std::string price     = request->pop_front ();
  std::string volume    = request->pop_front ();

  if (operation.compare("SELL") == 0)
        handle_sell_request (price, volume, reply_to);
  else
  if (operation.compare("BUY") == 0)
        handle_buy_request (price, volume, reply_to);
  else {
        zclock_log ("E: invalid message: ");
        request->dump();
  }

  return 0;
}

int Digger::handle_sell_request(std::string &price, 
                                std::string &volume,
                                std::string *reply_to)
{
    s_console("I: SELL to '%s' price=%s volume=%s",
              reply_to->c_str(), price.c_str(), volume.c_str());
    return 0;
}

int Digger::handle_buy_request(std::string &price, 
                                std::string &volume,
                                std::string *reply_to)
{
    s_console("I: BUY to '%s' price=%s volume=%s",
              reply_to->c_str(), price.c_str(), volume.c_str());
    return 0;
}

int main(int argc, char **argv)
{
  int verbose = (argc > 1 && (0 == strcmp (argv [1], "-v")));
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

