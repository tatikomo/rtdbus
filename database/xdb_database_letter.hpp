#if !defined XDB_DATABASE_LETTER_HPP
#define XDB_DATABASE_LETTER_HPP
#pragma once

#include <stdint.h>

#include "xdb_database_broker.hpp"

class Service;
class Worker;

/* Хранимое в БД сообщение Клиента Сервису */
class Letter
{
  public:
    Letter(int64_t, int64_t);
    const int64_t GetWORKER_ID();
    const int64_t GetSERVICE_ID();

  private:
    int64_t m_WorkerId;
    int64_t m_ServiceId;
};

#endif

