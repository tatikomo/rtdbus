#pragma once
#if !defined XDB_CORE_ATTRIBUTE_HPP
#define XDB_CORE_ATTRIBUTE_HPP

#include <string>
#include <map>

#include <stdint.h>

#include "config.h"
#include "xdb_core_error.hpp"

namespace xdb {

#if 0
typedef char   shortlabel_t[SHORTLABEL_LENGTH+1];
typedef char   longlabel_t[LABEL_LENGTH+1];
typedef char   univname_t[UNIVNAME_LENGTH+1];
typedef char   code_t[CODE_LENGTH+1];
#else
typedef std::string   shortlabel_t;
typedef std::string   longlabel_t;
typedef std::string   univname_t;
typedef std::string   code_t;
#endif

// NB: формат хранения строк UTF-8
typedef struct
{
  uint16_t size; // 16384
  char *data;
} variable_t;

typedef union
{
  int8_t   val_int8;
  uint8_t  val_uint8;
  int16_t  val_int16;
  uint16_t val_uint16;
  int32_t  val_int32;
  uint32_t val_uint32;
  int64_t  val_int64;
  uint64_t val_uint64;
  variable_t val_bytes; // NB: максимальное значение строки =16384
  float    val_float;
  double   val_double;
} AttrVal_t;

// Коды элементарных типов данных eXtremeDB, которые мы используем
typedef enum
{
  DB_TYPE_UNDEF     = 0,
  DB_TYPE_INT8      = 1,
  DB_TYPE_UINT8     = 2,
  DB_TYPE_INT16     = 3,
  DB_TYPE_UINT16    = 4,
  DB_TYPE_INT32     = 5,
  DB_TYPE_UINT32    = 6,
  DB_TYPE_INT64     = 7,
  DB_TYPE_UINT64    = 8,
  DB_TYPE_FLOAT     = 9,
  DB_TYPE_DOUBLE    = 10,
  DB_TYPE_BYTES     = 11, // переменная длина строки
  DB_TYPE_BYTES4    = 12,
  DB_TYPE_BYTES8    = 13,
  DB_TYPE_BYTES12   = 14,
  DB_TYPE_BYTES16   = 15,
  DB_TYPE_BYTES20   = 16,
  DB_TYPE_BYTES32   = 17,
  DB_TYPE_BYTES48   = 18,
  DB_TYPE_BYTES64   = 19,
  DB_TYPE_BYTES80   = 20,
  DB_TYPE_BYTES128  = 21,
  DB_TYPE_BYTES256  = 22,
  DB_TYPE_LAST      = 23 // fake type, used for limit array types
} DbType_t;

/* перечень значимых атрибутов и их типов */
typedef struct
{
  univname_t name;      /* имя атрибута */
  DbType_t   db_type;   /* его тип - целое, дробь, строка */
  AttrVal_t  value;     /* значение атрибута */
} AttributeInfo_t;

typedef std::map  <const std::string, AttributeInfo_t> AttributeMap_t;
typedef std::map  <const std::string, AttributeInfo_t>::iterator AttributeMapIterator_t;
typedef std::pair <const std::string, AttributeInfo_t> AttributeMapPair_t;

class DbConnection;
class Data;
class Point;

class Attribute
{
  public:
    Attribute();
    ~Attribute();
    // Вернуть алиас атрибута
    const char* getAlias() const;
    // Вернуть тип атрибута
    DbType_t    getAttributeType() const;
    // Вернуть объект-подключение
    DbConnection* getConnection();
    // Вернуть определение СЕ
    std::string getDefinition() const;
    //
    int getDeType();
    // Получить point-представление точки, к которой 
    // принадлежит этот атрибут
    Point* getPoint();
    //
    const char* getPointAlias() const;
    //
    const char* getPointName() const;
    // Прочитать значение экземпляра
    Data* read();
    // Записать значение экземпляра
    const Error& write(Data&);

  private:
    DISALLOW_COPY_AND_ASSIGN(Attribute);
    Error     m_last_error;
    std::string m_univname;
    std::string m_alias;
    DbType_t    m_type;
};

} //namespace xdb

#endif
