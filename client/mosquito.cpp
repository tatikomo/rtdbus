// Утилита для множественного чтения/записи данных в БДРВ
// GEV 2015/01/01
///////////////////////////////////////////////////////////////////////////

#include "glog/logging.h"

#include "xdb_common.hpp" // AttributeInfo_t

#include "mdp_zmsg.hpp"
#include "mdp_client_async_api.hpp"

#include "msg_message.hpp"
#include "msg_adm.hpp"
#include "msg_sinf.hpp"

#include "mosquito.hpp"

// ----------------------------------------------------------
Mosquito::Mosquito(std::string broker, int verbose, WorkMode_t given_mode)
  : mdp::mdcli(broker, verbose),
    m_factory(new msg::MessageFactory("db_mosquito")),
    m_mode(given_mode)
{
}

// ----------------------------------------------------------
Mosquito::~Mosquito()
{
  delete m_factory;
}

// ----------------------------------------------------------
int Mosquito::recv(msg::Letter* &result)
{
  int status;
  mdp::zmsg* req = NULL;

  assert(&result);
  result = NULL;

  status = mdcli::recv(req);

  if (RECEIVE_OK == status)
  {
    result = m_factory->create(req);
    delete req;
  }

  return status;
}

// ----------------------------------------------------------
msg::Letter* Mosquito::create_message(rtdbMsgType type)
{
  return m_factory->create(type);
}

// ----------------------------------------------------------
bool Mosquito::process_read_response(msg::Letter* report)
{
  bool status = false;
  msg::ReadMulti* response = dynamic_cast<msg::ReadMulti*>(report);

  for (std::size_t idx = 0; idx < response->num_items(); idx++)
  {
    const msg::Value& attr_val = response->item(idx);
    std::cout << "type=" << attr_val.type() << "\t"
              << attr_val.tag() << " = " << attr_val.as_string() << std::endl;
  }

  return status;
}

// ----------------------------------------------------------
bool Mosquito::process_sbs_update_response(msg::Letter* report)
{
  bool status = false;
  msg::SubscriptionEvent* response = dynamic_cast<msg::SubscriptionEvent*>(report);

  std::cout << std::endl
            << "exchg#" << response->header()->exchange_id()
            << " intr#" << response->header()->interest_id()
            << " orig:" << response->header()->proc_origin()
            << " dest:" << response->header()->proc_dest()
            << " tm:"   << response->header()->time_mark()
            << std::endl;
  for (std::size_t idx = 0; idx < response->num_items(); idx++)
  {
    const msg::Value& attr_val = response->item(idx);
    std::cout << "type=" << attr_val.type() << "\t"
              << attr_val.tag() << " = " << attr_val.as_string() << std::endl;
  }

  return status;
}

// ----------------------------------------------------------
bool Mosquito::process_history(msg::Letter* report)
{
  bool status = false;
  // буфер для прочитанного элемента предыстории
  msg::hist_attr_t item;
  msg::HistoryRequest* response = dynamic_cast<msg::HistoryRequest*>(report);

  std::cout << std::endl
            << "exchg#" << response->header()->exchange_id()
            << " intr#" << response->header()->interest_id()
            << " orig:" << response->header()->proc_origin()
            << " dest:" << response->header()->proc_dest()
            << " tm:"   << response->header()->time_mark()
            << std::endl;
  std::cout << "History " << response->tag() << " existance(" << response->existance()
            << ") received " << response->num_required_samples()
            << " of " << response->num_read_samples() << " item(s)" << std::endl;
  for (int idx = 0; idx < response->num_read_samples(); idx++)
  {
    if (true == response->get(idx, item)) {
      std::cout << idx+1 << "/" << response->num_read_samples()
                << " time=" << item.datehourm
                << "\t" << item.val
                << "\t" << (unsigned int)item.valid
                << std::endl;
    }
    else {
      LOG(ERROR) << "Unable to get history item " << idx;
    }
  }

  return status;
}

