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

// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_egsa_host.hpp"

int main(int argc, char *argv[])
{
  int rc = NOK;
  int opt;
  EGSA_Host* instance = NULL;
  std::string service_name = EXCHANGE_NAME; //[SERVICE_NAME_MAXLEN + 1];
  std::string broker_endpoint = ENDPOINT_BROKER; //[ENDPOINT_MAXLEN + 1];

  ::google::InstallFailureSignalHandler();

  // Значения по-умолчанию
//  strcpy(broker_endpoint, ENDPOINT_BROKER);
//  strcpy(service_name, EXCHANGE_NAME);

  while ((opt = getopt (argc, argv, "b:s:")) != -1)
  {
     switch (opt)
     {
       case 'b': // точка подключения к Брокеру
//         strncpy(broker_endpoint, optarg, ENDPOINT_MAXLEN);
//         broker_endpoint[ENDPOINT_MAXLEN] = '\0';
         broker_endpoint.assign(optarg);
         break;

       case 's': // название собственной Службы
//         strncpy(service_name, optarg, SERVICE_NAME_MAXLEN);
//         service_name[SERVICE_NAME_MAXLEN] = '\0';
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

  instance = new EGSA_Host(broker_endpoint, service_name, 1);
  LOG(INFO) << "Start host EGSA instance";

  rc = instance->run();

  LOG(INFO) << "Finish host EGSA instance, rc=" << rc;
  //::google::protobuf::ShutdownProtobufLibrary();

  return (OK == rc)? EXIT_SUCCESS : EXIT_FAILURE;
}

