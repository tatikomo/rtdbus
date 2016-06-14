#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"
#include "modbus.h"

// Служебные файлы RTDBUS
#include "msg_common.h"
#include "mdp_worker_api.hpp"
#include "mdp_zmsg.hpp"

#include "exchange_modbus_cli.hpp"
#include "wmbus_cli.hpp"

using namespace mdp;

MBusClient::MBusClient(const std::string& broker_endpoint, const std::string& service)
 : mdwrk(broker_endpoint, service, true),
   m_impl(NULL),
   m_status(0)
{
  LOG(INFO) << "create new modbus client worker";
  // Название конфигурационного файла локальной тестовой СС
  char config_filename[2000 + 1] = "BI4500.json";
  m_impl = new RTDBUS_Modbus_client(config_filename);
}

MBusClient::~MBusClient()
{
  delete m_impl;
  LOG(INFO) << "destroy modbus client worker";
}

int main(int argc, char* argv[])
{
  std::string broker_endpoint = ENDPOINT_BROKER;
  std::string service_name = "BI4001";
  bool is_service_name_given = false;
  int opt;
  int verbose = 0;
  MBusClient* client = NULL;
  char one_argument[SERVICE_NAME_MAXLEN + 1] = "";
  static const char *arguments_template =
              "-b <broker_address> "
              "-s <service_name> [-v] ";
  char arguments_out[255];

  while ((opt = getopt (argc, argv, "b:vs:")) != -1)
  {
    switch (opt)
    {
        case 'v': // режим подробного вывода
          verbose = 1;
          break;

       case 'b': // точка подключения к Брокеру
         broker_endpoint.assign(optarg);
         break;

        case 's':
          strncpy(one_argument, optarg, SERVICE_NAME_MAXLEN);
          one_argument[SERVICE_NAME_MAXLEN] = '\0';
          service_name.assign(one_argument);
          is_service_name_given = true;
          std::cout << "Service is \"" << service_name << "\"" << std::endl;
          break;
    }
  }

  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();

  client = new MBusClient(broker_endpoint, service_name);
  delete client;

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();

  return EXIT_SUCCESS;
}