// ----------------------------------------------------------
// Подписаться на рассылку Группы для заданного Сервиса
int Mosquito::subscript(const std::string& service_name, const std::string& group_name)
{
  int ret;

  // 1. Проверить доступность Сервиса service_name, получив для него строку подключения
  // 2. Подключиться к заданному Сервису
  // 3. Послать Сервису сообщение об активации подписки
  ret = mdcli::subscript(service_name, group_name);
  return ret;
}

// ----------------------------------------------------------
void* fillValueByType(xdb::AttributeInfo_t& attr_info, std::string& input_val)
{
//  std::cout << "check val: " << input_val << std::endl;
  void* ret_val = NULL;
  struct tm given_time;
  std::string::size_type point_pos;

  switch(attr_info.type)
  {
    case xdb::DB_TYPE_LOGICAL:
        attr_info.value.fixed.val_bool = (atoi(input_val.c_str()) > 0);
        ret_val = static_cast<void*>(&attr_info.value.fixed.val_bool);
    break;
    case xdb::DB_TYPE_INT8:
        attr_info.value.fixed.val_int8 = atoi(input_val.c_str());
        ret_val = static_cast<void*>(&attr_info.value.fixed.val_int8);
    break;
    case xdb::DB_TYPE_UINT8:
        attr_info.value.fixed.val_uint8 = atoi(input_val.c_str());
        ret_val = static_cast<void*>(&attr_info.value.fixed.val_uint8);
    break;
    case xdb::DB_TYPE_INT16:
        attr_info.value.fixed.val_int16 = atoi(input_val.c_str());
        ret_val = static_cast<void*>(&attr_info.value.fixed.val_int16);
    break;
    case xdb::DB_TYPE_UINT16:
        attr_info.value.fixed.val_uint16 = atoi(input_val.c_str());
        ret_val = static_cast<void*>(&attr_info.value.fixed.val_uint16);
    break;
    case xdb::DB_TYPE_INT32:
        attr_info.value.fixed.val_int32 = atol(input_val.c_str());
        ret_val = static_cast<void*>(&attr_info.value.fixed.val_int32);
    break;
    case xdb::DB_TYPE_UINT32:
        attr_info.value.fixed.val_uint32 = atol(input_val.c_str());
        ret_val = static_cast<void*>(&attr_info.value.fixed.val_uint32);
    break;
    case xdb::DB_TYPE_INT64:
        attr_info.value.fixed.val_int64 = atoll(input_val.c_str());
        ret_val = static_cast<void*>(&attr_info.value.fixed.val_int64);
    break;
    case xdb::DB_TYPE_UINT64:
        attr_info.value.fixed.val_uint64 = atoll(input_val.c_str());
        ret_val = static_cast<void*>(&attr_info.value.fixed.val_uint64);
    break;
    case xdb::DB_TYPE_FLOAT:
        attr_info.value.fixed.val_float = atof(input_val.c_str());
        ret_val = static_cast<void*>(&attr_info.value.fixed.val_float);
    break;
    case xdb::DB_TYPE_DOUBLE:
        attr_info.value.fixed.val_double = atof(input_val.c_str());
        ret_val = static_cast<void*>(&attr_info.value.fixed.val_double);
    break;
    case xdb::DB_TYPE_BYTES:
        assert(attr_info.value.dynamic.val_string);
        attr_info.value.dynamic.val_string->assign(input_val);
        ret_val = static_cast<void*>(attr_info.value.dynamic.val_string);
    break;

    case xdb::DB_TYPE_BYTES4:
    case xdb::DB_TYPE_BYTES8:
    case xdb::DB_TYPE_BYTES12:
    case xdb::DB_TYPE_BYTES16:
    case xdb::DB_TYPE_BYTES20:
    case xdb::DB_TYPE_BYTES32:
    case xdb::DB_TYPE_BYTES48:
    case xdb::DB_TYPE_BYTES64:
    case xdb::DB_TYPE_BYTES80:
    case xdb::DB_TYPE_BYTES128:
    case xdb::DB_TYPE_BYTES256:
        assert(attr_info.value.dynamic.varchar);
        if (attr_info.value.dynamic.size <= xdb::var_size[attr_info.type])
        {
          strncpy(attr_info.value.dynamic.varchar,
                  input_val.c_str(),
                  attr_info.value.dynamic.size);
        }
        else
        {
          LOG(ERROR) << "Allocated buffer (" << attr_info.value.dynamic.size
                     << " less then needed (" << xdb::var_size[attr_info.type];
        }
        ret_val = static_cast<void*>(attr_info.value.dynamic.val_string);
    break;

    case xdb::DB_TYPE_ABSTIME:
        strptime(input_val.c_str(), D_DATE_FORMAT_STR, &given_time);
        attr_info.value.fixed.val_time.tv_sec = given_time.tm_sec;
        point_pos = input_val.find_last_of('.');

       // Если точка найдена, и она не последняя в строке
        if ((point_pos != std::string::npos) && point_pos != input_val.size())
          attr_info.value.fixed.val_time.tv_usec = atoi(input_val.substr(point_pos + 1).c_str());
        else
          attr_info.value.fixed.val_time.tv_usec = 0;

        ret_val = static_cast<void*>(&attr_info.value.fixed.val_time);
    break;

    case xdb::DB_TYPE_UNDEF:
        // TODO: что делать при попытке сохранения данных неопределенного типа?
      std::cerr << "E: Undefined type " << attr_info.type << std::endl;
    break;

    default:
      std::cerr << "E: Unsupported type " << attr_info.type << std::endl;
    break;
  }

  return ret_val;
}

