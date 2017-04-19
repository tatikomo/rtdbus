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
// общий интерфейс сообщений
#include "msg_common.h"
#include "msg_message.hpp"
// сообщения общего порядка
#include "msg_adm.hpp"
// сообщения по работе с БДРВ
#include "msg_sinf.hpp"
#include "mdp_zmsg.hpp"
#include "mdp_common.h"
#include "mdp_worker_api.hpp"
#include "exchange_sysacq_intf.hpp"
#include "exchange_sysacq_modbus.hpp"
#include "exchange_config_sac.hpp"
#include "wsysacq_cli.hpp"

using namespace mdp;

extern volatile int interrupt_worker;

#if 0
    int run();
    int stop();
  private:
    // Подчиненный интерфейс системы сбора
    SysAcqInterface *m_impl;
    // Сокет связи с подчиненным интерфейсом системы сбора
    zmq::socket_t    m_slave;
    // Внутреннее состояние
    int m_status;
    // Фабрика сообщений
    msg::MessageFactory *m_message_factory;
    // Нить подчиненного интерфейса с Системой Сбора
    std::thread* m_servant_thread;
#endif


// --------------------------------------------------------------------------------
SA_Module_Instance::SA_Module_Instance(const std::string& broker_endpoint,  // Точка подключения к Брокеру
                                       const std::string& service)          // Название Службы
 : mdwrk(broker_endpoint, service, 1 /* num threads */, true /* use direct connection */),
   m_impl(NULL),
   m_slave(m_context, ZMQ_PAIR),
   m_status(STATUS_OK),
   m_servant_thread(NULL),
   m_config(NULL),
   m_message_factory(NULL)
{
  // Значения общих характеристик Системы Сбора
  sa_common_t common_data;
  // Класс для разбора конфигурационных файлов
  m_config = new AcquisitionSystemConfig(service + ".json");

  // Прочесть общие характеристики
  m_config->load_common(common_data);

  LOG(INFO) << "create new system acquisition worker";

  // В зависимости от полученного типа системы сбора
  // будет создаваться соответствующий экземпляр.
  switch(common_data.nature) {
    case GOF_D_SAC_NATURE_EELE:
      m_impl = new Modbus_Client_Interface(service, m_context, *m_config);
      break;
    default:
      LOG(FATAL) << "Unsupported System Acquisition type: " << common_data.nature;
      assert(0 == 1);
  }

  m_message_factory = new msg::MessageFactory(service.c_str());
  // Изменить время таймаута приема сообщения
  set_recv_timeout(1000000); // 1 sec
  // Изменения активизируются после нового подключения к Брокеру
  connect_to_broker();
  // Подключение к SysAcqInterface::run
  m_slave.bind(s_SA_INTERNAL_EXCHANGE_ENDPOINT);
}

// --------------------------------------------------------------------------------
SA_Module_Instance::~SA_Module_Instance()
{
  LOG(INFO) << "start system acquisition destructor";
  delete m_message_factory;
  delete m_config;
  delete m_impl;
  m_slave.close();
  LOG(INFO) << "finish system acquisition destructor";
}

