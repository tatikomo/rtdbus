#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // exit()

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_database.hpp"

XDBDatabase::XDBDatabase(const char* name)
{
    assert (name);
    m_state = XDBDatabase::UNINITIALIZED;
    strncpy(m_name, name, DBNAME_MAXLEN);
    m_name[DBNAME_MAXLEN] = '\0';

//    fprintf(stdout, "\tXDBDatabase(%p, %s)\n", (void*)this, name);
//    fflush(stdout);
}

XDBDatabase::~XDBDatabase()
{
//  fprintf(stdout, "~XDBDatabase(%p, %s)\n", (void*)this, m_name);
  Disconnect();
}

const char* XDBDatabase::DatabaseName()
{
    return m_name;
}

const XDBDatabase::DBState XDBDatabase::State()
{
    return m_state;
}

bool XDBDatabase::TransitionToState(DBState new_state)
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

bool XDBDatabase::Connect()
{
    return TransitionToState(CONNECTED);
}

bool XDBDatabase::Disconnect()
{
    return TransitionToState(DISCONNECTED);
}

