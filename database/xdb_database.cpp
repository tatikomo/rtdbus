#include <assert.h>
#include "xdb_database.hpp"
#include "xdb_database_impl.hpp"

XDBDatabase::XDBDatabase(const char* name)
{
    impl = new XDBDatabaseImpl(this, name);
    assert (impl);
    m_state = XDBDatabase::DISCONNECTED;
}

XDBDatabase::~XDBDatabase()
{
    assert (impl);
    impl->Disconnect();
    TransitionToState(DELETED);
    delete impl;
}

const char* XDBDatabase::DatabaseName()
{
    assert (impl);
    return impl->DatabaseName();
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
    bool status;

    assert (impl);
    if (true == (status = impl->Open()))
    {
        status = TransitionToState(OPENED);
    }
    return status;
}

bool XDBDatabase::Connect()
{
    bool status;

    assert (impl);
    if (true == (status = impl->Connect()))
    {
        status = TransitionToState(CONNECTED);
    }
    return status;
}

bool XDBDatabase::Disconnect()
{
    bool status = false;
    
    assert (impl);
    if (TransitionToState(DISCONNECTED))
        status = impl->Disconnect();

    return status;
}

