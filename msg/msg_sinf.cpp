#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "google/protobuf/stubs/common.h"
#include "proto/common.pb.h"
#include "proto/sinf.pb.h"
#include "msg_message.hpp"
#include "msg_message_impl.hpp"
#include "msg_sinf.hpp"

#include "xdb_rtap_const.hpp"   // типы данных xdb::DbType_t

using namespace msg;

// таблица соответствия "тип данных как индекс" => "размер строковой части для set_s_value()" 
static size_t var_size[xdb::DB_TYPE_LAST+1] = {
    /* DB_TYPE_UNDEF     = 0    */  0,
    /* DB_TYPE_INT8      = 1    */  0,
    /* DB_TYPE_UINT8     = 2    */  0,
    /* DB_TYPE_INT16     = 3    */  0,
    /* DB_TYPE_UINT16    = 4    */  0,
    /* DB_TYPE_INT32     = 5    */  0,
    /* DB_TYPE_UINT32    = 6    */  0,
    /* DB_TYPE_INT64     = 7    */  0,
    /* DB_TYPE_UINT64    = 8    */  0,
    /* DB_TYPE_FLOAT     = 9    */  0,
    /* DB_TYPE_DOUBLE    = 10   */  0,
    /* DB_TYPE_BYTES     = 11   */  65535, // переменная длина строки
    /* DB_TYPE_BYTES4    = 12   */  4,
    /* DB_TYPE_BYTES8    = 13   */  8,
    /* DB_TYPE_BYTES12   = 14   */  12,
    /* DB_TYPE_BYTES16   = 15   */  16,
    /* DB_TYPE_BYTES20   = 16   */  20,
    /* DB_TYPE_BYTES32   = 17   */  32,
    /* DB_TYPE_BYTES48   = 18   */  48,
    /* DB_TYPE_BYTES64   = 19   */  64,
    /* DB_TYPE_BYTES80   = 20   */  80,
    /* DB_TYPE_BYTES128  = 21   */  128,
    /* DB_TYPE_BYTES256  = 22   */  256,
    /* DB_TYPE_LAST      = 23   */  0
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Универсальная установка первоначальных значений
Value::Value(std::string& name, xdb::DbType_t type, void* val)
  : m_tag(name),
    m_type(type)
{
  memset((void*)&m_value, '\0', sizeof(m_value));

  switch (m_type)
  {
    case xdb::DB_TYPE_INT32:
      m_value.val_i32 = *static_cast<int32_t*>(val);
    break;
    case xdb::DB_TYPE_UINT32:
      m_value.val_ui32 = *static_cast<uint32_t*>(val);
    break;
    case xdb::DB_TYPE_INT64:
      m_value.val_i64 = *static_cast<int64_t*>(val);
    break;
    case xdb::DB_TYPE_UINT64:
      m_value.val_ui64 = *static_cast<uint64_t*>(val);
    break;
    case xdb::DB_TYPE_FLOAT:
      m_value.val_float = *static_cast<float*>(val);
    break;
    case xdb::DB_TYPE_DOUBLE:
      m_value.val_double = *static_cast<double*>(val);
    break;
    case xdb::DB_TYPE_BYTES:
      m_value.val_string = new std::string(*static_cast<std::string*>(val));
    break;
    case xdb::DB_TYPE_BYTES4:
    case xdb::DB_TYPE_BYTES8:
    case xdb::DB_TYPE_BYTES12:
    case xdb::DB_TYPE_BYTES16:
    case xdb::DB_TYPE_BYTES20:
    case xdb::DB_TYPE_BYTES32:
    case xdb::DB_TYPE_BYTES48:
    case xdb::DB_TYPE_BYTES64:
    case xdb::DB_TYPE_BYTES80:
    case xdb::DB_TYPE_BYTES128:
    case xdb::DB_TYPE_BYTES256:
      m_value.val_char = new char[var_size[m_type]+1];
      strncpy(m_value.val_char, static_cast<char*>(val), var_size[m_type]);
      m_value.val_char[var_size[m_type]] = '\0';
    break;
    default:
    break;
  }
}

// Инициализация
Value::Value(/*const RTDBM::ValueUpdate* */ const void* vu)
{
  const RTDBM::ValueUpdate* temp = (RTDBM::ValueUpdate*)vu;
//  m_impl->set_tag(vu.tag());
//  m_type = vu.type();
  memset((void*)&m_value, '\0', sizeof(m_value));
  m_tag.assign(temp->tag());
  m_type = static_cast<xdb::DbType_t>(temp->type());

  switch(m_type)
  {
    case xdb::DB_TYPE_INT32:
      m_value.val_i32 = temp->i32_value();
    break;
    case xdb::DB_TYPE_UINT32:
      m_value.val_ui32 = temp->i32_value();
    break;
    case xdb::DB_TYPE_INT64:
      m_value.val_i64 = temp->i64_value();
    break;
    case xdb::DB_TYPE_UINT64:
      m_value.val_ui64 = temp->i64_value();
    break;
    case xdb::DB_TYPE_FLOAT:
      m_value.val_float = temp->f_value();
    break;
    case xdb::DB_TYPE_DOUBLE:
      m_value.val_double = temp->g_value();
    break;

    case xdb::DB_TYPE_BYTES:
      // NB Предыдущее ненулевое значение игнорируем, т.к. оно может быть
      // таковым из-за одновременного размещения в памяти и других типов данных
      m_value.val_string = new std::string;
      m_value.val_string->assign(temp->s_value());
#if 0
      std::cout << "NEW string size=" << m_value.val_string->size() 
                << " " << m_value.val_string << std::endl;
#endif
    break;

    case xdb::DB_TYPE_BYTES4:
    case xdb::DB_TYPE_BYTES8:
    case xdb::DB_TYPE_BYTES12:
    case xdb::DB_TYPE_BYTES16:
    case xdb::DB_TYPE_BYTES20:
    case xdb::DB_TYPE_BYTES32:
    case xdb::DB_TYPE_BYTES48:
    case xdb::DB_TYPE_BYTES64:
    case xdb::DB_TYPE_BYTES80:
    case xdb::DB_TYPE_BYTES128:
    case xdb::DB_TYPE_BYTES256:
      // NB Предыдущее ненулевое значение игнорируем, т.к. оно может быть
      // таковым из-за одновременного размещения в памяти и других типов данных
      m_value.val_char = new char[var_size[m_type]+1];

      if (temp->s_value().size() != var_size[m_type])
        std::cout << "E01: size doesn't equal (" 
                  << temp->s_value().size() << ", "
                  << var_size[m_type] << ")" << std::endl;

      strncpy(m_value.val_char, temp->s_value().c_str(), var_size[m_type]);
      m_value.val_char[var_size[m_type]] = '\0';
#if 0
      std::cout << "NEW char* type=" << m_type 
                << " size=" << var_size[m_type]
                << " \"" << m_value.val_char << "\"" << std::endl;
#endif
    break;

    default: // TODO: обработать этот случай
    std::cout << "\nбяка" << std::endl;
    break;
  }
}

#if 0
// Инициализация
void Value::CopyFrom(/*const RTDBM::ValueUpdate* */ const void* vu)
{
  const RTDBM::ValueUpdate* temp = (RTDBM::ValueUpdate*)vu;
//  m_impl->set_tag(vu.tag());
//  m_type = vu.type();
  m_tag.assign(temp->tag());
  m_type = static_cast<xdb::DbType_t>(temp->type());

  switch(m_type)
  {
    case:
    break;
  }
}
#endif

// Установка начальных значений с неявным указанием типа
Value::Value(std::string& name, int32_t val)
  : m_tag(name),
    m_type(xdb::DB_TYPE_UINT32)
{
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_UINT32);
//  m_impl->set_i32_value(val);
  memset((void*)&m_value, '\0', sizeof(m_value));
  m_value.val_i32 = val;
}

