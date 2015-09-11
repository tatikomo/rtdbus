// Утилита для множественного чтения/записи данных в БДРВ
// GEV 2013/01/01
///////////////////////////////////////////////////////////////////////////

#include <glog/logging.h>

#include "xdb_common.hpp" // AttributeInfo_t

#include "mdp_zmsg.hpp"
#include "mdp_client_async_api.hpp"

#include "msg_message.hpp"
#include "msg_adm.hpp"
#include "msg_sinf.hpp"

#include "mosquito.hpp"

Mosquito::Mosquito(std::string broker, int verbose, WorkMode_t given_mode)
  : mdp::mdcli(broker, verbose),
    m_factory(new msg::MessageFactory("db_mosquito")),
    m_mode(given_mode)
{
}

Mosquito::~Mosquito()
{
  delete m_factory;
}

msg::Letter* Mosquito::recv()
{
  msg::Letter* result = NULL;
  mdp::zmsg* req = mdcli::recv();

  if (req)
    result = m_factory->create(req);

  return result;
}

msg::Letter* Mosquito::create_message(rtdbMsgType type)
{
  return m_factory->create(type);
}

// NB: этот массив содержит полные названия тегов, с атрибутом.
#if 0
xdb::AttributeInfo_t array_of_parameters[10] = {
  { "/KD4005/1/GOV020.OBJCLASS", xdb::DB_TYPE_INT32,   xdb::ATTR_NOT_FOUND, 0, { 0, NULL, NULL } },
  { "/KD4005/2/GOV020.STATUS",   xdb::DB_TYPE_UINT32,  xdb::ATTR_NOT_FOUND, 0, { 0, NULL, NULL } },
  { "/KD4005/GOV70.SA_ref",      xdb::DB_TYPE_INT64,   xdb::ATTR_NOT_FOUND, 0, { 0, NULL, NULL } },
  { "/KD4005/3/GOV040.DATEHOURM",xdb::DB_TYPE_ABSTIME, xdb::ATTR_NOT_FOUND, 0, { 0, NULL, NULL } },
  { "/KD4005/GOV41A.VALIDCHANGE",xdb::DB_TYPE_UINT8,   xdb::ATTR_NOT_FOUND, 0, { 0, NULL, NULL } },
  { "/KD4005/GOVD5.VALID",       xdb::DB_TYPE_UINT8,   xdb::ATTR_NOT_FOUND, 0, { 0, NULL, NULL } },
  { "/KD4005/2/PT04.VAL",        xdb::DB_TYPE_DOUBLE,  xdb::ATTR_NOT_FOUND, 0, { 0, NULL, NULL } },
  { "/KD4005/2/PT04.SHORTLABEL", xdb::DB_TYPE_BYTES16, xdb::ATTR_NOT_FOUND, 0, { 0, NULL, NULL } },
  { "/KD4005/1/RY05.VAL",        xdb::DB_TYPE_DOUBLE,  xdb::ATTR_NOT_FOUND, 0, { 0, NULL, NULL } },
  { "/KD4005/2/RY052.SHORTLABEL",xdb::DB_TYPE_BYTES16, xdb::ATTR_NOT_FOUND, 0, { 0, NULL, NULL } }
};

