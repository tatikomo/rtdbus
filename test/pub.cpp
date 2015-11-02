// g++ -g pub.cpp -o pub -I../libzmq/include -I../cppzmq -L../libzmq/src/.libs -lzmq
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/gev/ITG/sandbox/libzmq/src/.libs
#include "zmq.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>

int main (int argc, char *argv[])
{
  zmq::context_t context (1);
  zmq::socket_t publisher (context, ZMQ_PUB);
  publisher.bind("tcp://lo:5560");
//  publisher.bind("ipc://sbs.ipc");				// Not usable on Windows.

  const char *group_name = (argc > 1)? argv [1]: "KD4005_SS";
  std::cout << "RTDBUS publishing server for group: " << group_name << std::endl;

  //  Process 100 updates

  std::cout << "wait 10 sec..." << std::endl;
  sleep (10);
  std::cout << "done, start updates" << std::endl;


  int update_nbr;
  for (update_nbr = 0; update_nbr < 100; update_nbr++) {

    zmq::message_t update;
    std::string code, empty, head, body;

    code = "CODE_" + update_nbr;
    head = "HEAD_" + update_nbr;
    body = "BODY_" + update_nbr;

    size_t sz = (strlen(group_name) + 1/* terminal null*/) + code.size() + head.size() + body.size();
    zmq::message_t message(sz);

    snprintf ((char *) message.data(), sz,
        	"%s %s %s %s", group_name, code.c_str(), head.c_str(), body.c_str());
    publisher.send(message);
  }

  std::cout << "Send 100 updates for sbs '"<< group_name <<"'"
            << std::endl;
  return 0;
}
