//
//  Hello World server in C++
//  Send REQ to tcp://localhost:12000
//
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>

using namespace zmq;

int main () 
{
    (void) setlocale( LC_CTYPE, "" );
    (void) setlocale( LC_COLLATE, "" );
    (void) setlocale( LC_TIME, "" );
    (void) setlocale( LC_MONETARY, "" );

    zmq::context_t context (1);
    zmq::socket_t tube(context, ZMQ_REQ);

    try
    {
      tube.connect("tcp://localhost:12000");
    }
    catch (zmq::error_t error)
    {
      std::cout << "Fatal error: " << error.what() << std::endl;
      exit(-1);
    }

    std::cout << "Connected to localhost:12000" << std::endl;
    while (true) {
        try
        {
          zmq::message_t request;
          memcpy ((void *) request.data (), "REQU", 5); // 5 = sizeof("string") + 1

          //  Send request to frontend
          tube.send (request);
          std::cout << "Send request" << std::endl;

          sleep (1);

          //  Wait for reply from frontend
          zmq::message_t reply;
          tube.recv (&reply);
          std::cout << "Accept reply" << std::endl;
        }
        catch (zmq::error_t error)
        {
          std::cout << "Fatal error: " << error.what() << std::endl;
          exit(-1);
        }

    }
    return 0;
}
