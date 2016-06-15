#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "modbus.h"
//#ifndef VERBOSE
#include "glog/logging.h"
//#endif

// Служебные файлы RTDBUS
#include "exchange_sysacq_modbus.hpp"

extern volatile int g_interrupt;

// ==========================================================================================
int main(int argc, char* argv[])
{
  RTDBUS_Modbus_client *mbus_client = NULL;
  // Название конфигурационного файла локальной тестовой СС
  char config_filename[2000 + 1] = "BI4500.json";
  int opt;
  int rc = NOK;

//#ifndef VERBOSE
  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();
//#endif

  LOG(INFO) << "RTDBUS MODBUS client " << argv[0] << " START";

  while ((opt = getopt (argc, argv, "c:")) != -1)
  {
     switch (opt)
     {
       case 'c': // Конфигурационный файл системы сбора
         strncpy(config_filename, optarg, 2000);
         config_filename[2000] = '\0';
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

  mbus_client = new RTDBUS_Modbus_client(config_filename);

  rc = mbus_client->prepare();

  while (STATUS_OK_SHUTTINGDOWN != mbus_client->status())
  {
    mbus_client->quantum();
#if 0
    switch (mbus_client->status())
    {
      case STATUS_OK: // Ещё не подключён, все в порядке
      break;
      case STATUS_OK_SMAD_LOAD:     // Ещё не подключён, InternalSMAD загружена
      case STATUS_OK_CONNECTED:     // Подключён, все в порядке
      case STATUS_OK_NOT_CONNECTED: // Не подключён, требуется переподключение
      case STATUS_OK_SHUTTINGDOWN:  // Не подключён, выполняется останов
      case STATUS_FAIL_NEED_RECONNECTED: // Подключён, требуется переподключение
      case STATUS_FAIL_TO_RECONNECT:// Не подключён, переподключение не удалось
      case STATUS_OK_SHUTDOWN:      // Нормальное завершение работы
      case STATUS_FATAL_SMAD:       // Нет возможности продолжать работу из-за проблем с InternalSMAD
      case STATUS_FATAL_CONFIG:     // Нет возможности продолжать работу из-за проблем с конфигурационными файлами
      case STATUS_FATAL_RUNTIME:    // Нет возможности продолжать работу из-за проблем с ОС
      default:
        LOG(ERROR) << "Unexpected state: " << mbus_client->status();
        assert(0 == 1);
    }
#endif
    usleep(mbus_client->timewait());

    if (g_interrupt) { // Прерывание по получении сигнала
      LOG(INFO) << "Shutting down";
      break;
    }
  }

  delete mbus_client;

  LOG(INFO) << "RTDBUS MODBUS client " << argv[0] << " FINISH, rc=" << rc;

//#ifndef VERBOSE
  ::google::ShutdownGoogleLogging();
//#endif

  return (STATUS_OK_SHUTTINGDOWN == rc)? EXIT_SUCCESS : EXIT_FAILURE;
}

