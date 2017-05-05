//
//  Send ASKLIFE to PROCESS example.
//

// Общесистемные заголовочные файлы
#include <string>
// Служебные заголовочные файлы сторонних утилит
#include "google/protobuf/stubs/common.h"
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "config.h"

#include "mdp_common.h"
#include "mdp_zmsg.hpp"
#include "mdp_client_async_api.hpp"
#include "msg_message.hpp"
#include "proto/common.pb.h"
#include "pulsar.hpp"

#include "../tools/helper.hpp"
#include "../tools/helper.cpp"

const unsigned int MAX_LETTERS = 1200;

//static unsigned int system_exchange_id = 100000;
//static unsigned int user_exchange_id = 1;
static unsigned int num_iter = 1200;


rtdbExchangeId id_list[MAX_LETTERS];
Pulsar::Pulsar(const std::string& broker, int verbose)
    :
    mdcli(broker, verbose),
    m_channel(ChannelType::PERSISTENT) // По умолчанию обмен сообщениями со Службой через Брокер
{
  usleep(500000); // Подождать 0.5 сек, чтобы ZMQ успело физически подключиться к Брокеру

  // Подготовиться к работе, заполнив связь "Название сообщения" <=> "Тип сообщения"
  init();
}

Pulsar::~Pulsar()
{
}

void Pulsar::init()
{
  m_msg_type_hash.insert(MessageTypesHashPair_t("ADG_D_MSG_EXECRESULT", ADG_D_MSG_EXECRESULT));
  m_msg_type_hash.insert(MessageTypesHashPair_t("ADG_D_MSG_ENDINITACK", ADG_D_MSG_ENDINITACK));
  m_msg_type_hash.insert(MessageTypesHashPair_t("ADG_D_MSG_INIT",       ADG_D_MSG_INIT));
  m_msg_type_hash.insert(MessageTypesHashPair_t("ADG_D_MSG_STOP",       ADG_D_MSG_STOP));
  m_msg_type_hash.insert(MessageTypesHashPair_t("ADG_D_MSG_ENDALLINIT", ADG_D_MSG_ENDALLINIT));
  m_msg_type_hash.insert(MessageTypesHashPair_t("ADG_D_MSG_ASKLIFE",    ADG_D_MSG_ASKLIFE));
  m_msg_type_hash.insert(MessageTypesHashPair_t("ADG_D_MSG_LIVING",     ADG_D_MSG_LIVING));
  m_msg_type_hash.insert(MessageTypesHashPair_t("ADG_D_MSG_STARTAPPLI", ADG_D_MSG_STARTAPPLI));
  m_msg_type_hash.insert(MessageTypesHashPair_t("ADG_D_MSG_STARTAPPLI", ADG_D_MSG_STARTAPPLI));
  m_msg_type_hash.insert(MessageTypesHashPair_t("ADG_D_MSG_DIFINIT",    ADG_D_MSG_DIFINIT));
  m_msg_type_hash.insert(MessageTypesHashPair_t("SIG_D_MSG_READ_MULTI", SIG_D_MSG_READ_MULTI));
  m_msg_type_hash.insert(MessageTypesHashPair_t("SIG_D_MSG_WRITE_MULTI",SIG_D_MSG_WRITE_MULTI));
}

rtdbMsgType Pulsar::getMsgTypeByName(std::string& type_name)
{
  rtdbMsgType found = MESSAGE_TYPE_UNKNOWN;

  std::cout << "looking \"" << type_name << "\" message" << std::endl;
  const MessageTypesHash_t::const_iterator it = m_msg_type_hash.find(type_name);
  if (it != m_msg_type_hash.end())
  {
    found = it->second;
  }

  return found;
}


void Pulsar::fire_messages()
{
  switch(m_channel)
  {
    case ChannelType::DIRECT:
    fire_direct();
    break;

    case  ChannelType::PERSISTENT:
    fire_persistent();
    break;

    default:
    LOG(ERROR) << "Unsupported message transport: " << m_channel;
  }
}

void Pulsar::fire_direct()
{
  LOG(INFO) << "send direct messages to a service";
}

void Pulsar::fire_persistent()
{
  LOG(INFO) << "send messages to a service via broker";
}

