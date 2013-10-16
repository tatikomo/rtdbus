#if !defined XDB_RTAP_ERROR_H_
#define XDB_RTAP_ERROR_H_
#pragma once

#include "config.h"

namespace xdb
{

typedef enum
{
  rtE_NONE  = 0,
  rtE_UNKNOWN,
  rtE_NOT_IMPLEMENTED,
  rtE_STRING_TOO_LONG,
  rtE_STRING_IS_EMPTY,
  rtE_LAST
} ErrorType_t;

class RtError
{
  public:
    static const int MaxErrorCode = rtE_LAST;
    // Пустая инициализация
    RtError();
    // Инициализация по образцу
    RtError(const RtError&);
    // Инициализация по коду
    RtError(ErrorType_t);
    ~RtError();
    // Получить код ошибки
    ErrorType_t getCode() const;
    // Установить код ошибки
    bool set(ErrorType_t);
    // Получить символьное описание ошибки
    const char* what() const;

  private:
    void init();
    static const char* m_error_descriptions[MaxErrorCode + 1];
    ErrorType_t m_error_type;
};

}

#endif
