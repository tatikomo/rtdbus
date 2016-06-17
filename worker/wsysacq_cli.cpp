#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <thread>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

// Служебные файлы RTDBUS
#include "mdp_zmsg.hpp"
// общий интерфейс сообщений
#include "msg_common.h"
#include "msg_message.hpp"
// сообщения общего порядка
#include "msg_adm.hpp"
// сообщения по работе с БДРВ
#include "msg_sinf.hpp"
#include "mdp_worker_api.hpp"
#include "exchange_sysacq_intf.hpp"
#include "exchange_sysacq_modbus.hpp"
//#include "exchange_config_sac.hpp"
#include "wsysacq_cli.hpp"

using namespace mdp;

extern volatile int interrupt_worker;

// --------------------------------------------------------------------------------
SystemAcquisitionModule::SystemAcquisitionModule(const std::string& broker_endpoint,  // Точка подключения к Брокеру
                       const std::string& service,          // Название Службы
                       const std::string& config_filename)  // Название конфигурационного файла локальной тестовой СС
 : mdwrk(broker_endpoint, service, true),
   m_impl(NULL),
   m_to_slave(m_context, ZMQ_PAIR),
   m_status(0),
   m_message_factory(new msg::MessageFactory(service.c_str())),
   m_servant_thread(NULL)
{
  LOG(INFO) << "create new system acquisition worker";
  // TODO: На основании конфигурационного файла определить, что за тип у данной системы сбора
#warning "TODO: determine the given SA type by it's config file"

  // Значения общих характеристик Системы Сбора
  sa_common_t common_data;
#if 0
  // Класс для разбора конфигурационных файлов
  AcquisitionSystemConfig config(config_filename);
  // Прочесть общие харатеристики
  config.load_common(common_data);
#else
  // NB: Сейчас пока считаем, что это всегда будет MODBUS
  common_data.nature = GOF_D_SAC_NATURE_EELE;
#endif

  switch(common_data.nature) {
    case GOF_D_SAC_NATURE_EELE:
      m_impl = new RTDBUS_Modbus_client(config_filename, &m_context);
      break;
    default:
      LOG(FATAL) << "Unsupported System Acquisition type: " << common_data.nature;
      assert(0 == 1);
  }

  // Изменить время таймаута приема сообщения
  set_recv_timeout(1000000); // 1 sec
  // Изменения активизируются после нового подключения к Брокеру
  connect_to_broker();

  m_to_slave.bind(s_SA_INTERNAL_EXCHANGE_ENDPOINT);
}

// --------------------------------------------------------------------------------
SystemAcquisitionModule::~SystemAcquisitionModule()
{
  LOG(INFO) << "start system acquisition destructor";
  delete m_impl;
  m_to_slave.close();
  delete m_message_factory;
  LOG(INFO) << "finish system acquisition destructor";
}

// --------------------------------------------------------------------------------
// Нельзя устраивать задержки с помощью sleep, поскольку в это время процесс не получает
// сообщения Брокера и Клиентов, что ведет к нарушению работы.
int SystemAcquisitionModule::run()
{
  const char* fname = "run";
  int status = m_impl->prepare();
  int old_status = 255;
  int new_status = 255;
  bool status_changed = false;
  std::string *reply_to = new std::string;
  mdp::zmsg *request  = NULL;

  interrupt_worker = 0;

#if (VERBOSE > 2)
  LOG(INFO) << fname << ": start";
#endif
  // Запустить нить интерфейса
  m_servant_thread = new std::thread(std::bind(&SysAcqInterface::run, m_impl));

  // Ожидание завершения работы
  while (!interrupt_worker
     && (STATUS_OK_SHUTTINGDOWN != (status = m_impl->status())))
  {
    request = NULL;

    // NB: Функция recv возвращает только PERSISTENT-сообщения
    request = recv (reply_to);
    if (request)
    {
#if (VERBOSE > 5)
      LOG(INFO) << fname << ": got a message";
#endif

      handle_request (request, reply_to);

      delete request;
    }
  
#if 0
    if (old_status != m_impl->status()) {
      old_status = m_impl->status();
      status_changed = true;
    }

    status = m_impl->quantum();

    if (status_changed) {
      new_status = m_impl->status();
      switch(new_status) {
        case STATUS_FAIL_NEED_RECONNECTED:
        case STATUS_OK_NOT_CONNECTED:
          if (old_status == new_status) {
            // подождать секунду, идет переподключение
            sleep(1);
          }
          break;

        case STATUS_OK_CONNECTED:
          usleep(100000);
          if (old_status == new_status)
            status_changed = false;
          break;

        default:
          LOG(WARNING) << "Unsupported combination: " << old_status << " and " << new_status;
      }
    }
    //
#endif
  }

  delete reply_to;

  // Остановить нить подчиненного интерфейса СС
  m_impl->stop();
  m_servant_thread->join();
  delete m_servant_thread;

#if (VERBOSE > 2)
  LOG(INFO) << fname << ": finish";
#endif
  return status;
}

