/*
 * ============================================================================
 * Процесс обработки и накопления архивных данных
 * Аналоговые значения выбираются из БДРВ раз в минуту и заносятся в таблицу
 * минутных семплов.
 *
 * В минутные семплы попадают текущие значения и достоверность атрибута.
 * В пятиминутные семплы попадают средние арифметические значений и максимальное
 * значение достоверности из минутных семплов.
 *
 * Дискретные значения поступают в виде сообщений от DiggerWorker при изменении
 * значений соответствующих атрибутов VALIDCHANGE.
 *
 * eugeni.gorovoi@gmail.com
 * 01/11/2015
 * ============================================================================
 */

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

#include "warchivist.hpp"

Archivist::Archivist(std::string& broker_endpoint, std::string& service)
 : mdp::mdwrk(broker_endpoint, service, 1),
   m_broker_endpoint(broker_endpoint),
   m_service_name(service)
{
  LOG(INFO) << "Start service " << m_service_name;
}

Archivist::~Archivist()
{
  LOG(INFO) << "Finish service " << m_service_name;
}

int main(int argc, char *argv[])
{
  Archivist* archivist = NULL;
  int  opt;
  char service_name[SERVICE_NAME_MAXLEN + 1];
  char broker_endpoint[ENDPOINT_MAXLEN + 1];

  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();

  // Значения по-умолчанию
  strcpy(broker_endpoint, ENDPOINT_BROKER);
  strcpy(service_name, ARCHIVIST_NAME);

  while ((opt = getopt (argc, argv, "b:s:")) != -1)
  {
     switch (opt)
     {
       case 'b': // точка подключения к Брокеру
         strncpy(broker_endpoint, optarg, ENDPOINT_MAXLEN);
         broker_endpoint[ENDPOINT_MAXLEN] = '\0';
         break;

       case 's': // название собственной Службы
         strncpy(service_name, optarg, SERVICE_NAME_MAXLEN);
         service_name[SERVICE_NAME_MAXLEN] = '\0';
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

  std::string bro_endpoint(broker_endpoint);
  std::string srv_name(service_name);
  archivist = new Archivist(bro_endpoint, srv_name);

  sleep (1);

  delete archivist;

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();

  return 0;
}

