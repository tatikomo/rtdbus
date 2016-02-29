#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream> // std::cerr

#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

#include "proto/common.pb.h"
#include "proto/sinf.pb.h"
#include "msg_message.hpp"
#include "msg_message_impl.hpp"
#include "msg_sinf.hpp"

#include "xdb_common.hpp"   // типы данных xdb::DbType_t, xdb::datetime_t, xdb::sampler_type_t

using namespace msg;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Универсальная установка первоначальных значений
Value::Value(std::string& _name, xdb::DbType_t _type, void* val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(_name);
  m_instance.type = _type;
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));

  switch (m_instance.type)
  {
    case xdb::DB_TYPE_LOGICAL:
      m_instance.value.fixed.val_bool = *static_cast<bool*>(val);
    break;
    case xdb::DB_TYPE_INT8:
      m_instance.value.fixed.val_int8 = *static_cast<int8_t*>(val);
    break;
    case xdb::DB_TYPE_UINT8:
      m_instance.value.fixed.val_uint8 = *static_cast<uint8_t*>(val);
    break;
    case xdb::DB_TYPE_INT16:
      m_instance.value.fixed.val_int16 = *static_cast<int16_t*>(val);
    break;
    case xdb::DB_TYPE_UINT16:
      m_instance.value.fixed.val_uint16 = *static_cast<uint16_t*>(val);
    break;
    case xdb::DB_TYPE_INT32:
      m_instance.value.fixed.val_int32 = *static_cast<int32_t*>(val);
    break;
    case xdb::DB_TYPE_UINT32:
      m_instance.value.fixed.val_uint32 = *static_cast<uint32_t*>(val);
    break;
    case xdb::DB_TYPE_INT64:
      m_instance.value.fixed.val_int64 = *static_cast<int64_t*>(val);
    break;
    case xdb::DB_TYPE_UINT64:
      m_instance.value.fixed.val_uint64 = *static_cast<uint64_t*>(val);
    break;
    case xdb::DB_TYPE_FLOAT:
      m_instance.value.fixed.val_float = *static_cast<float*>(val);
    break;
    case xdb::DB_TYPE_DOUBLE:
      m_instance.value.fixed.val_double = *static_cast<double*>(val);
    break;
    case xdb::DB_TYPE_BYTES:
      m_instance.value.dynamic.val_string = new std::string(*static_cast<std::string*>(val));
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
      m_instance.value.dynamic.size = xdb::var_size[m_instance.type];
      m_instance.value.dynamic.varchar = new char[xdb::var_size[m_instance.type]+1];
      strncpy(m_instance.value.dynamic.varchar, static_cast<char*>(val), xdb::var_size[m_instance.type]);
      m_instance.value.dynamic.varchar[xdb::var_size[m_instance.type]] = '\0';
    break;
    case xdb::DB_TYPE_ABSTIME:
//      std::cout << "msg_sinf: sec=" << static_cast<timeval*>(val)->tv_sec << std::endl; //1
      m_instance.value.fixed.val_time.tv_sec = static_cast<timeval*>(val)->tv_sec;
      m_instance.value.fixed.val_time.tv_usec = static_cast<timeval*>(val)->tv_usec;
      //memcpy(i&m_instance.value.fixed.val_time, static_cast<timeval*>(val), sizeof(timeval));
    break;
    default:
      LOG(ERROR)<< __FILE__ << ":" << __LINE__
                << ": Value::Value() unsupported attr '" << m_instance.name
                << "' type: " << m_instance.type;
    break;
  }
}

// Инициализация
Value::Value(/*RTDBM::ValueUpdate* */ void* vu)
  : m_rtdbm_valueupdate_instance(vu)
{
  RTDBM::ValueUpdate* temp = static_cast<RTDBM::ValueUpdate*>(m_rtdbm_valueupdate_instance);
  xdb::datetime_t datetime;
//  m_impl->set_tag(vu.tag());
//  m_instance.type = vu.type();
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.name.assign(temp->tag());
  m_instance.type = static_cast<xdb::DbType_t>(temp->type());

  switch(m_instance.type)
  {
    case xdb::DB_TYPE_LOGICAL:
      m_instance.value.fixed.val_bool = static_cast<bool>(temp->b_value());
    break;
    case xdb::DB_TYPE_INT8:
      m_instance.value.fixed.val_int8 = static_cast<int8_t>(temp->i32_value());
    break;
    case xdb::DB_TYPE_UINT8:
      m_instance.value.fixed.val_uint8 = static_cast<uint8_t>(temp->i32_value());
    break;
    case xdb::DB_TYPE_INT16:
      m_instance.value.fixed.val_int16 = static_cast<int8_t>(temp->i32_value());
    break;
    case xdb::DB_TYPE_UINT16:
      m_instance.value.fixed.val_uint16 = static_cast<uint16_t>(temp->i32_value());
    break;
    case xdb::DB_TYPE_INT32:
      m_instance.value.fixed.val_int32 = temp->i32_value();
    break;
    case xdb::DB_TYPE_UINT32:
      m_instance.value.fixed.val_uint32 = temp->i32_value();
    break;
    case xdb::DB_TYPE_INT64:
      m_instance.value.fixed.val_int64 = temp->i64_value();
    break;
    case xdb::DB_TYPE_UINT64:
      m_instance.value.fixed.val_uint64 = temp->i64_value();
    break;
    case xdb::DB_TYPE_FLOAT:
      m_instance.value.fixed.val_float = temp->f_value();
    break;
    case xdb::DB_TYPE_DOUBLE:
      m_instance.value.fixed.val_double = temp->g_value();
    break;

    case xdb::DB_TYPE_BYTES:
      // NB Предыдущее ненулевое значение игнорируем, т.к. оно может быть
      // таковым из-за одновременного размещения в памяти и других типов данных
      m_instance.value.dynamic.val_string = new std::string;
      m_instance.value.dynamic.val_string->assign(temp->s_value());
#if 0
      std::cout << "NEW string size=" << m_instance.value.dynamic.val_string->size() 
                << " " << m_instance.value.dynamic.val_string << std::endl;
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
      m_instance.value.dynamic.size = xdb::var_size[m_instance.type];
      m_instance.value.dynamic.varchar = new char[xdb::var_size[m_instance.type]+1];

      // Странно - размер данных не нулевой, но не совпадает с ожидаемым
      if (temp->s_value().size() && (temp->s_value().size() != xdb::var_size[m_instance.type]))
        LOG(ERROR)<< "Value::Value(void*) " << m_instance.name
                  << ": size doesn't equal (" 
                  << temp->s_value().size() << ", "
                  << xdb::var_size[m_instance.type] << ")";

      strncpy(m_instance.value.dynamic.varchar, temp->s_value().c_str(), xdb::var_size[m_instance.type]);
//      m_instance.value.dynamic.varchar[xdb::var_size[m_instance.type]] = '\0';
#if 0
      std::cout << "NEW char* type=" << m_instance.type 
                << " size=" << xdb::var_size[m_instance.type]
                << " \"" << m_instance.value.dynamic.varchar << "\"" << std::endl;
#endif
    break;

    case xdb::DB_TYPE_ABSTIME:
      datetime.common = temp->i64_value();
      m_instance.value.fixed.val_time.tv_sec = datetime.part[0];
      m_instance.value.fixed.val_time.tv_usec= datetime.part[1];
//      std::cout << "msg_sinf: sec64=" << temp->i64_value()
//                << " s:" << m_instance.value.fixed.val_time.tv_sec
//                << " u:" << m_instance.value.fixed.val_time.tv_usec
//                << std::endl; //1
    break;

    case xdb::DB_TYPE_UNDEF:
      // Не нужно инициализировать значение, поскольку тип параметра пока неизвестен
    break;

    default:
      LOG(ERROR)<< ": unsupported type " << m_instance.type << " for " << m_instance.name;
    break;
  }
}