Value::Value(std::string& name, int64_t val)
  : m_tag(name),
    m_type(xdb::DB_TYPE_UINT64)
{
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_UINT64);
//  m_impl->set_i32_value(val);
  memset((void*)&m_value, '\0', sizeof(m_value));
  m_value.val_i64 = val;
}

Value::Value(std::string& name, float val)
  : m_tag(name),
    m_type(xdb::DB_TYPE_FLOAT)
{
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_FLOAT);
//  m_impl->set_i32_value(val);
  memset((void*)&m_value, '\0', sizeof(m_value));
  m_value.val_float = val;
}

Value::Value(std::string& name, double val)
  : m_tag(name),
    m_type(xdb::DB_TYPE_DOUBLE)
{
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_DOUBLE);
//  m_impl->set_i32_value(val);
  memset((void*)&m_value, '\0', sizeof(m_value));
  m_value.val_double = val;
}

Value::Value(std::string& name, const char* val, size_t size)
  : m_tag(name),
    m_type(xdb::DB_TYPE_BYTES)
{
//  m_impl->set_tag(name);

//  switch (size)
//  {
//    case   4: m_impl->set_type(RTDBM::DB_TYPE_BYTES4);   break;
//    case   8: m_impl->set_type(RTDBM::DB_TYPE_BYTES8);   break;
//    case  12: m_impl->set_type(RTDBM::DB_TYPE_BYTES12);  break;
//    case  16: m_impl->set_type(RTDBM::DB_TYPE_BYTES16);  break;
//    case  20: m_impl->set_type(RTDBM::DB_TYPE_BYTES20);  break;
//    case  32: m_impl->set_type(RTDBM::DB_TYPE_BYTES32);  break;
//    case  48: m_impl->set_type(RTDBM::DB_TYPE_BYTES48);  break;
//    case  64: m_impl->set_type(RTDBM::DB_TYPE_BYTES64);  break;
//    case  80: m_impl->set_type(RTDBM::DB_TYPE_BYTES80);  break;
//    case 128: m_impl->set_type(RTDBM::DB_TYPE_BYTES128); break;
//    case 256: m_impl->set_type(RTDBM::DB_TYPE_BYTES256); break;
//    default:
//              // TODO: обработать этот случай
//              m_impl->set_type(RTDBM::DB_TYPE_BYTES);   break;
//  }

  memset((void*)&m_value, '\0', sizeof(m_value));

  switch (size)
  {
    case   4: m_type = xdb::DB_TYPE_BYTES4;   break;
    case   8: m_type = xdb::DB_TYPE_BYTES8;   break;
    case  12: m_type = xdb::DB_TYPE_BYTES12;  break;
    case  16: m_type = xdb::DB_TYPE_BYTES16;  break;
    case  20: m_type = xdb::DB_TYPE_BYTES20;  break;
    case  32: m_type = xdb::DB_TYPE_BYTES32;  break;
    case  48: m_type = xdb::DB_TYPE_BYTES48;  break;
    case  64: m_type = xdb::DB_TYPE_BYTES64;  break;
    case  80: m_type = xdb::DB_TYPE_BYTES80;  break;
    case 128: m_type = xdb::DB_TYPE_BYTES128; break;
    case 256: m_type = xdb::DB_TYPE_BYTES256; break;
    default:
              // TODO: обработать этот случай
              m_type = xdb::DB_TYPE_BYTES4;   break;
  }

  m_value.val_char = new char[var_size[m_type]+1];
  strcpy(m_value.val_char, val);
}

