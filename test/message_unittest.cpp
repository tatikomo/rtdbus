#if defined HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <string>

// Служебные заголовочные файлы сторонних утилит
#include "google/protobuf/stubs/common.h"
#include "gtest/gtest.h"
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "helper.hpp"
#include "mdp_zmsg.hpp"
#include "mdp_common.h"
#include "msg_message.hpp"
#include "msg_adm.hpp"
#include "msg_sinf.hpp"
#include "proto/rtdbus.pb.h"
#include "xdb_broker_letter.hpp"
#include "xdb_common.hpp"

std::string        service_name_1 = "service_name_1";
std::string        pb_serialized_header;
std::string        pb_serialized_request;
RTDBM::Header      pb_header;
RTDBM::ExecResult  pb_exec_result_request;
xdb::Letter       *letter = NULL;
mdp::zmsg         *message = NULL;

// Сообщения, создаваемые фабрикой сообщений в зависимости от указанного типа
msg::AskLife        *ask_life       = NULL;
msg::ReadMulti      *readMultiMsg   = NULL;
msg::WriteMulti     *writeMultiMsg  = NULL;
msg::HistoryRequest *queryHistoryMsg = NULL;

const msg::Header   *head = NULL;
const msg::Data     *data = NULL;
static const char   *my_name = (const char*)"message_test";
msg::MessageFactory *message_factory = NULL;

xdb::AttributeInfo_t array_of_parameters[10];