void allocate_TestSINF_parameters()
{
  for (int i=0; i<10; i++)
  {
    switch(array_of_parameters[i].type)
    {
      case xdb::DB_TYPE_INT32:
        array_of_parameters[i].value.fixed.val_int32 = 1;
#ifdef PRINT
        printf("test[%d]=%s %d %d\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_int32);
#endif
      break;
      case xdb::DB_TYPE_UINT32:
        array_of_parameters[i].value.fixed.val_uint32 = 2;
#ifdef PRINT
        printf("test[%d]=%s %d %d\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_int32);
#endif
      break;
      case xdb::DB_TYPE_INT64:
        array_of_parameters[i].value.fixed.val_int64 = 3;
#ifdef PRINT
        printf("test[%d]=%s %d %lld\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_int64);
#endif
      break;
      case xdb::DB_TYPE_UINT64:
        array_of_parameters[i].value.fixed.val_uint64 = 4;
#ifdef PRINT
        printf("test[%d]=%s %d %lld\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_int64);
#endif
      break;
      case xdb::DB_TYPE_FLOAT:
        array_of_parameters[i].value.fixed.val_float = 5.678;
#ifdef PRINT
        printf("test[%d]=%s %d %f\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_float);
#endif
      break;
      case xdb::DB_TYPE_DOUBLE:
        array_of_parameters[i].value.fixed.val_double = 6.789;
#ifdef PRINT
        printf("test[%d]=%s %d %g\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_double);
#endif
      break;
      case xdb::DB_TYPE_BYTES:
        array_of_parameters[i].value.dynamic.val_string = new std::string("Строка с русским текстом, цифрами 1 2 3, и точкой.");
#ifdef PRINT
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.dynamic.val_string->c_str());
#endif
      break;
      case xdb::DB_TYPE_BYTES4:
        array_of_parameters[i].value.dynamic.size = 4*sizeof(wchar_t);
        array_of_parameters[i].value.dynamic.varchar = new char[array_of_parameters[i].value.dynamic.size + 1];
        strcpy(array_of_parameters[i].value.dynamic.varchar, "русс");
        //array_of_parameters[i].value.dynamic.varchar[4] = '\0';
#ifdef PRINT
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].val_bytes.data);
#endif
      break;

      case xdb::DB_TYPE_BYTES12:
        array_of_parameters[i].value.dynamic.size = 12*sizeof(wchar_t);
        array_of_parameters[i].value.dynamic.varchar = new char[array_of_parameters[i].value.dynamic.size + 1];
        strcpy(array_of_parameters[i].value.dynamic.varchar, "[12]русский");
        //array_of_parameters[i].value.dynamic.varchar[4] = '\0';
#ifdef PRINT
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].val_bytes.data);
#endif
      break;

      case xdb::DB_TYPE_BYTES16:
        array_of_parameters[i].value.dynamic.size = 16*sizeof(wchar_t);
        array_of_parameters[i].value.dynamic.varchar = new char[array_of_parameters[i].value.dynamic.size + 1];
        strcpy(array_of_parameters[i].value.dynamic.varchar, "[16]русская Ф");
        //array_of_parameters[i].value.dynamic.varchar[4] = '\0';
#ifdef PRINT
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].val_bytes.data);
#endif
      break;

      case xdb::DB_TYPE_BYTES48:
        array_of_parameters[i].value.dynamic.size = 48*sizeof(wchar_t);
        array_of_parameters[i].value.dynamic.varchar = new char[array_of_parameters[i].value.dynamic.size + 1];
        strcpy(array_of_parameters[i].value.dynamic.varchar, "[48]РУССКИЙ йцукен фывапрол");
        //array_of_parameters[i].value.dynamic.varchar[48] = '\0';
#ifdef PRINT
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.dynamic.varchar);
#endif
      break;
      case xdb::DB_TYPE_BYTES256:
        array_of_parameters[i].value.dynamic.size = 256*sizeof(wchar_t);
        array_of_parameters[i].value.dynamic.varchar = new char[array_of_parameters[i].value.dynamic.size + 1];
        strcpy(array_of_parameters[i].value.dynamic.varchar, "[256]Съешь еще этих мягких булочек, да выпей чаю! XYZ");
        //array_of_parameters[i].value.dynamic.varchar[256] = '\0';
#ifdef PRINT
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.dynamic.varchar);
#endif
      break;

      case xdb::DB_TYPE_ABSTIME:
//        time_t given_time = time(0);
//        strftime(s_date, D_DATE_FORMAT_LEN, D_DATE_FORMAT_STR, localtime(&given_time));
//        strcat(s_date, ".99999");
        array_of_parameters[i].value.fixed.val_time.tv_sec = time(0);
        array_of_parameters[i].value.fixed.val_time.tv_usec = 0;
#ifdef PRINT
        printf("test[%d]=%s %d %lld\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_time.tv_sec);
#endif
      break;

      default:
        std::cout << "бебе" << std::endl;
      break;
    }
  }
}

void release_TestSINF_parameters()
{
  for (int i=0; i<=9; i++)
  {
    switch(array_of_parameters[i].type)
    {
      case xdb::DB_TYPE_BYTES:
        delete array_of_parameters[i].value.dynamic.varchar;
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
        delete [] array_of_parameters[i].value.dynamic.varchar;
      break;
      default:
        // do nothing
      break;
    }
  }
}
#endif

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

void* fillValueByType(xdb::AttributeInfo_t& attr_info, std::string& input_val)
{
  std::cout << "check val: " << input_val << std::endl;
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
#warning "Possibility of buffer overflow here"
          strcpy(attr_info.value.dynamic.varchar, input_val.c_str());
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
      std::cout << "E: Undefined type " << attr_info.type << std::endl;
    break;

    default:
      std::cout << "E: Unsupported type " << attr_info.type << std::endl;
    break;
  }

  return ret_val;
}

#if !defined _FUNCTIONAL_TEST

