#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits>
#include <string>

#include "glog/logging.h"

#include "mdp_zmsg.hpp"
#include "msg_message.hpp"
#include "msg_message_impl.hpp"
#include "msg_adm.hpp"

using namespace msg;

// TODO: Создать набор классов, создающих готовое к передаче сообщение типа zmsg
//
// А) Класс "Фабрика сообщений"
//  При создании указываются:
//  1. Версия системы сообщений
//  2. Версия rtdbus(?)
//  3. PID процесса-отправителя
//
// Б) Метод "Создание сообщения по заданным условиям"
//  На вход подаются:
//  1. Код сообщения (ASKLIFE и т.п.)
//  2. Идентификатор последовательности (монотонно возрастающее, уникальное в пределах сессии)
//  3. Идентификатор ответа (уникальный номер в пределах последовательности)
//  Метод самостоятельно заполняет служебные поля (А.1, А.2)
//  На выходе - уникальный класс, соответствующий типу Б.1
//
// В) Метод "Отправка сообщения на заданный сокет"
//

////////////////////////////////////////////////////////////////////////////////////////////////////
Header::Header() : m_impl(new HeaderImpl())
{
//  LOG(INFO) << "Header::Header() [" << this << "]";
}

Header::Header(const std::string& frame) : m_impl (new HeaderImpl(frame))
{
//  LOG(INFO) << "Header::Header(" << frame << ") [" << this << "]";
}

Header::~Header()
{
//  LOG(INFO) << "Header::~Header() [" << this << "]";
  delete m_impl;
}

HeaderImpl *Header::impl() const
{
  assert(m_impl);
  return m_impl;
}

