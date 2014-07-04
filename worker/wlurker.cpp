#include <assert.h>
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

#include "xdb_rtap_application.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"

#include "wlurker.hpp"

#include "msg_common.h"
#include "mdp_worker_api.hpp"
#include "mdp_letter.hpp"
#include "mdp_zmsg.hpp"

extern int s_interrupted;

Lurker::Lurker(std::string broker, std::string service, int verbose)
   : mdp::mdwrk(broker, service, verbose)
{
  m_appli = new xdb::rtap::RtApplication("LURKER");
  m_appli->setOption("OF_CREATE",1);    // Создать если БД не было ранее
  m_appli->setOption("OF_RDWR",1);      // Открыть БД для чтения/записи
  m_appli->setEnvName("RTAP");
  m_appli->initialize();

  m_environment = m_appli->getEnvironment("RTAP");
  m_db_connection = m_environment->createConnection();
}

Lurker::~Lurker()
{
  delete m_db_connection;
  delete m_environment;
  // NB Проверить - RtEnvironment удаляется в деструкторе RtApplication ?
  delete m_appli;
}

int Lurker::handle_request(mdp::zmsg* request, std::string*& reply_to)
{
  rtdbMsgType msgType;

  assert (request->parts () >= 2);
  LOG(INFO) << "Process new request with " << request->parts() 
            << " parts and reply to " << *reply_to;

  mdp::Letter *letter = new mdp::Letter(request);
  if (letter->GetVALIDITY())
  {
    msgType = letter->header().get_usr_msg_type();

    switch(msgType)
    {
      case SIG_D_MSG_READ_SINGLE:
      case SIG_D_MSG_READ_MULTI:
       handle_read(letter, reply_to);
       break;

      case SIG_D_MSG_WRITE_SINGLE:
      case SIG_D_MSG_WRITE_MULTI:
       handle_write(letter, reply_to);
       break;

      default:
       LOG(ERROR) << "Unsupported request type: " << msgType;
    }
  }
  else
  {
    LOG(ERROR) << "Readed letter "<<letter->header().get_exchange_id()<<" not valid";
  }

  delete letter;
  return 0;
}

int Lurker::handle_read(mdp::Letter*, std::string*)
{
  LOG(INFO) << "Processing read request";
  return 0;
}

int Lurker::handle_write(mdp::Letter*, std::string*)
{
  LOG(INFO) << "Processing write request";
  return 0;
}


int main(int argc, char **argv)
{
  int  verbose = (argc > 1 && (0 == strcmp (argv [1], "-v")));
  char service_name[SERVICE_NAME_MAXLEN + 1];
  bool is_service_name_given = false;
  int  opt;
  Lurker *engine = NULL;

  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();

  while ((opt = getopt (argc, argv, "vs:")) != -1)
  {
     switch (opt)
     {
       case 'v':
         verbose = 1;
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
    std::cout << "Service name not given.\nUse '-s <service>' option.\n";
    return(1);
  }

  try
  {
    engine = new Lurker("tcp://localhost:5555", service_name, verbose);

    LOG(INFO) << "Hello lurker!";

    while (!s_interrupted) 
    {
       std::string *reply_to = new std::string;
       mdp::zmsg   *request  = NULL;

       request = engine->recv (reply_to);
       if (request)
       {
//         letter = new (request);
         engine->handle_request (request/*letter*/, reply_to);
         delete request;
       }
       else
       {
         s_interrupted = true; // Worker has been interrupted
       }
       delete reply_to;
    }
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
  }
  delete engine;

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();
  return 0;
}

