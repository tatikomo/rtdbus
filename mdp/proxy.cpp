#include "mdp_common.h"
#include "mdp_broker.hpp"
#include "proxy.hpp"

//  Finally here is the main task. We create a new broker instance and
//  then processes messages on the broker socket.
int main (int argc, char *argv [])
{
    int verbose = (argc > 1 && (0 == strncmp(argv [1], "-v", 3)));
    ustring sender;
    ustring empty;
    ustring header;
    int more;           //  Multipart detection
    Broker *broker = NULL;

    try
    {
      s_version_assert (3, 2);
      s_catch_signals ();
      broker = new Broker(verbose);
      /*
       * NB: "tcp://lo:5555" использует локальный интерфейс, 
       * что удобно для мониторинга wireshark-ом.
       */
      broker->bind ("tcp://*:5555");

      broker->start_brokering();
    }
    catch (zmq::error_t err)
    {
        std::cout << "E: " << err.what() << std::endl;
    }

    if (s_interrupted) {
        printf ("W: interrupt received, shutting down...\n");
    }

    delete broker;

    return 0;
}