bool Header::valid()
{
  return impl()->valid();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Вернуть сериализованный заголовок
const std::string& Header::get_serialized()
{
  return impl()->get_serialized();
}

bool Header::ParseFrom(const std::string& frame)
{
  return impl()->ParseFrom(frame);
}

uint32_t Header::protocol_version() const
{
  return impl()->instance().protocol_version();
}

rtdbExchangeId Header::exchange_id() const
{
  return impl()->instance().exchange_id();
}

rtdbExchangeId Header::interest_id() const
{
  return impl()->instance().interest_id();
}

rtdbPid Header::source_pid() const
{
  return impl()->instance().source_pid();
}

const std::string& Header::proc_dest() const
{
  return impl()->instance().proc_dest();
}

const std::string& Header::proc_origin() const
{
  return impl()->instance().proc_origin();
}

rtdbMsgType Header::sys_msg_type() const
{
  return static_cast<rtdbMsgType>(m_impl->instance().sys_msg_type());
}

rtdbMsgType Header::usr_msg_type() const
{
  return static_cast<rtdbMsgType>(m_impl->instance().usr_msg_type());
}

uint64_t Header::time_mark() const
{
  return impl()->instance().time_mark();
}

void Header::set_protocol_version(uint32_t val)
{
  impl()->mutable_instance().set_protocol_version(val);
}

void Header::set_exchange_id(rtdbExchangeId val)
{
  impl()->mutable_instance().set_exchange_id(val);
}

void Header::set_interest_id(rtdbExchangeId val)
{
  impl()->mutable_instance().set_interest_id(val);
}

void Header::set_proc_dest(const std::string& val)
{
  impl()->mutable_instance().set_proc_dest(val);
}

void Header::set_proc_origin(const std::string& val)
{
  impl()->mutable_instance().set_proc_origin(val);
}

void Header::set_sys_msg_type(rtdbMsgType val)
{
  impl()->mutable_instance().set_sys_msg_type(val);
}

void Header::set_usr_msg_type(rtdbMsgType val)
{
  impl()->mutable_instance().set_usr_msg_type(val);
}

void Header::set_time_mark(uint64_t val)
{
  return impl()->instance().set_time_mark(val);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Data::Data(rtdbMsgType type) : m_impl(new DataImpl(type))
{
//  LOG(INFO) << "Data::Data(" << type << ") [" << this << "]";
}

Data::Data(rtdbMsgType type, const std::string& frame) : m_impl(new DataImpl(type, frame))
{
//  LOG(INFO) << "Data::Data(" << type << ", " << frame << ") [" << this << "]";
}

Data::~Data()
{
//  LOG(INFO) << "Data::~Data() [" << this << "]";
  delete m_impl;
}

void Data::modified(bool new_val)
{
  impl()->modified(new_val);
}

bool Data::modified()
{
  return impl()->modified();
}

bool Data::valid()
{
  return impl()->valid();
}

bool Data::ParseFrom(const std::string& frame)
{
  return impl()->ParseFrom(frame);
}

// Вернуть сериализованный заголовок
const std::string& Data::get_serialized()
{
  return impl()->get_serialized();
}

// Вернуть фактический класс PROTOBUF, хранящий данное сообщение 
DataImpl* Data::impl() const
{
  assert(m_impl);
  return m_impl;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
#if 0
Letter::Letter(zmsg* _instance) :
  m_data_needs_reserialization(true),
  m_header_needs_reserialization(true),
  m_body_instance(NULL)
{
  assert(_instance);
  // Прочитать служебные поля транспортного сообщения zmsg
  // и восстановить на его основе прикладное сообщение.
  int msg_frames = _instance->parts();
  // два последних фрейма - заголовок и тело сообщения
  assert(msg_frames >= 2);

  const std::string* head = _instance->get_part(msg_frames-2);
  const std::string* body = _instance->get_part(msg_frames-1);

  assert (head);
  assert (body);

  m_serialized_header.assign(head->data(), head->size());
  m_serialized_data.assign(body->data(), body->size());

  if (true == m_header_instance.ParseFrom(m_serialized_header))
  {
    m_initialized = UnserializeFrom(m_header_instance.usr_msg_type(), &m_serialized_data);
  }
  else
  {
    LOG(ERROR) << "Unable to unserialize header";
    hex_dump(m_serialized_header);
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// Сообщение целиком
Letter::Letter(rtdbMsgType _type, rtdbExchangeId _id)
  : m_header(new Header()),
    m_data(new Data(_type))
{
//  LOG(INFO) << "Letter::Letter(" << _type << ", " << _id << ")";
}
 
////////////////////////////////////////////////////////////////////////////////////////////////////
// Создание нового сообщения с уже десериализованым заголовком
Letter::Letter(Header* head_pb, const std::string& body_str)
  : m_header(head_pb)
{
  assert(m_header);

  m_data = new Data(m_header->usr_msg_type(), body_str);
//  LOG(INFO) << "Letter::Letter(" << head_pb << ", " << body_str << ")";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// входные параметры - фреймы заголовка и нагрузки из zmsg
Letter::Letter(const std::string& head_str, const std::string& body_str)
  : m_header(new Header(head_str))
{
  // TODO: отбрасывать сообщения с более новым протоколом
  //assert(m_header->protocol_version() == m_version_message_system);

  m_data = new Data(m_header->usr_msg_type(), body_str);
}
 
// признак того, была ли модификация данных
void Letter::modified(bool new_val)
{
  data()->modified(new_val);
}

bool Letter::modified()
{
  return data()->modified();
}

#if 0
////////////////////////////////////////////////////////////////////////////////////////////////////
// На основе типа сообщения и сериализованных данных.
// Если последний параметр равен NULL, создается пустая структура 
// нужного типа с тем, чтобы потом её заполнил пользователь самостоятельно.
Letter::Letter(const rtdbMsgType user_type, const std::string& dest, const std::string* data) :
  m_initialized(false),
  m_header(new Header()),
  m_data(new Data()),
  m_system_type(USER_MESSAGE_TYPE),
  m_user_type(user_type),
  m_interest_id(0)
{
  m_header = new Header();
  m_header->set_protocol_version(1);
  m_header->set_exchange_id(generate_next_exchange_id());
  m_header->set_interest_id(0);
  m_header->set_proc_dest(dest);
  m_header->set_proc_origin("DELME");
  m_header->set_sys_msg_type(USER_MESSAGE_TYPE);
  m_header->set_usr_msg_type(user_type);

  m_initialized = m_data->ParseFrom(&data);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
Letter::~Letter()
{
//  LOG(INFO) << "Letter destructor";
  delete m_header;
  delete m_data;
}

Header *Letter::header()
{
  assert(m_header);
  return m_header;
}

Data *Letter::data()
{
  assert(m_data);
  return m_data;
}

// признак корректности данных объекта
bool Letter::valid()
{
  return (header()->valid() && data()->valid());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Установить значения полей "Отправитель" и "Получатель"
void Letter::set_origin(const char* _origin)
{
  m_header->set_proc_origin(_origin);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Letter::set_destination(const char* _destination)
{
  m_header->set_proc_dest(_destination);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
rtdbExchangeId Letter::generate_next_exchange_id()
{
  rtdbExchangeId current_id = m_header->exchange_id();

  // Циклический счетчик от 1 до INT_MAX
  if (current_id > std::numeric_limits<unsigned int>::max())
    current_id = 0;

  m_header->set_exchange_id(++current_id);

  return current_id;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Letter::dump()
{
  LOG(INFO) << "dump usr_message_type:" << header()->usr_msg_type()
            << " exchange_id:" << header()->exchange_id()
            << " interest_id:" << header()->interest_id();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO:
// 1. получить PID процесса
// 2. получить название локального процесса
MessageFactory::MessageFactory(const char* procname)
  : m_version_message_system(1),
    m_version_rtdbus(1),
    m_pid(getpid()),
    m_exchange_id(0)
{
  m_default_serialized_header.clear();
  strncpy(m_source_procname, procname, SERVICE_NAME_MAXLEN);
  m_source_procname[SERVICE_NAME_MAXLEN] = '\0';
  // NB: Начальные значения m_default_pb_header выставляются конструктором по-умолчанию
}
 
MessageFactory::~MessageFactory()
{
}
 
Letter* MessageFactory::create(rtdbMsgType type)
{
  Letter *created = NULL;

  ++m_exchange_id;

  switch (type)
  {
    case ADG_D_MSG_EXECRESULT:
        created = new ExecResult();
    break;
    case ADG_D_MSG_ENDINITACK:
    break;
    case ADG_D_MSG_INIT:
    break;
    case ADG_D_MSG_STOP:
    break;
    case ADG_D_MSG_ENDALLINIT:
    break;
    case ADG_D_MSG_ASKLIFE:
        created = new AskLife();
    break;
    case ADG_D_MSG_LIVING:
    break;
    case ADG_D_MSG_STARTAPPLI:
    break;
    case ADG_D_MSG_STOPAPPLI:
    break;
    case ADG_D_MSG_ADJTIME:
    break;
 
    default:
      LOG(ERROR) << "Unsupported message type " << type; 
  }

  // Заполнение полей, имеющих значения не "по-умолчанию"
  created->header()->set_exchange_id(m_exchange_id);
  created->header()->set_usr_msg_type(type);
  created->header()->set_sys_msg_type(USER_MESSAGE_TYPE);
  created->header()->set_proc_origin(m_source_procname);

  return created;
}

Letter* MessageFactory::create(mdp::zmsg* request)
{
  Letter *created = NULL;

  // Прочитать служебные поля транспортного сообщения zmsg
  // и восстановить на его основе прикладное сообщение.
  // Первый фрейм - адрес возврата,
  // Два последних фрейма - заголовок и тело сообщения.
  int msg_frames = request->parts();
  assert(msg_frames >= 2);

  const std::string* head_str = request->get_part(msg_frames-2);
  const std::string* body_str = request->get_part(msg_frames-1);

  assert (head_str);
  assert (body_str);

  // Есть непустые данные для десериализации
  if ((head_str && head_str->size()) && (body_str && body_str->size()))
  {
    // Создать экземпляр из полученных фреймов заголовка и данных 
    created = unserialize(*head_str, *body_str);
  }
  else
  {
    LOG(ERROR) << "Unable to restore received message (head="<<head_str<<", data="<<body_str<<")";
  }

  return created;
}

#if 0
Использование m_default_pb_header влечет за собой создание экземпляра msg::Header
////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Создать экземпляр данных соответствующего типа, и десериализовать его на основе входной строки.
 * Если входная строка пуста (это возможно, когда сообщение создается Клиентом), то:
 * 1. Присвоить элементам восстановленного класса значения "по умолчанию", т.к. иначе protobuf 
 * не сериализует этот экземпляр данных.
 * 2. Заполнить тестовыми сериализованными данными строковый буфер m_serialized_data
 */
Letter* MessageFactory::unserialize(rtdbMsgType user_type, const std::string& source)
{
  Letter *produced = NULL;

  // подготовка стандартного заголовка
  rtdbExchangeId exchg_id = m_default_pb_header.exchange_id();
  m_default_pb_header.set_exchange_id(++exchg_id);
  m_default_serialized_header = m_default_pb_header.get_serialized();

  switch (user_type)
  {
    case ADG_D_MSG_EXECRESULT:
      produced = new ExecResult(m_default_serialized_header, source);
      break;

    case ADG_D_MSG_ASKLIFE:
      produced = new AskLife(m_default_serialized_header, source);
      break;

    default:
      LOG(ERROR) << "Unknown message type " << user_type;
      break;
  }

  return produced;
}
#endif

// Создание сообщения на основе фреймов заголовка и сериализованных данных.
Letter* MessageFactory::unserialize(const std::string& pb_header, const std::string& pb_data)
{
  Letter *created = NULL;
  // Десериализовать заголовок.
  // Ответственность за удаление m_header несет объект created
  Header *fresh_header = new Header(pb_header);

  assert(fresh_header);

  // TODO: отбрасывать сообщения с более новым протоколом
  if (fresh_header->protocol_version() > m_version_message_system)
  {
    LOG(INFO) << "Received more recent communication protocol version: "
              << fresh_header->protocol_version()
              << " than we have: " << m_version_message_system;
    delete fresh_header;
    return NULL;
  }

  rtdbMsgType sys_type  = fresh_header->sys_msg_type();
  rtdbMsgType user_type = fresh_header->usr_msg_type();

  // Пока обрабатываем только пользовательские типы сообщений
  assert(sys_type == USER_MESSAGE_TYPE);

  switch (user_type)
  {
    case ADG_D_MSG_EXECRESULT:
        created = new ExecResult(fresh_header, pb_data);
    break;
    case ADG_D_MSG_ENDINITACK:
    break;
    case ADG_D_MSG_INIT:
    break;
    case ADG_D_MSG_STOP:
    break;
    case ADG_D_MSG_ENDALLINIT:
    break;
    case ADG_D_MSG_ASKLIFE:
        created = new AskLife(fresh_header, pb_data);
    break;
    case ADG_D_MSG_LIVING:
    break;
    case ADG_D_MSG_STARTAPPLI:
    break;
    case ADG_D_MSG_STOPAPPLI:
    break;
    case ADG_D_MSG_ADJTIME:
    break;
 
    default:
      LOG(ERROR) << "Unsupported message type " << user_type; 
  }

  return created;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