// Создать сообщение и заполнить ему требуемые поля
mdp::zmsg* generateNewOrder(msg::MessageFactory* factory, rtdbExchangeId id, const char* service_name, rtdbMsgType type)
{
  msg::Letter *letter;
  mdp::zmsg* request = new mdp::zmsg ();

  letter = factory->create(type);
  assert(letter);
  assert(letter->header()->usr_msg_type() == type);

  letter->header()->set_exchange_id(id);
  letter->set_destination(service_name);

  // Единственное поле, которое должен устанавливать клиент напрямую
  //letter->data()->set_status(1);
  // NB: для этого нужно городить огромный switch с перечислением всех известных типов сообщений
  std::string hd = letter->header()->get_serialized();
  std::string dt = letter->data()->get_serialized();
  hex_dump(hd);
  hex_dump(dt);

  request->push_front(dt);
  request->push_front(hd);

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
  int status;
  int rc = OK;
  Pulsar    *client    = NULL;
  msg::Letter *letter = NULL;
  msg::MessageFactory *message_factory = NULL;
  rtdbMsgType          mess_type = ADG_D_MSG_ASKLIFE;
  bool                 is_type_name_given = false;
  std::string type_name;//[SERVICE_NAME_MAXLEN + 1];
  int verbose = 0;
  int service_status;   // 200|400|404|501
  int num_received = 0; // общее количество полученных сообщений
  int num_good_received = 0; // количество полученных сообщений, на которые мы ожидали ответ
  int opt;
  std::string service_name_str;
  char service_endpoint[ENDPOINT_MAXLEN + 1];
  // Указано название Службы
  bool is_service_name_given = false;
  ChannelType channel = ChannelType::PERSISTENT;
  // По умолчанию обмен сообщениями со Службой через Брокер
  rtdbExchangeId sent_exchange_id;
  std::string broker_endpoint = ENDPOINT_BROKER;

  ::google::InstallFailureSignalHandler();
  ::google::InitGoogleLogging(argv[0]);

  while ((opt = getopt (argc, argv, "b:vdn:s:t:")) != -1)
  {
     switch (opt)
     {
       case 'v':
         verbose = 1;
         break;

       case 'b': // точка подключения к Брокеру
         broker_endpoint.assign(optarg);
         break;

       case 'n':
         num_iter = atoi(optarg);
         if (num_iter > MAX_LETTERS)
         {
            std::cout << "W: truncate given messages count ("<<num_iter<<") to "<<MAX_LETTERS<<std::endl;
            num_iter = MAX_LETTERS;
         }
         break;

       case 'd': // Direct - прямые сообщения в адрес Службы
         channel = ChannelType::DIRECT;
         break;

       case 's':
         char service_name[SERVICE_NAME_MAXLEN + 1];
         strncpy(service_name, optarg, SERVICE_NAME_MAXLEN);
         service_name[SERVICE_NAME_MAXLEN] = '\0';
         service_name_str.assign(service_name);
         is_service_name_given = true;
         break;

       case 't':
         char message_type[SERVICE_NAME_MAXLEN + 1];
         strncpy(message_type, optarg, SERVICE_NAME_MAXLEN);
         message_type[SERVICE_NAME_MAXLEN] = '\0';
         type_name.assign(message_type);
         is_type_name_given = true;
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
    std::cout << "Service name not given.\nUse -s <name> [-v] [-d] [-t <message type name>] option.\n";
    return(1);
  }

  client = new Pulsar (broker_endpoint, verbose);

  if (!is_type_name_given)
  {
    std::cout << "Message type name not given, will use ASKLIFE.\n";
    mess_type = ADG_D_MSG_ASKLIFE;
  }
  else if (MESSAGE_TYPE_UNKNOWN == (mess_type = client->getMsgTypeByName(type_name)))
  {
    std::cout << "Given message type name (" << type_name << ") is not known, will use ASKLIFE.\n";
    mess_type = ADG_D_MSG_ASKLIFE;
  }

  try
  {
    std::cout << "Checking '" << service_name_str << "' status " << std::endl;
    service_status = client->ask_service_info(service_name_str.c_str(), service_endpoint, ENDPOINT_MAXLEN);


    switch(service_status)
    {
      case 200:
      std::cout << service_name_str << " status OK : " << service_status << std::endl;
      std::cout << "Get point to " << service_endpoint << std::endl;
      break;

      case 400:
      std::cout << service_name_str << " status BAD_REQUEST : " << service_status << std::endl;
      break;

      case 404:
      std::cout << service_name_str << " status NOT_FOUND : " << service_status << std::endl;
      break;

      case 501:
      std::cout << service_name_str << " status NOT_SUPPORTED : " << service_status << std::endl;
      break;

      default:
      std::cout << service_name_str << " status UNKNOWN : " << service_status << std::endl;
    }

    if (200 != service_status)
    {
      std::cout << "Service '" << service_name_str << "' is not running, exiting" << std::endl;
      exit(service_status);
    }

    message_factory = new msg::MessageFactory("pulsar");

    for (sent_exchange_id=1; sent_exchange_id<=num_iter; sent_exchange_id++)
    {
      request = generateNewOrder(message_factory, sent_exchange_id, service_name_str.c_str(), mess_type);

      // Способ передачи указан в channel:
      // - через Брокера (PERSISTENT)
      // - напрямую процессу Службы (DIRECT)
      client->send (service_name_str, request, channel);

      if (verbose)
        std::cout << "["<<sent_exchange_id<<"/"<<num_iter<<"] Send" << std::endl;
      delete request;

      id_list[sent_exchange_id] = SENT_OUT;

#ifdef ASYNC_EXCHANGE
      // Асинхронный обмен
    }

    //  Wait for all trading reports
    while (1) {
        status = client->recv(report);
        if (Pulsar::RECEIVE_OK != status)
            break;

        letter = message_factory->create(report);
        num_received++;
        rtdbExchangeId recv_message_exchange_id = letter->header()->exchange_id();
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
            std::cout << "interest_id:" << letter->header().instance().interest_id() << std::endl;
            std::cout << "from:"        << letter->header().instance().proc_origin() << std::endl;
            std::cout << "to:"          << letter->header().instance().proc_dest() << std::endl;
            pb_asklife = static_cast<RTDBM::AskLife*>(letter->data());
            std::cout << "user status:"<< pb_asklife->status() << std::endl;
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
        // Синхронный обмен
        //-------------------------------------------------------------
        status = client->recv(report);
        if (Pulsar::RECEIVE_OK != status)
            break;

        letter = message_factory->create(report);
        num_received++;
        //rtdbExchangeId recv_message_exchange_id = letter->header()->exchange_id();
        rtdbExchangeId recv_message_exchange_id = letter->header()->exchange_id();
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
      rc = NOK;
  }
  delete message_factory;
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
  return (OK == rc)? EXIT_SUCCESS : EXIT_FAILURE;
}

