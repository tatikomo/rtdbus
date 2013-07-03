#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // exit()

#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"
#include "xdb_database_common.h"

#ifdef __cplusplus
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

    fprintf(stdout, "\tXDBDatabaseImpl(%p, %s)\n", (void*)self, name);
    fflush(stdout);
}

XDBDatabaseImpl::~XDBDatabaseImpl()
{
    fprintf(stdout, "\t~XDBDatabaseImpl(%p, %s)\n", (void*)m_self, m_name);
    if (false == Disconnect())
      fprintf(stdout, "disconnecting failure\n");
    fflush(stdout);
}

const char* XDBDatabaseImpl::DatabaseName()
{
    return m_name;
}

bool XDBDatabaseImpl::Connect()
{
    bool status = false;
    MCO_RET rc;
    mco_runtime_info_t info;

    mco_get_runtime_info(&info);
    if (!info.mco_shm_supported)
    {
      fprintf(stdout, "\nThis program requires shared memory database runtime\n");
      return false;
    }

    /* Set the error handler to be called from the eXtremeDB runtime if a fatal error occurs */
    mco_error_set_handler(&errhandler);
    /* Set the error handler to be called from the eXtremeDB runtime if a fatal error occurs */
    mco_error_set_handler_ex(&extended_errhandler);

    fprintf(stdout, "\n\tUser-defined error handler set\n");
    show_runtime_info("GEV");

    rc = mco_runtime_start();
    rc_check("Runtime starting", rc);
    if (!rc)
      status = true;

    return status;
}

bool XDBDatabaseImpl::Open()
{
    bool status = false;

    fprintf(stdout, "XDBDatabaseImpl Open()\n");
    return status;
}

bool XDBDatabaseImpl::Disconnect()
{
    MCO_RET rc;
    bool result = false;

    rc = mco_runtime_stop();
    rc_check("Runtime stop", rc);
    if (!rc)
      result = true;

    return result;
}

