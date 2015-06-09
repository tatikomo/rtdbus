#pragma once
#ifndef MSG_ADM_HPP
#define MSG_ADM_HPP

#include <string>

#include "config.h"
#include "msg_common.h"
#include "msg_message.hpp"

namespace msg {

////////////////////////////////////////////////////////////////////////////////////////////////////
// Сообщения, участвующие в общем взаимодействии процессов [ADG_D_MSG_*]
// TODO: внедрить автогенерацию файлов по описателям в msg/proto/common.proto
// Список:
//
////////////////////////////////////////////////////////////////////////////////////////////////////
class AskLife : public Letter
{
   public:
     // Фактические значения не получены, значит нужно присвоить
     // значения по умолчанию, иначе protobuf не сериализует пустые поля
     AskLife();
     AskLife(rtdbExchangeId);
     AskLife(Header*, const std::string& body);
     AskLife(const std::string& head, const std::string& body);
    ~AskLife();

     int  status();
     void set_status(int);

   private:
 };
 
class ExecResult : public Letter
 {
   public:
     ExecResult();
     ExecResult(rtdbExchangeId);
     ExecResult(Header*, const std::string& body);
     ExecResult(const std::string& head, const std::string& body);
    ~ExecResult();

     int  exec_result();
     void set_exec_result(int);

     int  failure_cause();
     void set_failure_cause(int);

   private:
 };

} // namespace msg

#endif

