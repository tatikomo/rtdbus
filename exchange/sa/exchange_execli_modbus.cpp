#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "modbus.h"
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_sysacq_modbus.hpp"

extern volatile int g_interrupt;

// ==========================================================================================
int main(int argc, char* argv[])
{
  RTDBUS_Modbus_client *mbus_client = NULL;
  // Название конфигурационного файла локальной тестовой СС
  std::string service_name = "BI4500";
  std::string broker_endpoint = ENDPOINT_BROKER;
  int opt;
  int rc = NOK;

  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();

  LOG(INFO) << "RTDBUS MODBUS client " << argv[0] << " START";

  while ((opt = getopt (argc, argv, "s:")) != -1)
  {
     switch (opt)
     {
        case 'b': // точка подключения к Брокеру
         broker_endpoint.assign(optarg, SERVICE_NAME_MAXLEN);
         break;

       case 's': // Название Службы
         // NB: Конфигурационный файл системы сбора получается из названия Службы
         service_name.assign(optarg, SERVICE_NAME_MAXLEN);
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

  mbus_client = new RTDBUS_Modbus_client(broker_endpoint, service_name);

  rc = mbus_client->prepare();

  // Бесконечный цикл обработки сообщений
  // Выход из цикла - завершение работы
  mbus_client->run();

//1    mbus_client->quantum();
  switch (mbus_client->status())
  {
    case STATUS_OK: // Ещё не подключён, все в порядке
    case STATUS_OK_SMAD_LOAD:     // Ещё не подключён, InternalSMAD загружена
    case STATUS_OK_CONNECTED:     // Подключён, все в порядке
    case STATUS_OK_NOT_CONNECTED: // Не подключён, требуется переподключение
    case STATUS_OK_SHUTTINGDOWN:  // Не подключён, выполняется останов
    case STATUS_OK_SHUTDOWN:      // Нормальное завершение работы
      LOG(INFO) << "Normal, status=" << mbus_client->status();
      break;

    case STATUS_FAIL_NEED_RECONNECTED: // Подключён, требуется переподключение
    case STATUS_FAIL_TO_RECONNECT:// Не подключён, переподключение не удалось
      LOG(INFO) << "Fail, status=" << mbus_client->status();
      break;

    case STATUS_FATAL_SMAD:       // Нет возможности продолжать работу из-за проблем с InternalSMAD
    case STATUS_FATAL_CONFIG:     // Нет возможности продолжать работу из-за проблем с конфигурационными файлами
    case STATUS_FATAL_RUNTIME:    // Нет возможности продолжать работу из-за проблем с ОС
      LOG(INFO) << "Fatal, status=" << mbus_client->status();

    default:
      LOG(ERROR) << "Unexpected, status=" << mbus_client->status();
      assert(0 == 1);
  }

  delete mbus_client;

  LOG(INFO) << "RTDBUS MODBUS client " << argv[0] << " FINISH, rc=" << rc;

  ::google::ShutdownGoogleLogging();

  return (STATUS_OK_SHUTTINGDOWN == rc)? EXIT_SUCCESS : EXIT_FAILURE;
}

