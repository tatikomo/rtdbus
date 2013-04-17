#include <assert.h>
#include "xdb_database.hpp"
#include "xdb_database_impl.hpp"

XDBDatabase::XDBDatabase(const char* name)
{
    impl = new XDBDatabaseImpl(this, name);
    assert (impl);
}

XDBDatabase::~XDBDatabase()
{
    assert (impl);
    impl->Disconnect();
    delete impl;
}

const char* XDBDatabase::DatabaseName()
{
    assert (impl);
    return impl->DatabaseName();
}

const XDBDatabase::DBState XDBDatabase::State()
{
    assert (impl);
    return impl->State();
}

bool XDBDatabase::Open()
{
    assert (impl);
    return impl->Connect();
}

bool XDBDatabase::Connect()
{
    assert (impl);
    return impl->Connect();
}

bool XDBDatabase::Disconnect()
{
    assert (impl);
    return impl->Disconnect();
}

