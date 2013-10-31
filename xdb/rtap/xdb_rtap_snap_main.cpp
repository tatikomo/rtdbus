#include "glog/logging.h"
#include "config.h"

#include "xdb_rtap_connection.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_application.hpp"
#include "xdb_rtap_database.hpp"

#include "xdb_rtap_snap_main.hpp"

using namespace xdb;

xdb::RtApplication *app = NULL;
xdb::RtEnvironment *env = NULL;
xdb::RtDbConnection *connection = NULL;
char database_name[SERVICE_NAME_MAXLEN + 1];
char file_name[400];
const char* command_name_LOAD_FROM_XML = "load";
const char* command_name_SAVE_TO_XML = "save";
bool verbose = false;
Commands_t command;

int main(int argc, char** argv)
{
  bool is_database_name_given = false;
  bool is_command_name_given = false;
  bool is_file_name_given = false;
  bool is_translation_given = false;
  char command_name[SERVICE_NAME_MAXLEN + 1];
  int  opt;

  file_name[0] = '\0';
  command_name[0] = '\0';
  database_name[0] = '\0';

  while ((opt = getopt (argc, argv, "vc:f:e:g")) != -1)
  {
     switch (opt)
     {
        case 'v':
          verbose = true;
        break;

        // Режим трансляции instances_total.dat в XML
        case 'g':
          is_translation_given = true;
        break;

        // Задание имени файла
        // -f <имя файла>
        case 'f':
          strncpy(file_name, optarg, SERVICE_NAME_MAXLEN);
          file_name[SERVICE_NAME_MAXLEN] = '\0';
          is_file_name_given = true;
        break;

        // -c <имя команды>
        // команда может быть:
        //   save = сохранить данные из рабочей базы в XML
        //   load = восстановить данные в рабочую базу из XML
        case 'c':
          strncpy(command_name, optarg, SERVICE_NAME_MAXLEN);
          command_name[SERVICE_NAME_MAXLEN] = '\0';

          if (0 == strncmp(command_name,
                           command_name_LOAD_FROM_XML,
                           strlen(command_name_LOAD_FROM_XML)))
          {
            command = LOAD_FROM_XML;
            is_command_name_given = true;
          }
          else if (0 == strncmp(command_name,
                                command_name_SAVE_TO_XML,
                                strlen(command_name_SAVE_TO_XML)))
          {
            command = SAVE_TO_XML;
            is_command_name_given = true;
          }
          else
          {
            LOG(ERROR) << "Unknown comand '" << command_name << "'";
          }
        break;

        case 'e':
          strncpy(database_name, optarg, SERVICE_NAME_MAXLEN);
          database_name[SERVICE_NAME_MAXLEN] = '\0';
          is_database_name_given = true;
        break;

        case '?':
          if (optopt == 'n')
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
          else if (isprint (optopt))
            fprintf (stderr, "Unknown option `-%c'.\n", optopt);
          else
            fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        return 1;

        default:
          abort ();
    }
  }

//  if (false == is_command_name_given)
//    LOG(ERROR) << "Command '-c <name>' not given";
//  if (false == is_database_name_given)
//    LOG(ERROR) << "Database '-e <name>' not given";
  if (is_translation_given)
  {
    if (!is_file_name_given)
    {
      LOG(WARNING) << "Unspecified input file name, 'instances_total.dat' will be used";
      is_file_name_given = true;
      strcpy(file_name, "instances_total.dat");
    }
    // взять входной файл и выдать его в поток выхода
    translateInstance(file_name);
  }
  else
  {
      if ((false == is_command_name_given) || (false == is_database_name_given))
      {
        LOG(ERROR) << "Exiting";
        return 1;
      }

      // Все в порядке, начинаем работу
      RtApplication *app = new RtApplication("xdb_snap");
      app->setOption("OF_RDWR", 1);
      env = app->getEnvironment(database_name);
      
      switch (command)
      {
        case LOAD_FROM_XML:
          if (true == loadFromXML(env, file_name))
          {
            LOG(INFO) << "XML data was successfuly loaded";
          }
        break;

        case SAVE_TO_XML:
          if (true == saveToXML(env, file_name))
          {
            LOG(INFO) << "XML data was successfuly saved";
          }
        break;
      }


      delete connection;
      delete env;
      delete app;
  }

  return 0;
}

