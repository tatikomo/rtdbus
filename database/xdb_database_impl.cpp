#include <assert.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "xdb_database.hpp"
#include "xdb_database_impl.hpp"

XDBDatabaseImpl::XDBDatabaseImpl(const XDBDatabase* self, const char* name)
{
    assert (self);
    assert (name);
    m_self = self;
    strncpy(m_name, name, DATABASE_NAME_MAXLEN);
    m_name[DATABASE_NAME_MAXLEN] = '\0';
    m_state = XDBDatabase::DISCONNECTED;

    printf("\tXDBDatabaseImpl(%p, %s)\n", (void*)self, name);
}

XDBDatabaseImpl::~XDBDatabaseImpl()
{
    printf("\t~XDBDatabaseImpl(%p, %s)\n", (void*)m_self, m_name);
    if (false == Disconnect())
      printf("disconnecting failure\n");

    m_state = XDBDatabase::DELETED;
}

const char* XDBDatabaseImpl::DatabaseName()
{
    return m_name;
}

const XDBDatabase::DBState XDBDatabaseImpl::State()
{
    return m_state;
}

bool XDBDatabaseImpl::Connect()
{
    bool status = false;
    MCO_RET rc;
    mco_runtime_info_t info;

    mco_get_runtime_info(&info);
    if (!info.mco_shm_supported)
    {
      printf("\nThis program requires shared memory database runtime\n");
      return false;
    }

    rc = mco_runtime_start();
    if (rc)
    {
      printf("runtime starting failure: %d\n", rc);
      return false;
    }

    status = true;
    m_state = XDBDatabase::CONNECTED;
    return status;
}

bool XDBDatabaseImpl::Open()
{
    bool status = false;

    printf("XDBDatabaseImpl Open()\n");
//    m_state = XDBDatabase::OPENED;
    return status;
}

bool XDBDatabaseImpl::Disconnect()
{
    MCO_RET rc;

    rc = mco_runtime_stop();
    if (rc)
    {
        printf("\nCould not stop runtime: %d\n", rc);
    }

    m_state = XDBDatabase::DISCONNECTED;
    return true;
}

