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
     virtual ~AskLife();

     // Получить значения Состояния из сообщения.
     // Вернет true, если поле "Состояние" в сообщении есть.
     bool exec_result(int&);
     void set_exec_result(int);

   private:
 };
 
class ExecResult : public Letter
 {
   public:
     ExecResult();
     ExecResult(rtdbExchangeId);
     ExecResult(Header*, const std::string& body);
     ExecResult(const std::string& head, const std::string& body);
     virtual ~ExecResult();

     int  exec_result();
     void set_exec_result(int);

     // Получить значения структуры из сообщения.
     // Вернет true, если структура в сообщении есть, и заполнит параметры значениями.
     bool failure_cause(int&, std::string&);
     void set_failure_cause(int, std::string&);

   private:
 };

} // namespace msg

#endif

