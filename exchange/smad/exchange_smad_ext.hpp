#if !defined EXCHANGE_SMAD_EXTERNAL_HPP
#define EXCHANGE_SMAD_EXTERNAL_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "sqlite3.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"

class ExternalSMAD
{
  public:
    ExternalSMAD(const char*);
   ~ExternalSMAD();

    smad_connection_state_t state() { return m_state; };
    smad_connection_state_t connect();

  private:
    DISALLOW_COPY_AND_ASSIGN(ExternalSMAD);
    sqlite3* m_db;
    smad_connection_state_t  m_state;
    char*    m_db_err;
    char*    m_snapshot_filename;
};

#endif

