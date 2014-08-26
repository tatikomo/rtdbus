#include <assert.h>
#include <string.h>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_rtap_data.hpp"

using namespace xdb;

RtData::RtData() : m_modified(false)
{
}

RtData::~RtData()
{
}

void RtData::init()
{
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


