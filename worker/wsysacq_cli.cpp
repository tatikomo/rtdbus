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
   m_status(STATE_INIT_NONE),
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
#if (VERBOSE > 2)
  const char* fname = "run";
#endif
  int status = m_impl->prepare();
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
  
//1    status = m_impl->quantum();
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

    // Здесь следует обрабатывать только запросы общего характера, не требующие подключения к БД
    // К ним относятся все запросы, влияющие на общее функционирование и управление.
    switch(msgType)
    {
      // -----------------------------------------------------------------------------------
      // Группа сообщений, которые могут быть получены в любой момент
      case ADG_D_MSG_ASKLIFE:   // Запрос "Жив", ответ ADG_D_MSG_LIVING
        rc = handle_asklife(letter, reply_to);
        break;
      case ADG_D_MSG_STOP:      // Запрос останова работы, ответ ADG_D_MSG_EXECRESULT
        rc = handle_stop(letter, reply_to);
        break;

      // -----------------------------------------------------------------------------------
      // Группа сообщений, которые допустимо принимать только ДО завершения инициализации
      case ADG_D_MSG_INIT:      // Запрос выполнения первоначальной иницициализации данным модулем
      case ADG_D_MSG_DIFINIT:   // Запрос выполнения иницициализации данным модулем после рестарта 
        rc = handle_init(letter, reply_to); // Ответ ADG_D_MSG_EXECRESULT
        break;
      case ADG_D_MSG_ENDALLINIT:// Сообщение об общем завершении инициализации
        rc = handle_end_all_init(letter, reply_to); // Ответ ADG_D_MSG_ENDINITACK
        break;

      // -----------------------------------------------------------------------------------
      // Группа сообщений, которые могут быть получены только ПОСЛЕ завершения инициализации
      case ECH_D_MSG_INT_REQUEST:   // Запрос на доступ к данным, ответ ECH_D_MSG_INT_REPLY
        rc = handle_internal_request(letter, reply_to);
        break;

      // -----------------------------------------------------------------------------------
      // Неподдерживаемые сообщения
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
// Запрос выполнения иницициализации
// Ответ: ADG_D_MSG_EXECRESULT
int SystemAcquisitionModule::handle_init(msg::Letter* letter, std::string* reply_to)
{
  int rc;
  int code;
  std::string message;

  LOG(ERROR) << "Got ADG_D_MSG_INIT";

  switch(m_status) {
    case STATE_INIT_OK:
      code = 1;
      message.assign("<OK>");
      break;
    default:
      code = 0;
      message.assign("<FAILURE>");
  }

  rc = response_as_exec_result(letter, reply_to, code, message);

  return rc;
}

// --------------------------------------------------------------------------------
// Сообщение об общем завершении инициализации
// Ответ: ADG_D_MSG_ENDINITACK
int SystemAcquisitionModule::handle_end_all_init(msg::Letter* letter, std::string* reply_to)
{
  int rc;
  int code;
  std::string message;

  LOG(ERROR) << "Got ADG_D_MSG_ENDINITACK";

  switch(m_status) {
    case STATUS_OK:
      code = STATE_INIT_OK;
      message.assign("<OK>");
      break;
    default:
      code = 0;
      message.assign("<FAILURE>");
  }

  rc = response_as_exec_result(letter, reply_to, code, message);

  return rc;
}

// --------------------------------------------------------------------------------
// Запрос на доступ к данным
int SystemAcquisitionModule::handle_internal_request(msg::Letter* letter, std::string* reply_to)
{
  int rc = OK;
  msg::SimpleReply *simple_reply = static_cast<msg::SimpleReply*>(m_message_factory->create(ECH_D_MSG_INT_REPLY));

  assert(letter->header()->usr_msg_type() == ECH_D_MSG_INT_REQUEST);

  simple_reply->header()->set_exchange_id(letter->header()->exchange_id());
  simple_reply->header()->set_interest_id(letter->header()->interest_id() + 1); // TODO: Возможно ли переполнение?
  simple_reply->header()->set_proc_dest(*reply_to);

  mdp::zmsg *response = simple_reply->get_zmsg();
  response->wrap(reply_to->c_str(), "");

  send_to_broker((char*) MDPW_REPORT, NULL, response);

  delete response;
  delete simple_reply;

  return rc;
}

#if 0
// --------------------------------------------------------------------------------
// Запрос
int SystemAcquisitionModule::handle_simple_response(msg::Letter* letter, std::string* reply_to, int status)
{
  int rc = OK;
  const rtdbMsgType usr_request_type = letter->header()->usr_msg_type();
  msg::SimpleReply *simple_reply = NULL;
  rtdbMsgType usr_reply_type;

  switch (usr_request_type) {
     //case ADG_D_MSG_LIVING   : - уже обрабатывается в handle_asklife()
     case ADG_D_MSG_ENDALLINIT : usr_reply_type = ADG_D_MSG_EXECRESULT; break;
     case ADG_D_MSG_ENDALLINIT : usr_reply_type = ADG_D_MSG_ENDINITACK; break;
     case ECH_D_MSG_INT_REQUEST: usr_reply_type = ECH_D_MSG_INT_REPLY;  break;
       LOG(INFO) << "Got usr msg type #" << usr_request_type << ", send msg type #" << usr_reply_type;
       simple_reply = static_cast<msg::SimpleReply*>(m_message_factory->create(usr_reply_type));
       break;

      default :
        rc = NOK;
  }

  send_to_broker((char*) MDPW_REPORT, NULL, response);

  delete response;
  delete simple_reply;

  return rc;
}
#endif

// --------------------------------------------------------------------------------
int SystemAcquisitionModule::handle_asklife(msg::Letter* letter, std::string* reply_to)
{
  int rc = OK;
  msg::SimpleReply *simple_reply = static_cast<msg::SimpleReply*>(m_message_factory->create(ADG_D_MSG_LIVING));

  assert(letter->header()->usr_msg_type() == ADG_D_MSG_ASKLIFE);
  simple_reply->header()->set_exchange_id(letter->header()->exchange_id());
  simple_reply->header()->set_interest_id(letter->header()->interest_id() + 1); // TODO: Возможно ли переполнение?
  simple_reply->header()->set_proc_dest(*reply_to);

  mdp::zmsg *response = simple_reply->get_zmsg();
  response->wrap(reply_to->c_str(), "");

  LOG(INFO) << "Got ASKLIFE "
            << " sid:"    << letter->header()->exchange_id()
            << " iid:"    << letter->header()->interest_id()
            << " origin:" << letter->header()->proc_origin()
            << " dest:"   << letter->header()->proc_dest();
  LOG(INFO) << "Send LIVING "
            << " sid:"    << simple_reply->header()->exchange_id()
            << " iid:"    << simple_reply->header()->interest_id()
            << " origin:" << simple_reply->header()->proc_origin()
            << " dest:"   << simple_reply->header()->proc_dest();

  send_to_broker((char*) MDPW_REPORT, NULL, response);

  delete response;
  delete simple_reply;

  return rc;
}

// --------------------------------------------------------------------------------
// Запрос останова работы
// Ответ: ADG_D_MSG_EXECRESULT
int SystemAcquisitionModule::handle_stop(msg::Letter* letter, std::string* reply_to)
{
  int rc;
  int code;
  std::string message;

  // Отправить в сокет to_slave команду завершения работы
  m_to_slave.send("STOP", 4, 0);

  // TODO: остановить деятельность
  
  LOG(ERROR) << "Got ADG_D_MSG_STOP";

  code = 1;
  message.assign("<OK>");

  rc = response_as_exec_result(letter, reply_to, code, message);

  interrupt_worker = 1;

  return rc;
}

// --------------------------------------------------------------------------------
// Ответ сообщением на простейшие запросы, в ответе только числовой код
int SystemAcquisitionModule::response_as_simple_reply(msg::Letter* letter, std::string* reply_to, int code)
{
  int rc = OK;
  msg::SimpleReply *simple_reply;

  switch(letter->header()->usr_msg_type())
  {
    case ADG_D_MSG_ENDALLINIT:
      simple_reply = static_cast<msg::SimpleReply*>(m_message_factory->create(ADG_D_MSG_ENDINITACK));
      break;
    case ADG_D_MSG_ASKLIFE:
      simple_reply = static_cast<msg::SimpleReply*>(m_message_factory->create(ADG_D_MSG_LIVING));
      break;
    default:
      LOG(ERROR) << "Message type #" << letter->header()->usr_msg_type()
                 << " can't be interpreted as SIMPLE_REQUEST";
      assert(0 == 1);
  }

  simple_reply->header()->set_exchange_id(letter->header()->exchange_id());
  simple_reply->header()->set_interest_id(letter->header()->interest_id() + 1); // TODO: Возможно ли переполнение?
  simple_reply->header()->set_proc_dest(*reply_to);

  mdp::zmsg *response = simple_reply->get_zmsg();
  response->wrap(reply_to->c_str(), "");

  LOG(INFO) << "Got SIMPLE_REQUEST #" << letter->header()->usr_msg_type()
            << " sid:"    << letter->header()->exchange_id()
            << " iid:"    << letter->header()->interest_id()
            << " origin:" << letter->header()->proc_origin()
            << " dest:"   << letter->header()->proc_dest();
  LOG(INFO) << "Send SIMPLE_REPLY #" << simple_reply->header()->usr_msg_type()
            << " sid:"    << simple_reply->header()->exchange_id()
            << " iid:"    << simple_reply->header()->interest_id()
            << " origin:" << simple_reply->header()->proc_origin()
            << " dest:"   << simple_reply->header()->proc_dest();

  send_to_broker((char*) MDPW_REPORT, NULL, response);

  delete response;
  delete simple_reply;

  return rc;

}

// --------------------------------------------------------------------------------
// Ответ сообщением EXEC_RESULT на запросы, в ответе числовой код (обязательно) и строка (возможно)
int SystemAcquisitionModule::response_as_exec_result(msg::Letter* letter,
                                                     std::string* reply_to,
                                                     int code,
                                                     const std::string& note)
{
  //const rtdbMsgType usr_type = letter->header()->usr_msg_type();
  msg::ExecResult *exec_reply = static_cast<msg::ExecResult*>(m_message_factory->create(ADG_D_MSG_EXECRESULT));
  int rc = OK;

  exec_reply->header()->set_exchange_id(letter->header()->exchange_id());
  exec_reply->header()->set_interest_id(letter->header()->interest_id() + 1); // TODO: Возможно ли переполнение?
  exec_reply->header()->set_proc_dest(*reply_to);
  if (!note.empty())
    exec_reply->set_failure_cause(code, note);
  else
    exec_reply->set_exec_result(code);

  mdp::zmsg *response = exec_reply->get_zmsg();
  response->wrap(reply_to->c_str(), "");

  send_to_broker((char*) MDPW_REPORT, NULL, response);

  delete response;
  delete exec_reply;

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