// --------------------------------------------------------------------------------
// Нельзя устраивать задержки с помощью sleep, поскольку в это время процесс не получает
// сообщения Брокера и Клиентов, что ведет к нарушению работы.
int SA_Module_Instance::run()
{
#if (VERBOSE > 2)
  const char* fname = "run";
#endif
  // Фабрика сообщений

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

    // Получить входящие сообщения (Брокер, другие Службы, Подписка,...)
    request = recv (reply_to);
    if (request)
    {
#if (VERBOSE > 5)
      LOG(INFO) << fname << ": got a message";
#endif

      // Продолжить локальную обработку в соответствии с типом полученного сообщения
      // Если сообщение не перехватывается интерфейсом верхнего уровня,
      // передать его через сокет slave в интерфейс нижнего уровня.
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
int SA_Module_Instance::stop()
{
  // Отправить в сокет slave команду завершения работы
  m_slave.send("STOP", 4, 0);
  return OK;
}

// --------------------------------------------------------------------------------
// Запрос останова работы
// Ответ: ADG_D_MSG_EXECRESULT
int SA_Module_Instance::handle_stop(msg::Letter* letter, std::string* reply_to)
{
  int rc;
  int code;
  std::string message;

  LOG(ERROR) << "Got ADG_D_MSG_STOP";

  // Остановить нить SysAcqInterface::run()
  stop();
  code = 1;
  message.assign("<OK>");

  rc = response_as_exec_result(letter, reply_to, code, message);

  interrupt_worker = 1;

  return rc;
}


// --------------------------------------------------------------------------------
// Запрос выполнения иницициализации
// Ответ: ADG_D_MSG_EXECRESULT
int SA_Module_Instance::handle_init(msg::Letter* letter, std::string* reply_to)
{
  int rc;
  int code;
  std::string message;

  LOG(ERROR) << "Got ADG_D_MSG_INIT";

  switch(m_status) {
    case STATUS_OK:
      // LoadSMAD()
    case STATUS_OK_SMAD_LOAD: // Ещё не подключён, InternalSMAD загружена
    case STATUS_OK_NOT_CONNECTED: // Не подключён, требуется переподключение
      LOG(INFO) << "Ready to init";
      code = 1;
      message.assign("<OK>");
      break;

    case STATUS_OK_CONNECTED: // Подключён, все в порядке
      LOG(INFO) << "Disconnect, ready to init";
      // Disconnect()
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
int SA_Module_Instance::handle_end_all_init(msg::Letter* letter, std::string* reply_to)
{
  int rc;
  int code;
  std::string message;

  LOG(ERROR) << "Got ADG_D_MSG_ENDINITACK";

  switch(m_status) {
    case STATUS_OK_NOT_CONNECTED:
      if (OK == (m_status = m_impl->connect())) {
        LOG(INFO) << "Connection successful";
        message.assign("<OK>");
      }
      else {
        LOG(INFO) << "Connection failure";
        message.assign("<FAILURE>");
      }
      code = m_status;
      break;

    default:
      LOG(WARNING) << "Try to finish initialization on currentrly wrong status:" << m_status;
      code = m_status;
      message.assign("<FAILURE>");
  }

  rc = response_as_exec_result(letter, reply_to, code, message);

  return rc;
}

// --------------------------------------------------------------------------------
// Ответ сообщением на простейшие запросы, в ответе только числовой код
int SA_Module_Instance::response_as_simple_reply(msg::Letter* letter, std::string* reply_to, int code)
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
  // TODO: Возможно ли переполнение?
  simple_reply->header()->set_interest_id(letter->header()->interest_id() + 1);
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
int SA_Module_Instance::response_as_exec_result(msg::Letter* letter,
                                             std::string* reply_to,
                                             int code,
                                             const std::string& note)
{
  //const rtdbMsgType usr_type = letter->header()->usr_msg_type();
  msg::ExecResult *exec_reply =
    static_cast<msg::ExecResult*>(m_message_factory->create(ADG_D_MSG_EXECRESULT));
  int rc = OK;

  exec_reply->header()->set_exchange_id(letter->header()->exchange_id());
  // TODO: Возможно ли переполнение?
  exec_reply->header()->set_interest_id(letter->header()->interest_id() + 1);
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
// Локальная предварительная обработка сообщений из внешнего мира
// После определения типа запроса:
// - если запрос обработывается на локальном уровне - выполнить его
// - если запрос не перехватывается локально - его обработкой занимается SysAcqInterface
int SA_Module_Instance::handle_request(mdp::zmsg* request, std::string* reply_to)
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
      case ECH_D_MSG_INT_REQUEST:   // Специфичные запросы передаются интерфейсу SysAcqInterface
        //1 rc = handle_request(letter, reply_to);
        // а) Разобрать, что это за запрос
        // б) Сформировать запрос соответствующего типа в адрес Интерфейса второго уровня
        // в) Периодически опрашивать сокет Интерфейса второго уровня до истечения таймаута
        //    или получения ответа
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
int SA_Module_Instance::handle_asklife(msg::Letter* letter, std::string* reply_to)
{
  int rc = OK;
  msg::SimpleReply *simple_reply =
    static_cast<msg::SimpleReply*>(m_message_factory->create(ADG_D_MSG_LIVING));

  assert(letter->header()->usr_msg_type() == ADG_D_MSG_ASKLIFE);
  simple_reply->header()->set_exchange_id(letter->header()->exchange_id());
  // TODO: Возможно ли переполнение?
  simple_reply->header()->set_interest_id(letter->header()->interest_id() + 1);
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
// Запрос на доступ к данным
int SA_Module_Instance::handle_internal_request(msg::Letter* letter, std::string* reply_to)
{
  int rc = OK;
  msg::SimpleReply *simple_reply =
      static_cast<msg::SimpleReply*>(m_message_factory->create(ECH_D_MSG_INT_REPLY));

  assert(letter->header()->usr_msg_type() == ECH_D_MSG_INT_REQUEST);

  simple_reply->header()->set_exchange_id(letter->header()->exchange_id());
  // TODO: Возможно ли переполнение?
  simple_reply->header()->set_interest_id(letter->header()->interest_id() + 1);
  simple_reply->header()->set_proc_dest(*reply_to);

  mdp::zmsg *response = simple_reply->get_zmsg();
  response->wrap(reply_to->c_str(), "");

  send_to_broker((char*) MDPW_REPORT, NULL, response);

  delete response;
  delete simple_reply;

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
  int opt;
  int verbose = 0;
  SA_Module_Instance* client = NULL;
  static const char *arguments_out =
              "[-b <broker_address>] "
              "[-s <service_name>] "
              "[-v] ";

  while ((opt = getopt (argc, argv, "b:vs:c:")) != -1)
  {
    switch (opt)
    {
       case 'v': // режим подробного вывода
         verbose = 1;
         break;

       case 'b': // точка подключения к Брокеру
         broker_endpoint.assign(optarg, SERVICE_NAME_MAXLEN);
         break;

       case 's':
         service_name.assign(optarg, SERVICE_NAME_MAXLEN);
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

  if (is_service_name_given) {
    config_name = service_name + ".json";

    if (!exist(config_name)) {
      std::cerr << "Configuration file '" << config_name << "' is missing" << std::endl;
      return EXIT_FAILURE;
    }
  }
  else {
    std::cerr << "Must support service name" << std::endl;
    return EXIT_FAILURE;
  }

  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();

  client = new SA_Module_Instance(broker_endpoint, service_name);

  client->run();

  delete client;

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();

  return EXIT_SUCCESS;
}

