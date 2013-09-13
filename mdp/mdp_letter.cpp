#include <limits>
#include <string>

#include "glog/logging.h"

#include "zmsg.hpp"
#include "mdp_letter.hpp"

using namespace mdp;

// TODO: возможно следует создавать копию вместо хранения ссылки
Letter::Letter(zmsg* _instance)
{
  assert(_instance);
  // Прочитать служебные поля транспортного сообщения zmsg
  // и восстановить на его основе прикладное сообщение.
  int msg_frames = _instance->parts();
  // два последних фрейма - заголовок и тело сообщения
  assert(msg_frames >= 2);

  m_header.ParseFrom(_instance->get_part(msg_frames-2));
  m_data = _instance->get_part(msg_frames-1);

  m_source_procname.assign("GEV");  // TODO: подставить сюда название своего процесса
  m_initialized = UnserializeFrom(m_header.get_usr_msg_type(), m_data);
}

// На основе типа сообщения и сериализованных данных
Letter::Letter(const rtdbMsgType user_type, const std::string& dest, const std::string& b)
{
  m_source_procname.assign("GEV"); // TODO: подставить сюда название своего процесса
  // TODO Создать Header самостоятельно
  m_header.instance().set_protocol_version(1);
  m_header.instance().set_exchange_id(GenerateExchangeId());
  m_header.instance().set_source_pid(getpid());
  m_header.instance().set_proc_dest(dest);
  m_header.instance().set_proc_origin(m_source_procname);
  m_header.instance().set_sys_msg_type(USER_MESSAGE_TYPE);
  m_header.instance().set_usr_msg_type(user_type);

  m_data.assign(b);

  m_initialized = UnserializeFrom(user_type, m_data);
}

Letter::~Letter()
{
  LOG(INFO) << "Letter destructor";
}

rtdbExchangeId Letter::GenerateExchangeId()
{
  if (m_exchange_id > std::numeric_limits<int>::max())
    m_exchange_id = 0;

  // NB: переменная не инициализирована специально, для получения случайного
  // начального значения. Далее нужно реализовать правильный механизм генерации.
  return ++m_exchange_id;
}

bool Letter::UnserializeFrom(const int user_type, const std::string& b)
{
  bool status = false;

  switch (user_type)
  {
    case ADG_D_MSG_EXECRESULT:
      m_body_instance = new RTDBM::ExecResult;
      status = m_body_instance->ParseFromString(b);
      break;

    default:
      LOG(ERROR) << "Unknown message type " << user_type;
  }
  return status;
}

