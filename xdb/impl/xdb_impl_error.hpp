#pragma once
#ifndef XDB_IMPL_ERROR_HPP
#define XDB_IMPL_ERROR_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

namespace xdb {

typedef enum
{
  rtE_NONE  = 0,
  rtE_UNKNOWN,
  rtE_NOT_IMPLEMENTED,
  rtE_STRING_TOO_LONG,
  rtE_STRING_IS_EMPTY,
  rtE_DB_NOT_FOUND,
  rtE_DB_NOT_OPENED,
  rtE_DB_NOT_DISCONNECTED,
  rtE_XML_OPEN,
  rtE_XML_IMPORT,
  rtE_XML_EXPORT,
  rtE_INCORRECT_DB_STATE,
  rtE_SNAPSHOT_WRITE,
  rtE_SNAPSHOT_READ,
  rtE_SNAPSHOT_NOT_EXIST,
  rtE_RUNTIME_FATAL,
  rtE_RUNTIME_WARNING,
  rtE_LAST
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

  private:
    void init();
    static bool  m_initialized;
    static char* m_error_descriptions[MaxErrorCode + 1];
    ErrorCode_t  m_error_code;
};

} //namespace xdb

#endif
