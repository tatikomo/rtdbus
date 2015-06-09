#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "google/protobuf/stubs/common.h"
#include "proto/common.pb.h"
#include "msg_message.hpp"
#include "msg_message_impl.hpp"
#include "msg_adm.hpp"

using namespace msg;

////////////////////////////////////////////////////////////////////////////////////////////////////
AskLife::AskLife() : Letter(ADG_D_MSG_ASKLIFE, 0) {}

AskLife::AskLife(rtdbExchangeId _id) : Letter(ADG_D_MSG_ASKLIFE, _id) {}

AskLife::AskLife(Header* head, const std::string& body) : Letter(head, body)
{
  assert(header()->usr_msg_type() == ADG_D_MSG_ASKLIFE);
}
 
AskLife::AskLife(const std::string& head, const std::string& body) : Letter(head, body)
{
  assert(header()->usr_msg_type() == ADG_D_MSG_ASKLIFE);
}
 
AskLife::~AskLife() {}
 
int AskLife::status()
{
  return dynamic_cast<RTDBM::AskLife*>(data()->impl()->instance())->status();
}
 
void AskLife::set_status(int _new_val)
{
  dynamic_cast<RTDBM::AskLife*>(data()->impl()->mutable_instance())->set_status(_new_val);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ExecResult::ExecResult() : Letter(ADG_D_MSG_EXECRESULT, 0) {}
ExecResult::ExecResult(rtdbExchangeId _id) : Letter(ADG_D_MSG_EXECRESULT, _id) {}

ExecResult::ExecResult(Header* head, const std::string& body) : Letter(head, body) 
{
  assert(header()->usr_msg_type() == ADG_D_MSG_EXECRESULT);
}

ExecResult::ExecResult(const std::string& head, const std::string& body) : Letter(head, body)
{
  assert(header()->usr_msg_type() == ADG_D_MSG_EXECRESULT);
}
 
ExecResult::~ExecResult() {}
 
int ExecResult::exec_result()
{
  return dynamic_cast<RTDBM::ExecResult*>(data()->impl()->instance())->exec_result();
}

void ExecResult::set_exec_result(int _new_val)
{
  dynamic_cast<RTDBM::ExecResult*>(data()->impl()->mutable_instance())->set_exec_result(_new_val);
}

int ExecResult::failure_cause()
{
  return dynamic_cast<RTDBM::ExecResult*>(data()->impl()->instance())->failure_cause();
}

void ExecResult::set_failure_cause(int _new_val)
{
  dynamic_cast<RTDBM::ExecResult*>(data()->impl()->mutable_instance())->set_failure_cause(_new_val);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

