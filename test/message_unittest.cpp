#include <string>
#include <google/protobuf/stubs/common.h>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "helper.hpp"
#include "zmsg.hpp"
#include "mdp_common.h"
#include "msg/msg_letter.hpp"
#include "proto/common.pb.h"

char *service_name_1 = "service_name_1";
char *worker_identity_1 = "@W0000001";

TEST(TestLetter, CREATE)
{
  zmsg *msg = new zmsg();
/*
Эмуляция запроса Клиента Службе NYSE - "купить 2000 за 800"
[000] 
[006] MDPC0X
[004] NYSE
[003] BUY
[004] 2000
[003] 800
*/
  msg->push_front("800");
  msg->push_front("2000");
  msg->push_front("BUY");
  msg->push_front(service_name_1);
  msg->push_front((char*)MDPC_CLIENT);
  msg->push_front(const_cast<char*>(EMPTY_FRAME));

  Letter *letter = new Letter(msg);

  ASSERT_TRUE(letter != NULL);
  std::cout << "Sender: " << letter->sender() << std::endl;
  std::cout << "Receiver: " << letter->receiver() << std::endl;
  std::cout << "Service: " << letter->service_name() << std::endl;

  delete letter;
}

TEST(TestProtobuf, VERSION)
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;
}

TEST(TestProtobuf, CREATE)
{
  CommonRequest pb_request;
  CommonRequest pb_responce_1;
  int         field_1_int32_A,  field_1_int32_B;
  std::string field_2_string_A, field_2_string_B;
  int         field_3_int32_A,  field_3_int32_B;
  CommonRequest pb_responce_2;

  pb_request.set_field_1_int32(0);
  pb_request.set_field_2_string("fuu");
  pb_request.set_field_3_int32(1);

  // serialize the request to a string
  std::string pb_serialized;
  pb_request.SerializeToString(&pb_serialized);

  // Сравнить по-разному восстановленные структуры
  // A. Восстановление штатными средствами protobuf
  // B. Восстановление средствами protobuf из экземпляра zmsg
  zmsg *msg = new zmsg();
  msg->push_front(const_cast<char*>(pb_serialized.c_str()));
  Letter *letter = new Letter(msg);

  // (A)
  pb_responce_1.ParseFromString(pb_serialized);
  field_1_int32_A  = pb_responce_1.field_1_int32();
  field_2_string_A = pb_responce_1.field_2_string();
  field_3_int32_A  = pb_responce_1.field_3_int32();
/*
  std::cout << req_nb_1 << std::endl;
  std::cout << req_id_1 << std::endl;
  std::cout << req_ident_1 << std::endl;
*/
  ustring fuu = msg->pop_front();
  // (B)
  pb_responce_2.ParseFromString(fuu);

  field_1_int32_B  = pb_responce_2.field_1_int32();
  field_2_string_B = pb_responce_2.field_2_string();
  field_3_int32_B  = pb_responce_2.field_3_int32();
/*
  std::cout << req_nb_2 << std::endl;
  std::cout << req_id_2 << std::endl;
  std::cout << req_ident_2 << std::endl;
*/
  EXPECT_EQ(field_1_int32_A,  field_1_int32_B);
  EXPECT_EQ(field_2_string_A, field_2_string_B);
  EXPECT_EQ(field_3_int32_A,  field_3_int32_B);
}

int main(int argc, char** argv)
{
  google::InitGoogleLogging(argv[0]);
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

