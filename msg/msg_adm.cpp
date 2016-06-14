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
 
bool AskLife::exec_result(int &val)
{
  bool exec_result_exist = static_cast<RTDBM::SimpleRequest*>(data()->impl()->instance())->has_exec_result();

  if (exec_result_exist)
  {
    val = static_cast<RTDBM::SimpleRequest*>(data()->impl()->instance())->exec_result();
  }
  else val = RTDBM::GOF_D_UNDETERMINED;

  return exec_result_exist;
}
 
void AskLife::set_exec_result(int _new_val)
{
  RTDBM::gof_t_ExecResult ex_result;
  if (!RTDBM::gof_t_ExecResult_IsValid(_new_val))
  {
    ex_result = RTDBM::GOF_D_UNDETERMINED;
    //LOG(ERROR) << "Unable to parse gof_t_ExecResult from (" << _new_val << ")";
  }
  else ex_result = static_cast<RTDBM::gof_t_ExecResult>(_new_val);

  static_cast<RTDBM::SimpleRequest*>(data()->impl()->mutable_instance())->set_exec_result(ex_result);
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
  return static_cast<RTDBM::ExecResult*>(data()->impl()->instance())->exec_result();
}

void ExecResult::set_exec_result(int _new_val)
{
  RTDBM::gof_t_ExecResult ex_result;
  if (!RTDBM::gof_t_ExecResult_IsValid(_new_val))
  {
    ex_result = RTDBM::GOF_D_UNDETERMINED;
    //LOG(ERROR) << "Unable to parse gof_t_ExecResult from (" << _new_val << ")";
  }
  else ex_result = static_cast<RTDBM::gof_t_ExecResult>(_new_val);

  static_cast<RTDBM::ExecResult*>(data()->impl()->mutable_instance())->set_exec_result(ex_result);
}

bool ExecResult::failure_cause(int& code, std::string& text)
{
  bool failure_cause_exist = static_cast<RTDBM::ExecResult*>(data()->impl()->instance())->has_failure_cause();

  if (failure_cause_exist)
  {
    code = static_cast<RTDBM::ExecResult*>(data()->impl()->instance())->failure_cause().error_code();
    text.assign(static_cast<RTDBM::ExecResult*>(data()->impl()->instance())->failure_cause().error_text());
  }
  else
  {
    code = 0;
    text.clear();
  }
  return failure_cause_exist;
}

void ExecResult::set_failure_cause(int _new_val, std::string& _new_cause)
{
  static_cast<RTDBM::ExecResult*>(data()->impl()->mutable_instance())->mutable_failure_cause()->set_error_code(_new_val);
  static_cast<RTDBM::ExecResult*>(data()->impl()->mutable_instance())->mutable_failure_cause()->set_error_text(_new_cause);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SimpleReply::SimpleReply(int type) : Letter(type, 0) {}

SimpleReply::SimpleReply(int type, rtdbExchangeId id) : Letter(type, id) {}

SimpleReply::SimpleReply(Header* head, const std::string& body) : Letter(head, body) {}

SimpleReply::SimpleReply(const std::string& head, const std::string& body) : Letter(head, body) {}

SimpleReply::~SimpleReply() {}

////////////////////////////////////////////////////////////////////////////////////////////////////

SimpleRequest::SimpleRequest(int type) : Letter(type, 0) {}

SimpleRequest::SimpleRequest(int type, rtdbExchangeId id) : Letter(type, id) {}

SimpleRequest::SimpleRequest(Header* head, const std::string& body) : Letter(head, body) {}

SimpleRequest::SimpleRequest(const std::string& head, const std::string& body) : Letter(head, body) {}

SimpleRequest::~SimpleRequest() {}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Запуск и останов приложения
StartStop::StartStop() : Letter(0, 0) {}
StartStop::StartStop(rtdbExchangeId id) : Letter(0, id) {}
StartStop::StartStop(Header* head, const std::string& body) : Letter(head, body) {}
StartStop::StartStop(const std::string& head, const std::string& body) : Letter(head, body) {}
StartStop::~StartStop() {}

////////////////////////////////////////////////////////////////////////////////////////////////////
