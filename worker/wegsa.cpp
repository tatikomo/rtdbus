/*
 * ============================================================================
 * Процесс обслуживания источников и потребителей информации
 *
 * eugeni.gorovoi@gmail.com
 * 01/04/2016
 * ============================================================================
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_egsa_impl.hpp"

int main(int argc, char *argv[])
{
  int rc = NOK;
  int opt;
  EGSA* instance = NULL;
  // Значения по-умолчанию
  std::string service_name = EXCHANGE_NAME;
  std::string broker_endpoint = ENDPOINT_BROKER;

  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();

  while ((opt = getopt (argc, argv, "b:s:")) != -1)
  {
     switch (opt)
     {
       case 'b': // точка подключения к Брокеру
         broker_endpoint.assign(optarg);
         break;

       case 's': // название собственной Службы
         service_name.assign(optarg);
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

  instance = new EGSA(broker_endpoint, service_name/*, 1*/);
  LOG(INFO) << "Start host EGSA instance";

  rc = instance->run();

  LOG(INFO) << "Finish host EGSA instance, rc=" << rc;

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();

  return (OK == rc)? EXIT_SUCCESS : EXIT_FAILURE;
}

