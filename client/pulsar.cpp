//
//  Send ASKLIFE to PROCESS example.

#include <string>
#include <google/protobuf/stubs/common.h>
#include <glog/logging.h>

#include "zmsg.hpp"
#include "pulsar.hpp"
#include "mdp_client_async_api.hpp"
#include "mdp_letter.hpp"

#include "mdp_common.h"
#include "msg_message.hpp"
#include "proto/common.pb.h"

static int system_exchange_id = 100000;
static int user_exchange_id = 1;
const int num_iter = 1000;

Pulsar::Pulsar(std::string broker, int verbose) : mdcli(broker, verbose)
{
}

mdp::zmsg* generateNewOrder()
{
  std::string pb_serialized_request;
  std::string pb_serialized_header;
  RTDBM::Header     pb_header;
  RTDBM::AskLife    pb_request_asklife;
  RTDBM::ExecResult pb_responce_exec_result;
  mdp::zmsg* request = new mdp::zmsg ();

  // TODO: все поля pb_header должны заполняться автоматически, скрыто от клиента!
  pb_header.set_protocol_version(1);
  pb_header.set_exchange_id(system_exchange_id++);
  pb_header.set_source_pid(getpid());
  pb_header.set_proc_dest("world");
  pb_header.set_proc_origin("pulsar");
  pb_header.set_sys_msg_type(100);
  // Значение этого поля может передаваться служебной фукции 'ОТПРАВИТЬ'
  pb_header.set_usr_msg_type(ADG_D_MSG_ASKLIFE);

  // Единственное поле, которое может устанавливать клиент напрямую
  pb_request_asklife.set_user_exchange_id(user_exchange_id++);

  pb_header.SerializeToString(&pb_serialized_header);
  pb_request_asklife.SerializeToString(&pb_serialized_request);

  request->push_front(pb_serialized_request);
  request->push_front(pb_serialized_header);

  return request;
}

/*
 * Отправить службе NYSE запрос ASKLIFE
 * Служба должна ответить сообщением EXECRESULT
 */
int main (int argc, char *argv [])
{
  int verbose   = (argc > 1 && (strcmp (argv [1], "-v") == 0));
  int num_received = 0;
  ::google::InstallFailureSignalHandler();
  ::google::InitGoogleLogging(argv[0]);

  mdp::zmsg *request   = NULL;
  mdp::zmsg *report    = NULL;
  Pulsar    *client    = new Pulsar ("tcp://localhost:5555", verbose);
  mdp::Letter *letter = NULL;
  RTDBM::AskLife*    pb_asklife;

  try
  {
    for (int i=0; i<num_iter; i++)
    {
      request = generateNewOrder();

      client->send ("NYSE", request);
      if (verbose)
        std::cout << "["<<i+1<<"/"<<num_iter<<"] Send" << std::endl;
      delete request;
    }

    //  Wait for all trading reports
    while (1) {
        report = client->recv();
        if (report == NULL)
            break;
//        report->dump();
        
        letter = new mdp::Letter(report);
        num_received++;
        if (verbose)
        {
            std::cout << "gotcha!"      << std::endl;
            std::cout << "proto ver:"   << (int) letter->header().instance().protocol_version() << std::endl;
            std::cout << "sys exch_id:" << letter->header().instance().exchange_id() << std::endl;
            std::cout << "from:"        << letter->header().instance().proc_origin() << std::endl;
            std::cout << "to:"          << letter->header().instance().proc_dest() << std::endl;
            pb_asklife = static_cast<RTDBM::AskLife*>(letter->data());
            std::cout << "user exch_id:"<< pb_asklife->user_exchange_id() << std::endl;
            std::cout << "==========================================" << std::endl;
        }
        else
        {
          if (!(num_received % 10))
           std::cout << ".";

          if (!(num_received % 100))
           std::cout << "|";

          fflush(stdout);
        }

        delete report;
        delete letter;
    }
  }
  catch (zmq::error_t err)
  {
      std::cout << "E: " << err.what() << std::endl;
  }
  delete client;

  std::cout << std::endl;
  std::cout << "Received "<<num_received<<" of "<<num_iter<<std::endl;

  ::google::ShutdownGoogleLogging();
  return 0;
}

