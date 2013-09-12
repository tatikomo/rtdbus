//
//  Send ASKLIFE to PROCESS example.

#include <string>
#include <google/protobuf/stubs/common.h>
#include <glog/logging.h>

#include "zmsg.hpp"
#include "pulsar.hpp"
#include "mdp_client_async_api.hpp"

#include "mdp_common.h"
#include "msg_message.hpp"
#include "proto/common.pb.h"

Pulsar::Pulsar(std::string broker, int verbose) : mdcli(broker, verbose)
{
}

/*
 * Отправить службе NYSE запрос ASKLIFE
 * Служба должна ответить сообщением EXECRESULT
 */
int main (int argc, char *argv [])
{
  int       verbose   = (argc > 1 && (strcmp (argv [1], "-v") == 0));
  ::google::InstallFailureSignalHandler();
  ::google::InitGoogleLogging(argv[0]);

  zmsg    * request   = NULL;
  zmsg    * report    = NULL;
  Pulsar  * client    = new Pulsar ("tcp://localhost:5555", verbose);
  std::string pb_serialized_request;
  std::string pb_serialized_header;
  RTDBM::Header     pb_header;
  RTDBM::AskLife    pb_request_asklife;
  RTDBM::ExecResult pb_responce_exec_result;
//  Letter *payload = NULL;

  static int user_exchange_id = 1;

  try
  {
    request = new zmsg ();
    // TODO: все поля pb_header должны заполняться автоматически, скрыто от клиента!
    pb_header.set_protocol_version(1);
    pb_header.set_exchange_id(9999999);
    pb_header.set_source_pid(getpid());
    pb_header.set_proc_dest("Проба");
    pb_header.set_proc_origin("Я");
    pb_header.set_sys_msg_type(100);
    // Значение этого поля может передаваться служебной фукции 'ОТПРАВИТЬ'
    pb_header.set_usr_msg_type(ADG_D_MSG_ASKLIFE);

    // Единственное поле, которое может устанавливать клиент напрямую
    pb_request_asklife.set_user_exchange_id(user_exchange_id++);

    pb_header.SerializeToString(&pb_serialized_header);
    pb_request_asklife.SerializeToString(&pb_serialized_request);

    request->push_front(pb_serialized_request);
    request->push_front(pb_serialized_header);

    client->send ("NYSE", request);
    delete request;

    //  Wait for all trading reports
    while (1) {
        report = client->recv();
        if (report == NULL)
            break;
//        payload = new Letter(report);
        
        std::cout << "gotcha!" << std::endl;
        delete report;
    }
  }
  catch (zmq::error_t err)
  {
      std::cout << "E: " << err.what() << std::endl;
  }
  delete client;
//  delete payload;

  ::google::ShutdownGoogleLogging();
  return 0;
}

