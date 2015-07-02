#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "glog/logging.h"

// Генерированные прототипы
#include "proto/rtdbus.pb.h"

#include "msg_message_impl.hpp"
#include "helper.hpp"

using namespace msg;

////////////////////////////////////////////////////////////////////////////////////////////////
HeaderImpl::HeaderImpl()
  : m_validity(false),
    m_modified(true)
{
//  LOG(INFO) << "HeaderImpl() [" << this << "]";
  set_initial_values();
}

HeaderImpl::HeaderImpl(const std::string& frame)
  : m_modified(true)
{
  m_pb_serialized.resize(32);
  if (false == (m_validity = m_instance.ParseFromString(frame)))
  {
    LOG(ERROR) << "Unable to unserialize header";
  }

  LOG(INFO) << "HeaderImpl(" << frame << ")=" << m_validity <<" [" << this << "]";
}

RTDBM::Header& HeaderImpl::instance()
{
//  LOG(INFO) << "HeaderImpl::instance modified?: " << m_modified <<" [" << this << "]";
  return m_instance;
}

RTDBM::Header& HeaderImpl::mutable_instance()
{
  // Даже если изменений не будет, все равно при запросе данных сериализуем заново
  m_modified = true;
  return m_instance;
}

HeaderImpl::~HeaderImpl()
{
//  LOG(INFO) << "~HeaderImpl() [" << this << "]";
}

bool HeaderImpl::ParseFrom(const std::string& frame)
{
  return (true == (m_validity = m_instance.ParseFromString(frame)));
}

const std::string& HeaderImpl::get_serialized()
{
//  LOG(INFO) << "HeaderImpl::get_serialized valid?:" << valid()
//            << " modified?: " << modified();
  if (valid() && modified())
  {
    // TODO: _ДО_ этого должны быть явно инициализированы все элементы объекта,
    // иначе protobuf их не сериализует.
    if (true == (m_validity = m_instance.SerializeToString(&m_pb_serialized)))
    {
      modified(false);
    }
    else
    {
      LOG(ERROR) << "Unable to serialize header";
      m_pb_serialized.clear();
    }
  }

  return m_pb_serialized;
}

void HeaderImpl::set_initial_values()
{
//  LOG(INFO) << "HeaderImpl::set_initial_values [" << this << "]";
  time_t now = time(0);
  m_instance.set_protocol_version(1);
  m_instance.set_exchange_id(0);
  m_instance.set_interest_id(0);
  m_instance.set_source_pid(0);
  m_instance.set_sys_msg_type(USER_MESSAGE_TYPE);
  m_instance.set_usr_msg_type(0);
  m_instance.set_time_mark(now);

  modified(true);
  m_validity = true;
}



////////////////////////////////////////////////////////////////////////////////////////////////
DataImpl::DataImpl(rtdbMsgType type)
  : m_protobuf_instance(NULL),
    m_type(type),
    m_validity(false),
    m_modified(true)
{
  m_data.resize(130);
  m_data.clear();
  m_pb_serialized.resize(130);
//  LOG(INFO) << "DataImpl(" << type << ") [" << this << "]";
  // создать объект с данными, соответствующими переданному типу
  syncronize();
}

DataImpl::DataImpl(rtdbMsgType type, const std::string& frame)
  : m_protobuf_instance(NULL),
    m_type(type),
    m_data(frame),
    m_validity(false),
    m_modified(true)
{
//  LOG(INFO) << "DataImpl(" << type << ", " << frame << ") [" << this << "]";
  // создать объект с данными, соответствующими переданному типу
  syncronize();
}

DataImpl::~DataImpl()
{
//  LOG(INFO) << "~DataImpl() [" << this << "]";
  delete m_protobuf_instance;
}

::google::protobuf::Message* DataImpl::instance()
{
//  LOG(INFO) << "DataImpl::instance() modified?:" << modified() <<" [" << this << "]";
  return m_protobuf_instance;
}

