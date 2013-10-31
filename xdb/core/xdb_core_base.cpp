#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // exit()

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_core_base.hpp"

using namespace xdb;

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

/*
 * UNINITIALIZED = 1, // первоначальное состояние
 * INITIALIZED   = 2, // инициализирован runtime
 * ATTACHED      = 3, // вызван mco_db_open
 * CONNECTED     = 4, // вызван mco_db_connect
 * DISCONNECTED  = 5, // вызван mco_db_disconnect
 * CLOSED        = 6  // вызван mco_db_close
 */
bool Database::TransitionToState(DBState new_state)
{
  bool transition_correctness = false;
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

  return transition_correctness;
}

bool Database::Connect()
{
    return TransitionToState(CONNECTED);
}

bool Database::Disconnect()
{
    return TransitionToState(DISCONNECTED);
}

bool Database::Init()
{
    return TransitionToState(INITIALIZED);
}

