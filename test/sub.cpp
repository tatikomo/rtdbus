// g++ -g sub.cpp -o sub -I../libzmq/include -I../cppzmq -L../libzmq/src/.libs -lzmq
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/gev/ITG/sandbox/libzmq/src/.libs
#include "zmq.hpp"
#include <iostream>
#include <sstream>
#include <string>

#define SEND_TIMEOUT_MSEC 1000000
#define RECV_TIMEOUT_MSEC 3000000

std::vector<std::string> m_part_data;

bool recv(zmq::socket_t& socket)
{
   int more;
   int iteration = 0;
   size_t more_size = sizeof(more);
   char *uuidstr = NULL;

   m_part_data.clear();

   while(1)
   {
      iteration++;
      zmq::message_t message(0);
      try {
         if (!socket.recv(&message, 0)) {
            std::cerr << "zmsg::recv() false" << std::endl;
            return false;
         }
      } catch (zmq::error_t error) {
         std::cerr << "Catch '" << error.what() << "', code=" << error.num() << std::endl;
         return false;
      }
      std::string data;
      data.assign(static_cast<const char*>(message.data()), message.size());
      data[message.size()] = 0;

      m_part_data.push_back(data);

      socket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
      if (!more) {
         break;
      }
   }
   return true;
}

int main (int argc, char *argv[])
{
  int linger = 0;
  int hwm = 100;
  int send_timeout_msec = SEND_TIMEOUT_MSEC; // 1 sec
  int recv_timeout_msec = RECV_TIMEOUT_MSEC; // 3 sec

    zmq::context_t context (1);
    const char* conn_str = "tcp://localhost:5560";

    //  Socket to talk to server
    zmq::socket_t subscriber (context, ZMQ_SUB);

    subscriber.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    subscriber.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    subscriber.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));
    subscriber.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));

    subscriber.connect(conn_str);
    //subscriber.connect("ipc://sbs.ipc");

	const char *filter = (argc > 1)? argv [1]: "KD4005_SS";
    subscriber.setsockopt(ZMQ_SUBSCRIBE, filter, strlen (filter));
    std::cout << "Collecting updates of '" << filter
              << "' from " << conn_str << std::endl;

    //  Process 100 updates
    int update_nbr = 0;
    while (true)
    {
        recv(subscriber);

        std::cout << "#" << update_nbr++ << "\t"
                  << " size: " << m_part_data.size() << "\t"
                  << " code:'" << m_part_data[0] << "'\t"
                  << " head sz:" << m_part_data[2].size() << "\t"
                  << " body sz:" << m_part_data[3].size()
                  << std::endl;
    }

    std::cout 	<< "Done updates for sbs '"<< filter <<"'"
    			<< std::endl;
    return 0;
}