#if 0
// Инициализация
void Value::CopyFrom(/*const RTDBM::ValueUpdate* */ const void* vu)
{
  const RTDBM::ValueUpdate* temp = (RTDBM::ValueUpdate*)vu;
//  m_impl->set_tag(vu.tag());
//  m_instance.type = vu.type();
  m_instance.name.assign(temp->tag());
  m_instance.type = static_cast<xdb::DbType_t>(temp->type());

  switch(m_instance.type)
  {
    case:
    break;
  }
}
#endif

// Установка начальных значений с неявным указанием типа
Value::Value(std::string& name, bool val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_LOGICAL;
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_LOGICAL);
//  m_impl->set_i32_value(val);
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.value.fixed.val_bool = val;
}

Value::Value(std::string& name, int8_t val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_INT8;
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_INT8);
//  m_impl->set_i32_value(val);
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.value.fixed.val_int8 = val;
}

Value::Value(std::string& name, uint8_t val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_UINT8;
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_UINT8);
//  m_impl->set_i32_value(val);
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.value.fixed.val_uint8 = val;
}

Value::Value(std::string& name, int16_t val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_INT16;
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_INT16);
//  m_impl->set_i32_value(val);
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.value.fixed.val_int16 = val;
}

Value::Value(std::string& name, uint16_t val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_UINT16;
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_UINT16);
//  m_impl->set_i32_value(val);
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.value.fixed.val_uint16 = val;
}

Value::Value(std::string& name, int32_t val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_INT32;
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_INT32);
//  m_impl->set_i32_value(val);
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.value.fixed.val_int32 = val;
}

Value::Value(std::string& name, uint32_t val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_UINT32;
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_UINT32);
//  m_impl->set_i32_value(val);
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.value.fixed.val_uint32 = val;
}

Value::Value(std::string& name, int64_t val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_INT64;
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_INT64);
//  m_impl->set_i32_value(val);
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.value.fixed.val_int64 = val;
}

Value::Value(std::string& name, uint64_t val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_UINT64;
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_UINT64);
//  m_impl->set_i32_value(val);
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.value.fixed.val_uint64 = val;
}

Value::Value(std::string& name, float val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_FLOAT;
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_FLOAT);
//  m_impl->set_i32_value(val);
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.value.fixed.val_float = val;
}

Value::Value(std::string& name, double val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_DOUBLE;
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_DOUBLE);
//  m_impl->set_i32_value(val);
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.value.fixed.val_double = val;
}

Value::Value(std::string& name, const char* val, size_t size)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_BYTES;
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

  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));

  switch (size)
  {
    case   4: m_instance.type = xdb::DB_TYPE_BYTES4;   break;
    case   8: m_instance.type = xdb::DB_TYPE_BYTES8;   break;
    case  12: m_instance.type = xdb::DB_TYPE_BYTES12;  break;
    case  16: m_instance.type = xdb::DB_TYPE_BYTES16;  break;
    case  20: m_instance.type = xdb::DB_TYPE_BYTES20;  break;
    case  32: m_instance.type = xdb::DB_TYPE_BYTES32;  break;
    case  48: m_instance.type = xdb::DB_TYPE_BYTES48;  break;
    case  64: m_instance.type = xdb::DB_TYPE_BYTES64;  break;
    case  80: m_instance.type = xdb::DB_TYPE_BYTES80;  break;
    case 128: m_instance.type = xdb::DB_TYPE_BYTES128; break;
    case 256: m_instance.type = xdb::DB_TYPE_BYTES256; break;
    default:
              // TODO: обработать этот случай
              m_instance.type = xdb::DB_TYPE_BYTES4;   break;
  }

  m_instance.value.dynamic.size = xdb::var_size[m_instance.type];
  m_instance.value.dynamic.varchar = new char[xdb::var_size[m_instance.type]+1];
  strcpy(m_instance.value.dynamic.varchar, val);
}

