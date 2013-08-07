//
//  Simple trading example.
//  It is purpose is to demonstate how to use mdp_client interface.

#include "zmsg.hpp"
#include "trader.hpp"
#include "mdp_client_async_api.hpp"


Trader::Trader(std::string broker, int verbose) : mdcli(broker, verbose)
{
}

int main (int argc, char *argv [])
{
    int       verbose   = (argc > 1 && (strcmp (argv [1], "-v") == 0));
    char    * s_price   = NULL;
    zmsg    * request   = NULL;
    zmsg    * report    = NULL;
    Trader  * client    = new Trader ("tcp://localhost:5555", verbose);
    int       count;

  try
  {
    s_price = new char[10];
    //  Send 100 sell orders
    for (count = 0; count < 2; count++) {
        request = new zmsg ();
        request->push_front ((char*)"8");    // volume
        sprintf(s_price, "%d", count + 1000);
        request->push_front ((char*)s_price);
        request->push_front ((char*)"SELL");
        client->send ("NYSE", request);
        delete request;
    }

    //  Send 1 buy order.
    //  This order will match all sell orders.
    request = new zmsg ();
    request->push_front ((char*)"800");      // volume
    request->push_front ((char*)"2000");     // price
    request->push_front ((char*)"BUY");
    client->send ("NYSE", request);
    delete request;

    //  Wait for all trading reports
    while (1) {
        report = client->recv ();
        if (report == NULL)
            break;
        assert (report->parts () >= 2);
        
        std::string report_type = report->pop_front ();
        std::string volume      = report->pop_front ();

        printf ("%s %s shares\n",  report_type.c_str(), volume.c_str());
        delete report;
    }
  }
  catch (zmq::error_t err)
  {
      std::cout << "E: " << err.what() << std::endl;
  }
  delete s_price;
  delete client;
  return 0;
}