void allocate_TestSINF_parameters()
{
  char uname[60];
  xdb::DbType_t current_type[10] = {
   xdb::DB_TYPE_INT32,
   xdb::DB_TYPE_UINT32,
   xdb::DB_TYPE_INT64,
   xdb::DB_TYPE_UINT64,
   xdb::DB_TYPE_FLOAT,
   xdb::DB_TYPE_DOUBLE,
   xdb::DB_TYPE_BYTES,
   xdb::DB_TYPE_BYTES4,
   xdb::DB_TYPE_BYTES48,
   xdb::DB_TYPE_BYTES256 };

  for (int i=0; i<=9; i++)
  {
    sprintf(uname, "/K4%d001/SITE.VAL", i);
    array_of_parameters[i].name.assign(uname);
    array_of_parameters[i].type = current_type[i];
    array_of_parameters[i].quality = xdb::ATTR_NOT_FOUND;
    memset((void*)&array_of_parameters[i].value, '\0', sizeof(xdb::AttrVal_t));

    switch(array_of_parameters[i].type)
    {
      case xdb::DB_TYPE_INT32:
        array_of_parameters[i].value.fixed.val_int32 = 1;
        printf("test[%d]=%s %d %d\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_int32);
      break;
      case xdb::DB_TYPE_UINT32:
        array_of_parameters[i].value.fixed.val_uint32 = 2;
        printf("test[%d]=%s %d %d\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_int32);
      break;
      case xdb::DB_TYPE_INT64:
        array_of_parameters[i].value.fixed.val_int64 = 3;
        printf("test[%d]=%s %d %lld\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_int64);
      break;
      case xdb::DB_TYPE_UINT64:
        array_of_parameters[i].value.fixed.val_uint64 = 4;
        printf("test[%d]=%s %d %lld\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_int64);
      break;
      case xdb::DB_TYPE_FLOAT:
        array_of_parameters[i].value.fixed.val_float = 5.678;
        printf("test[%d]=%s %d %f\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_float);
      break;
      case xdb::DB_TYPE_DOUBLE:
        array_of_parameters[i].value.fixed.val_double = 6.789;
        printf("test[%d]=%s %d %g\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.fixed.val_double);
      break;
      case xdb::DB_TYPE_BYTES:
        array_of_parameters[i].value.dynamic.val_string = new std::string("Строка с русским текстом, цифрами 1 2 3, и точкой.");
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.dynamic.val_string->c_str());
      break;
      case xdb::DB_TYPE_BYTES4:
        array_of_parameters[i].value.dynamic.size = 4;
        array_of_parameters[i].value.dynamic.varchar = new char[sizeof(wchar_t)*4 + 1];
        strcpy(array_of_parameters[i].value.dynamic.varchar, "русс");
        //array_of_parameters[i].value.dynamic.varchar[4] = '\0';
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.dynamic.varchar);
      break;
      case xdb::DB_TYPE_BYTES48:
        array_of_parameters[i].value.dynamic.size = 48;
        array_of_parameters[i].value.dynamic.varchar = new char[sizeof(wchar_t)*48 + 1];
        strcpy(array_of_parameters[i].value.dynamic.varchar, "BYTES48:РУССКИЙ текст йцукен фывапрол");
        //array_of_parameters[i].value.dynamic.varchar[48] = '\0';
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.dynamic.varchar);
      break;
      case xdb::DB_TYPE_BYTES256:
        array_of_parameters[i].value.dynamic.size = 256;
        array_of_parameters[i].value.dynamic.varchar = new char[sizeof(wchar_t)*256 + 1];
        strcpy(array_of_parameters[i].value.dynamic.varchar, "BYTES256:1234567890ABCDEFGHIJKLOMNOPQRSTUWXYZ");
        //array_of_parameters[i].value.dynamic.varchar[256] = '\0';
        printf("test[%d]=%s %d \"%s\"\n", i,
                array_of_parameters[i].name.c_str(),
                array_of_parameters[i].type,
                array_of_parameters[i].value.dynamic.varchar);
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
        delete array_of_parameters[i].value.dynamic.val_string;
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

TEST(TestProtobuf, PROTO_VERSION)
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;
}

/*
 * Создает экземпляр zmsg, содержащий нагрузку из
 * заголовка и структуры ExecResult
 * NB: после использования необходимо удалить message
 */
TEST(TestProtobuf, EXEC_RESULT)
{
  std::string error_text("поддерживается русский текст");
  pb_header.set_protocol_version(1);
  pb_header.set_exchange_id(9999999);
  pb_header.set_interest_id(1234);
  pb_header.set_source_pid(9999);
  pb_header.set_proc_dest("В чащах юга жил-был Цитрус?");
  pb_header.set_proc_origin("Да! Но фальшивый экземпляр.");
  pb_header.set_sys_msg_type(100);
  pb_header.set_usr_msg_type(ADG_D_MSG_EXECRESULT);
  pb_header.set_time_mark(time(0));

  pb_exec_result_request.set_exec_result(RTDBM::GOF_D_PARTSUCCESS);
  pb_exec_result_request.mutable_failure_cause()->set_error_code(5);
  pb_exec_result_request.mutable_failure_cause()->set_error_text(error_text);

  pb_header.SerializeToString(&pb_serialized_header);
  pb_exec_result_request.SerializeToString(&pb_serialized_request);

  message = new mdp::zmsg();
  ASSERT_TRUE(message);

  message->push_front(pb_serialized_request);
  message->push_front(pb_serialized_header);
  message->push_front(const_cast<char*>(EMPTY_FRAME));
  message->push_front(const_cast<char*>(my_name));
  message->dump();
}

/*
 * Создать экземпляр Payload на основе zmsg и сравнить
 * восстановленные на его основе данные с образцом
 */
TEST(TestPayload, CREATE_FROM_MSG)
{
  std::string serialized_header;
  std::string serialized_request;

  try
  {
    letter = new xdb::Letter(message);
    ASSERT_TRUE(letter != NULL);

    serialized_header = letter->GetHEADER();
    serialized_request= letter->GetDATA();

    EXPECT_EQ(serialized_header,  pb_serialized_header);
    EXPECT_EQ(serialized_request, pb_serialized_request);
  }
  catch(...)
  {
    LOG(ERROR) << "Unable to create letter";
  }
}

TEST(TestPayload, ACCESS)
{
  RTDBM::ExecResult exec_result;
  msg::Header* header = new msg::Header(letter->GetHEADER());
  ASSERT_TRUE(header);

  exec_result.ParseFromString(letter->GetDATA());

  EXPECT_EQ(pb_header.protocol_version(),            header->protocol_version());
  EXPECT_EQ((rtdbExchangeId)pb_header.exchange_id(), header->exchange_id());
  EXPECT_EQ((rtdbExchangeId)pb_header.interest_id(), header->interest_id());
  EXPECT_EQ((rtdbPid)pb_header.source_pid(),         header->source_pid());
  EXPECT_EQ(pb_header.proc_dest(),        header->proc_dest());
  EXPECT_EQ(pb_header.proc_origin(),      header->proc_origin());
  EXPECT_EQ(pb_header.sys_msg_type(),     header->sys_msg_type());
  EXPECT_EQ(pb_header.usr_msg_type(),     header->usr_msg_type());
  EXPECT_EQ(pb_header.time_mark(),        header->time_mark());

  EXPECT_EQ(pb_exec_result_request.exec_result(),      exec_result.exec_result());

  EXPECT_EQ(pb_exec_result_request.has_failure_cause(), true);
  EXPECT_EQ(exec_result.has_failure_cause(), true);
  EXPECT_EQ(exec_result.failure_cause().error_code(), pb_exec_result_request.failure_cause().error_code());
  EXPECT_EQ(exec_result.failure_cause().error_text().compare(pb_exec_result_request.failure_cause().error_text()), 0);

  delete header;
}

TEST(TespPayload, DELETE)
{
  delete message;
  delete letter;
}

TEST(TestMessage, CREATE_FACTORY)
{
  std::string   attr_type_name;
  xdb::DbType_t attr_type = xdb::DB_TYPE_UNDEF;
  const char* attr_type_char = NULL;
  bool status;

  message_factory = new msg::MessageFactory(my_name);
  ASSERT_TRUE(message_factory);

  attr_type = xdb::DB_TYPE_ABSTIME;
  attr_type_char = message_factory->GetDbNameFromType(attr_type);
  ASSERT_TRUE(attr_type_char);
  EXPECT_EQ(strcmp(attr_type_char, "ABS_TIME"), 0);

  attr_type_name = "LOGICAL";
  status = message_factory->GetDbTypeFromString(attr_type_name, attr_type);
  ASSERT_TRUE(status == true);
  EXPECT_EQ(attr_type, xdb::DB_TYPE_LOGICAL);
}

TEST(TestMessage, CREATE_LETTER)
{
  // Начало сериализации данных тестового запроса AskLife
  RTDBM::Header         pb_header_asklife;
  RTDBM::SimpleRequest  pb_simple_request;
  RTDBM::gof_t_ExecResult idx;
//  RTDBM::gof_t_ExecResult pb_exec_value;
  int exec_value;
  std::string     pb_serialized_header_asklife;
  std::string     pb_serialized_request_asklife;

  pb_header_asklife.set_protocol_version(1);
  pb_header_asklife.set_exchange_id(12345678);
  pb_header_asklife.set_interest_id(1);
  pb_header_asklife.set_source_pid(9999);
  pb_header_asklife.set_proc_dest("DAD");
  pb_header_asklife.set_proc_origin("SON");
  pb_header_asklife.set_sys_msg_type(USER_MESSAGE_TYPE);
  pb_header_asklife.set_usr_msg_type(ADG_D_MSG_ASKLIFE);
  pb_header_asklife.set_time_mark(time(0));

  pb_simple_request.SerializeToString(&pb_serialized_request_asklife);
  // д.б. пустая строка, поскольку protobuf не сериализует неинициализированные поля
  EXPECT_EQ(pb_serialized_request_asklife.size(), 0);

  idx = RTDBM::gof_t_ExecResult_MIN;
  pb_simple_request.set_exec_result(idx);
  EXPECT_EQ(pb_simple_request.exec_result(), idx);
  pb_simple_request.SerializeToString(&pb_serialized_request_asklife);
  // Содержит нормальные значения, т.к. была проведена инициализация
  EXPECT_EQ((pb_serialized_request_asklife.size() > 0), true);

  idx = RTDBM::gof_t_ExecResult_MAX;
  pb_simple_request.set_exec_result(idx);
  EXPECT_EQ(pb_simple_request.exec_result(), idx);
  pb_simple_request.SerializeToString(&pb_serialized_request_asklife);
  // Содержит нормальные значения, т.к. была проведена инициализация
  EXPECT_EQ((pb_serialized_request_asklife.size() > 0), true);

  pb_simple_request.set_exec_result(RTDBM::GOF_D_PARTSUCCESS);
  EXPECT_EQ(pb_simple_request.exec_result(), RTDBM::GOF_D_PARTSUCCESS);

  pb_simple_request.SerializeToString(&pb_serialized_request_asklife);

  pb_header_asklife.SerializeToString(&pb_serialized_header_asklife);
//  hex_dump(pb_serialized_header_asklife);
//  hex_dump(pb_serialized_request_asklife);
  // Завершение сериализации запроса AskLife

  ask_life = static_cast<msg::AskLife*>(message_factory->unserialize(
                                pb_serialized_header_asklife,
                                pb_serialized_request_asklife));
  ASSERT_TRUE(ask_life);

  //ask_life->dump();

  head = ask_life->header();
  ASSERT_TRUE(head);

  data = ask_life->data();
  ASSERT_TRUE(data);

  EXPECT_EQ(ask_life->header()->usr_msg_type(), ADG_D_MSG_ASKLIFE);
  EXPECT_EQ(ask_life->header()->proc_origin().compare("SON"), 0);
  EXPECT_EQ(ask_life->header()->proc_dest().compare("DAD"), 0);
  EXPECT_EQ(ask_life->header()->exchange_id(), 12345678);
  EXPECT_EQ(ask_life->header()->interest_id(), 1);
  EXPECT_EQ(ask_life->header()->sys_msg_type(), USER_MESSAGE_TYPE);
  EXPECT_EQ(ask_life->exec_result(exec_value), true);
  EXPECT_EQ(exec_value, 2); // RTDBM::GOF_D_PARTSUCCESS
//  hex_dump(ask_life->header()->get_serialized());
//  hex_dump(ask_life->data()->get_serialized());

  delete ask_life;

  ask_life = static_cast<msg::AskLife*>(message_factory->create(ADG_D_MSG_ASKLIFE));
  ASSERT_TRUE(ask_life);
  EXPECT_EQ(ask_life->header()->usr_msg_type(), ADG_D_MSG_ASKLIFE);
  EXPECT_EQ(ask_life->header()->sys_msg_type(), USER_MESSAGE_TYPE);
  EXPECT_EQ(ask_life->header()->exchange_id(), 1); // т.к. это первый вызов create()
  EXPECT_EQ(ask_life->header()->interest_id(), 0); // 0 по-умолчанию

  ask_life->set_exec_result(1); // set RTDBM::GOF_D_SUCCESS
  EXPECT_EQ(ask_life->exec_result(exec_value), true);
  EXPECT_EQ(exec_value, 1);     // get RTDBM::GOF_D_SUCCESS
//  hex_dump(ask_life->data()->get_serialized());

  ask_life->dump();

  delete ask_life;

  ask_life = static_cast<msg::AskLife*>(message_factory->create(ADG_D_MSG_ASKLIFE));
  ASSERT_TRUE(ask_life);
  EXPECT_EQ(ask_life->header()->usr_msg_type(), ADG_D_MSG_ASKLIFE);
  EXPECT_EQ(ask_life->header()->sys_msg_type(), USER_MESSAGE_TYPE);
  EXPECT_EQ(ask_life->header()->exchange_id(), 2); // т.к. уже второй вызов create()
  EXPECT_EQ(ask_life->header()->interest_id(), 0); // 0 по-умолчанию

}

TEST(TestMessage, DESTROY_LETTER)
{
  delete ask_life;
}

TEST(TestMessage, CREATE_SINF)
{
  readMultiMsg   = static_cast<msg::ReadMulti*>  (message_factory->create(SIG_D_MSG_READ_MULTI));
  writeMultiMsg  = static_cast<msg::WriteMulti*> (message_factory->create(SIG_D_MSG_WRITE_MULTI));
  queryHistoryMsg = static_cast<msg::HistoryRequest*> (message_factory->create(SIG_D_MSG_REQ_HISTORY));
}

//
// Протестировать поведение сообщений при добавлении в них всех типов данных
// с последующим корректным чтением.
//


TEST(TestMessage, USE_SINF)
{
  std::string tag_out = "incorrect initial value";
  char buffer [50];
  char s_date [D_DATE_FORMAT_LEN + 1];
  struct tm result_time;
  time_t given_time;
  xdb::DbType_t type_out = xdb::DB_TYPE_UNDEF;
  xdb::Quality_t quality_out;
  int32_t  val_int32;
/*  uint32_t val_uint32;
  int64_t  val_int64;
  uint64_t val_ui64;
  float    val_float = 0.0;
  double   val_double = 0.0;
  char     val_chars[256+1] = "";
  std::string val_string;*/
  int idx;

  // Инициировать заполнение тестового массива данными
  allocate_TestSINF_parameters();

  // ======================================================
  // Проверка объекта сообщения для чтения одного тега
  // ======================================================
  EXPECT_EQ(readMultiMsg->num_items(), 0);
  readMultiMsg->add(array_of_parameters[0].name,
                    array_of_parameters[0].type,
                    static_cast<void*>(&array_of_parameters[0].value.fixed.val_int32));

  bool is_success = readMultiMsg->get(0, tag_out, type_out, quality_out, static_cast<void*>(&val_int32));
  EXPECT_EQ(is_success, true);
  EXPECT_EQ(tag_out.compare(array_of_parameters[0].name), 0);
  EXPECT_EQ(type_out, array_of_parameters[0].type);
  EXPECT_EQ(val_int32,  array_of_parameters[0].value.fixed.val_int32);

  // ======================================================
  // Проверка объекта сообщения для чтения нескольких тегов
  // ======================================================
  EXPECT_EQ(readMultiMsg->num_items(), 1);
  // потому что в сообщение уже добавлена одна запись
  for (idx = 1; idx <= 9; idx++)
  {
    if (array_of_parameters[idx].type == xdb::DB_TYPE_BYTES)
    {
    readMultiMsg->add(array_of_parameters[idx].name,
                      array_of_parameters[idx].type,
                      static_cast<void*>(array_of_parameters[idx].value.dynamic.val_string));
    }
    else if ((xdb::DB_TYPE_BYTES4 <= array_of_parameters[idx].type)
          && (array_of_parameters[idx].type <= xdb::DB_TYPE_BYTES256))
    {
    readMultiMsg->add(array_of_parameters[idx].name,
                      array_of_parameters[idx].type,
                      static_cast<void*>(array_of_parameters[idx].value.dynamic.varchar));
    }
    else
    {
    readMultiMsg->add(array_of_parameters[idx].name,
                      array_of_parameters[idx].type,
                      static_cast<void*>(&array_of_parameters[idx].value.fixed.val_uint64));
    }

    // потому что в сообщение уже была добавлена одна запись
    EXPECT_EQ(readMultiMsg->num_items(), idx + 1);

    const msg::Value& todo1 = readMultiMsg->item(idx);
    EXPECT_EQ(todo1.tag().compare(array_of_parameters[idx].name), 0);
    EXPECT_EQ(todo1.type(), array_of_parameters[idx].type);
    switch(todo1.type())
    {
        case xdb::DB_TYPE_LOGICAL:
            EXPECT_EQ(todo1.raw().fixed.val_bool, array_of_parameters[idx].value.fixed.val_bool);
        break;
        case xdb::DB_TYPE_INT8:
            EXPECT_EQ(todo1.raw().fixed.val_int32, array_of_parameters[idx].value.fixed.val_int32);
        break;
        case xdb::DB_TYPE_UINT8:
            EXPECT_EQ(todo1.raw().fixed.val_int32, array_of_parameters[idx].value.fixed.val_uint32);
        break;
        case xdb::DB_TYPE_INT16:
            EXPECT_EQ(todo1.raw().fixed.val_int32, array_of_parameters[idx].value.fixed.val_int32);
        break;
        case xdb::DB_TYPE_UINT16:
            EXPECT_EQ(todo1.raw().fixed.val_int32, array_of_parameters[idx].value.fixed.val_uint32);
        break;
        case xdb::DB_TYPE_INT32:
            EXPECT_EQ(todo1.raw().fixed.val_int32, array_of_parameters[idx].value.fixed.val_int32);
        break;
        case xdb::DB_TYPE_UINT32:
            EXPECT_EQ(todo1.raw().fixed.val_uint32, array_of_parameters[idx].value.fixed.val_uint32);
        break;
        case xdb::DB_TYPE_INT64:
            EXPECT_EQ(todo1.raw().fixed.val_int64, array_of_parameters[idx].value.fixed.val_int64);
        break;
        case xdb::DB_TYPE_UINT64:
            EXPECT_EQ(todo1.raw().fixed.val_uint64, array_of_parameters[idx].value.fixed.val_uint64);
        break;
        case xdb::DB_TYPE_FLOAT:
            EXPECT_EQ(todo1.raw().fixed.val_float, array_of_parameters[idx].value.fixed.val_float);
        break;
        case xdb::DB_TYPE_DOUBLE:
            EXPECT_EQ(todo1.raw().fixed.val_double, array_of_parameters[idx].value.fixed.val_double);
        break;
        case xdb::DB_TYPE_BYTES:
//            std::cout << "GEV " << *todo1.raw().dynamic.val_string << "\nGEV " << *array_of_parameters[idx].value.dynamic.val_string << std::endl;
//            std::cout << "GEV " << *todo1.raw().dynamic.val_string << "\nGEV " << *array_of_parameters[idx].value.dynamic.val_string << std::endl;
            EXPECT_EQ(todo1.raw().dynamic.val_string->compare(*array_of_parameters[idx].value.dynamic.val_string), 0);
        break;
        case xdb::DB_TYPE_BYTES4:
        case xdb::DB_TYPE_BYTES8:
        case xdb::DB_TYPE_BYTES12:
        case xdb::DB_TYPE_BYTES16:
        case xdb::DB_TYPE_BYTES20:
        case xdb::DB_TYPE_BYTES48:
        case xdb::DB_TYPE_BYTES80:
        case xdb::DB_TYPE_BYTES128:
        case xdb::DB_TYPE_BYTES256:
//            std::cout << todo1.raw().dynamic.varchar << " <-> " << array_of_parameters[idx].value.dynamic.varchar << std::endl;
            EXPECT_EQ(strcmp(todo1.raw().dynamic.varchar, array_of_parameters[idx].value.dynamic.varchar), 0);
        break;

        case xdb::DB_TYPE_ABSTIME:
          given_time = todo1.raw().fixed.val_time.tv_sec;
          localtime_r(&given_time, &result_time);
          strftime(s_date, D_DATE_FORMAT_LEN, D_DATE_FORMAT_STR, &result_time);
          snprintf(buffer, D_DATE_FORMAT_W_MSEC_LEN, "%s.%06ld", s_date, todo1.raw().fixed.val_time.tv_usec);
          std::cout << "time: " << buffer << std::endl; //1
        break;

        default:
          std::cout << "readMultiMsg test: unsupported type " << todo1.type() << std::endl;
        break;
    }
  }

  // ======================================================
  // Проверка объекта сообщения для записи одного тега
  // ======================================================
  EXPECT_EQ(writeMultiMsg->num_items(), 0);
  writeMultiMsg->add(array_of_parameters[0].name,
                     array_of_parameters[0].type,
                     static_cast<void*>(&array_of_parameters[0].value.fixed.val_int32));
  EXPECT_EQ(writeMultiMsg->num_items(), 1);

  // ======================================================
  // Проверка объекта сообщения для записи нескольких тегов
  // ======================================================
  for (idx = 1; idx <= 9; idx++)
  {
    if (array_of_parameters[idx].type == xdb::DB_TYPE_BYTES)
    {
    writeMultiMsg->add(array_of_parameters[idx].name,
                       array_of_parameters[idx].type,
                       static_cast<void*>(array_of_parameters[idx].value.dynamic.val_string));
    }
    else if ((xdb::DB_TYPE_BYTES4 <= array_of_parameters[idx].type)
          && (array_of_parameters[idx].type <= xdb::DB_TYPE_BYTES256))
    {
    writeMultiMsg->add(array_of_parameters[idx].name,
                       array_of_parameters[idx].type,
                       static_cast<void*>(array_of_parameters[idx].value.dynamic.varchar));
    }
    else
    {
    writeMultiMsg->add(array_of_parameters[idx].name,
                       array_of_parameters[idx].type,
                       static_cast<void*>(&array_of_parameters[idx].value.fixed.val_uint64));
    }

    // потому что в сообщение уже была добавлена одна запись
    EXPECT_EQ(writeMultiMsg->num_items(), idx + 1);

    const msg::Value& todo2 = writeMultiMsg->item(idx);
    EXPECT_EQ(todo2.tag().compare(array_of_parameters[idx].name), 0);
    EXPECT_EQ(todo2.type(), array_of_parameters[idx].type);

    switch(todo2.type())
    {
        case xdb::DB_TYPE_LOGICAL:
            EXPECT_EQ(todo2.raw().fixed.val_bool, array_of_parameters[idx].value.fixed.val_bool);
        break;
        case xdb::DB_TYPE_INT8:
            EXPECT_EQ(todo2.raw().fixed.val_int32, array_of_parameters[idx].value.fixed.val_int32);
        break;
        case xdb::DB_TYPE_UINT8:
            EXPECT_EQ(todo2.raw().fixed.val_uint32, array_of_parameters[idx].value.fixed.val_uint32);
        break;
        case xdb::DB_TYPE_INT16:
            EXPECT_EQ(todo2.raw().fixed.val_int32, array_of_parameters[idx].value.fixed.val_int32);
        break;
        case xdb::DB_TYPE_UINT16:
            EXPECT_EQ(todo2.raw().fixed.val_uint32, array_of_parameters[idx].value.fixed.val_uint32);
        break;
        case xdb::DB_TYPE_INT32:
            EXPECT_EQ(todo2.raw().fixed.val_int32, array_of_parameters[idx].value.fixed.val_int32);
        break;
        case xdb::DB_TYPE_UINT32:
            EXPECT_EQ(todo2.raw().fixed.val_uint32, array_of_parameters[idx].value.fixed.val_uint32);
        break;
        case xdb::DB_TYPE_INT64:
            EXPECT_EQ(todo2.raw().fixed.val_int64, array_of_parameters[idx].value.fixed.val_int64);
        break;
        case xdb::DB_TYPE_UINT64:
            EXPECT_EQ(todo2.raw().fixed.val_uint64, array_of_parameters[idx].value.fixed.val_uint64);
        break;
        case xdb::DB_TYPE_FLOAT:
            EXPECT_EQ(todo2.raw().fixed.val_float, array_of_parameters[idx].value.fixed.val_float);
        break;
        case xdb::DB_TYPE_DOUBLE:
            EXPECT_EQ(todo2.raw().fixed.val_double, array_of_parameters[idx].value.fixed.val_double);
        break;
        case xdb::DB_TYPE_BYTES:
//            std::cout << todo2.raw().dynamic.val_string << " <-> " << array_of_parameters[idx].value.dynamic.val_string << std::endl;
            EXPECT_EQ(todo2.raw().dynamic.val_string->compare(*array_of_parameters[idx].value.dynamic.val_string), 0);
        break;
        case xdb::DB_TYPE_BYTES4:
        case xdb::DB_TYPE_BYTES8:
        case xdb::DB_TYPE_BYTES12:
        case xdb::DB_TYPE_BYTES16:
        case xdb::DB_TYPE_BYTES20:
        case xdb::DB_TYPE_BYTES48:
        case xdb::DB_TYPE_BYTES80:
        case xdb::DB_TYPE_BYTES128:
        case xdb::DB_TYPE_BYTES256:
//            std::cout << todo2.raw().dynamic.varchar << " <-> " << array_of_parameters[idx].value.dynamic.varchar << std::endl;
            EXPECT_EQ(strcmp(todo2.raw().dynamic.varchar, array_of_parameters[idx].value.dynamic.varchar), 0);
        break;

        default:
           std::cout << "readMultiMsg test: unsupported type " << todo2.type() << std::endl;
        break;
    }
  }
}

TEST(TestMessage, USE_HISTORY)
{
  std::string probe_tag = "/KD4001/FY01";
  time_t start = 1000000000;
  int samples = 100;
  int htype = RTDBM::PER_1_MINUTE;

  queryHistoryMsg->set(probe_tag, start, samples, htype);

  EXPECT_TRUE(probe_tag.compare(queryHistoryMsg->tag()) == 0);
  EXPECT_TRUE(queryHistoryMsg->num_required_samples() == samples);
  EXPECT_TRUE(queryHistoryMsg->history_type() == htype);
  EXPECT_TRUE(queryHistoryMsg->start_time() == start);
  // Пока не было физической проверки существования в HDB
  EXPECT_TRUE(queryHistoryMsg->existance() == false);
}

TEST(TestMessage, DESTROY_SINF)
{
  delete readMultiMsg;
  delete writeMultiMsg;
  delete queryHistoryMsg;
  // Освободить выделенную память для успокоения valgrind
  release_TestSINF_parameters();
}

TEST(TestMessage, DESTROY_FACTORY)
{
  delete message_factory;
  message_factory = NULL;
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InstallFailureSignalHandler();
  ::google::InitGoogleLogging(argv[0]);
  int retval = RUN_ALL_TESTS();
  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();
  return retval;
}