Value::Value(std::string& name, std::string& val)
  : m_rtdbm_valueupdate_instance(NULL)
{
  m_instance.name.assign(name);
  m_instance.type = xdb::DB_TYPE_BYTES;
//  m_impl->set_tag(name);
//  m_impl->set_type(RTDBM::DB_TYPE_BYTES);
//  m_impl->set_s_value(val);
  memset((void*)&m_instance.value, '\0', sizeof(xdb::AttrVal_t));
  m_instance.value.dynamic.val_string = new std::string(val);
}

// Деструктор
Value::~Value()
{
//  delete m_impl;
  switch(m_instance.type)
  {
    case xdb::DB_TYPE_BYTES:
#if 0
      if (m_instance.value.dynamic.val_string)
        std::cout << "DELETE string size=" << m_instance.value.dynamic.val_string->size()
                  << " \"" << m_instance.value.dynamic.val_string << "\"" << std::endl;
#endif
      delete m_instance.value.dynamic.val_string;
      m_instance.value.dynamic.val_string = NULL;
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
      if (m_instance.value.dynamic.varchar)
        std::cout << "DELETE char* type=" << m_instance.type
                  << " size=" << m_instance.value.dynamic.size
                  << " \"" << m_instance.value.dynamic.varchar << "\"" << std::endl;
#endif
      delete [] m_instance.value.dynamic.varchar;
      m_instance.value.dynamic.varchar = NULL;
    break;

    default:
      // Остальные типы данных освобождать не нужно
    break;
  }
}

const std::string Value::as_string() const
{
  std::string value;
  char buffer [50];
  char s_date [D_DATE_FORMAT_LEN + 1];
  struct tm result_time;
  time_t given_time;

  switch(m_instance.type)
  {
    case xdb::DB_TYPE_LOGICAL:
      sprintf(buffer, "%d", m_instance.value.fixed.val_bool);
      value.assign(buffer);
    break;
    case xdb::DB_TYPE_INT8:
      sprintf(buffer, "%+d", (signed int)m_instance.value.fixed.val_int8);
      value.assign(buffer);
    break;
    case xdb::DB_TYPE_UINT8:
      sprintf(buffer, "%d",  (unsigned int)m_instance.value.fixed.val_uint8);
      value.assign(buffer);
    break;
    case xdb::DB_TYPE_INT16:
      sprintf(buffer, "%+d", m_instance.value.fixed.val_int16);
      value.assign(buffer);
    break;
    case xdb::DB_TYPE_UINT16:
      sprintf(buffer, "%d", m_instance.value.fixed.val_uint16);
      value.assign(buffer);
    break;
    case xdb::DB_TYPE_INT32:
      sprintf(buffer, "%+d", m_instance.value.fixed.val_int32);
      value.assign(buffer);
    break;
    case xdb::DB_TYPE_UINT32:
      sprintf(buffer, "%d", m_instance.value.fixed.val_uint32);
      value.assign(buffer);
    break;
    case xdb::DB_TYPE_INT64:
      sprintf(buffer, "%+lld", m_instance.value.fixed.val_int64);
      value.assign(buffer);
    break;
    case xdb::DB_TYPE_UINT64:
      sprintf(buffer, "%lld", m_instance.value.fixed.val_uint64);
      value.assign(buffer);
    break;
    case xdb::DB_TYPE_FLOAT:
      sprintf(buffer, "%f", m_instance.value.fixed.val_float);
      value.assign(buffer);
    break;
    case xdb::DB_TYPE_DOUBLE:
      sprintf(buffer, "%g", m_instance.value.fixed.val_double);
      value.assign(buffer);
    break;
    case xdb::DB_TYPE_BYTES:
      if (m_instance.value.dynamic.val_string)
        value.assign(*m_instance.value.dynamic.val_string);
      else
        value.assign("<NULL>");
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
      value.assign(m_instance.value.dynamic.varchar); //, m_instance.value.dynamic.size);
    break;

    case xdb::DB_TYPE_ABSTIME:
      given_time = m_instance.value.fixed.val_time.tv_sec;
      localtime_r(&given_time, &result_time);
      strftime(s_date, D_DATE_FORMAT_LEN, D_DATE_FORMAT_STR, &result_time);
      snprintf(buffer, D_DATE_FORMAT_W_MSEC_LEN, "%s.%06ld", s_date, m_instance.value.fixed.val_time.tv_usec);
      value.assign(buffer);
//      std::cout << "msg_sinf: sec=" << m_instance.value.fixed.val_time.tv_sec
//                << " str: " << buffer << std::endl; //1
    break;

    case xdb::DB_TYPE_UNDEF:
      // Допустимо, если значение атрибута не было прочитано из БДРВ
    break;

    default:
      LOG(ERROR)<< ": unsupported data type: " << m_instance.type
                << " for " << m_instance.name;
    break;
  }

  return value;
}

