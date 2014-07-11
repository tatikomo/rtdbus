#pragma once
#if !defined XDB_CORE_ERROR_HPP
#define XDB_CORE_ERROR_HPP

#include "config.h"

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
  rtE_XML_NOT_OPENED,
  rtE_INCORRECT_DB_TRANSITION_STATE,
  rtE_SNAPSHOT_WRITE,
  rtE_SNAPSHOT_READ,
  rtE_RUNTIME_FATAL,
  rtE_RUNTIME_WARNING,
  rtE_LAST
} ErrorType_t;

class Error
{
  public:
    static const int MaxErrorCode = rtE_LAST;
    // Пустая инициализация
    Error();
    // Инициализация по образцу
    Error(const Error&);
    // Инициализация по коду
    Error(ErrorType_t);

    ~Error();
    // Получить код ошибки
    int getCode() const;
    // Установить код ошибки
    void set(ErrorType_t);
    // Установить код ошибки
    void set(const Error& _err) { m_error_type = _err.getCode(); };

    // true, если не было ошибки
    bool Ok() const { return m_error_type == rtE_NONE; };
    // true, если была ошибка
    bool NOk() const { return m_error_type != rtE_NONE; };

    // Сбросить код ошибки
    void clear() { m_error_type = rtE_NONE; };
    // Получить символьное описание ошибки
    const char* what() const;

  private:
    void init();
    static bool  m_initialized;
    static char* m_error_descriptions[MaxErrorCode + 1];
    int          m_error_type;
};

} //namespace xdb

#endif
