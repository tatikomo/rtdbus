#include <assert.h>
#include <string.h>

#include "config.h"
#include "xdb_rtap_error.hpp"
#include "xdb_rtap_data.hpp"

using namespace xdb;

const char* RtData::DbTypeDescription[DB_TYPE_LAST];
bool RtData::m_initialized = false;

RtData::RtData() : m_modified(false)
{
  init();
}

RtData::~RtData()
{
}

void RtData::init()
{
  if (!m_initialized)
  {
    m_initialized = true;
    DbTypeDescription[DB_TYPE_UNDEF]       = "undef";
    DbTypeDescription[DB_TYPE_INTEGER8]    = "int8";
    DbTypeDescription[DB_TYPE_INTEGER16]   = "int16";
    DbTypeDescription[DB_TYPE_INTEGER32]   = "int32";
    DbTypeDescription[DB_TYPE_INTEGER64]   = "int64";
    DbTypeDescription[DB_TYPE_FLOAT]       = "float";
    DbTypeDescription[DB_TYPE_DOUBLE]      = "double";
    DbTypeDescription[DB_TYPE_BYTES]       = "bytes";
  }

  m_modified = false;
}

uint8_t  RtData::getInt8Value() const
{
  return m_attr_value.val_int8;
}

uint16_t  RtData::getInt16Value() const
{
  return m_attr_value.val_int16;
}

uint32_t  RtData::getInt32Value() const
{
  return m_attr_value.val_int32;
}

float  RtData::getFloatValue() const
{
  return m_attr_value.val_float;
}

double  RtData::getDoubleValue() const
{
  return m_attr_value.val_double;
}

char* RtData::getBytesValue() const
{
  return m_attr_value.val_bytes.data;
}

void RtData::setValue(uint8_t _val)
{
  m_attr_value.val_int8 = static_cast<int8_t>(_val);
  m_modified = true;
}

void RtData::setValue(int8_t _val)
{
  m_attr_value.val_int8 = _val;
  m_modified = true;
}

void RtData::setValue(uint16_t _val)
{
  m_attr_value.val_int16 = static_cast<int16_t>(_val);
  m_modified = true;
}

void RtData::setValue(int16_t _val)
{
  m_attr_value.val_int16 = _val;
  m_modified = true;
}

void RtData::setValue(uint32_t _val)
{
  m_attr_value.val_int32 = static_cast<int32_t>(_val);
  m_modified = true;
}

void RtData::setValue(int32_t _val)
{
  m_attr_value.val_int32 = _val;
  m_modified = true;
}

void RtData::setValue(float _val)
{
  m_attr_value.val_float = _val;
  m_modified = true;
}

void RtData::setValue(double _val)
{
  m_attr_value.val_double = _val;
  m_modified = true;
}

void RtData::setValue(char* _val)
{
  assert(_val);

  m_attr_value.val_bytes.data = _val;
  m_attr_value.val_bytes.size = strlen(_val);
  m_modified = true;
}

void RtData::setValue(variable_t _val)
{
  m_attr_value.val_bytes.data = _val.data;
  m_attr_value.val_bytes.size = _val.size;
  m_modified = true;
}


