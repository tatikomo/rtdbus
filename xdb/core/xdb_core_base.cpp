#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // exit()

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_core_common.h"
#include "xdb_core_base.hpp"

using namespace xdb;

Database::Database(const char* name) : m_state(DB_STATE_UNINITIALIZED)
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

DBState Database::State() const
{
    return m_state;
}

void Database::setError(ErrorType_t _new_error_code)
{
  m_last_error.set(_new_error_code);
}


/*
 * DB_STATE_UNINITIALIZED = 1, // первоначальное состояние
 * DB_STATE_INITIALIZED   = 2, // инициализирован runtime
 * DB_STATE_ATTACHED      = 3, // вызван mco_db_open
 * DB_STATE_CONNECTED     = 4, // вызван mco_db_connect
 * DB_STATE_DISCONNECTED  = 5, // вызван mco_db_disconnect
 * DB_STATE_CLOSED        = 6  // вызван mco_db_close
 */
const Error& Database::TransitionToState(DBState new_state)
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
    case DB_STATE_DISCONNECTED:
    {
      switch (new_state)
      {
        case DB_STATE_CONNECTED:
        case DB_STATE_CLOSED:
        case DB_STATE_INITIALIZED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }
    }
    break;

    // Состояние "ПОДКЛЮЧЕНО" может перейти в состояние "ОТКЛЮЧЕНО"
    case DB_STATE_CONNECTED:
    {
      switch (new_state)
      {
        case DB_STATE_DISCONNECTED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }
    }
    break;

    // Состояние "ПРИСОЕДИНЕНО" может перейти в состояние "ПОДКЛЮЧЕНО"\"ОТКЛЮЧЕНО"
    case DB_STATE_ATTACHED:
    {
      switch (new_state)
      {
        case DB_STATE_CONNECTED:
        case DB_STATE_DISCONNECTED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }
    }
    break;

    // Состояние "ЗАКРЫТО" не может перейти ни в какое другое состояние
    case DB_STATE_CLOSED:
    {
       transition_correctness = false;
    }
    break;

    case DB_STATE_INITIALIZED:
    {
      switch (new_state)
      {
        case DB_STATE_ATTACHED:
        case DB_STATE_CONNECTED:
        case DB_STATE_DISCONNECTED:
          transition_correctness = true;
        break;
        default:
          transition_correctness = false;
      }

    }
    break;

    // Неинициализированное состояние БД может перейти в "ОТКЛЮЧЕНО" или "ЗАКРЫТО"
    case DB_STATE_UNINITIALIZED:
    {
      switch (new_state)
      {
        case DB_STATE_DISCONNECTED:
        case DB_STATE_INITIALIZED:
        case DB_STATE_CLOSED:
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
    setError(rtE_INCORRECT_DB_TRANSITION_STATE);

  return getLastError();
}

const Error&  Database::Connect()
{
    return TransitionToState(DB_STATE_CONNECTED);
}

const Error&  Database::Disconnect()
{
    return TransitionToState(DB_STATE_DISCONNECTED);
}

const Error&  Database::Init()
{
    return TransitionToState(DB_STATE_INITIALIZED);
}

const Error& Database::Create()
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

const Error& Database::LoadSnapshot(const char*)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

const Error& Database::StoreSnapshot(const char*)
{
  setError(rtE_NOT_IMPLEMENTED);
  return getLastError();
}

