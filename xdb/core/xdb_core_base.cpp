#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // exit()

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_broker_base.hpp"

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

bool Database::TransitionToState(DBState new_state)
{
  bool transition_correctness = false;
  /* 
   * TODO проверить допустимость перехода из 
   * старого в новое состояние
   */
  switch (m_state)
  {
    // Состояние "ОТКЛЮЧЕНО" может перейти в "ПОДКЛЮЧЕНО" или "РАЗРУШЕНО"
    case DISCONNECTED:
    {
      switch (new_state)
      {
        case CONNECTED:
        case DELETED:
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

    // Состояние "РАЗРУШЕНО" не может перейти ни в какое другое состояние
    case DELETED:
    {
       transition_correctness = false;
    }
    break;

    // Неинициализированное состояние БД может перейти в "ОТКЛЮЧЕНО" или "РАЗРУШЕНО"
    case UNINITIALIZED:
    {
      switch (new_state)
      {
        case DISCONNECTED:
        case DELETED:
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

