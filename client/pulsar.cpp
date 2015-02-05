//
//  Send ASKLIFE to PROCESS example.

#include <string>
#include <google/protobuf/stubs/common.h>
#include <glog/logging.h>

#include "config.h"

#include "mdp_common.h"
#include "mdp_zmsg.hpp"
#include "mdp_client_async_api.hpp"
#include "mdp_letter.hpp"
#include "msg_message.hpp"
#include "proto/common.pb.h"
#include "pulsar.hpp"

const unsigned int MAX_LETTERS = 1200;

//static unsigned int system_exchange_id = 100000;
static unsigned int user_exchange_id = 1;
static unsigned int num_iter = 1200;

rtdbExchangeId id_list[MAX_LETTERS];
typedef enum {
  SENT_OUT      = 1,
  RECEIVED_IN   = 2
} Status;

Pulsar::Pulsar(std::string broker, int verbose) : mdcli(broker, verbose)
{
}

mdp::zmsg* generateNewOrder(rtdbExchangeId id)
{
  std::string pb_serialized_request;
  std::string pb_serialized_header;
  RTDBM::Header     pb_header;
  RTDBM::AskLife    pb_request_asklife;
  RTDBM::ExecResult pb_responce_exec_result;
  mdp::zmsg* request = new mdp::zmsg ();

  // TODO: все поля pb_header должны заполняться автоматически, скрыто от клиента!
  pb_header.set_protocol_version(1);
  pb_header.set_exchange_id(id);
  pb_header.set_source_pid(getpid());
  pb_header.set_proc_dest("world");
  pb_header.set_proc_origin("pulsar");
  pb_header.set_sys_msg_type(100);
  // Значение этого поля может передаваться служебной фукции 'ОТПРАВИТЬ'
  pb_header.set_usr_msg_type(ADG_D_MSG_ASKLIFE);

  // Единственное поле, которое может устанавливать клиент напрямую
  pb_request_asklife.set_user_exchange_id(user_exchange_id++);

  if (false == pb_header.SerializeToString(&pb_serialized_header))
  {
    std::cout << "Unable to serialize header for exchange id "<<id<<std::endl;
  }

  if (false == pb_request_asklife.SerializeToString(&pb_serialized_request))
  {
    std::cout << "Unable to serialize data for exchange id "<<id<<std::endl;
  }

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
  mdp::zmsg *request   = NULL;
  mdp::zmsg *report    = NULL;
  Pulsar    *client    = NULL;
  mdp::Letter *letter = NULL;
  RTDBM::AskLife* pb_asklife = NULL;
  int verbose = 0;
  int service_status;   // 200|400|404|501
  int num_received = 0; // общее количество полученных сообщений
  int num_good_received = 0; // количество полученных сообщений, на которые мы ожидали ответ
  int opt;
  char service_name[SERVICE_NAME_MAXLEN + 1];
  char service_endpoint[ENDPOINT_MAXLEN + 1];
  bool is_service_name_given = false;
  rtdbExchangeId sent_exchange_id;

  ::google::InstallFailureSignalHandler();
  ::google::InitGoogleLogging(argv[0]);

  while ((opt = getopt (argc, argv, "vn:s:")) != -1)
  {
     switch (opt)
     {
       case 'v':
         verbose = 1;
         break;

       case 'n':
         num_iter = atoi(optarg);
         if (num_iter > MAX_LETTERS)
         {
            std::cout << "W: truncate given messages count ("<<num_iter<<") to "<<MAX_LETTERS<<std::endl;
            num_iter = MAX_LETTERS;
         }
         break;

       case 's':
         strncpy(service_name, optarg, SERVICE_NAME_MAXLEN);
         service_name[SERVICE_NAME_MAXLEN] = '\0';
         is_service_name_given = true;
         break;

       case '?':
         if (optopt == 'n')
           fprintf (stderr, "Option -%c requires an argument.\n", optopt);
         else if (isprint (optopt))
           fprintf (stderr, "Unknown option `-%c'.\n", optopt);
         else
           fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
         return 1;
       default:
         abort ();
     }
  }

  if (!is_service_name_given)
  {
    std::cout << "Service name not given.\nUse -s <name> option.\n";
    return(1);
  }

  client = new Pulsar ("tcp://localhost:5555", verbose);

  try
  {
    std::cout << "Checking '" << service_name << "' status " << std::endl;
    service_status = client->ask_service_info(service_name, service_endpoint, ENDPOINT_MAXLEN);

    switch(service_status)
    {
      case 200:
      std::cout << service_name << " status OK : " << service_status << std::endl;
      std::cout << "Get point to " << service_endpoint << std::endl;
      break;

      case 400:
      std::cout << service_name << " status BAD_REQUEST : " << service_status << std::endl;
      break;

      case 404:
      std::cout << service_name << " status NOT_FOUND : " << service_status << std::endl;
      break;

      case 501:
      std::cout << service_name << " status NOT_SUPPORTED : " << service_status << std::endl;
      break;

      default:
      std::cout << service_name << " status UNKNOWN : " << service_status << std::endl;
    }

    if (200 != service_status)
    {
      std::cout << "Service '" << service_name << "' is not running, exiting" << std::endl;
      exit(service_status);
    }

    for (sent_exchange_id=1; sent_exchange_id<=num_iter; sent_exchange_id++)
    {
      request = generateNewOrder(sent_exchange_id);

      client->send (service_name, request);
      if (verbose)
        std::cout << "["<<sent_exchange_id<<"/"<<num_iter<<"] Send" << std::endl;
      delete request;

      id_list[sent_exchange_id] = SENT_OUT;
#if 0
    }

    //  Wait for all trading reports
    while (1) {
        report = client->recv();
        if (report == NULL)
            break;
        
        letter = new mdp::Letter(report);
        num_received++;
        rtdbExchangeId recv_message_exchange_id = (static_cast<RTDBM::AskLife*>(letter->data()))->user_exchange_id();
        if ((1 <= recv_message_exchange_id) && (recv_message_exchange_id <= num_iter))
        {
          // Идентификатор в пределах положенного
          if (id_list[recv_message_exchange_id] != SENT_OUT)
          {
            LOG(ERROR) << "Get responce with unwilling exchange id: "<<recv_message_exchange_id;
          }
          else if (id_list[recv_message_exchange_id] == SENT_OUT)
          {
            id_list[recv_message_exchange_id] = RECEIVED_IN;
            num_good_received++;
          }
        }
        else
        {
          LOG(ERROR) << "Get responce with unknown exchange id: "<<recv_message_exchange_id;
        }

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
#else
        report = client->recv();
        if (report == NULL)
            break;

        if (verbose)
          report->dump();
            
        letter = new mdp::Letter(report);
        num_received++;
        rtdbExchangeId recv_message_exchange_id = (static_cast<RTDBM::AskLife*>(letter->data()))->user_exchange_id();
        if ((1 <= recv_message_exchange_id) && (recv_message_exchange_id <= num_iter))
        {
          // Идентификатор в пределах положенного
          if (id_list[recv_message_exchange_id] != SENT_OUT)
          {
            LOG(ERROR) << "Got responce with unwilling exchange id: "<<recv_message_exchange_id;
          }
          else if (id_list[recv_message_exchange_id] == SENT_OUT)
          {
            id_list[recv_message_exchange_id] = RECEIVED_IN;
            num_good_received++;
          }
        }
        else
        {
          LOG(ERROR) << "Got responce with unknown exchange id: "<<recv_message_exchange_id;
        }

        if (!(num_received % 100))
        {
           std::cout << "|";
        }
        else if (!(num_received % 10))
        {
           std::cout << ".";
        }
        fflush(stdout);

        delete report;
        delete letter;
    } // end for
#endif
  }
  catch (zmq::error_t err)
  {
      std::cout << "E: " << err.what() << std::endl;
  }
  delete client;

  std::cout << std::endl;
  std::cout << "Received "<<num_received<<" of "<<num_iter
  <<", exchange id was approved for "<<num_good_received<<std::endl;

  // Проверить, все ли отправленные сообщения получены
  if (num_good_received < num_received)
  {
    for (unsigned int k=0; k<num_iter; k++)
    {
      if (id_list[k] != RECEIVED_IN)
      {
        std::cout << "Response for letter with id "<<k<<" wasn't receided"<<std::endl;
      }
    }
  }

  ::google::ShutdownGoogleLogging();
  return 0;
}