// --------------------------------------------------------------------------------
int SystemAcquisitionModule::handle_request(mdp::zmsg* request, std::string*& reply_to)
{
  int rc = OK;
  rtdbMsgType msgType;

  assert (request->parts () >= 2);
  LOG(INFO) << "Process new request with " << request->parts() 
            << " parts and reply to " << *reply_to;

  msg::Letter *letter = m_message_factory->create(request);
  if (letter->valid())
  {
    msgType = letter->header()->usr_msg_type();

    switch(msgType)
    {
      // Здесь следует обрабатывать только запросы общего характера, не требующие подключения к БД
      // К ним относятся все запросы, влияющие на общее функционирование и управление.
      case ADG_D_MSG_ASKLIFE:
       rc = handle_asklife(letter, reply_to);
       break;

      case ADG_D_MSG_STOP:
       rc = handle_stop(letter, reply_to);
       break;

      default:
       LOG(ERROR) << "Unsupported request type: " << msgType;
       rc = NOK;
    }
  }
  else
  {
    LOG(ERROR) << "Readed letter "<<letter->header()->exchange_id()<<" not valid";
    rc = NOK;
  }

  delete letter;
  return rc;
}

// --------------------------------------------------------------------------------
int SystemAcquisitionModule::handle_asklife(msg::Letter* letter, std::string* reply_to)
{
  int rc = OK;
  msg::AskLife   *msg_ask_life = static_cast<msg::AskLife*>(letter);
  mdp::zmsg      *response = new mdp::zmsg();
  int exec_val = 1;

  msg_ask_life->set_exec_result(exec_val);

  response->push_front(const_cast<std::string&>(msg_ask_life->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_ask_life->header()->get_serialized()));
  response->wrap(reply_to->c_str(), "");

  LOG(INFO) << "Processing ASKLIFE from " << *reply_to
            << " has status:" << msg_ask_life->exec_result(exec_val)
            << " sid:" << msg_ask_life->header()->exchange_id()
            << " iid:" << msg_ask_life->header()->interest_id()
            << " dest:" << msg_ask_life->header()->proc_dest()
            << " origin:" << msg_ask_life->header()->proc_origin();

  send_to_broker((char*) MDPW_REPORT, NULL, response);

  delete response;

  return rc;
}

// --------------------------------------------------------------------------------
int SystemAcquisitionModule::handle_stop(msg::Letter* letter, std::string* reply_to)
{
  int rc = OK;
  msg::SimpleRequest *msg_stop = static_cast<msg::SimpleRequest*>(letter);
  int exec_val = 1;
  mdp::zmsg *response = new mdp::zmsg();

  // Отправить в сокет to_slave команду завершения работы
  m_to_slave.send("STOP", 4, 0);
  
  msg_stop->set_exec_result(exec_val);

  response->push_front(const_cast<std::string&>(msg_stop->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_stop->header()->get_serialized()));
  response->wrap(reply_to->c_str(), "");

  LOG(INFO) << "Processing STOP from " << *reply_to
            << " has status:" << msg_stop->exec_result(exec_val)
            << " sid:" << msg_stop->header()->exchange_id()
            << " iid:" << msg_stop->header()->interest_id()
            << " dest:" << msg_stop->header()->proc_dest()
            << " origin:" << msg_stop->header()->proc_origin();

  send_to_broker((char*) MDPW_REPORT, NULL, response);
  delete response;

  interrupt_worker = 1;

  return rc;
}

// --------------------------------------------------------------------------------
// Проверка существования указанного файла
bool exist(const std::string& filename)
{
  LOG(INFO) << "Skip '" << filename << "' existance, suppose it is exist";
  return true;
}

// --------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  std::string broker_endpoint = ENDPOINT_BROKER;
  std::string service_name = "BI4001";
  bool is_service_name_given = false;
  std::string config_name = "BI4001.json";
  bool is_config_name_given = false;
  int opt;
  int verbose = 0;
  SystemAcquisitionModule* client = NULL;
  static const char *arguments_out =
              "[-b <broker_address>] "
              "[-s <service_name>] "
              "[-v] "
              "-c <config file> ";

  while ((opt = getopt (argc, argv, "b:vs:c:")) != -1)
  {
    switch (opt)
    {
       case 'v': // режим подробного вывода
         verbose = 1;
         break;

       case 'b': // точка подключения к Брокеру
         broker_endpoint.assign(optarg);
         break;

       case 'c': // имя файла конфигурации
         is_config_name_given = true;
         config_name.assign(optarg);
         std::cout << "Config is \"" << config_name << "\"" << std::endl;
         break;

       case 's':
         service_name.assign(optarg);
         is_service_name_given = true;
         std::cout << "Service is \"" << service_name << "\"" << std::endl;
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
         return EXIT_FAILURE;

       default:
         std::cout << "Usage: " << arguments_out << std::endl;
         abort ();
    }
  }

  if (!is_config_name_given) {
    if (is_service_name_given) {
      config_name = service_name + ".json";
    }
    if (!exist(config_name)) {
      std::cerr << "Must support configuration file!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (!is_service_name_given) {
    // TODO: Если не было предоставлено, достать его из конфигурационного файла.
    // NB: а пока поругаемся и выйдем.
    std::cerr << "Must support service name!" << std::endl;
    return EXIT_FAILURE;
  }

  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();

  client = new SystemAcquisitionModule(broker_endpoint, service_name, config_name);

  client->run();

  delete client;

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();

  return EXIT_SUCCESS;
}

