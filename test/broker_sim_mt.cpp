//  Многонитевой имитатор Брокера
//  Параметры командной строки
//  1. Числовое значение количества нитей {1..10}, по-умолчанию 5.
//
//  Asynchronous client-to-server (DEALER to ROUTER)
//
#include <vector>
#include <thread>
#include <memory>
#include <functional>

#include "zmq.hpp"
#include "zhelpers.hpp"

int interrupt_broker = 0;
const char *endpoint = "tcp://localhost:5555";
const char *compiler_options = "g++ -g broker_sim_mt.cpp -o broker_sim_mt -std=c++11 -I/home/gev/ITG/sandbox/cppzmq -I../mdp -I/home/gev/ITG/sandbox/libzmq/include -L/home/gev/ITG/sandbox/libzmq/src/.libs -lzmq";

void signal_handler (int /*signal_value*/)
{
    interrupt_broker = 1;
}

void catch_signals ()
{
    struct sigaction action;
    action.sa_handler = signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}

class broker_instance {
public:
    broker_instance()
        : ctx_(1),
          client_socket_(ctx_, ZMQ_DEALER)
    {
    printf("broker_instance\n");
    }

    void start() {
        // generate random identity
        char identity[10] = {};
        sprintf(identity, "%04X-%04X", within(0x10000), within(0x10000));
        printf("%s\n", identity);
        client_socket_.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
        client_socket_.connect(endpoint);

        zmq::pollitem_t items[] = {{ (void*)client_socket_, 0, ZMQ_POLLIN, 0 }};
        int request_nbr = 0;
        try {
            while (!interrupt_broker) {
                for (int i = 0; i < 2; ++i) {
                    // 10 milliseconds
                    zmq::poll(items, 1, 10);
                    if (items[0].revents & ZMQ_POLLIN) {
                        printf("\n%s ", identity);
                        s_dump(client_socket_);
                    }
#if 0
                    else {
                    printf(".");
                    }
#endif
                }
                char request_string[16] = {};
                sprintf(request_string, "request #%d", ++request_nbr);
                client_socket_.send(request_string, strlen(request_string));
            }
        }
        catch (std::exception &e)
        {
        printf("%s\n", e.what());
        }
    }

private:
    zmq::context_t ctx_;
    zmq::socket_t client_socket_;
};

#define THREAD_MAX 10

int main(int argc, char*argv[])
{
    broker_instance *ct[THREAD_MAX];
    std::thread *t[THREAD_MAX];
    unsigned int threads = 5;

    catch_signals();
    printf("Start async mdp::broker emulation at %s\nCompiler options: %s\n",
            endpoint,
            compiler_options);

    if (argc == 2)
    {
      threads = atoi(argv[1]);
      if ((0 < threads) && (threads < THREAD_MAX))
      {
        printf("Creating %d threads\n", threads);
      }
      else
      {
        printf("Arguments: %s <num threads {1..%d}>\nSet to default 5.\n",
                argv[0], THREAD_MAX);
        threads = 5;
      }
    }

    for (int i=0; i<threads; i++)
    {
      ct[i] = new broker_instance();

      t[i] = new std::thread (std::bind(&broker_instance::start, ct[i]));

      t[i]->detach();
    }

    getchar();

    return 0;
}

