#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <google/protobuf/stubs/common.h>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "helper.hpp"
#include "mdp_zmsg.hpp"
#include "mdp_common.h"
#include "msg_message.hpp"
#include "msg_adm.hpp"
#include "proto/common.pb.h"
#include "xdb_broker_letter.hpp"

std::string        service_name_1 = "service_name_1";
std::string        pb_serialized_header;
std::string        pb_serialized_request;
RTDBM::Header      pb_header;
RTDBM::ExecResult  pb_exec_result_request;
xdb::Letter       *letter = NULL;
mdp::zmsg         *message = NULL;

msg::MessageFactory *message_factory = NULL;
// Сообщения, создаваемые фабрикой сообщений в зависимости от указанного типа
msg::AskLife        *ask_life = NULL;
const msg::Header   *head = NULL;
const msg::Data     *data = NULL;
static const char   *my_name = (const char*)"message_test";

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
  pb_header.set_protocol_version(1);
  pb_header.set_exchange_id(9999999);
  pb_header.set_interest_id(1234);
  pb_header.set_source_pid(9999);
  pb_header.set_proc_dest("В чащах юга жил-был Цитрус?");
  pb_header.set_proc_origin("Да! Но фальшивый экземпляр.");
  pb_header.set_sys_msg_type(100);
  pb_header.set_usr_msg_type(ADG_D_MSG_EXECRESULT);
  pb_header.set_time_mark(time(0));

  pb_exec_result_request.set_exec_result(23145);
  pb_exec_result_request.set_failure_cause(5);

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
  EXPECT_EQ(pb_exec_result_request.failure_cause(),    exec_result.failure_cause());

  delete header;
}

TEST(TespPayload, DELETE)
{
  delete message;
  delete letter;
}

TEST(TestMessage, CREATE_FACTORY)
{
  message_factory = new msg::MessageFactory(my_name);
  ASSERT_TRUE(message_factory);
}

TEST(TestMessage, CREATE_LETTER)
{
  // Начало сериализации данных тестового запроса AskLife
  RTDBM::Header   pb_header_asklife;
  RTDBM::AskLife  pb_request_asklife;
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

  pb_request_asklife.SerializeToString(&pb_serialized_request_asklife);
  // д.б. пустая строка, поскольку protobuf не сериализует неинициализированные поля
  EXPECT_EQ(pb_serialized_request_asklife.size(), 0);

  for (int idx = 1; idx<5; idx++)
  {
    pb_request_asklife.set_status(idx);
    EXPECT_EQ(pb_request_asklife.status(), idx);

    pb_request_asklife.SerializeToString(&pb_serialized_request_asklife);

    // Содержит нормальные значения, т.к. была проведена инициализация
    EXPECT_EQ((pb_serialized_request_asklife.size() > 0), true);
  }

  pb_request_asklife.set_status(54321);
  EXPECT_EQ(pb_request_asklife.status(), 54321);

  pb_request_asklife.SerializeToString(&pb_serialized_request_asklife);

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
  EXPECT_EQ(ask_life->status(), 54321);
//  hex_dump(ask_life->header()->get_serialized());
//  hex_dump(ask_life->data()->get_serialized());

  delete ask_life;

  ask_life = static_cast<msg::AskLife*>(message_factory->create(ADG_D_MSG_ASKLIFE));
  ASSERT_TRUE(ask_life);
  EXPECT_EQ(ask_life->header()->usr_msg_type(), ADG_D_MSG_ASKLIFE);
  EXPECT_EQ(ask_life->header()->sys_msg_type(), USER_MESSAGE_TYPE);
  EXPECT_EQ(ask_life->header()->exchange_id(), 1); // т.к. это первый вызов create()
  EXPECT_EQ(ask_life->header()->interest_id(), 0); // 0 по-умолчанию

  ask_life->set_status(0xDEAD);
  EXPECT_EQ(ask_life->status(), 0xDEAD);
//  hex_dump(ask_life->data()->get_serialized());

  ask_life->dump();

  delete ask_life;

  ask_life = static_cast<msg::AskLife*>(message_factory->create(ADG_D_MSG_ASKLIFE));
  ASSERT_TRUE(ask_life);
  EXPECT_EQ(ask_life->header()->usr_msg_type(), ADG_D_MSG_ASKLIFE);
  EXPECT_EQ(ask_life->header()->sys_msg_type(), USER_MESSAGE_TYPE);
  EXPECT_EQ(ask_life->header()->exchange_id(), 2); // т.к. уже второй вызов create()
  EXPECT_EQ(ask_life->header()->interest_id(), 0); // 0 по-умолчанию

//  ask_life->set_status(0xBEAF);
//  EXPECT_EQ(ask_life->status(), 0xBEAF);
//  hex_dump(ask_life->data()->get_serialized());

//  ask_life->dump();
}

TEST(TestMessage, DESTROY_LETTER)
{
  delete ask_life;
}

TEST(TestMessage, DESTROY_FACTORY)
{
  delete message_factory;
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

