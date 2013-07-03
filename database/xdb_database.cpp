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
    m_state = XDBDatabase::DISCONNECTED;
    strncpy(m_name, name, DBNAME_MAXLEN);
    m_name[DBNAME_MAXLEN] = '\0';

    fprintf(stdout, "\tXDBDatabase(%p, %s)\n", (void*)this, name);
    fflush(stdout);
}

XDBDatabase::~XDBDatabase()
{
  fprintf(stdout, "\t~XDBDatabase(%p, %s)\n", (void*)this, m_name);
  if (TransitionToState(DELETED) == true)
  {
     Disconnect();
  }
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
  /* 
   * TODO проверить допустимость перехода из 
   * старого в новое состояние
   */
  if (new_state == m_state)
    return false;
    
  m_state = new_state;
  return true;
}

bool XDBDatabase::Open()
{
    return TransitionToState(OPENED);
}

bool XDBDatabase::Connect()
{
    return TransitionToState(CONNECTED);
}

bool XDBDatabase::Disconnect()
{
    return TransitionToState(DISCONNECTED);
}

