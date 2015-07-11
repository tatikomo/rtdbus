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
     m_factory->create(req);

  return result;
}

msg::Letter* Mosquito::create_message(rtdbMsgType type)
{
  return m_factory->create(type);
}

// NB: этот массив содержит полные названия тегов, с атрибутом.
xdb::AttributeInfo_t array_of_parameters[10] = {
  { "/KD4001/GOV022.OBJCLASS", xdb::DB_TYPE_INT32,  0 },
  { "/KD4001/GOV023.STATUS", xdb::DB_TYPE_UINT32, 0 },
  { "/KD4001/GOV024.SA_ref", xdb::DB_TYPE_INT64,  0 },
  { "/KD4001/GOV025.DATEHOURM", xdb::DB_TYPE_UINT64, 0 },
  { "/KD4001/GOV026.VAL", xdb::DB_TYPE_FLOAT,  0 },
  { "/KD4001/GOV027.VAL", xdb::DB_TYPE_DOUBLE, 0 },
  { "/KD4001/GOV028.VAL", xdb::DB_TYPE_BYTES,  0 },
  { "/KD4001/GOV029.VAL", xdb::DB_TYPE_BYTES4, 0 },
  { "/KD4001/GOV030.VAL", xdb::DB_TYPE_BYTES48, 0 },
  { "/KD4001/GOV031.SHORTLABEL", xdb::DB_TYPE_BYTES256, 0 }
};

