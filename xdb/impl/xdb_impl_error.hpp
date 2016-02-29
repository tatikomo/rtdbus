#pragma once
#ifndef XDB_IMPL_ERROR_HPP
#define XDB_IMPL_ERROR_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

namespace xdb {

typedef enum
{
  rtE_NONE              = 0,
  rtE_UNKNOWN           = 1,
  rtE_NOT_IMPLEMENTED   = 2,
  rtE_STRING_TOO_LONG   = 3,
  rtE_STRING_IS_EMPTY   = 4,
  rtE_DB_NOT_FOUND      = 5,
  rtE_DB_NOT_OPENED     = 6,
  rtE_DB_NOT_DISCONNECTED   = 7,
  rtE_XML_OPEN          = 8,
  rtE_XML_IMPORT        = 9,
  rtE_XML_EXPORT        = 10,
  rtE_INCORRECT_DB_STATE= 11,
  rtE_SNAPSHOT_WRITE    = 12,
  rtE_SNAPSHOT_READ     = 13,
  rtE_SNAPSHOT_NOT_EXIST= 14,
  rtE_RUNTIME_FATAL     = 15,
  rtE_RUNTIME_ERROR     = 16,
  rtE_RUNTIME_WARNING   = 17,
  rtE_TABLE_CREATE      = 18,
  rtE_TABLE_READ        = 19,
  rtE_TABLE_WRITE       = 20,
  rtE_TABLE_DELETE      = 21,
  rtE_POINT_CREATE      = 22,
  rtE_POINT_READ        = 23,
  rtE_POINT_WRITE       = 24,
  rtE_POINT_DELETE      = 25,
  rtE_ILLEGAL_PARAMETER_VALUE       = 26,
  rtE_POINT_NOT_FOUND   = 27,
  rtE_ATTR_NOT_FOUND    = 28,
  rtE_ILLEGAL_TAG_NAME  = 29,
  rtE_ALREADY_EXIST     = 30,
  rtE_CONNECTION_INVALID= 31,
  rtE_DATABASE_INSTANCE_DUPLICATE   = 32,
  rtE_LAST              = 33
} ErrorCode_t;

class Error
{
  public:
    static const int MaxErrorCode = rtE_LAST;
    // Пустая инициализация
    Error();
    // Инициализация по образцу
    Error(const Error&);
    // Инициализация по коду
    Error(ErrorCode_t);

    ~Error();
    // Получить код ошибки
    int getCode() const;
    ErrorCode_t code() const { return m_error_code; };
    // Установить код ошибки
    void set(ErrorCode_t);
    // Установить код ошибки
    void set(const Error& _err) { m_error_code = _err.m_error_code; };
    // true, если не было ошибки
    bool Ok() const { return m_error_code == rtE_NONE; };
    // true, если была ошибка
    bool NOk() const { return m_error_code != rtE_NONE; };

    // Сбросить код ошибки
    void clear() { m_error_code = rtE_NONE; };
    // Получить символьное описание ошибки
    const char* what() const;
    static const char* what(int);

  private:
    ErrorCode_t  m_error_code;
};

} //namespace xdb

#endif
