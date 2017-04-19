#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

// Служебные файлы RTDBUS
#include "mdp_common.h"
#include "wtimekeeper.hpp"

extern int interrupt_worker;

int TimeKeeper::handle_request(mdp::zmsg* request, std::string*& reply_to)
{
  int rc = OK;

  assert (request->parts () >= 1);
  LOG(INFO) << "Process new request with " << request->parts() 
    << " parts and reply to " << reply_to;

  request->dump();

  request->unwrap();
  request->wrap(reply_to->c_str(), EMPTY_FRAME);

  request->dump();
  send_to_broker((char*) MDPW_REPORT, NULL, request);

  return rc;
}

int main(int argc, char* argv[])
{
  int rc = OK;
  int verbose = (argc > 1 && (0 == strcmp (argv [1], "-v")));

  ::google::InstallFailureSignalHandler();
  ::google::InitGoogleLogging(argv[0]);

  try
  {
    TimeKeeper *keeper = new TimeKeeper("tcp://localhost:5555", "timekeeper", verbose);

    LOG(INFO) << "TIMEKEEPER START";

    while (!interrupt_worker) 
    {
       std::string *reply_to = new std::string;
       mdp::zmsg   *request  = NULL;

       request = keeper->recv (reply_to);
       if (request)
         keeper->handle_request (request, reply_to);
       else
         break;          // Worker has been interrupted
    }
    delete keeper;
  }
  catch(zmq::error_t err)
  {
    std::cout << "E: " << err.what() << std::endl;
    rc = NOK;
  }

  LOG(INFO) << "TIMEKEEPER FINISH";

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();

  return (OK == rc)? EXIT_SUCCESS : EXIT_FAILURE;
}