// Перенести изменения, находящиеся в AttrVal, в сериализованный объект под контролем protobuf
void Value::flush()
{
  RTDBM::ValueUpdate *updater = static_cast<RTDBM::ValueUpdate*>(m_rtdbm_valueupdate_instance);
  RTDBM::DbType pb_type = RTDBM::DB_TYPE_UNDEF;
  RTDBM::Quality pb_quality = RTDBM::ATTR_ERROR;
  xdb::datetime_t datetime;

  updater->set_tag(m_instance.name);

  // NB: Требуется поддерживать синхронность между xdb::DbType_t и RTDBM::DbType (!)
  if (RTDBM::DbType_IsValid(m_instance.type))
    pb_type = static_cast<RTDBM::DbType>(m_instance.type);
  updater->set_type(pb_type);

  if (RTDBM::Quality_IsValid(m_instance.quality))
    pb_quality = static_cast<RTDBM::Quality>(m_instance.quality);
  updater->set_quality(pb_quality);

  switch(m_instance.type)
  {
    case xdb::DB_TYPE_LOGICAL:
        updater->set_b_value(m_instance.value.fixed.val_bool);
    break;
    case xdb::DB_TYPE_INT8:
        updater->set_i32_value(m_instance.value.fixed.val_int8);
    break;
    case xdb::DB_TYPE_UINT8:
        updater->set_i32_value(m_instance.value.fixed.val_uint8);
    break;
    case xdb::DB_TYPE_INT16:
        updater->set_i32_value(m_instance.value.fixed.val_int16);
    break;
    case xdb::DB_TYPE_UINT16:
        updater->set_i32_value(m_instance.value.fixed.val_uint16);
    break;
    case xdb::DB_TYPE_INT32:
        updater->set_i32_value(m_instance.value.fixed.val_int32);
    break;
    case xdb::DB_TYPE_UINT32:
        updater->set_i32_value(m_instance.value.fixed.val_uint32);
    break;
    case xdb::DB_TYPE_INT64:
        updater->set_i64_value(m_instance.value.fixed.val_int64);
    break;
    case xdb::DB_TYPE_UINT64:
        updater->set_i64_value(m_instance.value.fixed.val_uint64);
    break;
    case xdb::DB_TYPE_FLOAT:
        updater->set_f_value(m_instance.value.fixed.val_float);
    break;
    case xdb::DB_TYPE_DOUBLE:
        updater->set_g_value(m_instance.value.fixed.val_double);
    break;
    case xdb::DB_TYPE_BYTES:
        updater->set_s_value(*m_instance.value.dynamic.val_string);
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
        updater->set_s_value(m_instance.value.dynamic.varchar, xdb::var_size[m_instance.type]);
    break;
    case xdb::DB_TYPE_ABSTIME:
        datetime.part[0] = m_instance.value.fixed.val_time.tv_sec;
        datetime.part[1] = m_instance.value.fixed.val_time.tv_usec;
        updater->set_i64_value(datetime.common);
//        std::cout << "msg_sinf: flush sec=" << datetime.part[0]
//                << " usec=" << datetime.part[1]
//                << " int64=" << datetime.common << std::endl; //1
    break;
    case xdb::DB_TYPE_LAST:
    case xdb::DB_TYPE_UNDEF:
        // TODO: что делать при попытке сохранения данных неопределенного типа?
    break;

    default:
      LOG(ERROR)<< ": unsupported type " << m_instance.type;
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

// Добавить в набор тегов чтения новый элемент
//
// val может быть NULL, если заполняется на стороне Клиента,
// в этом случае значение должно быть заполнено на стороне Сервиса.
void ReadMulti::add(std::string& tag, xdb::DbType_t type, void* val)
{
  RTDBM::ValueUpdate *updater = static_cast<RTDBM::ReadMulti*>(data()->impl()->instance())->add_update_item();
  RTDBM::DbType pb_type;
  xdb::datetime_t datetime;

  // NB: Требуется поддерживать синхронность между xdb::DbType_t и RTDBM::DbType (!)
  if (RTDBM::DbType_IsValid(type))
    pb_type = static_cast<RTDBM::DbType>(type);
  else
    pb_type = RTDBM::DB_TYPE_UNDEF;

  updater->set_tag(tag);
  updater->set_type(pb_type);
  updater->set_quality(RTDBM::ATTR_ERROR);

  switch(type)
  {
    case xdb::DB_TYPE_LOGICAL:
      if (val)
        updater->set_b_value(*(static_cast<bool*>(val)));
    break;
    case xdb::DB_TYPE_INT8:
      if (val)
        updater->set_i32_value(*(static_cast<int8_t*>(val)));
    break;
    case xdb::DB_TYPE_UINT8:
      if (val)
        updater->set_i32_value(*(static_cast<uint8_t*>(val)));
    break;
    case xdb::DB_TYPE_INT16:
      if (val)
        updater->set_i32_value(*(static_cast<int16_t*>(val)));
    break;
    case xdb::DB_TYPE_UINT16:
      if (val)
        updater->set_i32_value(*(static_cast<uint16_t*>(val)));
    break;
    case xdb::DB_TYPE_INT32:
      if (val)
        updater->set_i32_value(*(static_cast<int32_t*>(val)));
    break;
    case xdb::DB_TYPE_UINT32:
      if (val)
        updater->set_i32_value(*(static_cast<uint32_t*>(val)));
    break;
    case xdb::DB_TYPE_INT64:
      if (val)
        updater->set_i64_value(*(static_cast<int64_t*>(val)));
    break;
    case xdb::DB_TYPE_UINT64:
      if (val)
        updater->set_i64_value(*(static_cast<uint64_t*>(val)));
    break;
    case xdb::DB_TYPE_FLOAT:
      if (val)
        updater->set_f_value(*(static_cast<float*>(val)));
    break;
    case xdb::DB_TYPE_DOUBLE:
      if (val)
        updater->set_g_value(*(static_cast<double*>(val)));
    break;
    case xdb::DB_TYPE_BYTES:
      if (val)
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
      if (val)
        updater->set_s_value(static_cast<char*>(val), xdb::var_size[type]);
    break;

    case xdb::DB_TYPE_ABSTIME:
      if (val)
      {
        datetime.part[0] = static_cast<struct timeval*>(val)->tv_sec;
        datetime.part[1] = static_cast<struct timeval*>(val)->tv_usec;
        updater->set_i64_value(datetime.common);
//        std::cout << "msg_sinf ReadMulti::add sec=" << datetime.part[0]
//            << " usec=" << datetime.part[1]
//            << " i64=" << datetime.common
//            << std::endl; //1
      }
    break;

    case xdb::DB_TYPE_LAST:
    case xdb::DB_TYPE_UNDEF:
        // TODO: что делать при попытке сохранения данных неопределенного типа?
    break;

    default:
      LOG(ERROR)<< ": unsupported type " << type;
    break;
  }
}

std::size_t ReadMulti::num_items()
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
bool ReadMulti::get(std::size_t idx, std::string& tag, xdb::DbType_t& type, xdb::Quality_t& qual, void* val)
{
   bool status = true;
   xdb::datetime_t datetime;
   const RTDBM::ValueUpdate& item =
         static_cast<RTDBM::ReadMulti*>(data()->impl()->instance())->update_item(idx);
 
  assert(val);

  type = static_cast<xdb::DbType_t>(item.type());
  qual = static_cast<xdb::Quality_t>(item.quality());
  tag = item.tag();
 
  switch (type)
  {
     case RTDBM::DB_TYPE_LOGICAL:
        *(static_cast<bool*>(val)) = item.b_value();
     break;
     case RTDBM::DB_TYPE_INT8:
        *(static_cast<int8_t*>(val)) = item.i32_value();
     break;
     case RTDBM::DB_TYPE_UINT8:
        *(static_cast<uint8_t*>(val)) = item.i32_value();
     break;
     case RTDBM::DB_TYPE_INT16:
        *(static_cast<int16_t*>(val)) = item.i32_value();
     break;
     case RTDBM::DB_TYPE_UINT16:
        *(static_cast<uint16_t*>(val)) = item.i32_value();
     break;
     case RTDBM::DB_TYPE_INT32:
        *(static_cast<int32_t*>(val)) = item.i32_value();
     break;
     case RTDBM::DB_TYPE_UINT32:
        *(static_cast<uint32_t*>(val)) = item.i32_value();
     break;
     case RTDBM::DB_TYPE_INT64:
        *(static_cast<int64_t*>(val)) = item.i64_value();
     break;
     case RTDBM::DB_TYPE_UINT64:
        *(static_cast<uint64_t*>(val)) = item.i64_value();
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
        (static_cast<std::string*>(val))->assign(item.s_value(), 0, xdb::var_size[type]);
     break;

     case RTDBM::DB_TYPE_ABSTIME:
        datetime.common = item.i64_value();
        (static_cast<struct timeval*>(val)->tv_sec)  = datetime.part[0];
        (static_cast<struct timeval*>(val)->tv_usec) = datetime.part[1];
//        std::cout << "msg_sinf: i64=" << datetime.common
//                << " sec=" << datetime.part[0] << " usec=" << datetime.part[1]
//                << std::endl; //1
//       *(static_cast<uint64_t*>(val)) = item.i64_value();
     break;

     default:
       LOG(ERROR)<< ": ReadMulti::get() unsupported type: " << type;
       // Неизвестный тип
       status = false;
       val = NULL;
     break;  
  }
 
  return status;
}

bool ReadMulti::get(std::size_t idx, std::string& tag, xdb::DbType_t& type, xdb::Quality_t& qual, xdb::AttrVal_t& param)
{
  bool status = true;
  xdb::datetime_t datetime;
  const RTDBM::ValueUpdate& item =
        static_cast<RTDBM::ReadMulti*>(data()->impl()->instance())->update_item(idx);

  type = static_cast<xdb::DbType_t>(item.type());
  qual = static_cast<xdb::Quality_t>(item.quality());
  tag = item.tag();

  switch (type)
  {
    case RTDBM::DB_TYPE_LOGICAL:
        param.fixed.val_bool   = static_cast<bool>(item.b_value());
    break;
    case RTDBM::DB_TYPE_INT8:
        param.fixed.val_int8   = static_cast<int8_t>(item.i32_value());
    break;
    case RTDBM::DB_TYPE_UINT8:
        param.fixed.val_uint8  = static_cast<uint8_t>(item.i32_value());
    break;
    case RTDBM::DB_TYPE_INT16:
        param.fixed.val_int16  = static_cast<int16_t>(item.i32_value());
    break;
    case RTDBM::DB_TYPE_UINT16:
        param.fixed.val_uint16 = static_cast<uint16_t>(item.i32_value());
    break;
    case RTDBM::DB_TYPE_INT32:
        param.fixed.val_int32  = static_cast<int32_t>(item.i32_value());
    break;
    case RTDBM::DB_TYPE_UINT32:
        param.fixed.val_uint32 = static_cast<uint32_t>(item.i32_value());
    break;
    case RTDBM::DB_TYPE_INT64:
        param.fixed.val_int64  = static_cast<int64_t>(item.i64_value());
    break;
    case RTDBM::DB_TYPE_UINT64:
        param.fixed.val_uint64 = static_cast<uint64_t>(item.i64_value());
    break;
    case RTDBM::DB_TYPE_FLOAT:
        param.fixed.val_float  = static_cast<float>(item.f_value());
    break;
    case RTDBM::DB_TYPE_DOUBLE:
        param.fixed.val_double = static_cast<double>(item.g_value());
    break;
    case RTDBM::DB_TYPE_BYTES:
        param.dynamic.val_string = new std::string(item.s_value());
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
        // но может передать и указатель на string, поскольку она не знает, каким типом
        // является значение.
#warning "определись с типом буфера для char* в ReadMulti::get"
         param.dynamic.varchar = new char[xdb::var_size[type] + 1];
         strncpy(param.dynamic.varchar, item.s_value().c_str(), xdb::var_size[type]);
    break;

    case RTDBM::DB_TYPE_ABSTIME:
        datetime.common = static_cast<uint64_t>(item.i64_value());
        param.fixed.val_time.tv_sec  = datetime.part[0];
        param.fixed.val_time.tv_usec = datetime.part[1];
//        std::cout << "msg_sinf: i64=" << datetime.common
//                << " sec=" << datetime.part[0] << " usec=" << datetime.part[1]
//                << std::endl; //1
    break;

    case RTDBM::DB_TYPE_UNDEF:
      // Не определен тип атрибута - допустимо при ошибке нахождения точки в БДРВ
    break;

    default:
       // Неизвестный тип
       status = false;
       LOG(ERROR)<< ": unsupported '" << tag << "' type " << type; //1
    break;  
  }

  return status;
}

// Вернуть экземпляр Value, предназначенных для хранения данных из БДРВ
// NB: после завершения работ по модификации данных необходимо перенести
// изменения из AttrVal в RTDBM::ValueUpdate
Value& ReadMulti::item(std::size_t idx)
{
  RTDBM::ValueUpdate* pb_updater = 
        static_cast<RTDBM::ReadMulti*>(data()->impl()->instance())->mutable_update_item(idx);

  // Создать новый
  delete m_updater;
  m_updater = new Value(static_cast<void*>(pb_updater));

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

void ReadMulti::set_type(std::size_t idx, xdb::DbType_t type)
{
  RTDBM::ValueUpdate* pb_updater = 
        static_cast<RTDBM::ReadMulti*>(data()->impl()->instance())->mutable_update_item(idx);

  if (RTDBM::DbType_IsValid(type))
    pb_updater->set_type(static_cast<RTDBM::DbType>(type));
  else
    pb_updater->set_type(RTDBM::DB_TYPE_UNDEF);
}

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
  updater->set_quality(RTDBM::ATTR_ERROR);

  switch(type)
  {
    case xdb::DB_TYPE_LOGICAL:
        updater->set_b_value(*(static_cast<bool*>(val)));
    break;
    case xdb::DB_TYPE_INT8:
        updater->set_i32_value(*(static_cast<int8_t*>(val)));
    break;
    case xdb::DB_TYPE_UINT8:
        updater->set_i32_value(*(static_cast<uint8_t*>(val)));
    break;
    case xdb::DB_TYPE_INT16:
        updater->set_i32_value(*(static_cast<int16_t*>(val)));
    break;
    case xdb::DB_TYPE_UINT16:
        updater->set_i32_value(*(static_cast<uint16_t*>(val)));
    break;
    case xdb::DB_TYPE_INT32:
        updater->set_i32_value(*(static_cast<int32_t*>(val)));
    break;
    case xdb::DB_TYPE_UINT32:
        updater->set_i32_value(*(static_cast<uint32_t*>(val)));
    break;
    case xdb::DB_TYPE_INT64:
        updater->set_i64_value(*(static_cast<int64_t*>(val)));
    break;
    case xdb::DB_TYPE_UINT64:
        updater->set_i64_value(*(static_cast<uint64_t*>(val)));
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
        updater->set_s_value(static_cast<char*>(val), xdb::var_size[type]);
    break;

    case xdb::DB_TYPE_LAST:
    case xdb::DB_TYPE_UNDEF:
        // TODO: что делать при попытке сохранения данных неопределенного типа? 
    break;

    default:
      LOG(ERROR)<< ": Unsupported type " << type;
    break;
  }
}

Value& WriteMulti::item(std::size_t idx)
{
  RTDBM::ValueUpdate* pb_updater = 
        static_cast<RTDBM::WriteMulti*>(data()->impl()->instance())->mutable_update_item(idx);

  // Создать новый
  delete m_updater;
  m_updater = new Value(static_cast<void*>(pb_updater));

  return *m_updater;
}

std::size_t WriteMulti::num_items()
{
  return static_cast<RTDBM::WriteMulti*>(data()->impl()->instance())->update_item_size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
 
SubscriptionControl::SubscriptionControl()
 : Letter(SIG_D_MSG_GRPSBS_CTRL, 0), m_updater(NULL)
{
  LOG(INFO) << "On SubscriptionControl()";
}

SubscriptionControl::SubscriptionControl(const std::string& _head, const std::string& _body)
 : Letter(_head, _body), m_updater(NULL)
{
  assert(header()->usr_msg_type() == SIG_D_MSG_GRPSBS_CTRL);
  LOG(INFO) << "SubscriptionControl("<<_head<<", " << _body <<")";
}

SubscriptionControl::SubscriptionControl(Header* _head, const std::string& _body)
 : Letter(_head, _body), m_updater(NULL)
{
  assert(header()->usr_msg_type() == SIG_D_MSG_GRPSBS_CTRL);
  LOG(INFO) << "On SubscriptionControl(header, body)";
}

SubscriptionControl::~SubscriptionControl()
{
  LOG(INFO) << "On ~SubscriptionControl()";
  delete m_updater;
}

// Установить состояние Группе
void SubscriptionControl::set_ctrl(int _code)
{
  static_cast<RTDBM::SubscriptionControl*>(data()->impl()->instance())->set_ctrl(_code);
}

// Вернуть код текущего состояния Группы
int SubscriptionControl::ctrl()
{
  return static_cast<RTDBM::SubscriptionControl*>(data()->impl()->instance())->ctrl();
}

// Установить имя Группы
void SubscriptionControl::set_name(std::string& _name)
{
  static_cast<RTDBM::SubscriptionControl*>(data()->impl()->instance())->set_name(_name);
}

const std::string& SubscriptionControl::name()
{
  return static_cast<RTDBM::SubscriptionControl*>(data()->impl()->instance())->name();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////

SubscriptionEvent::SubscriptionEvent()
 : Letter(SIG_D_MSG_GRPSBS, 0), m_updater(NULL)
{
  LOG(INFO) << "On SubscriptionEvent()";
}

SubscriptionEvent::SubscriptionEvent(const std::string& _head, const std::string& _body)
 : Letter(_head, _body), m_updater(NULL)
{
  assert(header()->usr_msg_type() == SIG_D_MSG_GRPSBS);
  LOG(INFO) << "On SubscriptionEvent(head, body)";
}

SubscriptionEvent::SubscriptionEvent(Header* _head, const std::string& _body)
 : Letter(_head, _body), m_updater(NULL)
{
  assert(header()->usr_msg_type() == SIG_D_MSG_GRPSBS);
  LOG(INFO) << "On SubscriptionEvent(header, body)";
}

SubscriptionEvent::~SubscriptionEvent()
{
  LOG(INFO) << "On ~SubscriptionEvent()";
  delete m_updater;
}

// Добавить в пул еще один тег
void SubscriptionEvent::add(std::string& tag, xdb::DbType_t type, void* val)
{
  RTDBM::ValueUpdate *updater = static_cast<RTDBM::SubscriptionEvent*>(data()->impl()->instance())->add_update_item();
  RTDBM::DbType pb_type;

  assert(val);
//1  std::cerr << "On SubscriptionEvent::add()" << std::endl;

  // NB: Требуется поддерживать синхронность между xdb::DbType_t и RTDBM::DbType (!)
  if (RTDBM::DbType_IsValid(type))
    pb_type = static_cast<RTDBM::DbType>(type);
  else
    pb_type = RTDBM::DB_TYPE_UNDEF;

  updater->set_tag(tag);
  updater->set_type(pb_type);
  updater->set_quality(RTDBM::ATTR_ERROR);

  switch(type)
  {
    case xdb::DB_TYPE_LOGICAL:
        updater->set_b_value(*(static_cast<bool*>(val)));
    break;
    case xdb::DB_TYPE_INT8:
        updater->set_i32_value(*(static_cast<int8_t*>(val)));
    break;
    case xdb::DB_TYPE_UINT8:
        updater->set_i32_value(*(static_cast<uint8_t*>(val)));
    break;
    case xdb::DB_TYPE_INT16:
        updater->set_i32_value(*(static_cast<int16_t*>(val)));
    break;
    case xdb::DB_TYPE_UINT16:
        updater->set_i32_value(*(static_cast<uint16_t*>(val)));
    break;
    case xdb::DB_TYPE_INT32:
        updater->set_i32_value(*(static_cast<int32_t*>(val)));
    break;
    case xdb::DB_TYPE_UINT32:
        updater->set_i32_value(*(static_cast<uint32_t*>(val)));
    break;
    case xdb::DB_TYPE_INT64:
        updater->set_i64_value(*(static_cast<int64_t*>(val)));
    break;
    case xdb::DB_TYPE_UINT64:
        updater->set_i64_value(*(static_cast<uint64_t*>(val)));
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
        updater->set_s_value(static_cast<char*>(val), xdb::var_size[type]);
    break;

    case xdb::DB_TYPE_LAST:
    case xdb::DB_TYPE_UNDEF:
        // TODO: что делать при попытке сохранения данных неопределенного типа? 
    break;

    default:
      LOG(ERROR) << ": Unsupported type " << type;
    break;
  }
}

// Получить значения параметра по заданному индексу
bool SubscriptionEvent::get(std::size_t idx, std::string& tag, xdb::DbType_t& type, xdb::Quality_t& qual, xdb::AttrVal_t& param)
{
  bool status = false;
  xdb::datetime_t datetime;
  const RTDBM::ValueUpdate& item =
        static_cast<RTDBM::SubscriptionEvent*>(data()->impl()->instance())->update_item(idx);

  type = static_cast<xdb::DbType_t>(item.type());
  qual = static_cast<xdb::Quality_t>(item.quality());
  tag = item.tag();

  switch (type)
  {
    case RTDBM::DB_TYPE_LOGICAL:
        param.fixed.val_bool   = static_cast<bool>(item.b_value());
    break;
    case RTDBM::DB_TYPE_INT8:
        param.fixed.val_int8   = static_cast<int8_t>(item.i32_value());
    break;
    case RTDBM::DB_TYPE_UINT8:
        param.fixed.val_uint8  = static_cast<uint8_t>(item.i32_value());
    break;
    case RTDBM::DB_TYPE_INT16:
        param.fixed.val_int16  = static_cast<int16_t>(item.i32_value());
    break;
    case RTDBM::DB_TYPE_UINT16:
        param.fixed.val_uint16 = static_cast<uint16_t>(item.i32_value());
    break;
    case RTDBM::DB_TYPE_INT32:
        param.fixed.val_int32  = static_cast<int32_t>(item.i32_value());
    break;
    case RTDBM::DB_TYPE_UINT32:
        param.fixed.val_uint32 = static_cast<uint32_t>(item.i32_value());
    break;
    case RTDBM::DB_TYPE_INT64:
        param.fixed.val_int64  = static_cast<int64_t>(item.i64_value());
    break;
    case RTDBM::DB_TYPE_UINT64:
        param.fixed.val_uint64 = static_cast<uint64_t>(item.i64_value());
    break;
    case RTDBM::DB_TYPE_FLOAT:
        param.fixed.val_float  = static_cast<float>(item.f_value());
    break;
    case RTDBM::DB_TYPE_DOUBLE:
        param.fixed.val_double = static_cast<double>(item.g_value());
    break;
    case RTDBM::DB_TYPE_BYTES:
        param.dynamic.val_string = new std::string(item.s_value());
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
#warning "определись с типом буфера для char* в SubscriptionEvent::get"
         param.dynamic.varchar = new char[xdb::var_size[type] + 1];
         strncpy(param.dynamic.varchar, item.s_value().c_str(), xdb::var_size[type]);
    break;

    case RTDBM::DB_TYPE_ABSTIME:
        datetime.common = static_cast<uint64_t>(item.i64_value());
        param.fixed.val_time.tv_sec  = datetime.part[0];
        param.fixed.val_time.tv_usec = datetime.part[1];
//        std::cout << "msg_sinf: i64=" << datetime.common
//                << " sec=" << datetime.part[0] << " usec=" << datetime.part[1]
//                << std::endl; //1
    break;

    case RTDBM::DB_TYPE_UNDEF:
      // Не определен тип атрибута - допустимо при ошибке нахождения точки в БДРВ
    break;

    default:
       // Неизвестный тип
       status = false;
       LOG(ERROR) << ": unsupported '" << tag << "' type " << type;
    break;  
  }

  return status;
}

// Получить структуру со значениями параметра с заданным индексом
Value& SubscriptionEvent::item(std::size_t idx)
{
  RTDBM::ValueUpdate* pb_updater = 
        static_cast<RTDBM::SubscriptionEvent*>(data()->impl()->instance())->mutable_update_item(idx);

  // Создать новый
  delete m_updater;
  m_updater = new Value(static_cast<void*>(pb_updater));

  return *m_updater; 
}

// Получить количество элементов в пуле
std::size_t SubscriptionEvent::num_items()
{
  return static_cast<RTDBM::SubscriptionEvent*>(data()->impl()->instance())->update_item_size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////

HistoryRequest::HistoryRequest()
 : Letter(SIG_D_MSG_REQ_HISTORY, 0)
{
  LOG(INFO) << "On HistoryRequest()";
}

HistoryRequest::HistoryRequest(const std::string& _head, const std::string& _body)
 : Letter(_head, _body)
{
  assert(header()->usr_msg_type() == SIG_D_MSG_REQ_HISTORY);
  LOG(INFO) << "On HistoryRequest(head, body)";
}

HistoryRequest::HistoryRequest(Header* _head, const std::string& _body)
 : Letter(_head, _body)
{
  assert(header()->usr_msg_type() == SIG_D_MSG_REQ_HISTORY);
  LOG(INFO) << "On HistoryRequest(header, body)";
}

HistoryRequest::~HistoryRequest()
{
  LOG(INFO) << "On ~HistoryRequest()";
}

// Указать в пул еще один тег
void HistoryRequest::set(std::string& tag, time_t start, int num_req_samples, int history_type)
{
  RTDBM::HistoryType pb_htype = RTDBM::PER_NONE;
  RTDBM::HistoryRequest* pb_request =
        static_cast<RTDBM::HistoryRequest*>(data()->impl()->instance());

  assert(pb_request);

  pb_request->set_tag(tag);

  // NB: синхронизовать со схемой БДРВ
  // NB: Требуется поддерживать синхронность между xdb::DbType_t и RTDBM::DbType (!)
  if (RTDBM::HistoryType_IsValid(history_type))
    pb_htype = static_cast<RTDBM::HistoryType>(history_type);
  else {
    pb_htype = RTDBM::PER_NONE;
    LOG(ERROR) << "Unsupported history depth: " << history_type;
  }
  pb_request->set_history_type(pb_htype);

  pb_request->set_num_required_samples(num_req_samples);

  pb_request->set_time_start(start);
}

// Добавить в пул еще одну запись
void HistoryRequest::add(time_t start, double new_val, int new_validity)
{
  RTDBM::HistoryItem* new_item = NULL;
  RTDBM::HistoryRequest* pb_request =
        static_cast<RTDBM::HistoryRequest*>(data()->impl()->instance());

  new_item = pb_request->add_item();

  if (RTDBM::gof_t_AcqInfoValid_IsValid(new_validity)) {
    new_item->set_valid(static_cast<RTDBM::gof_t_AcqInfoValid>(new_validity));
  }
  else new_item->set_valid(RTDBM::GOF_D_ACQ_INVALID);
  
  new_item->set_value(new_val);

  new_item->set_frame(start);
}

// Установить признак существования запрошенного тега в HDB
void HistoryRequest::set_existance(bool is_exist)
{
  RTDBM::HistoryRequest* pb_request =
        static_cast<RTDBM::HistoryRequest*>(data()->impl()->instance());
  pb_request->set_existance(is_exist);
}

// Получить значения записи с заданным индексом idx
bool HistoryRequest::get(int index, hist_attr_t& out)
{
  bool status;
  RTDBM::HistoryRequest* pb_request =
        static_cast<RTDBM::HistoryRequest*>(data()->impl()->instance());

  if ((0 <= index) && (index < pb_request->item_size())) {
    const RTDBM::HistoryItem& pb_item = pb_request->item(index);
    out.valid = pb_item.valid();
    out.val = pb_item.value();
    out.datehourm = pb_item.frame();
    status = true;
  }
  else {
    out.datehourm = 0;
    out.val = 0;
    out.valid = 0;
    status = false;
  }

  return status;
}

// Вернуть название тега, по которому запрошена предыстория
const std::string& HistoryRequest::tag()
{
  RTDBM::HistoryRequest* pb_request =
        static_cast<RTDBM::HistoryRequest*>(data()->impl()->instance());

  if (!pb_request->has_tag()) {
    LOG(WARNING) << "HistoryRequest: get empty tag";
    assert (1 == 0);
  }

  return pb_request->tag();
}

// Вернуть отметку времени начала периода запрашиваемой предыстории
time_t HistoryRequest::start_time()
{
  return static_cast<RTDBM::HistoryRequest*>(data()->impl()->instance())->time_start();
}

// Вернуть количество семплов запрашиваемой предыстории
int HistoryRequest::num_required_samples()
{
  return static_cast<RTDBM::HistoryRequest*>(data()->impl()->instance())->num_required_samples();
}

xdb::sampler_type_t HistoryRequest::history_type()
{
  int htype_in = static_cast<RTDBM::HistoryRequest*>(data()->impl()->instance())->history_type();
  xdb::sampler_type_t htype_out;

  switch(htype_in) {
    case 1:  htype_out = xdb::PERIOD_1_MINUTE;  break;
    case 2:  htype_out = xdb::PERIOD_5_MINUTES; break;
    case 3:  htype_out = xdb::PERIOD_HOUR;      break;
    case 4:  htype_out = xdb::PERIOD_DAY;       break;
    case 5:  htype_out = xdb::PERIOD_MONTH;     break;

    case 0:  // NB: break пропущен специально
    default: htype_out = xdb::PERIOD_NONE;    break;
  }

  return htype_out;
}

// Признак существования запрошенного тега в HDB (true = существует)
bool HistoryRequest::existance()
{
  return static_cast<RTDBM::HistoryRequest*>(data()->impl()->instance())->existance();
}

// Вернуть количество прочитанных семплов предыстории
int HistoryRequest::num_read_samples()
{
  return static_cast<RTDBM::HistoryRequest*>(data()->impl()->instance())->item_size();
}