::google::protobuf::Message* DataImpl::mutable_instance()
{
  modified(true);
  return m_protobuf_instance;
}

void DataImpl::set_initial_values()
{
//  LOG(INFO) << "DataImpl::set_initial_values() type:" << m_type
//            << " m_protobuf_instance:" << m_protobuf_instance
//            << " valid?:" << valid() << " [" << this << "]";

  RTDBM::gof_t_ExecResult ex_result = RTDBM::GOF_D_SUCCESS;

  // Объект с данными не должен быть ранее созданным
  assert(!m_protobuf_instance);

  // Изменения могут закончиться неудачно
  valid(true);

  if (!m_protobuf_instance)
  {
    // Присвоить значения по умолчанию, поскольку иначе protobuf не сериализует эти поля
    switch(m_type)
    {
      case ADG_D_MSG_EXECRESULT:
        m_protobuf_instance = new RTDBM::ExecResult();
        static_cast<RTDBM::ExecResult*>(m_protobuf_instance)->set_exec_result(ex_result);
        //static_cast<RTDBM::ExecResult*>(m_protobuf_instance)->set_failure_cause(0);
      break;

      case ADG_D_MSG_ASKLIFE:
        m_protobuf_instance = new RTDBM::SimpleRequest();
      break;

      case SIG_D_MSG_READ_MULTI:
        m_protobuf_instance = new RTDBM::ReadMulti();
      break;

      case SIG_D_MSG_WRITE_MULTI:
        m_protobuf_instance = new RTDBM::WriteMulti();
      break;

      default:
        LOG(ERROR) << "Unsupported message type: " << m_type;
      break;
    }
    // Значения изменились
    modified(true);
  }
  else
  {
    LOG(ERROR) << "Try to re-init object type:" << m_type;
  }
}

bool DataImpl::ParseFrom(const std::string& frame)
{
  return (true == (m_validity = m_protobuf_instance->ParseFromString(frame)));
}

// Вернуть созданный объект PROTOBUF, содержащий данные после десериализации
// Вернуть NULL, если в процессе десереализации возникла ошибка
::google::protobuf::Message* DataImpl::syncronize()
{
//  LOG(INFO) << "DataImpl::syncronize() m_protobuf_instance=" << m_protobuf_instance
//            << " data.size=" << m_data.size() << " [" << this << "]";

  if (!m_protobuf_instance)
  {
    // Создать m_protobuf_instance и заполнить начальными значениями
    set_initial_values();

    // Проверить тип объекта - известен?
    if (m_protobuf_instance && m_data.size()) // и есть ли данные для десериализации?
    {
      // Есть с чего иницииализировать данные
      if (true == (m_validity = m_protobuf_instance->ParseFromString(m_data)))
      {
        LOG(INFO) << "DataImpl::syncronize() ParseFromString("<< m_data << ") OK [" << this << "]";
//        modified(false);
      }
      else
      {
        LOG(ERROR) << "Unserialization error for message type: " << m_type;
        // Сделаем вид, что ничего не трогали.
        delete m_protobuf_instance;
        m_protobuf_instance = NULL;
      }
    }
//    else
//    {
//        LOG(INFO) << "DataImpl::syncronize() m_protobuf_instance: " << m_protobuf_instance
//                  << " valid?:" << valid() << " [" << this << "]";
//    }
  }

  return m_protobuf_instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Вернуть сериализованный заголовок
const std::string& DataImpl::get_serialized()
{
//  LOG(INFO) << "DataImpl::get_serialized() valid?:" << valid()
//            << " modified?: " << modified();
  if (valid() && modified())
  {
    // TODO: _ДО_ этого должны быть явно инициализированы все элементы объекта,
    // иначе protobuf их не сериализует.
    if (true == (m_validity = m_protobuf_instance->SerializeToString(&m_pb_serialized)))
    {
      modified(false);
    }
  }

  return m_pb_serialized;
}