void allocate_TestSINF_parameters()
{
  for (int i=0; i<=9; i++)
  {
    switch(array_of_parameters[i].type)
    {
      case xdb::DB_TYPE_INT32:
        array_of_parameters[i].value.val_int32 = 1;
#ifdef PRINT
        printf("test[%d]=%s %d %d\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.val_int32);
#endif
      break;
      case xdb::DB_TYPE_UINT32:
        array_of_parameters[i].value.val_uint32 = 2;
#ifdef PRINT
        printf("test[%d]=%s %d %d\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.val_int32);
#endif
      break;
      case xdb::DB_TYPE_INT64:
        array_of_parameters[i].value.val_int64 = 3;
#ifdef PRINT
        printf("test[%d]=%s %d %lld\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.val_int64);
#endif
      break;
      case xdb::DB_TYPE_UINT64:
        array_of_parameters[i].value.val_uint64 = 4;
#ifdef PRINT
        printf("test[%d]=%s %d %lld\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.val_int64);
#endif
      break;
      case xdb::DB_TYPE_FLOAT:
        array_of_parameters[i].value.val_float = 5.678;
#ifdef PRINT
        printf("test[%d]=%s %d %f\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.val_float);
#endif
      break;
      case xdb::DB_TYPE_DOUBLE:
        array_of_parameters[i].value.val_double = 6.789;
#ifdef PRINT
        printf("test[%d]=%s %d %g\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.val_double);
#endif
      break;
      case xdb::DB_TYPE_BYTES:
        array_of_parameters[i].value.val_string = new std::string("Строка с русским текстом, цифрами 1 2 3, и точкой.");
#ifdef PRINT
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.val_string->c_str());
#endif
      break;
      case xdb::DB_TYPE_BYTES4:
        array_of_parameters[i].value.val_bytes.size = 4;
        array_of_parameters[i].value.val_bytes.data = new char[4 + 1];
        strncpy(array_of_parameters[i].value.val_bytes.data, "русс", 4);
        array_of_parameters[i].value.val_bytes.data[4] = '\0';
#ifdef PRINT
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].val_bytes.data);
#endif
      break;
      case xdb::DB_TYPE_BYTES48:
        array_of_parameters[i].value.val_bytes.size = 48;
        array_of_parameters[i].value.val_bytes.data = new char[48 + 1];
        strncpy(array_of_parameters[i].value.val_bytes.data, "48:РУССКИЙ йцукен фывапрол", 48);
        array_of_parameters[i].value.val_bytes.data[48] = '\0';
#ifdef PRINT
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.val_bytes.data);
#endif
      break;
      case xdb::DB_TYPE_BYTES256:
        array_of_parameters[i].value.val_bytes.size = 256;
        array_of_parameters[i].value.val_bytes.data = new char[256 + 1];
        strncpy(array_of_parameters[i].value.val_bytes.data, "BYTES256:1234567890ABCDEFGHIJKLOMNOPQRSTUWXYZ", 256);
        array_of_parameters[i].value.val_bytes.data[256] = '\0';
#ifdef PRINT
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.val_bytes.data);
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
        delete array_of_parameters[i].value.val_bytes.data;
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
        delete [] array_of_parameters[i].value.val_bytes.data;
      break;
      default:
        // do nothing
      break;
    }
  }
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
  mdp::ChannelType channel = mdp::ChannelType::PERSISTENT;
  char one_argument[SERVICE_NAME_MAXLEN + 1];
  char service_endpoint[ENDPOINT_MAXLEN + 1];
  int service_status;   // 200|400|404|501
  // название Сервиса
  std::string service_name;
  bool is_service_name_given = false;
  Mosquito::WorkMode_t mode;

  while ((opt = getopt (argc, argv, "vds:m:")) != -1)
  {
    switch (opt)
    {
        case 'v': // режим подробного вывода
          verbose = 1;
          break;

        case 'd': // Direct - прямые сообщения в адрес Сервиса
          channel = mdp::ChannelType::DIRECT;
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
    std::cout << "Service name not given.\nUse -s <name> [-v] [-d] [-m <mode>] option.\n";
    return(1);
  }
  std::cout << "Will use work mode " << mode << std::endl;

  mosquito = new Mosquito ("tcp://localhost:5555", verbose, mode);

  try
  {
    allocate_TestSINF_parameters();

    // Подготовить сообщение, заполнив названия интересующих тегов
    switch(mode)
    {
      case Mosquito::MODE_READ:
        request = mosquito->create_message(SIG_D_MSG_READ_MULTI);
        read_msg = dynamic_cast<msg::ReadMulti*>(request);
        assert(read_msg);

        for (int idx = 0; idx < 10; idx++)
        {
            if (array_of_parameters[idx].type == xdb::DB_TYPE_BYTES)
            {
            read_msg->add(array_of_parameters[idx].name,
                              array_of_parameters[idx].type,
                              static_cast<void*>(array_of_parameters[idx].value.val_string));
            }
            else if ((xdb::DB_TYPE_BYTES4 <= array_of_parameters[idx].type)
                  && (array_of_parameters[idx].type <= xdb::DB_TYPE_BYTES256))
            {
            read_msg->add(array_of_parameters[idx].name,
                              array_of_parameters[idx].type,
                              static_cast<void*>(array_of_parameters[idx].value.val_bytes.data));
            }
            else
            {
            read_msg->add(array_of_parameters[idx].name,
                              array_of_parameters[idx].type,
                              static_cast<void*>(&array_of_parameters[idx].value.val_uint64));
            }
        }
      break;

      case Mosquito::MODE_WRITE:
        request = mosquito->create_message(SIG_D_MSG_WRITE_MULTI);
        write_msg = dynamic_cast<msg::WriteMulti*>(request);
        assert(write_msg);

        for (int idx = 0; idx < 10; idx++)
        {
            if (array_of_parameters[idx].type == xdb::DB_TYPE_BYTES)
            {
            write_msg->add(array_of_parameters[idx].name,
                              array_of_parameters[idx].type,
                              static_cast<void*>(array_of_parameters[idx].value.val_string));
            }
            else if ((xdb::DB_TYPE_BYTES4 <= array_of_parameters[idx].type)
                  && (array_of_parameters[idx].type <= xdb::DB_TYPE_BYTES256))
            {
            write_msg->add(array_of_parameters[idx].name,
                              array_of_parameters[idx].type,
                              static_cast<void*>(array_of_parameters[idx].value.val_bytes.data));
            }
            else
            {
            write_msg->add(array_of_parameters[idx].name,
                              array_of_parameters[idx].type,
                              static_cast<void*>(&array_of_parameters[idx].value.val_uint64));
            }
        }
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

    //  Wait for report
    while (1) {
        report = mosquito->recv ();
        if (report == NULL)
            break;

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
          break;

          case SIG_D_MSG_READ_MULTI:
            std::cout << "Got SIG_D_MSG_READ_MULTI response: "
                      << report->header()->usr_msg_type() << std::endl;
          break;

          // NB: не должно быть такого ответа, должен быть ExecResult 
          case SIG_D_MSG_WRITE_MULTI:
            std::cout << "Error: expect ExecResult but received SIG_D_MSG_WRITE_MULTI response: "
                      << report->header()->usr_msg_type() << std::endl;
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

  release_TestSINF_parameters();
  delete mosquito;
  return 0;
}

#endif