int main (int argc, char *argv [])
{
  Mosquito  *mosquito  = NULL;
  mdp::zmsg         *transport_cell = NULL;
  msg::Letter       *request   = NULL;
  msg::Letter       *report    = NULL;
  msg::ReadMulti    *read_msg  = NULL;
  msg::WriteMulti   *write_msg = NULL;
  //msg::ProbeMsg   *probe_msg = NULL; Не реализовано
  int        opt;
  int        verbose;
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

  while ((opt = getopt (argc, argv, "vs:m:")) != -1)
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
          else {
            std::cout << "Unknown work mode, will use PROBE" << std::endl;
            mode = Mosquito::MODE_PROBE;
          }
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
    std::cout << "Service name not given.\nUse -s <name> [-v] [-m <read|write>] option.\n";
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
        request = mosquito->create_message(SIG_D_MSG_READ_MULTI);
        read_msg = dynamic_cast<msg::ReadMulti*>(request);
        assert(read_msg);
        num = 0;
#if 1
        while (std::cin >> input_tag)
        {
            read_msg->add(input_tag, xdb::DB_TYPE_UNDEF, 0);
            num++;
        }
#else
       for (int idx = 0; idx < 10; idx++)
       {
            if (array_of_parameters[idx].type == xdb::DB_TYPE_BYTES)
            {
            read_msg->add(array_of_parameters[idx].name,
                              array_of_parameters[idx].type,
                              static_cast<void*>(array_of_parameters[idx].value.dynamic.val_string));
            }
            else if ((xdb::DB_TYPE_BYTES4 <= array_of_parameters[idx].type)
                  && (array_of_parameters[idx].type <= xdb::DB_TYPE_BYTES256))
            {
            read_msg->add(array_of_parameters[idx].name,
                              array_of_parameters[idx].type,
                              static_cast<void*>(array_of_parameters[idx].value.dynamic.varchar));
            }
            else
            {
            read_msg->add(array_of_parameters[idx].name,
                              array_of_parameters[idx].type,
                              static_cast<void*>(&array_of_parameters[idx].value.fixed.val_uint8));
            }
        }
#endif
        LOG(INFO) << "Ready to read " << num << " parameters";
      break;

      case Mosquito::MODE_WRITE:
        request = mosquito->create_message(SIG_D_MSG_WRITE_MULTI);
        write_msg = dynamic_cast<msg::WriteMulti*>(request);
        assert(write_msg);
        num = 0;
#if 1
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
#else
        allocate_TestSINF_parameters();
        for (int idx = 0; idx < 10; idx++)
        {
            if (array_of_parameters[idx].type == xdb::DB_TYPE_BYTES)
            {
            write_msg->add(array_of_parameters[idx].name,
                              array_of_parameters[idx].type,
                              static_cast<void*>(array_of_parameters[idx].value.dynamic.val_string));
            }
            else if ((xdb::DB_TYPE_BYTES4 <= array_of_parameters[idx].type)
                  && (array_of_parameters[idx].type <= xdb::DB_TYPE_BYTES256))
            {
            write_msg->add(array_of_parameters[idx].name,
                              array_of_parameters[idx].type,
                              static_cast<void*>(array_of_parameters[idx].value.dynamic.varchar));
            }
            else
            {
            write_msg->add(array_of_parameters[idx].name,
                              array_of_parameters[idx].type,
                              static_cast<void*>(&array_of_parameters[idx].value.fixed.val_uint8));
            }
        }
#endif
        LOG(INFO) << "Ready to write " << num << " parameters";
      break;

      case Mosquito::MODE_PROBE:
        request = NULL; // TODO: 2015/06/30 не реализовано
      break;
    }

    // Есть чего сказать миру?
    if (request)
    {
      // запросим состояние интересующей Службы
      std::cout << "Checking '" << service_name << "' status " << std::endl;
      service_status = mosquito->ask_service_info(service_name.c_str(), service_endpoint, ENDPOINT_MAXLEN);

      switch(service_status)
      {
          case 200:
          std::cout << service_name << " status OK : " << service_status << std::endl;
          std::cout << "Get point to " << service_endpoint << std::endl;
          break;

          case 400:
          std::cout << service_name << " status BAD_REQUEST : " << service_status << std::endl;
          break;

          case 404:
          std::cout << service_name << " status NOT_FOUND : " << service_status << std::endl;
          break;

          case 501:
          std::cout << service_name << " status NOT_SUPPORTED : " << service_status << std::endl;
          break;

          default:
          std::cout << service_name << " status UNKNOWN : " << service_status << std::endl;
      }

      if (200 != service_status)
      {
          std::cout << "Service '" << service_name << "' is not running, exiting" << std::endl;
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
    while (!stop_receiving) {

        report = mosquito->recv ();
        if (report == NULL)
        {
            stop_receiving = true;
            break;
        }

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
            stop_receiving = true;
          break;

          // NB: не должно быть такого ответа, должен быть ExecResult 
          case SIG_D_MSG_WRITE_MULTI:
            std::cout << "Error: expect ExecResult but received SIG_D_MSG_WRITE_MULTI response: "
                      << report->header()->usr_msg_type() << std::endl;
            stop_receiving = true;
          break;

          default:
            std::cout << "Got unsupported response type: "
                      << report->header()->usr_msg_type() << std::endl;
        }

        delete report;
    }
  }
  catch (zmq::error_t err)
  {
      std::cout << "E: " << err.what() << std::endl;
  }

#if 0
  // for valgrind joy!
  release_TestSINF_parameters();
#endif

  delete mosquito;
  return 0;
}

#endif

