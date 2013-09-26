#include <limits>
#include <string>

#include "glog/logging.h"

#include "zmsg.hpp"
#include "mdp_letter.hpp"
#include "helper.hpp"

using namespace mdp;

Letter::Letter(zmsg* _instance)
{
  assert(_instance);
  m_body_instance = NULL;
  m_data_needs_reserialization = m_header_needs_reserialization = true;

  // Прочитать служебные поля транспортного сообщения zmsg
  // и восстановить на его основе прикладное сообщение.
  int msg_frames = _instance->parts();
  // два последних фрейма - заголовок и тело сообщения
  assert(msg_frames >= 2);

  m_serialized_header = _instance->get_part(msg_frames-2);
  m_serialized_data = _instance->get_part(msg_frames-1);

  if (true == m_header_instance.ParseFrom(m_serialized_header))
  {
    m_initialized = UnserializeFrom(m_header_instance.get_usr_msg_type(), &m_serialized_data);
  }
  else
  {
    LOG(ERROR) << "Unable to unserialize header";
    hex_dump(m_serialized_header);
  }
}

// На основе типа сообщения и сериализованных данных.
// Если последний параметр равен NULL, создается пустая структура 
// нужного типа с тем, чтобы потом её заполнил пользователь самостоятельно.
Letter::Letter(const rtdbMsgType user_type, const std::string& dest, const std::string* b)
{
  m_source_procname.assign("GEV"); // TODO: подставить сюда название своего процесса
  m_body_instance = NULL;
  m_data_needs_reserialization = m_header_needs_reserialization = true;

  // TODO Создать Header самостоятельно
  m_header_instance.instance().set_protocol_version(1);
  m_header_instance.instance().set_exchange_id(GenerateExchangeId());
  m_header_instance.instance().set_source_pid(getpid());
  m_header_instance.instance().set_proc_dest(dest);
  m_header_instance.instance().set_proc_origin(m_source_procname);
  m_header_instance.instance().set_sys_msg_type(USER_MESSAGE_TYPE);
  m_header_instance.instance().set_usr_msg_type(user_type);

  if (b)
  {
    m_serialized_data.assign(*b);
  }
  // создать m_body_instance нужного типа, заполнив его данными из буфера
  m_initialized = UnserializeFrom(user_type, &m_serialized_data);

  m_header_instance.instance().SerializeToString(&m_serialized_header);
  m_header_needs_reserialization = false;
}

Letter::~Letter()
{
//  LOG(INFO) << "Letter destructor";
  delete m_body_instance;
}

// Продоставление доступа только на чтение. Все модификации будут игнорированы.
::google::protobuf::Message* Letter::data()
{
//  if (!m_initialized)
//    return NULL;

  return m_body_instance;
}

// Предоставление модифицируемой версии данных. После этого, перед любым методом, 
// предоставляющим доступ к сериализованным данным, их сериализация должна быть повторена.
::google::protobuf::Message* Letter::mutable_data()
{
  m_data_needs_reserialization = true;

//  if (false == m_initialized)
//    return NULL;

  return m_body_instance;
}

// Предоставление модифицируемой версии данных. После этого, перед любым методом, 
// предоставляющим доступ к сериализованным данным, их сериализация должна быть повторена.
msg::Header& Letter::mutable_header()
{
  m_header_needs_reserialization = true;

  return m_header_instance;
}

// Установить значения полей "Отправитель" и "Получатель"
void Letter::SetOrigin(const char* _origin)
{
  m_header_needs_reserialization = true;
  m_header_instance.instance().set_proc_origin(_origin);
}

void Letter::SetDestination(const char* _destination)
{
  m_header_needs_reserialization = true;
  m_header_instance.instance().set_proc_dest(_destination);

}

rtdbExchangeId Letter::GenerateExchangeId()
{
  rtdbExchangeId current_id = m_header_instance.instance().exchange_id();

  if (current_id > std::numeric_limits<unsigned int>::max())
    m_header_instance.instance().set_exchange_id(0);

  m_header_instance.instance().set_exchange_id(++current_id);

  return current_id;
}

// Вернуть сериализованный заголовок
const std::string& Letter::SerializedHeader()
{
  if ((true == m_initialized) && m_header_needs_reserialization)
  {
    m_header_instance.instance().SerializeToString(&m_serialized_header);
    m_header_needs_reserialization = false;
  }
  return m_serialized_header;
}

// При необходимости провести сериализацию данных в буфер
const std::string& Letter::SerializedData()
{
  if ((true == m_initialized) && m_data_needs_reserialization)
  {
    m_body_instance->SerializeToString(&m_serialized_data);
    m_data_needs_reserialization = false;
  }
  return m_serialized_data;
}


/*
 * Создать экземпляр данных соответствующего типа, и десериализовать его на основе входной строки.
 * Если входная строка пуста (это возможно, когда сообщение создается Клиентом), то:
 * 1. Присвоить m_body_instance значения "по умолчанию", т.к. иначе protobuf 
 * не сериализует этот экземпляр данных.
 * 2. Заполнить тестовыми сериализованными данными строковый буфер m_serialized_data
 */
bool Letter::UnserializeFrom(const int user_type, const std::string* b)
{
  bool status = false;

  switch (user_type)
  {
    case ADG_D_MSG_EXECRESULT:
      m_body_instance = new RTDBM::ExecResult;
      // Присвоить значения по умолчанию, поскольку иначе protobuf не сериализует эти поля
      if (!b || !b->size())
      {
        static_cast<RTDBM::ExecResult*>(m_body_instance)->set_user_exchange_id(0);
        static_cast<RTDBM::ExecResult*>(m_body_instance)->set_exec_result(0);
        static_cast<RTDBM::ExecResult*>(m_body_instance)->set_failure_cause(0);
        // Заполнить строку сериализованными значениями по умолчанию
        static_cast<RTDBM::ExecResult*>(m_body_instance)->SerializeToString(&m_serialized_data);
        status = true;
      }
      break;

    case ADG_D_MSG_ASKLIFE:
      m_body_instance = new RTDBM::AskLife;
      if (!b || !b->size())
      {
        static_cast<RTDBM::AskLife*>(m_body_instance)->set_user_exchange_id(0);
        // Заполнить строку сериализованными значениями по умолчанию
        static_cast<RTDBM::AskLife*>(m_body_instance)->SerializeToString(&m_serialized_data);
        status = true;
      }
      break;

    default:
      LOG(ERROR) << "Unknown message type " << user_type;
      delete m_body_instance;
      m_body_instance = NULL;
      break;
  }

  if (m_body_instance && (b || b->size()))
  {
    if (true == (status = m_body_instance->ParseFromString(*b)))
    {
      m_data_needs_reserialization = false;
    }
    else LOG(ERROR) << "Unserialization error";
  }
    
  return status;
}

