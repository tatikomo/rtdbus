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
xdb::Letter       *payload = NULL;
mdp::zmsg         *message = NULL;
char              *my_name = "message_test";

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
  pb_header.set_source_pid(9999);
  pb_header.set_proc_dest("В чащах юга жил-был Цитрус?");
  pb_header.set_proc_origin("Да! Но фальшивый экземпляр.");
  pb_header.set_sys_msg_type(100);
  pb_header.set_usr_msg_type(ADG_D_MSG_EXECRESULT);

  pb_exec_result_request.set_user_exchange_id(9999999);
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
    payload = new xdb::Letter(message);
    ASSERT_TRUE(payload != NULL);

    serialized_header = payload->GetHEADER();
    serialized_request= payload->GetDATA();

    EXPECT_EQ(serialized_header,  pb_serialized_header);
    EXPECT_EQ(serialized_request, pb_serialized_request);
  }
  catch(...)
  {
    LOG(ERROR) << "Unable to create payload";
  }
}

TEST(TestPayload, ACCESS)
{
  RTDBM::ExecResult exec_result;
  msg::Header* header = new msg::Header(payload->GetHEADER());
  ASSERT_TRUE(header);

  exec_result.ParseFromString(payload->GetDATA());

  EXPECT_EQ(pb_header.protocol_version(),            header->get_protocol_version());
  EXPECT_EQ((rtdbExchangeId)pb_header.exchange_id(), header->get_exchange_id());
  EXPECT_EQ((rtdbPid)pb_header.source_pid(),         header->get_source_pid());
  EXPECT_EQ(pb_header.proc_dest(),        header->get_proc_dest());
  EXPECT_EQ(pb_header.proc_origin(),      header->get_proc_origin());
  EXPECT_EQ(pb_header.sys_msg_type(),     header->get_sys_msg_type());
  EXPECT_EQ(pb_header.usr_msg_type(),     header->get_usr_msg_type());

  EXPECT_EQ(pb_exec_result_request.user_exchange_id(), exec_result.user_exchange_id());
  EXPECT_EQ(pb_exec_result_request.exec_result(),      exec_result.exec_result());
  EXPECT_EQ(pb_exec_result_request.failure_cause(),    exec_result.failure_cause());

#if 0
  std::cout << "HEADER --------------------------- " << std::endl;
  std::cout << "Protocol version: " << (int) header->get_protocol_version() << std::endl;
  std::cout << "Exchange ID: "      << header->get_exchange_id() << std::endl;
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
  delete payload;
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

