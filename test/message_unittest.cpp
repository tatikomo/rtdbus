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
msg::Message        *messa = NULL;
const msg::Header   *head = NULL;
const msg::Payload  *body = NULL;
// Сообщения, создаваемые фабрикой сообщений в зависимости от указанного типа
msg::AskLife        *ask_life = NULL;

char              *my_name = (char*)"message_test";

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

  pb_exec_result_request.set_exec_result(23145);
  pb_exec_result_request.set_failure_cause(5);

  pb_header.SerializeToString(&pb_serialized_header);
  pb_exec_result_request.SerializeToString(&pb_serialized_request);

  message = new mdp::zmsg();
  ASSERT_TRUE(message);

  message->push_front(pb_serialized_request);
  message->push_front(pb_serialized_header);
  message->push_front(const_cast<char*>(EMPTY_FRAME));
  message->push_front(my_name);
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

  EXPECT_EQ(pb_header.protocol_version(),            header->get_protocol_version());
  EXPECT_EQ((rtdbExchangeId)pb_header.exchange_id(), header->get_exchange_id());
  EXPECT_EQ((rtdbExchangeId)pb_header.interest_id(), header->get_interest_id());
  EXPECT_EQ((rtdbPid)pb_header.source_pid(),         header->get_source_pid());
  EXPECT_EQ(pb_header.proc_dest(),        header->get_proc_dest());
  EXPECT_EQ(pb_header.proc_origin(),      header->get_proc_origin());
  EXPECT_EQ(pb_header.sys_msg_type(),     header->get_sys_msg_type());
  EXPECT_EQ(pb_header.usr_msg_type(),     header->get_usr_msg_type());

  EXPECT_EQ(pb_exec_result_request.exec_result(),      exec_result.exec_result());
  EXPECT_EQ(pb_exec_result_request.failure_cause(),    exec_result.failure_cause());

#if 0
  std::cout << "HEADER --------------------------- " << std::endl;
  std::cout << "Protocol version: " << (int) header->get_protocol_version() << std::endl;
  std::cout << "Exchange ID: "      << header->get_exchange_id() << std::endl;
  std::cout << "Interest ID: "      << header->get_interest_id() << std::endl;
  std::cout << "Source pid:"        << header->get_source_pid() << std::endl;
  std::cout << "Destination: "      << header->get_proc_dest() << std::endl;
  std::cout << "Originator: "       << header->get_proc_origin() << std::endl;
  std::cout << "Msg system type: "  << header->get_sys_msg_type() << std::endl;
  std::cout << "Usr system type: "  << header->get_usr_msg_type() << std::endl;
  
  std::cout << "MESSAGE --------------------------- " << std::endl;
  std::cout << "User exchange ID: "   << exec_result.user_exchange_id() << std::endl;
  std::cout << "Execution result: "   << exec_result.exec_result() << std::endl;
  std::cout << "User failure cause: " << exec_result.failure_cause() << std::endl;
#endif
  delete header;
}

TEST(TespPayload, DELETE)
{
  delete message;
  delete letter;
}

TEST(TestMessageFactory, CREATE)
{
  message_factory = new msg::MessageFactory();
  ASSERT_TRUE(message_factory);

  ask_life = static_cast<msg::AskLife*>(message_factory->create(ADG_D_MSG_ASKLIFE));
  ASSERT_TRUE(ask_life);

  ask_life->dump();
}

TEST(TestMessageFactory, DESTROY)
{
  delete message_factory;
}

TEST(TestMessage, CREATE)
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

  pb_request_asklife.set_status(654321);

  pb_header_asklife.SerializeToString(&pb_serialized_header_asklife);
  pb_request_asklife.SerializeToString(&pb_serialized_request_asklife);
  // Завершение сериализации запроса AskLife

  msg::AskLife* messa = new msg::AskLife(pb_serialized_header_asklife,
                           pb_serialized_request_asklife);
  ASSERT_TRUE(messa);

  head = messa->get_head();
  ASSERT_TRUE(head);

  body = messa->get_body();
  ASSERT_TRUE(body);

  EXPECT_EQ(messa->type(), ADG_D_MSG_ASKLIFE);
  EXPECT_EQ(messa->status(), 654321);

  messa->dump();
}

TEST(TestMessage, DESTROY)
{
  delete messa;
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

