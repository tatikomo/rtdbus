#include <stdio.h> // stderr
#include <stdlib.h> // abort()

#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_rtap_connection.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_application.hpp"
#include "xdb_rtap_database.hpp"

#include "xdb_rtap_snap.hpp"

using namespace xdb;

static char database_name[SERVICE_NAME_MAXLEN + 1];
static char file_path[400+1];
static const char* command_name_LOAD_FROM_XML = "load";
static const char* command_name_SAVE_TO_XML = "save";
static bool verbose = false;
static Commands_t command;

int main(int argc, char** argv)
{
  bool is_database_name_given = false;
  bool is_command_name_given = false;
  bool is_file_path_given = false;
  bool is_translation_given = false;
  char command_name[SERVICE_NAME_MAXLEN + 1];
  int  opt;
  RtConnection *connection = NULL;

  file_path[0] = '\0';
  command_name[0] = '\0';
  database_name[0] = '\0';

  while ((opt = getopt (argc, argv, "vc:p:e:g")) != -1)
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
        // -p <путь>
        case 'p':
          strncpy(file_path, optarg, 400);
          file_path[400] = '\0';
          is_file_path_given = true;
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
    if (!is_file_path_given)
    {
     getcwd(file_path, sizeof(file_path));
    }
    // взять входной файл и выдать его в поток выхода
    if (false == translateInstance(file_path))
    {
      LOG(ERROR) << "Can't translating " << file_path;
    }
  }
  else
  {
      if ((false == is_command_name_given) || (false == is_database_name_given))
      {
        LOG(ERROR) << "Command or database name not specified, exiting";
        return 1;
      }

      // Все в порядке, начинаем работу
      RtApplication *app = new RtApplication("xdb_snap");
      app->setOption("OF_TRUNCATE", 1);
      app->setOption("OF_RDWR", 1);
      RtEnvironment *env = app->loadEnvironment(database_name);
      
      switch (command)
      {
        case LOAD_FROM_XML:
          if (true == loadFromXML(env, file_path))
          {
            LOG(INFO) << "XML data was successfuly loaded";
          }
        break;

        case SAVE_TO_XML:
          if (true == saveToXML(env, file_path))
          {
            LOG(INFO) << "XML data was successfuly saved";
          }
        break;
      }


      delete connection;
      delete app;
  }

  return 0;
}