Value::Value(std::string& name, std::string& val)
  : m_tag(name),
    m_type(xdb::DB_TYPE_BYTES)
{
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_BYTES);
//  m_impl->set_s_value(val);
  memset((void*)&m_value, '\0', sizeof(m_value));
  m_value.val_string = new std::string(val);
}

// Деструктор
Value::~Value()
{
//  delete m_impl;
  switch(m_type)
  {
    case xdb::DB_TYPE_BYTES:
#if 0
      if (m_value.val_string)
        std::cout << "DELETE string size=" << m_value.val_string->size()
                  << " \"" << m_value.val_string << "\"" << std::endl;
#endif
      delete m_value.val_string;
      m_value.val_string = NULL;
    break;

    case xdb::DB_TYPE_BYTES4:
    case xdb::DB_TYPE_BYTES8:
    case xdb::DB_TYPE_BYTES12:
    case xdb::DB_TYPE_BYTES16:
    case xdb::DB_TYPE_BYTES20:
    case xdb::DB_TYPE_BYTES32:
    case xdb::DB_TYPE_BYTES48:
    case xdb::DB_TYPE_BYTES64:
    case xdb::DB_TYPE_BYTES80:
    case xdb::DB_TYPE_BYTES128:
    case xdb::DB_TYPE_BYTES256:
#if 0
      if (m_value.val_char)
        std::cout << "DELETE char* type=" << m_type
                  << " size=" << var_size[m_type]
                  << " \"" << m_value.val_char << "\"" << std::endl;
#endif
      delete [] m_value.val_char;
      m_value.val_char = NULL;
    break;

    default:
      // Остальные типы данных освобождать не нужно
    break;
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
ReadMulti::ReadMulti() : Letter(SIG_D_MSG_READ_MULTI, 0), m_updater(NULL) {}
ReadMulti::ReadMulti(rtdbExchangeId _id) : Letter(SIG_D_MSG_READ_MULTI, _id), m_updater(NULL) {}

ReadMulti::ReadMulti(Header* head, const std::string& body) : Letter(head, body), m_updater(NULL) 
{
  assert(header()->usr_msg_type() == SIG_D_MSG_READ_MULTI);
}

ReadMulti::ReadMulti(const std::string& head, const std::string& body) : Letter(head, body), m_updater(NULL)
{
  assert(header()->usr_msg_type() == SIG_D_MSG_READ_MULTI);
}
 
ReadMulti::~ReadMulti()
{
  delete m_updater;
}

void ReadMulti::add(std::string& tag, xdb::DbType_t type, void* val)
{
  RTDBM::ValueUpdate *updater = static_cast<RTDBM::ReadMulti*>(data()->impl()->instance())->add_update_item();
  RTDBM::DbType pb_type;

  assert(val);

  // NB: Требуется поддерживать синхронность между xdb::DbType_t и RTDBM::DbType (!)
  if (RTDBM::DbType_IsValid(type))
    pb_type = static_cast<RTDBM::DbType>(type);
  else
    pb_type = RTDBM::DB_TYPE_UNDEF;

  updater->set_tag(tag);
  updater->set_type(pb_type);

  switch(type)
  {
    // для булевского типа должен пользоваться get_b_value,
    // однако данный тип пока (2015-06-17) не поддерживается в XDB.
    case xdb::DB_TYPE_INT8:
    case xdb::DB_TYPE_UINT8:
    case xdb::DB_TYPE_INT16:
    case xdb::DB_TYPE_UINT16:
    case xdb::DB_TYPE_INT32:
    case xdb::DB_TYPE_UINT32:
        updater->set_i32_value(*(static_cast<int32_t*>(val)));
    break;

    case xdb::DB_TYPE_INT64:
    case xdb::DB_TYPE_UINT64:
        updater->set_i64_value(*(static_cast<int64_t*>(val)));
    break;

    case xdb::DB_TYPE_FLOAT:
        updater->set_f_value(*(static_cast<float*>(val)));
    break;

    case xdb::DB_TYPE_DOUBLE:
        updater->set_g_value(*(static_cast<double*>(val)));
    break;

    case xdb::DB_TYPE_BYTES:
        updater->set_s_value(*(static_cast<std::string*>(val)));
    break;

    case xdb::DB_TYPE_BYTES4:
    case xdb::DB_TYPE_BYTES8:
    case xdb::DB_TYPE_BYTES12:
    case xdb::DB_TYPE_BYTES16:
    case xdb::DB_TYPE_BYTES20:
    case xdb::DB_TYPE_BYTES32:
    case xdb::DB_TYPE_BYTES48:
    case xdb::DB_TYPE_BYTES64:
    case xdb::DB_TYPE_BYTES80:
    case xdb::DB_TYPE_BYTES128:
    case xdb::DB_TYPE_BYTES256:
        updater->set_s_value(static_cast<char*>(val), var_size[type]);
    break;

    case xdb::DB_TYPE_LAST:
    case xdb::DB_TYPE_UNDEF:
        // TODO: что делать при попытке сохранения данных неопределенного типа? 
    break;

    default:
      std::cout << "E: Unsupported type " << type << std::endl;
    break;
  }
}

int ReadMulti::num_items()
{
  return static_cast<RTDBM::ReadMulti*>(data()->impl()->instance())->update_item_size();
}

// Получить значения параметра под заданным индексом
// Входные аргументы
//   idx - индекс параметра (нумерация с 0)
// Выходные аргументы
//   tag - название параметра
//   type - тип параметра из БДРВ
//  Входной/выходной аргумент
//   val - указатель на аллоцированный вызывающей стороной буфер, принимающий значение параметра
bool ReadMulti::get(std::size_t idx, std::string& tag, xdb::DbType_t& type, void* val)
{
  bool status = true;
  const RTDBM::ValueUpdate& item =
        static_cast<RTDBM::ReadMulti*>(data()->impl()->instance())->update_item(idx);

  assert(val);

  type = static_cast<xdb::DbType_t>(item.type());
  tag = item.tag();

  switch (type)
  {
    // для булевского типа должен пользоваться get_b_value,
    // однако данный тип пока (2015-06-17) не поддерживается в XDB.
    case RTDBM::DB_TYPE_INT8:
    case RTDBM::DB_TYPE_UINT8:
    case RTDBM::DB_TYPE_INT16:
    case RTDBM::DB_TYPE_UINT16:
    case RTDBM::DB_TYPE_INT32:
    case RTDBM::DB_TYPE_UINT32:
        *(static_cast<int32_t*>(val)) = item.i32_value();
    break;

    case RTDBM::DB_TYPE_INT64:
    case RTDBM::DB_TYPE_UINT64:
        *(static_cast<int64_t*>(val)) = item.i64_value();
    break;

    case RTDBM::DB_TYPE_FLOAT:
        *(static_cast<float*>(val)) = item.f_value();
    break;

    case RTDBM::DB_TYPE_DOUBLE:
        *(static_cast<double*>(val)) = item.g_value();
    break;

    case RTDBM::DB_TYPE_BYTES:
        (static_cast<std::string*>(val))->assign(item.s_value());
    break;

    case RTDBM::DB_TYPE_BYTES4:
    case RTDBM::DB_TYPE_BYTES8:
    case RTDBM::DB_TYPE_BYTES12:
    case RTDBM::DB_TYPE_BYTES16:
    case RTDBM::DB_TYPE_BYTES20:
    case RTDBM::DB_TYPE_BYTES32:
    case RTDBM::DB_TYPE_BYTES48:
    case RTDBM::DB_TYPE_BYTES64:
    case RTDBM::DB_TYPE_BYTES80:
    case RTDBM::DB_TYPE_BYTES128:
    case RTDBM::DB_TYPE_BYTES256:
        // TODO: источник путаницы - вызывающая сторона может передать указатель на char
        // но может передать и указатель на string
#warning "определись с типом буфера для char* в ReadMulti::get "
        (static_cast<std::string*>(val))->assign(item.s_value(), 0, var_size[type]);
    break;

    default:
       // Неизвестный тип
       status = false;
       val = NULL;
    break;  
  }

  return status;
}

const Value& ReadMulti::item(std::size_t idx)
{
  const RTDBM::ValueUpdate& pb_updater = 
        static_cast<RTDBM::ReadMulti*>(data()->impl()->instance())->update_item(idx);

  // Создать новый
  delete m_updater;
  m_updater = new Value(static_cast<const void*>(&pb_updater));

  return *m_updater;
}

#if 0
const Value& ReadMulti::operator[](std::size_t idx)
{
  const RTDBM::ValueUpdate& pb_updater = 
        static_cast<RTDBM::ReadMulti*>(data()->impl()->instance())->update_item(idx);

  // Создать новый
  delete m_updater;
  m_updater = new Value((void*)&pb_updater);

  return *m_updater;
}
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
WriteMulti::WriteMulti() : Letter(SIG_D_MSG_WRITE_MULTI, 0), m_updater(NULL) {}

WriteMulti::WriteMulti(rtdbExchangeId _id) : Letter(SIG_D_MSG_WRITE_MULTI, _id), m_updater(NULL) {}

WriteMulti::WriteMulti(Header* head, const std::string& body) : Letter(head, body), m_updater(NULL)
{
  assert(header()->usr_msg_type() == SIG_D_MSG_WRITE_MULTI);
}
 
WriteMulti::WriteMulti(const std::string& head, const std::string& body) : Letter(head, body), m_updater(NULL)
{
  assert(header()->usr_msg_type() == SIG_D_MSG_WRITE_MULTI);
}
 
WriteMulti::~WriteMulti()
{
  delete m_updater;
}

// Входные аргументы
//   tag
//   type
// Входной-выходной агрумент:
//   val
//   Представляет указатель на ранее выделенный буфер для помещения в нем значения параметра
//
// Допустимые типы
//  DB_TYPE_BYTES       => std::string*
//  DB_TYPE_BYTES[?+]   => char*
//  остальные типы      => указатель на POD (обычно uint64_t*)
void WriteMulti::add(std::string& tag, xdb::DbType_t type, void* val)
{
  RTDBM::ValueUpdate *updater = static_cast<RTDBM::ReadMulti*>(data()->impl()->instance())->add_update_item();
  RTDBM::DbType pb_type;

  assert(val);

  // NB: Требуется поддерживать синхронность между xdb::DbType_t и RTDBM::DbType (!)
  if (RTDBM::DbType_IsValid(type))
    pb_type = static_cast<RTDBM::DbType>(type);
  else
    pb_type = RTDBM::DB_TYPE_UNDEF;

  updater->set_tag(tag);
  updater->set_type(pb_type);

  switch(type)
  {
    // для булевского типа должен пользоваться get_b_value,
    // однако данный тип пока (2015-06-17) не поддерживается в XDB.
    case xdb::DB_TYPE_INT8:
    case xdb::DB_TYPE_UINT8:
    case xdb::DB_TYPE_INT16:
    case xdb::DB_TYPE_UINT16:
    case xdb::DB_TYPE_INT32:
    case xdb::DB_TYPE_UINT32:
        updater->set_i32_value(*(static_cast<int32_t*>(val)));
    break;

    case xdb::DB_TYPE_INT64:
    case xdb::DB_TYPE_UINT64:
        updater->set_i64_value(*(static_cast<int64_t*>(val)));
    break;

    case xdb::DB_TYPE_FLOAT:
        updater->set_f_value(*(static_cast<float*>(val)));
    break;

    case xdb::DB_TYPE_DOUBLE:
        updater->set_g_value(*(static_cast<double*>(val)));
    break;

    case xdb::DB_TYPE_BYTES:
        updater->set_s_value(*static_cast<std::string*>(val));
    break;

    case xdb::DB_TYPE_BYTES4:
    case xdb::DB_TYPE_BYTES8:
    case xdb::DB_TYPE_BYTES12:
    case xdb::DB_TYPE_BYTES16:
    case xdb::DB_TYPE_BYTES20:
    case xdb::DB_TYPE_BYTES32:
    case xdb::DB_TYPE_BYTES48:
    case xdb::DB_TYPE_BYTES64:
    case xdb::DB_TYPE_BYTES80:
    case xdb::DB_TYPE_BYTES128:
    case xdb::DB_TYPE_BYTES256:
        updater->set_s_value(static_cast<char*>(val), var_size[type]);
    break;

    case xdb::DB_TYPE_LAST:
    case xdb::DB_TYPE_UNDEF:
        // TODO: что делать при попытке сохранения данных неопределенного типа? 
    break;

    default:
       std::cout << "E: Unsupported type " << type;
    break;
  }
}

const Value& WriteMulti::item(std::size_t idx)
{
  const RTDBM::ValueUpdate& pb_updater = 
        static_cast<RTDBM::WriteMulti*>(data()->impl()->instance())->update_item(idx);

  // Создать новый
  delete m_updater;
  m_updater = new Value(static_cast<const void*>(&pb_updater));

  return *m_updater;
}

int WriteMulti::num_items()
{
  return static_cast<RTDBM::WriteMulti*>(data()->impl()->instance())->update_item_size();
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
 