#if !defined _FUNCTIONAL_TEST

// ----------------------------------------------------------
int main (int argc, char *argv [])
{
  Mosquito  *mosquito  = NULL;
  mdp::zmsg         *transport_cell = NULL;
  msg::Letter       *request   = NULL;
  msg::Letter       *report    = NULL;
  // Множественное чтение
  msg::ReadMulti    *read_msg  = NULL;
  // Множественная запись
  msg::WriteMulti   *write_msg = NULL;
  // Управление Группами Подписки
  msg::SubscriptionControl *sbs_ctrl_msg = NULL;
  // Событие Группы Подписки
  //msg::SubscriptionEvent *sbs_event_msg = NULL;
  std::string group_name;
  bool is_group_name_given = false;
  // Блок параметров для запроса Истории
  bool is_point_name_given = false;
  std::string point_name;
  // значение по умолчанию - текущее время минус макс. количество минут в одной посылке
  time_t start_time = time(0) - MAX_PORTION_SIZE_LOADED_HISTORY * 60;
  int num_samples = 100;
  int history_type = xdb::PERIOD_1_MINUTE;
  // Запрос получения истории
  msg::HistoryRequest *history_req = NULL;

  //msg::ProbeMsg   *probe_msg = NULL; Не реализовано
  int opt;
  int verbose;
  int status; // код завершения функции чтения сообщения
  // прямые сообщения в адрес Сервиса
  mdp::ChannelType channel = mdp::ChannelType::DIRECT;
  char one_argument[SERVICE_NAME_MAXLEN + 1] = "";
  char service_endpoint[ENDPOINT_MAXLEN + 1] = "";
  int service_status;   // 200|400|404|501
  // название Сервиса
  std::string service_name;
  bool is_service_name_given = false;
  Mosquito::WorkMode_t mode;
  bool stop_receiving = false;
  std::string input_tag, input_type, input_val;
  int num = 0;
  xdb::AttributeInfo_t attr_info;
  static const char *arguments_template =
              "-s <service_name> [-v] "
              "[-g <sbs name>] "
              "[-p <point name>] "
              "[-n <num history samples>] "
              "[-t <history time start>] "
              "[-h <history depth>] "
              "[-m <%s|%s|%s|%s|%s>]";
  char arguments_out[255];

  while ((opt = getopt (argc, argv, "vs:m:g:t:p:n:h:")) != -1)
  {
    switch (opt)
    {
        case 'v': // режим подробного вывода
          verbose = 1;
          break;

        case 's':
          strncpy(one_argument, optarg, SERVICE_NAME_MAXLEN);
          one_argument[SERVICE_NAME_MAXLEN] = '\0';
          service_name.assign(one_argument);
          is_service_name_given = true;
          std::cout << "Destination service is \"" << service_name << "\"" << std::endl;
          break;

        case 'm': // mode - режим работы, запись или чтение в БДРВ
          strncpy(one_argument, optarg, SERVICE_NAME_MAXLEN);
          one_argument[SERVICE_NAME_MAXLEN] = '\0';

          if (0 == strncmp(one_argument, OPTION_MODE_READ, SERVICE_NAME_MAXLEN))
            mode = Mosquito::MODE_READ;
          else if (0 == strncmp(one_argument, OPTION_MODE_WRITE, SERVICE_NAME_MAXLEN))
            mode = Mosquito::MODE_WRITE;
          else if (0 == strncmp(one_argument, OPTION_MODE_PROBE, SERVICE_NAME_MAXLEN))
            mode = Mosquito::MODE_PROBE;
          else if (0 == strncmp(one_argument, OPTION_MODE_SUBSCRIBE, SERVICE_NAME_MAXLEN))
            mode = Mosquito::MODE_SUBSCRIBE;
          else if (0 == strncmp(one_argument, OPTION_MODE_HISTORY, SERVICE_NAME_MAXLEN))
            mode = Mosquito::MODE_HISTORY_REQ;
          else {
            std::cout << "Unknown work mode, will use PROBE" << std::endl;
            mode = Mosquito::MODE_PROBE;
          }
          break;

        case 'g': // название группы
          strncpy(one_argument, optarg, SERVICE_NAME_MAXLEN);
          one_argument[SERVICE_NAME_MAXLEN] = '\0';
          group_name.assign(one_argument);
          std::cout << "Group name is \"" << group_name << "\"" << std::endl;
          is_group_name_given = true;
          break;

        case 'p': // название тега для запроса его истории
          strncpy(one_argument, optarg, SERVICE_NAME_MAXLEN);
          one_argument[SERVICE_NAME_MAXLEN] = '\0';
          point_name.assign(one_argument);
          std::cout << "Point name is \"" << point_name << "\"" << std::endl;
          is_point_name_given = true;
          break;

        case 't': // начало интервала времени запроса истории
          strncpy(one_argument, optarg, SERVICE_NAME_MAXLEN);
          one_argument[SERVICE_NAME_MAXLEN] = '\0';
          start_time = atol(one_argument);
          std::cout << "Start history period is: " << ctime(&start_time) << std::endl;
          break;

        case 'n': // количество читаемых интервалов истории
          strncpy(one_argument, optarg, SERVICE_NAME_MAXLEN);
          one_argument[SERVICE_NAME_MAXLEN] = '\0';
          num_samples = atol(one_argument);
          std::cout << "Samples number is: " << num_samples << std::endl;
          break;

        case 'h': // тип читаемой истории
          strncpy(one_argument, optarg, SERVICE_NAME_MAXLEN);
          one_argument[SERVICE_NAME_MAXLEN] = '\0';
          history_type = atol(one_argument);
          std::cout << "History type is: " << history_type << std::endl;
          break;

        case '?':
          if ((optopt == 's') || (optopt == 'm'))
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

  google::InitGoogleLogging(argv[0]);

  if (!is_service_name_given)
  {
    snprintf(arguments_out,
             255,
             arguments_template,
             OPTION_MODE_READ,
             OPTION_MODE_WRITE,
             OPTION_MODE_PROBE,
             OPTION_MODE_SUBSCRIBE,
             OPTION_MODE_HISTORY);
    std::cout << "Service name not given." << std::endl
              << "Usage: " << arguments_out << std::endl;
    return(1);
  }
  std::cout << "Will use work mode " << mode << std::endl;

  mosquito = new Mosquito ("tcp://localhost:5555", verbose, mode);

  try
  {
    // Подготовить сообщение, заполнив названия интересующих тегов
    switch(mode)
    {
      case Mosquito::MODE_READ:
      //----------------------------
        request = mosquito->create_message(SIG_D_MSG_READ_MULTI);
        read_msg = dynamic_cast<msg::ReadMulti*>(request);
        assert(read_msg);
        num = 0;

        while (std::cin >> input_tag)
        {
            read_msg->add(input_tag, xdb::DB_TYPE_UNDEF, 0);
            num++;
        }
        LOG(INFO) << "Ready to read " << num << " parameters";
        break;

      case Mosquito::MODE_WRITE:
      //----------------------------
        request = mosquito->create_message(SIG_D_MSG_WRITE_MULTI);
        write_msg = dynamic_cast<msg::WriteMulti*>(request);
        assert(write_msg);
        num = 0;

        while (std::cin >> input_tag >> input_type >> input_val)
        {
          // Получить числовое значение типа по его символьному представлению
          if ((true == mosquito->message_factory()->GetDbTypeFromString(input_type, attr_info.type))
           && (xdb::DB_TYPE_UNDEF != attr_info.type))
          {
            // Попытаться прочитать значение в соответствии с прочитанным ранее типом
            if (fillValueByType(attr_info, input_val))
            {
              attr_info.name.assign(input_tag);
              write_msg->add(attr_info.name, attr_info.type, static_cast<void*>(&attr_info.value));
              num++;
            }
          }
        }

        if (!num)
        {
          delete request;
          request = NULL;
        }
        LOG(INFO) << "Ready to write " << num << " parameters";
        break;

      case Mosquito::MODE_PROBE:
      //----------------------------
        request = NULL; // TODO: 2015/06/30 не реализовано
        break;

      case Mosquito::MODE_SUBSCRIBE:
      //----------------------------
        // 1. Инициировать активацию подписки (NB: по умолчанию они все
        //    активны при создании). При этом сервер отвечает сообщением
        //    SIG_D_MSG_GRPSBS, содержащим ВСЕ значения группы (без учета
        //    признака модификации).
        // 2. Прочитать мгновенный срез всех значений атрибутов Группы
        // 3. Ожидать обновления сообщения или сигнала о завершении работы
        if (is_group_name_given)
        {
          // Активируем сокет подписки
          if (0 == mosquito->subscript(service_name, group_name))
          {
            // Сообщим Службе о готовности к приему данных
            request = mosquito->create_message(SIG_D_MSG_GRPSBS_CTRL);
            assert(request);
            sbs_ctrl_msg = dynamic_cast<msg::SubscriptionControl*>(request);
            sbs_ctrl_msg->set_name(group_name);
            // TODO: ввести перечисление возможных кодов
            // Сейчас (2015-10-01) 0 = SUSPEND, 1 = ENABLE
            sbs_ctrl_msg->set_ctrl(1); // TODO: ввести перечисление возможных кодов
          }
        }
        else
        {
          std::cerr << "SBS group name not supported, use '-g <name>'" << std::endl;
        }
        break;

      case Mosquito::MODE_HISTORY_REQ:
      //----------------------------
        if (is_point_name_given) {
          // Запросить у Службы истории тренд указанного параметра
          request = mosquito->create_message(SIG_D_MSG_REQ_HISTORY);
          history_req = dynamic_cast<msg::HistoryRequest*>(request);
          assert(history_req);
          history_req->set(point_name, start_time, num_samples, history_type);
        }
        else std::cerr << "Tag not scpecified, exiting" << std::endl;
        break;
    }

    // Есть чего сказать миру?
    if (request)
    {
      // запросим состояние интересующей Службы
      std::cout << "Checking '" << service_name << "' status " << std::endl;
      strncpy(one_argument, service_name.c_str(), SERVICE_NAME_MAXLEN);
      one_argument[SERVICE_NAME_MAXLEN] = '\0';
      service_status = mosquito->ask_service_info(one_argument/*service_name.c_str()*/, service_endpoint, ENDPOINT_MAXLEN);

      switch(service_status)
      {
          case 200:
          std::cout << service_name << " status OK : " << service_status << std::endl;
          std::cout << "Get point to " << service_endpoint << std::endl;
          break;

          case 400:
          std::cerr << service_name << " status BAD_REQUEST : " << service_status << std::endl;
          break;

          case 404:
          std::cerr << service_name << " status NOT_FOUND : " << service_status << std::endl;
          break;

          case 501:
          std::cerr << service_name << " status NOT_SUPPORTED : " << service_status << std::endl;
          break;

          default:
          std::cerr << service_name << " status UNKNOWN : " << service_status << std::endl;
      }

      if (200 != service_status)
      {
          std::cerr << "Service '" << service_name << "' is not running, exiting" << std::endl;
          exit(service_status);
      }

      request->set_destination(service_name);
      transport_cell = new mdp::zmsg ();
      transport_cell->push_front(const_cast<std::string&>(request->data()->get_serialized()));
      transport_cell->push_front(const_cast<std::string&>(request->header()->get_serialized()));
//      transport_cell->dump();

      // Отправить сообщение, напрямую или через Брокер
      mosquito->send (service_name, transport_cell, channel);
    }
    delete transport_cell;
    delete request;

    int cause_code = 0;
    std::string cause_text = "";
    msg::ExecResult *resp = NULL;

    stop_receiving = false;

    //  Wait for report
    while (!stop_receiving)
    {
      status = mosquito->recv (report);
//      LOG(INFO) << "GEV: mosquito->recv("<<report<<") return code: " << status;

      switch (status)
      {
        case Mosquito::RECEIVE_FATAL:
          stop_receiving = true;
          break;

        case Mosquito::RECEIVE_OK:
          assert(report);
          report->dump();

            switch(report->header()->usr_msg_type())
            {
              case ADG_D_MSG_EXECRESULT:
                resp = dynamic_cast<msg::ExecResult*>(report);

                resp->failure_cause(cause_code, cause_text);
                std::cout << "Got ADG_D_MSG_EXECRESULT response: "
                          << resp->header()->usr_msg_type()
                          << " status=" << resp->exec_result()
                          << " code=" << cause_code
                          << " text=\"" << cause_text << "\"" << std::endl;

                stop_receiving = true;
                break;

              case SIG_D_MSG_READ_MULTI:
                std::cout << "Got SIG_D_MSG_READ_MULTI response: "
                          << report->header()->usr_msg_type() << std::endl;
                mosquito->process_read_response(report);
                if (Mosquito::MODE_SUBSCRIBE == mode)
                {
                  // Для режима подписки это сообщение приходят однократно,
                  // для инициализации начальных значений атрибутов из группы
                  stop_receiving = false;
                }
                else
                {
                  stop_receiving = true;
                }
                break;

              // NB: не должно быть такого ответа, должен быть ExecResult
              case SIG_D_MSG_WRITE_MULTI:
                std::cerr << "Error: expect ExecResult but received SIG_D_MSG_WRITE_MULTI response: "
                          << report->header()->usr_msg_type() << std::endl;
                stop_receiving = true;
                break;

              // Очередная порция обновленных данных. Поступают циклически, без запроса.
              case SIG_D_MSG_GRPSBS:
                mosquito->process_sbs_update_response(report);
                stop_receiving = false;
                break;

              // Ответ на запрос истории
              case SIG_D_MSG_REQ_HISTORY:
                mosquito->process_history(report);
                stop_receiving = false;
                break;

              default:
                std::cerr << "Got unsupported response type: "
                          << report->header()->usr_msg_type() << std::endl;
            }

            delete report;
          break;

        case Mosquito::RECEIVE_ERROR:
          if (Mosquito::MODE_SUBSCRIBE == mode)
          {
            // Превышен таймаут получения данных, можно подождать еще
            std::cout << ".";
            continue;
          }
          else
          {
            stop_receiving = true;
            break;
          }

        default:
          LOG(ERROR) << "Unexpected recv() return code: " << status;
      } // конец switch-проверки статуса recv()
    }  // конец цикла ожидания нового сообщения
  } // конец блока try{}
  catch (zmq::error_t err)
  {
      std::cerr << "E: " << err.what() << std::endl;
  }

  delete mosquito;
  return 0;
}

#endif

