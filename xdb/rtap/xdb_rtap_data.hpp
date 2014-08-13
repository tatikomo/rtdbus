#pragma once
#ifndef XDB_RTAP_DATA_HPP
#define XDB_RTAP_DATA_HPP

#include <string>
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_impl_error.hpp"
#include "xdb_rtap_const.hpp"

namespace xdb {

class RtData
{
   public:
     RtData();
     ~RtData();

     // Returns a string describing the given data element type
     const char* deTypeToString(DbType_t);
     //
     uint8_t  getInt8Value() const;
     //
     uint16_t getInt16Value() const;
     //
     uint32_t getInt32Value() const;
     //
     float    getFloatValue() const;
     //
     double   getDoubleValue() const;
     // Предоставить доступ к внутреннему значению как к массиву байт
     char* getBytesValue() const;
     //
     DbType_t getDeType() const;
     //
     void setValue(uint8_t);
     void setValue(int8_t);
     void setValue(uint16_t);
     void setValue(int16_t);
     void setValue(uint32_t);
     void setValue(int32_t);
     void setValue(float);
     void setValue(double);
     void setValue(char*);
     void setValue(variable_t);

     const Error& getLastError() const { return m_last_error; }

   private:
     DISALLOW_COPY_AND_ASSIGN(RtData);
     void       init();
     Error      m_last_error;
     DbType_t   m_attr_type;
     AttrVal_t  m_attr_value;
     bool       m_modified;
};

} // namespace xdb

#endif

