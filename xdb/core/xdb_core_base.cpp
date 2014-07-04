#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // exit()

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_core_base.hpp"

using namespace xdb::core;

Database::Database(const char* name) : m_state(Database::UNINITIALIZED)
{
    assert (name);
    strncpy(m_name, name, DBNAME_MAXLEN);
    m_name[DBNAME_MAXLEN] = '\0';
}

Database::~Database()
{
  Disconnect();
}

const char* Database::DatabaseName() const
{
    return m_name;
}

Database::DBState Database::State() const
{
    return m_state;
}

void Database::setError(xdb::core::ErrorType_t _new_error_code)
{
  m_last_error.set(_new_error_code);
}


/*
 * UNINITIALIZED = 1, // первоначальное состояние
 * INITIALIZED   = 2, // инициализирован runtime
 * ATTACHED      = 3, // вызван mco_db_open
 * CONNECTED     = 4, // вызван mco_db_connect
 * DISCONNECTED  = 5, // вызван mco_db_disconnect
 * CLOSED        = 6  // вызван mco_db_close
 */
const xdb::core::Error& Database::TransitionToState(DBState new_state)
{
  bool transition_correctness = false;

  clearError();
  /* 
   * TODO проверить допустимость перехода из 
   * старого в новое состояние
   */
  switch (m_state)
  {
    // Состояние "ОТКЛЮЧЕНО" может перейти в "ПОДКЛЮЧЕНО" или "ЗАКРЫТО"
    case DISCONNECTED:
    {
      switch (new_state)
      {
        case CONNECTED:
        case CLOSED:
        case INITIALIZED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }
    }
    break;

    // Состояние "ПОДКЛЮЧЕНО" может перейти в состояние "ОТКЛЮЧЕНО"
    case CONNECTED:
    {
      switch (new_state)
      {
        case DISCONNECTED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }
    }
    break;

    // Состояние "ПРИСОЕДИНЕНО" может перейти в состояние "ПОДКЛЮЧЕНО"\"ОТКЛЮЧЕНО"
    case ATTACHED:
    {
      switch (new_state)
      {
        case CONNECTED:
        case DISCONNECTED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }
    }
    break;

    // Состояние "ЗАКРЫТО" не может перейти ни в какое другое состояние
    case CLOSED:
    {
       transition_correctness = false;
    }
    break;

    case INITIALIZED:
    {
      switch (new_state)
      {
        case ATTACHED:
        case CONNECTED:
        case DISCONNECTED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }

    }
    break;

    // Неинициализированное состояние БД может перейти в "ОТКЛЮЧЕНО" или "ЗАКРЫТО"
    case UNINITIALIZED:
    {
      switch (new_state)
      {
        case DISCONNECTED:
        case INITIALIZED:
        case CLOSED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }
    }
    break;
  }

  if (transition_correctness)
    m_state = new_state;
  else
    setError(xdb::core::rtE_INCORRECT_DB_TRANSITION_STATE);

  return getLastError();
}

const xdb::core::Error&  Database::Connect()
{
    return TransitionToState(CONNECTED);
}

const xdb::core::Error&  Database::Disconnect()
{
    return TransitionToState(DISCONNECTED);
}

const xdb::core::Error&  Database::Init()
{
    return TransitionToState(INITIALIZED);
}

const xdb::core::Error& Database::Create()
{
  setError(xdb::core::rtE_NOT_IMPLEMENTED);
  return getLastError();
}

const xdb::core::Error& Database::LoadSnapshot(const char*)
{
  setError(xdb::core::rtE_NOT_IMPLEMENTED);
  return getLastError();
}

const xdb::core::Error& Database::StoreSnapshot(const char*)
{
  setError(xdb::core::rtE_NOT_IMPLEMENTED);
  return getLastError();
}

