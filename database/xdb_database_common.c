#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"

#ifdef __cplusplus
}
#endif

#include "xdb_database_common.h"

/* implement error handler */
void errhandler(MCO_RET n)
{
    fprintf(stdout, "\neXtremeDB runtime fatal error: %d\n", n);
    exit(-1);
}

/* implement error handler */
void extended_errhandler(MCO_RET errcode, const char* file, int line)
{
  fprintf(stdout, "\neXtremeDB runtime fatal error: %d on line %d of file %s",
          errcode, line, file);
  exit(-1);
}

const char * mco_ret_string( MCO_RET rc, int * is_error ) {
    
  if ( is_error ) {
    (*is_error) = rc >= MCO_E_CORE;
  }

  switch ( rc ) {
    case MCO_S_OK:                        return "MCO_S_OK - Operation succeeded";
    case MCO_S_BUSY:                      return "MCO_S_BUSY - The instance is busy";
    case MCO_S_NOTFOUND:                  return "MCO_S_NOTFOUND - Lookup operation failed";
    case MCO_S_CURSOR_END:                return "MCO_S_CURSOR_END - Cursor cannot be moved";
    case MCO_S_CURSOR_EMPTY:              return "MCO_S_CURSOR_EMPTY - No objects in cursor";
    case MCO_S_DUPLICATE:                 return "MCO_S_DUPLICATE - Index restriction violated (duplicate)";
    case MCO_S_EVENT_RELEASED:            return "MCO_S_EVENT_RELEASED - Waiting thread was released";
    case MCO_S_DEAD_CONNECTION:           return "MCO_S_DEAD_CONNECTION - Database connection is invalid";

    case MCO_E_CORE:                      return "MCO_E_CORE - Core error";
    case MCO_E_INVALID_HANDLE:            return "MCO_E_INVALID_HANDLE - Invalid handle detected";
    case MCO_E_NOMEM:                     return "MCO_E_NOMEM - Not enough memory";
    case MCO_E_ACCESS:                    return "MCO_E_ACCESS - Transaction access level violation";
    case MCO_E_TRANSACT:                  return "MCO_E_TRANSACT - Transaction is in an error state";
    case MCO_E_INDEXLIMIT:                return "MCO_E_INDEXLIMIT - Vector index out of bounds";
    case MCO_E_EMPTYVECTOREL:             return "MCO_E_EMPTYVECTOREL - Vector element was not set (at given index)";
    case MCO_E_UNSUPPORTED:               return "MCO_E_UNSUPPORTED - Unsupported call";
    case MCO_E_EMPTYOPTIONAL:             return "MCO_E_EMPTYOPTIONAL - Optional structure has not been set";
    case MCO_E_EMPTYBLOB:                 return "MCO_E_EMPTYBLOB - The blob has not been set";
    case MCO_E_CURSOR_INVALID:            return "MCO_E_CURSOR_INVALID - The cursor is not valid";
    case MCO_E_ILLEGAL_TYPE:              return "MCO_E_ILLEGAL_TYPE - Type is not expected";
    case MCO_E_ILLEGAL_PARAM:             return "MCO_E_ILLEGAL_PARAM - Value is not expected";
    case MCO_E_CURSOR_MISMATCH:           return "MCO_E_CURSOR_MISMATCH - Cursor type and object type are incompatible";
    case MCO_E_DELETED:                   return "MCO_E_DELETED - The object has been deleted in the current transaction";
    case MCO_E_LONG_TRANSACTON:           return "MCO_E_LONG_TRANSACTON - The current transaction is too long";
    case MCO_E_INSTANCE_DUPLICATE:        return "MCO_E_INSTANCE_DUPLICATE - Duplicate database instance";
    case MCO_E_UPGRADE_FAILED:            return "MCO_E_UPGRADE_FAILED - Transaction upgrade failed; write transaction in progress";
    case MCO_E_NOINSTANCE:                return "MCO_E_NOINSTANCE - Database instance was not found";
    case MCO_E_OPENED_SESSIONS:           return "MCO_E_OPENED_SESSIONS - Failed to close database - it has open connections";
    case MCO_E_PAGESIZE:                  return "MCO_E_PAGESIZE - Page size is not acceptable";
    case MCO_E_WRITE_STREAM:              return "MCO_E_WRITE_STREAM - Write stream failure";
    case MCO_E_READ_STREAM:               return "MCO_E_READ_STREAM - Read stream failure";
    case MCO_E_LOAD_DICT:                 return "MCO_E_LOAD_DICT - Incompatible dictionary";
    case MCO_E_LOAD_DATA:                 return "MCO_E_LOAD_DATA - Corrupted image";
    case MCO_E_VERS_MISMATCH:             return "MCO_E_VERS_MISMATCH - Version mismatch";
    case MCO_E_VOLUNTARY_NOT_EXIST:       return "MCO_E_VOLUNTARY_NOT_EXIST - Voluntary index is not created";
    case MCO_E_EXCLUSIVE_MODE:            return "MCO_E_EXCLUSIVE_MODE - Database is open in exclusive mode";
    case MCO_E_MAXEXTENDS:                return "MCO_E_MAXEXTENDS - Maximum number of extends reached";
    case MCO_E_HIST_OBJECT:               return "MCO_E_HIST_OBJECT - Operation is illegal for old version of the object";
    case MCO_E_SHM_ERROR:                 return "MCO_E_SHM_ERROR - Failed to create/attach to shared memory";
    case MCO_E_NOTINIT:                   return "MCO_E_NOTINIT - Runtime was not initialized";
    case MCO_E_SESLIMIT:                  return "MCO_E_SESLIMIT - Sessions number limit reached";
    case MCO_E_INSTANCES_LIMIT:           return "MCO_E_INSTANCES_LIMIT - Too many instances";
    case MCO_E_MAXTRANSSIZE_LOCKED:       return "MCO_E_MAXTRANSSIZE_LOCKED - The maximum transaction size cannot be changed";
#if (EXTREMEDB_VERSION >= 41)
    case MCO_E_DEPRECATED:                return "MCO_E_DEPRECATED - Obsolete feature";
#endif
    case MCO_E_NOUSERDEF_FUNCS:           return "MCO_E_NOUSERDEF_FUNCS - User-defined index functions have not been registered";
#if (EXTREMEDB_VERSION >= 41)
    case MCO_E_CONFLICT:                  return "MCO_E_CONFLICT - MVCC conflict";
#endif
    case MCO_E_INMEM_ONLY_RUNTIME:        return "MCO_E_INMEM_ONLY_RUNTIME - In-memory only runtime linked";
#if (EXTREMEDB_VERSION >= 41)
    case MCO_E_ISOLATION_LEVEL_NOT_SUPPORTED:
                                          return "MCO_E_ISOLATION_LEVEL_NOT_SUPPORTED - Requested isolation level not supported";
    case MCO_E_REGISTRY_UNABLE_CREATE_CONNECT:
                                          return "MCO_E_REGISTRY_UNABLE_CREATE_CONNECT - Unable to make new registry";
    case MCO_E_REGISTRY_UNABLE_CONNECT:   return "MCO_E_REGISTRY_UNABLE_CONNECT - Unable to connect to existing registry";
    case MCO_E_REGISTRY_INVALID_SYNC:
                                              return "MCO_E_REGISTRY_INVALID_SYNC - Invalid sync detected in the registry";
    case MCO_E_CONVERSION:                return "MCO_E_CONVERSION - Conversion error in binary schema evolution";
#endif
/*  case MCO_E_NO_APPROPRIATE_INDEX:      return "MCO_E_NO_APPROPRIATE_INDEX - No appropriate index for the operation"; */
    case MCO_E_DISK:                      return "MCO_E_DISK - General disk error";
    case MCO_E_DISK_OPEN:                 return "MCO_E_DISK_OPEN - Unable to open persistent storage";
    case MCO_E_DISK_ALREADY_OPENED:       return "MCO_E_DISK_ALREADY_OPENED - Persistent storage already opened";
    case MCO_E_DISK_NOT_OPENED:           return "MCO_E_DISK_NOT_OPENED - Persistent storage not opened";
    case MCO_E_DISK_INVALID_PARAM:        return "MCO_E_DISK_INVALID_PARAM - Invalid disk parameters value";
    case MCO_E_DISK_PAGE_ACCESS:          return "MCO_E_DISK_PAGE_ACCESS - Disk Page Access";
    case MCO_E_DISK_OPERATION_NOT_ALLOWED:return "MCO_E_DISK_OPERATION_NOT_ALLOWED - Operation is not allowed";
    case MCO_E_DISK_ALREADY_CONNECTED:    return "MCO_E_DISK_ALREADY_CONNECTED - Persistent storage already connected";
    case MCO_E_DISK_KEY_TOO_LONG:         return "MCO_E_DISK_KEY_TOO_LONG - Index key too long";
    case MCO_E_DISK_TOO_MANY_INDICES:     return "MCO_E_DISK_TOO_MANY_INDICES - Too many indexes in persistent classes";
    case MCO_E_DISK_TOO_MANY_CLASSES:     return "MCO_E_DISK_TOO_MANY_CLASSES - Too many persistent classes";
    case MCO_E_DISK_SPACE_EXHAUSTED:      return "MCO_E_DISK_SPACE_EXHAUSTED - Persistent storage is out of space";
    case MCO_E_DISK_INCOMPATIBLE_LOG_TYPE:return "MCO_E_DISK_INCOMPATIBLE_LOG_TYPE - Incompatible database log type";
    case MCO_E_DISK_BAD_PAGE_SIZE:        return "MCO_E_DISK_BAD_PAGE_SIZE - Page size is not acceptable";
    case MCO_E_DISK_SYNC:                 return "MCO_E_DISK_SYNC - Faild operation on sync. primitive";
    case MCO_E_DISK_PAGE_POOL_EXHAUSTED:  return "MCO_E_DISK_PAGE_POOL_EXHAUSTED - Too many pinned disk pages";
    case MCO_E_DISK_CLOSE:                return "MCO_E_DISK_CLOSE - Error closing persistent storage";
    case MCO_E_DISK_TRUNCATE:             return "MCO_E_DISK_TRUNCATE - Unable to truncate persistent storage";
    case MCO_E_DISK_SEEK:                 return "MCO_E_DISK_SEEK - Unable to seek in persistent storage";
    case MCO_E_DISK_WRITE:                return "MCO_E_DISK_WRITE - Unable to write to persistent storage";
    case MCO_E_DISK_READ:                 return "MCO_E_DISK_READ - Unable to read from persistent storage";
    case MCO_E_DISK_FLUSH:                return "MCO_E_DISK_FLUSH - Unable to flush persistent storage";
    case MCO_E_DISK_TOO_HIGH_TREE:        return "MCO_E_DISK_TOO_HIGH_TREE - Index too big";
#if (EXTREMEDB_VERSION >= 41)
    case MCO_E_DISK_VERSION_MISMATCH:     return "MCO_E_DISK_VERSION_MISMATCH - version mismatch";
    case MCO_E_DISK_CONFLICT:             return "MCO_E_DISK_CONFLICT - MVCC conflict";
    case MCO_E_DISK_SCHEMA_CHANGED:       return "MCO_E_DISK_SCHEMA_CHANGED - database schema was changed";
    case MCO_E_DISK_CRC_MISMATCH:         return "MCO_E_DISK_CRC_MISMATCH - CRC is not matched for the loaded page";
#endif
    case MCO_E_XML:                       return "MCO_E_XML - General XML error";
    case MCO_E_XML_INVINT:                return "MCO_E_XML_INVINT - Invalid integer";
    case MCO_E_XML_INVFLT:                return "MCO_E_XML_INVFLT - Invalid float";
    case MCO_E_XML_INTOVF:                return "MCO_E_XML_INTOVF - Integer overflow";
    case MCO_E_XML_INVBASE:               return "MCO_E_XML_INVBASE - Invalid base for quad (10)";
    case MCO_E_XML_BUFSMALL:              return "MCO_E_XML_BUFSMALL - Buffer too small for double in fixed point format";
    case MCO_E_XML_VECTUNSUP:             return "MCO_E_XML_VECTUNSUP - Unsupported base type for vector";
    case MCO_E_XML_INVPOLICY:             return "MCO_E_XML_INVPOLICY - Invalid XML policy value";
    case MCO_E_XML_INVCLASS:              return "MCO_E_XML_INVCLASS - Object class and XML class are not the same";
    case MCO_E_XML_NO_OID:                return "MCO_E_XML_NO_OID - First field in XML object MUST be OID";
    case MCO_E_XML_INVOID:                return "MCO_E_XML_INVOID - Invalid data in OID field (hex code)";
    case MCO_E_XML_INVFLDNAME:            return "MCO_E_XML_INVFLDNAME - Invalid field name";
    case MCO_E_XML_FLDNOTFOUND:           return "MCO_E_XML_FLDNOTFOUND - Specified field was not found";
    case MCO_E_XML_INVENDTAG:             return "MCO_E_XML_INVENDTAG - Invalid closing tag name";
    case MCO_E_XML_UPDID:                 return "MCO_E_XML_UPDID - Can not update OID or AUTOID";
    case MCO_E_XML_INVASCII:              return "MCO_E_XML_INVASCII - Invalid XML coding in ascii string";
    case MCO_E_XML_INCOMPL:               return "MCO_E_XML_INCOMPL - XML data is incomplete (closing tag was not found)";
    case MCO_E_XML_ARRSMALL:              return "MCO_E_XML_ARRSMALL - Array is not large enough to hold all elements";
    case MCO_E_XML_INVARREL:              return "MCO_E_XML_INVARREL - Invalid name of array element";
    case MCO_E_XML_EXTRAXML:              return "MCO_E_XML_EXTRAXML - Extra XML found after parsing";
    case MCO_E_XML_NOTWF:                 return "MCO_E_XML_NOTWF - Not well-formed XML";
    case MCO_E_XML_UNICODE:               return "MCO_E_XML_UNICODE - Bad unicode conversion";
    case MCO_E_XML_NOINDEX:               return "MCO_E_XML_NOINDEX - Some class has no indexes; The database cannot be exported";

    case MCO_E_NW:                        return "MCO_E_NW - General Network error";
    case MCO_E_NW_FATAL:                  return "MCO_E_NW_FATAL - Fatal Network error";
    case MCO_E_NW_NOTSUPP:                return "MCO_E_NW_NOTSUPP - Network is not supported";
    case MCO_E_NW_CLOSE_CHANNEL:          return "MCO_E_NW_CLOSE_CHANNEL - Error closing Network channel";
    case MCO_E_NW_BUSY:                   return "MCO_E_NW_BUSY - Network busy (another listener?)";
    case MCO_E_NW_ACCEPT:                 return "MCO_E_NW_ACCEPT - Unable to accept an incoming connection";
    case MCO_E_NW_TIMEOUT:                return "MCO_E_NW_TIMEOUT - Timeout exceeded";
    case MCO_E_NW_INVADDR:                return "MCO_E_NW_INVADDR - Invalid address specified";
    case MCO_E_NW_NOMEM:                  return "MCO_E_NW_NOMEM - Not enought memory";
    case MCO_E_NW_CONNECT:                return "MCO_E_NW_CONNECT - Unable to connect";
    case MCO_E_NW_SENDERR:                return "MCO_E_NW_SENDERR - Unable to send data";
    case MCO_E_NW_RECVERR:                return "MCO_E_NW_RECVERR - Unable to receive data";
    case MCO_E_NW_CLOSED:                 return "MCO_E_NW_CLOSED - Connection was closed by the remote host";
    case MCO_E_NW_HANDSHAKE:              return "MCO_E_NW_HANDSHAKE - Handshake failed";
    case MCO_E_NW_CLOSE_SOCKET:           return "MCO_E_NW_CLOSE_SOCKET - Can not close socket";
    case MCO_E_NW_CREATEPIPE:             return "MCO_E_NW_CREATEPIPE - Unable to create a pipe";
    case MCO_E_NW_SOCKET:                 return "MCO_E_NW_SOCKET - Socket error";
    case MCO_E_NW_SOCKOPT:                return "MCO_E_NW_SOCKOPT - Unable to set socket option";
    case MCO_E_NW_BIND:                   return "MCO_E_NW_BIND - Unable to bind socket";
    case MCO_E_NW_SOCKIOCTL:              return "MCO_E_NW_SOCKIOCTL - Unable to ioctl socket";
    case MCO_E_NW_MAGIC:                  return "MCO_E_NW_MAGIC - Invalid magic value";
    case MCO_E_NW_INVMSGPARAM:            return "MCO_E_NW_INVMSGPARAM - Invalid message";
    case MCO_E_NW_WRONGSEQ:               return "MCO_E_NW_WRONGSEQ - Invalid message number";
    case MCO_E_NWMCAST_CLOSE_SOCKET:      return "MCO_E_NWMCAST_CLOSE_SOCKET - Unable to close multicast Socket";
    case MCO_E_NWMCAST_SOCKET:            return "MCO_E_NWMCAST_SOCKET - Multicast Socket error";
    case MCO_E_NWMCAST_SOCKOPT:           return "MCO_E_NWMCAST_SOCKOPT - Unable to set multicast socket option";
    case MCO_E_NWMCAST_RECV:              return "MCO_E_NWMCAST_RECV - Unable to receive data from multicast socket";
    case MCO_E_NWMCAST_BIND:              return "MCO_E_NWMCAST_BIND - Unable to bind multicast socket";
    case MCO_E_NWMCAST_NBIO:              return "MCO_E_NWMCAST_NBIO - Unable to ioctl multicast socket";
    case MCO_E_NW_KILLED_BY_REPLICA:      return "MCO_E_NW_KILLED_BY_REPLICA - the master connection was killed by replica";
            
    case MCO_E_HA:                        return "MCO_E_HA - General HA error";
    case MCO_E_HA_PROTOCOLERR:            return "MCO_E_HA_PROTOCOLERR - Protocol error";
    case MCO_E_HA_TIMEOUT:                return "MCO_E_HA_TIMEOUT - Protocol timeout";
    case MCO_E_HA_IOERROR:                return "MCO_E_HA_IOERROR - Input/Output error";
    case MCO_E_HA_MAXREPLICAS:            return "MCO_E_HA_MAXREPLICAS - Too many replicas requested";
    case MCO_E_HA_INIT:                   return "MCO_E_HA_INIT - Unable to initialize HA";
    case MCO_E_HA_RECEIVE:                return "MCO_E_HA_RECEIVE - Unable to receive HA message";
    case MCO_E_HA_NO_AUTO_OID:            return "MCO_E_HA_NO_AUTO_OID - No auto_oid index declared in the database schema";
    case MCO_E_HA_NOT_INITIALIZED:        return "MCO_E_HA_NOT_INITIALIZED - HA was not initialized";
    case MCO_E_HA_INVALID_MESSAGE:        return "MCO_E_HA_INVALID_MESSAGE - Invalid HA message ID";
    case MCO_E_HA_INVALID_PARAMETER:      return "MCO_E_HA_INVALID_PARAMETER - Invalid parameter";
    case MCO_E_HA_INVCHANNEL:             return "MCO_E_HA_INVCHANNEL - Invalid channel handler";
    case MCO_E_HA_INCOMPATIBLE_MODES:     return "MCO_E_HA_INCOMPATIBLE_MODES - Incompatible HA mode";
    case MCO_E_HA_CLOSE_TEMP:             return "MCO_E_HA_CLOSE_TEMP - Close temporary multicast channel";
    case MCO_E_HA_MULTICAST_NOT_SUPP:     return "MCO_E_HA_MULTICAST_NOT_SUPP - Multicast is not configured";
    case MCO_E_HA_HOTSYNCH_NOT_SUPP:      return "MCO_E_HA_HOTSYNCH_NOT_SUPP - Hot synchronization is not configured";
    case MCO_E_HA_ASYNCH_NOT_SUPP:        return "MCO_E_HA_ASYNCH_NOT_SUPP - Asynchronous replication is not configured";
    case MCO_E_HA_NO_MEM:                 return "MCO_E_HA_NO_MEM - Not enough memory to create communication layer descriptors";
    case MCO_E_HA_BAD_DESCRIPTOR:         return "MCO_E_HA_BAD_DESCRIPTOR - ha_t structure was not cleared before creation of base channel";
    case MCO_E_HA_CANCEL:                 return "MCO_E_HA_CANCEL - Connection was canceled";
    case MCO_E_HA_WRONG_DB_MAGIC:         return "MCO_E_HA_WRONG_DB_MAGIC - Wrong DB magic";
    case MCO_E_HA_COMMIT:                 return "MCO_E_HA_COMMIT - master commit error, break commit loop";
    case MCO_E_HA_MANYREPLICAS:           return "MCO_E_HA_MANYREPLICAS - Attempt to attach too many replicas";
    case MCO_E_NOT_MASTER:                return "MCO_E_NOT_MASTER - Master-mode was not set";
    case MCO_E_HA_STOPPED:                return "MCO_E_HA_STOPPED - Replication was stopped";
    case MCO_E_HA_NOWRITETXN:             return "MCO_E_HA_NOWRITETXN - Read-write transactions are prohibited on replica";
    case MCO_E_HA_PM_BUFFER:              return "MCO_E_HA_PM_BUFFER - Page memory buffer error";
    case MCO_E_HA_NOT_REPLICA:            return "MCO_E_HA_NOT_REPLICA - Not Currently in replica-mode";
#if (EXTREMEDB_VERSION >= 41)
    case MCO_E_HA_BAD_DICT:               return "MCO_E_HA_BAD_DICT - Master's db schema is incompatible with replica's";
    case MCO_E_HA_BINEV_NOT_SUPP:         return "MCO_E_HA_BINEV_NOT_SUPP - binary schema evolution is not configured";

    case MCO_E_UDA:                       return "MCO_E_UDA - General UDA error";
    case MCO_E_UDA_TOOMANY_ENTRIES:       return "MCO_E_UDA_TOOMANY_ENTRIES - Allocated entry number exceeded";
    case MCO_E_UDA_NAME_TOO_LONG:         return "MCO_E_UDA_NAME_TOO_LONG - Long entry name";
    case MCO_E_UDA_DUPLICATE:             return "MCO_E_UDA_DUPLICATE - Duplicate entry name";
    case MCO_E_UDA_DICT_NOTFOUND:         return "MCO_E_UDA_DICT_NOTFOUND - Dictionary (entry) not found (by dict_no, by name or by connection)";
    case MCO_E_UDA_STRUCT_NOTFOUND:       return "MCO_E_UDA_STRUCT_NOTFOUND - Structure not found (by struct_no or by name)";
    case MCO_E_UDA_FIELD_NOTFOUND:        return "MCO_E_UDA_FIELD_NOTFOUND - Field not found (by field_no or by name)";
    case MCO_E_UDA_INDEX_NOTFOUND:        return "MCO_E_UDA_INDEX_NOTFOUND - Index not found (by index_no or by name)";
    case MCO_E_UDA_IFIELD_NOTFOUND:       return "MCO_E_UDA_IFIELD_NOTFOUND - Indexed field not found (by ifield_no or by name)";
    case MCO_E_UDA_COLLATION_NOTFOUND:    return "MCO_E_UDA_COLLATION_NOTFOUND - collation not found (by collation_no or by name)";
    case MCO_E_UDA_STRUCT_NOT_CLASS:      return "MCO_E_UDA_STRUCT_NOT_CLASS - Structure is not class, so some operations are not allowed";
    case MCO_E_UDA_WRONG_KEY_NUM:         return "MCO_E_UDA_WRONG_KEY_NUM - Key number in lookup() and compare() differs from index spec";
    case MCO_E_UDA_WRONG_KEY_TYPE:        return "MCO_E_UDA_WRONG_KEY_TYPE - Key type in lookup() and compare() differs from index spec";
    case MCO_E_UDA_WRONG_OPCODE:          return "MCO_E_UDA_WRONG_OPCODE - Invalid OPCODE (e.g. not MCO_EQ for hash index)";
    case MCO_E_UDA_SCALAR:                return "MCO_E_UDA_SCALAR - Attempt to get mco_uda_length() on scalar field";
    case MCO_E_UDA_NOT_DYNAMIC:           return "MCO_E_UDA_NOT_DYNAMIC - Attempt to call mco_uda_field_alloc/free() for non-vector or optional struct field";
    case MCO_E_UDA_WRONG_VALUE_TYPE:      return "MCO_E_UDA_WRONG_VALUE_TYPE - Type of value and field are different in mco_uda_put()";
    case MCO_E_UDA_READONLY:              return "MCO_E_UDA_READONLY - Attempt to call mco_uda_put() for read-only fields : oid, autoid, autooid";
    case MCO_E_UDA_WRONG_CLASS_CODE:      return "MCO_E_UDA_WRONG_CLASS_CODE - Invalid class code";
    case MCO_E_UDA_DICT_NOT_DIRECT:       return "MCO_E_UDA_DICT_NOT_DIRECT - In mco_uda_db_open(), entry holds database pointer, not dictionary";
    case MCO_E_UDA_INDEX_NOT_USERDEF:     return "MCO_E_UDA_INDEX_NOT_USERDEF - Attempt to call mco_uda_register_udf() for non-userdef index";

    case MCO_E_TL:                        return "MCO_E_TL - TL base error code";
    case MCO_E_TL_INVAL:                  return "MCO_E_TL_INVAL - invalid argument value";
    case MCO_E_TL_ALREADY_STARTED:        return "MCO_E_TL_ALREADY_STARTED - TL already started";
    case MCO_E_TL_NOT_STARTED:            return "MCO_E_TL_NOT_STARTED - TL is not started";
    case MCO_E_TL_LOG_NOT_OPENED:         return "MCO_E_TL_LOG_NOT_OPENED - Log file is not opened";
    case MCO_E_TL_INVFORMAT:              return "MCO_E_TL_INVFORMAT - Completely corrupted LOG file";
    case MCO_E_TL_INVDATA:                return "MCO_E_TL_INVDATA - Broken record during recovering";
    case MCO_E_TL_IO_ERROR:               return "MCO_E_TL_IO_ERROR - Log file input/output error";
    case MCO_E_TL_NOT_ITERABLE:           return "MCO_E_TL_NOT_ITERABLE - LOG file created without flag MCO_TRANSLOG_ITERABLE or unsupported transaction manager is applied";
    case MCO_E_TL_NO_AUTO_OID:            return "MCO_E_TL_NO_AUTO_OID - No auto_oid index declared in the database schema";

    case MCO_E_NO_DIRECT_ACCESS:          return "MCO_E_NO_DIRECT_ACCESS - direct access to the structure is not possible";

    case MCO_E_DDL_NOMEM:                 return "MCO_E_DDL_NOMEM - dictionary can not fir in the reseerved area";
    case MCO_E_DDL_UNDEFINED_STRUCT:      return "MCO_E_DDL_UNDEFINED_STRUCT - struct referenced by field is not yet defined";
    case MCO_E_DDL_INVALID_TYPE:          return "MCO_E_DDL_INVALID_TYPE - unsupported field type";
    case MCO_E_DDL_FIELD_NOT_FOUND:       return "MCO_E_DDL_FIELD_NOT_FOUND - reference field is not found in the class";
    case MCO_E_DDL_INTERNAL_ERROR:        return "MCO_E_DDL_INTERNAL_ERROR - internal error";

    case MCO_E_CLUSTER:                   return "MCO_E_CLUSTER - Cluster base error code";
    case MCO_E_CLUSTER_NOT_INITIALIZED:   return "MCO_E_CLUSTER_NOT_INITIALIZED - non-cluster database";
    case MCO_E_CLUSTER_INVALID_PARAMETER: return "MCO_E_CLUSTER_INVALID_PARAMETER - invalid cluster parameters value";
    case MCO_E_CLUSTER_STOPPED:           return "MCO_E_CLUSTER_STOPPED - replication was stopped";
    case MCO_E_CLUSTER_PROTOCOLERR:       return "MCO_E_CLUSTER_PROTOCOLERR - replication protocol error";
    case MCO_E_CLUSTER_NOQUORUM:          return "MCO_E_CLUSTER_NOQUORUM - no node's quorum";
    case MCO_E_CLUSTER_BUSY:              return "MCO_E_CLUSTER_BUSY - can't stop cluster with active transactions";
    case MCO_E_CLUSTER_INCOMPATIBLE_MODE: return "MCO_E_CLUSTER_INCOMPATIBLE_MODE - incompatible modes on different nodes";
    case MCO_E_CLUSTER_SYNC:              return "MCO_E_CLUSTER_SYNC - error during synchronization";
#endif

    case MCO_E_EVAL:                      return "MCO_E_EVAL - Evaluation limit reached";
        
    default:                              break;
  }

  if ( rc >= MCO_ERR_DB && rc < MCO_ERR_DICT ) {
    return "MCO_ERR_DB - Database header is invalid";
  } else if ( rc >= MCO_ERR_DICT && rc < MCO_ERR_CURSOR ) {
    return "MCO_ERR_DICT - Database dictionary is invalid";
  } else if ( rc >= MCO_ERR_CURSOR && rc < MCO_ERR_PMBUF ) {
    return "MCO_ERR_CURSOR - Cursor";
  } else if ( rc >= MCO_ERR_PMBUF && rc < MCO_ERR_COMMON ) {
    return "MCO_ERR_PMBUF - PM buffer";
  } else if ( rc >= MCO_ERR_COMMON && rc < MCO_ERR_HEAP ) {
    return "MCO_ERR_COMMON - Common routines";
  } else if ( rc >= MCO_ERR_HEAP && rc < MCO_ERR_OBJ ) {
    return "MCO_ERR_HEAP - Heap manager";
  } else if ( rc >= MCO_ERR_OBJ && rc < MCO_ERR_BLOB ) {
    return "MCO_ERR_OBJ - Object allocator";
  } else if ( rc >= MCO_ERR_BLOB && rc < MCO_ERR_FREC ) {
    return "MCO_ERR_BLOB - Blob operation";
  } else if ( rc >= MCO_ERR_FREC && rc < MCO_ERR_VOLUNTARY ) {
    return "MCO_ERR_FREC - Record allocator";
  } else if ( rc >= MCO_ERR_VOLUNTARY && rc < MCO_ERR_LOADSAVE ) {
    return "MCO_ERR_VOLUNTARY - Voluntary index";
  } else if ( rc >= MCO_ERR_LOADSAVE && rc < MCO_ERR_PGMEM ) {
    return "MCO_ERR_LOADSAVE - Db save and load";
  } else if ( rc >= MCO_ERR_PGMEM && rc < MCO_ERR_EV_SYN ) {
    return "MCO_ERR_PGMEM - Page memory";
  } else if ( rc >= MCO_ERR_EV_SYN && rc < MCO_ERR_EV_ASYN ) {
    return "MCO_ERR_EV_SYN - Syncronous events";
  } else if ( rc >= MCO_ERR_EV_ASYN && rc < MCO_ERR_EV_W ) {
    return "MCO_ERR_EV_ASYN - Asyncronous events";
  } else if ( rc >= MCO_ERR_EV_W && rc < MCO_ERR_XML_W ) {
    return "MCO_ERR_EV_W - Event wrappers";
  } else if ( rc >= MCO_ERR_XML_W && rc < MCO_ERR_XML_SC ) {
    return "MCO_ERR_XML_W - XML serialization";
  } else if ( rc >= MCO_ERR_XML_SC && rc < MCO_ERR_BTREE ) {
    return "MCO_ERR_XML_SC - XML schema";
  } else if ( rc >= MCO_ERR_BTREE && rc < MCO_ERR_HASH ) {
    return "MCO_ERR_BTREE - Btree index";
  } else if ( rc >= MCO_ERR_HASH && rc < MCO_ERR_RECOV ) {
    return "MCO_ERR_HASH - Hash index";
  }
#if (EXTREMEDB_VERSION >=41)
  else if ( rc >= MCO_ERR_RECOV && rc < MCO_ERR_FCOPY) {
    return "MCO_ERR_RECOV - Recovery";
  }
  else if ( rc >= MCO_ERR_FCOPY && rc < MCO_ERR_INST ) {
    return "MCO_ERR_FCOPY - copy fields";
  }
#endif
  else if ( rc >= MCO_ERR_INST && rc < MCO_ERR_TRN ) {
    return "MCO_ERR_INST - Db instance";
  } else if ( rc >= MCO_ERR_TRN && rc < MCO_ERR_TMGR ) {
    return "MCO_ERR_TRN - Transaction";
  } else if ( rc >= MCO_ERR_TMGR && rc < MCO_ERR_SYNC ) {
    return "MCO_ERR_TMGR - Transaction manager";
  } else if ( rc >= MCO_ERR_SYNC && rc < MCO_ERR_ORDER ) {
    return "MCO_ERR_SYNC - General sync";
  } else if ( rc >= MCO_ERR_ORDER && rc < MCO_ERR_SEM ) {
    return "MCO_ERR_ORDER - Ordering and hash";
  } else if ( rc >= MCO_ERR_SEM && rc < MCO_ERR_SHM ) {
    return "MCO_ERR_SEM - Semaphores";
  } else if ( rc >= MCO_ERR_SHM && rc < MCO_ERR_SER ) {
    return "MCO_ERR_SHM - Shared memory";
  } else if ( rc >= MCO_ERR_SER && rc < MCO_ERR_HA ) {
    return "MCO_ERR_SER - Serialization";
  } else if ( rc >= MCO_ERR_HA && rc < MCO_ERR_DB_NOMEM ) {
    return "MCO_ERR_HA - High Availability";
  } else if ( rc >= MCO_ERR_DB_NOMEM && rc < MCO_ERR_OBJECT_HANDLE ) {
    return "MCO_ERR_DB_NOMEM - Insufficient memory";
  } else if ( rc >= MCO_ERR_OBJECT_HANDLE && rc < MCO_ERR_UNSUPPORTED_FLOAT ) {
    return "MCO_ERR_OBJECT_HANDLE - Invalid object handle";
  } else if ( rc >= MCO_ERR_UNSUPPORTED_FLOAT && rc < MCO_ERR_DB_NOMEM_HASH ) {
    return "MCO_ERR_UNSUPPORTED_FLOAT - Support of float type is disabled";
  } else if ( rc >= MCO_ERR_DB_NOMEM_HASH && rc < MCO_ERR_DB_NOMEM_HEAP ) {
    return "MCO_ERR_DB_NOMEM_HASH - Insufficient memory in hash index";
  } else if ( rc >= MCO_ERR_DB_NOMEM_HEAP && rc < MCO_ERR_DB_NOMEM_TRANS ) {
    return "MCO_ERR_DB_NOMEM_HEAP - Insufficient memory in heap manager";
  } else if ( rc >= MCO_ERR_DB_NOMEM_TRANS && rc < MCO_ERR_DB_NAMELONG ) {
    return "MCO_ERR_DB_NOMEM_TRANS - Insufficient memory in transaction manager";
  } else if ( rc >= MCO_ERR_DB_NAMELONG && rc < MCO_ERR_DB_VERS_MISMATCH ) {
    return "MCO_ERR_DB_NAMELONG - Name of the database is too long";
  } else if ( rc >= MCO_ERR_DB_VERS_MISMATCH && rc < MCO_ERR_RUNTIME ) {
    return "MCO_ERR_DB_VERS_MISMATCH - Version of eXtremeDB runtime mismatch";
  } else if ( rc >= MCO_ERR_RUNTIME && rc < MCO_ERR_INMEM_ONLY_RUNTIME ) {
    return "MCO_ERR_RUNTIME - Core runtime";
  } else if ( rc >= MCO_ERR_INMEM_ONLY_RUNTIME && rc < MCO_ERR_DISK ) {
    return "MCO_ERR_INMEM_ONLY_RUNTIME - Persistent class in the schema but inmem-only runtime";
  } else if ( rc >= MCO_ERR_DISK && rc < MCO_ERR_DISK_WRITE ) {
    return "MCO_ERR_DISK - General disk error";
  } else if ( rc >= MCO_ERR_DISK_WRITE && rc < MCO_ERR_DISK_READ ) {
    return "MCO_ERR_DISK_WRITE - Unable to write to persistent storage";
  } else if ( rc >= MCO_ERR_DISK_READ && rc < MCO_ERR_DISK_FLUSH ) {
    return "MCO_ERR_DISK_READ - Unable to read from persistent storage";
  } else if ( rc >= MCO_ERR_DISK_FLUSH && rc < MCO_ERR_DISK_CLOSE ) {
    return "MCO_ERR_DISK_FLUSH - Unable to flush to persistent storage";
  } else if ( rc >= MCO_ERR_DISK_CLOSE && rc < MCO_ERR_DISK_TRUNCATE ) {
    return "MCO_ERR_DISK_CLOSE - Error closing persistent storage";
  } else if ( rc >= MCO_ERR_DISK_TRUNCATE && rc < MCO_ERR_DISK_SEEK ) {
    return "MCO_ERR_DISK_TRUNCATE - Unable to truncate persistent storage";
  } else if ( rc >= MCO_ERR_DISK_SEEK && rc < MCO_ERR_DISK_OPEN ) {
    return "MCO_ERR_DISK_SEEK - Unable to seek in persistent storage";
  } else if ( rc >= MCO_ERR_DISK_OPEN && rc < MCO_ERR_DISK_ALREADY_OPENED ) {
    return "MCO_ERR_DISK_OPEN - Unable to open persistent storage";
  } else if ( rc >= MCO_ERR_DISK_ALREADY_OPENED && rc < MCO_ERR_DISK_NOT_OPENED ) {
    return "MCO_ERR_DISK_ALREADY_OPENED - Persistent storage already opened";
  } else if ( rc >= MCO_ERR_DISK_NOT_OPENED && rc < MCO_ERR_DISK_INVALID_PARAM ) {
    return "MCO_ERR_DISK_NOT_OPENED - Persistent storage was not opened";
  } else if ( rc >= MCO_ERR_DISK_INVALID_PARAM && rc < MCO_ERR_DISK_PAGE_ACCESS ) {
    return "MCO_ERR_DISK_INVALID_PARAM - Invalid parameter value";
  } else if ( rc >= MCO_ERR_DISK_PAGE_ACCESS && rc < MCO_ERR_DISK_INTERNAL_ERROR ) {
    return "MCO_ERR_DISK_PAGE_ACCESS - Page Access Fatal Error";
  } else if ( rc >= MCO_ERR_DISK_INTERNAL_ERROR && rc < MCO_ERR_DISK_OPERATION_NOT_ALLOWED ) {
    return "MCO_ERR_DISK_INTERNAL_ERROR - Internal Fatal Error";
  } else if ( rc >= MCO_ERR_DISK_OPERATION_NOT_ALLOWED && rc < MCO_ERR_DISK_ALREADY_CONNECTED ) {
    return "MCO_ERR_DISK_OPERATION_NOT_ALLOWED - Operation not allowed Fatal Error";
  } else if ( rc >= MCO_ERR_DISK_ALREADY_CONNECTED && rc < MCO_ERR_DISK_TOO_MANY_INDICES ) {
    return "MCO_ERR_DISK_ALREADY_CONNECTED - Persistent storage already connected";
  } else if ( rc >= MCO_ERR_DISK_TOO_MANY_INDICES && rc < MCO_ERR_DISK_TOO_MANY_CLASSES ) {
    return "MCO_ERR_DISK_TOO_MANY_INDICES - Too many indexes in persistent classes";
  } else if ( rc >= MCO_ERR_DISK_TOO_MANY_CLASSES && rc < MCO_ERR_DISK_SPACE_EXHAUSTED ) {
    return "MCO_ERR_DISK_TOO_MANY_CLASSES - Too many persistent classes";
  } else if ( rc >= MCO_ERR_DISK_SPACE_EXHAUSTED && rc < MCO_ERR_DISK_PAGE_POOL_EXHAUSTED ) {
    return "MCO_ERR_DISK_SPACE_EXHAUSTED - Persistent storage out of space";
  } else if ( rc >= MCO_ERR_DISK_PAGE_POOL_EXHAUSTED && rc < MCO_ERR_DISK_INCOMPATIBLE_LOG_TYPE ) {
    return "MCO_ERR_DISK_PAGE_POOL_EXHAUSTED - Too many pinned disk pages";
  } else if ( rc >= MCO_ERR_DISK_INCOMPATIBLE_LOG_TYPE && rc < MCO_ERR_DISK_BAD_PAGE_SIZE ) {
    return "MCO_ERR_DISK_INCOMPATIBLE_LOG_TYPE - Incompatible database log type";
  } else if ( rc >= MCO_ERR_DISK_BAD_PAGE_SIZE && rc < MCO_ERR_DISK_SYNC ) {
    return "MCO_ERR_DISK_BAD_PAGE_SIZE - Page size is not acceptable";
  }
#if (EXTREMEDB_VERSION >=41)
  else if ( rc >= MCO_ERR_DISK_SYNC && rc < MCO_ERR_DISK_CRC) {
    return "MCO_ERR_DISK_SYNC - Failed operation on sync. primitive";
  } else if ( rc >= MCO_ERR_DISK_CRC && rc < MCO_ERR_CHECKPIN ) {
    return "MCO_ERR_DISK_CRC - wrong CRC code of disk page";
  } else if ( rc >= MCO_ERR_CHECKPIN && rc < MCO_ERR_CONN ) {
    return "MCO_ERR_CHECKPIN - Unbalance pin/unpin";
  } else if ( rc >= MCO_ERR_CONN && rc < MCO_ERR_REGISTRY ) {
    return "MCO_ERR_CONN - Connection processing";
  } else if ( rc >= MCO_ERR_REGISTRY && rc < MCO_ERR_INDEX ) {
    return "MCO_ERR_REGISTRY - Registry processing";
  } else if ( rc >= MCO_ERR_INDEX && rc < MCO_ERR_VTMEM ) {
    return "MCO_ERR_INDEX - Index processing";
  } else if ( rc >= MCO_ERR_VTMEM && rc < MCO_ERR_VTDSK ) {
    return "MCO_ERR_VTMEM - In-mememory only runtime";
  } else if ( rc >= MCO_ERR_VTDSK && rc < MCO_ERR_RTREE ) {
    return "MCO_ERR_VTDSK - Mixed runtime";
  } else if ( rc >= MCO_ERR_RTREE && rc < MCO_ERR_UDA ) {
    return "MCO_ERR_RTREE - RTree index";
  } else if ( rc >= MCO_ERR_UDA && rc < MCO_ERR_PTREE ) {
    return "MCO_ERR_UDA - Uniform Data Access";
  } else if ( rc >= MCO_ERR_PTREE && rc < MCO_ERR_TL ) {
    return "MCO_ERR_PTREE - patricia tree index";
  } else if ( rc >= MCO_ERR_TL && rc < MCO_ERR_CLUSTER ) {
    return "MCO_ERR_TL - transaction log";
  } else if ( rc >= MCO_ERR_CLUSTER && rc < MCO_ERR_LAST ) {
    return "MCO_ERR_CLUSTER - cluster error";
  }
#endif
  
  return "Unknown error";
}

void rc_check(const char * msg, MCO_RET rc)
{
  if ( MCO_S_OK == rc )
    fprintf(stdout, "\n%s : Successful", msg);
  else
    fprintf(stdout, "\n%s : \n\tReturn Code %d: %s\n", msg, (unsigned int)rc, mco_ret_string(rc, 0));
  fflush(stdout);
}

void show_runtime_info(const char * lead_line)
{
  mco_runtime_info_t info;
  
  /* get runtime info */
  mco_get_runtime_info(&info);

  /* Core configuration parameters: */
  if ( *lead_line )
    fprintf( stdout, "%s", lead_line );

  fprintf( stdout, "\n" );
  fprintf( stdout, "\tEvaluation runtime ______ : %s\n", info.mco_evaluation_version   ? "yes":"no" );
  fprintf( stdout, "\tCheck-level _____________ : %d\n", info.mco_checklevel );
  fprintf( stdout, "\tMultithread support _____ : %s\n", info.mco_multithreaded        ? "yes":"no" );
  fprintf( stdout, "\tFixedrec support ________ : %s\n", info.mco_fixedrec_supported   ? "yes":"no" );
  fprintf( stdout, "\tShared memory support ___ : %s\n", info.mco_shm_supported        ? "yes":"no" );
  fprintf( stdout, "\tXML support _____________ : %s\n", info.mco_xml_supported        ? "yes":"no" );
  fprintf( stdout, "\tStatistics support ______ : %s\n", info.mco_stat_supported       ? "yes":"no" );
  fprintf( stdout, "\tEvents support __________ : %s\n", info.mco_events_supported     ? "yes":"no" );
  fprintf( stdout, "\tVersioning support ______ : %s\n", info.mco_versioning_supported ? "yes":"no" );
  fprintf( stdout, "\tSave/Load support _______ : %s\n", info.mco_save_load_supported  ? "yes":"no" );
  fprintf( stdout, "\tRecovery support ________ : %s\n", info.mco_recovery_supported   ? "yes":"no" );
#if (EXTREMEDB_VERSION >=41)
  fprintf( stdout, "\tRTree index support _____ : %s\n", info.mco_rtree_supported      ? "yes":"no" );
#endif
  fprintf( stdout, "\tUnicode support _________ : %s\n", info.mco_unicode_supported    ? "yes":"no" );
  fprintf( stdout, "\tWChar support ___________ : %s\n", info.mco_wchar_supported      ? "yes":"no" );
  fprintf( stdout, "\tC runtime _______________ : %s\n", info.mco_rtl_supported        ? "yes":"no" );
  fprintf( stdout, "\tSQL support _____________ : %s\n", info.mco_sql_supported        ? "yes":"no" );
  fprintf( stdout, "\tPersistent storage support: %s\n", info.mco_disk_supported       ? "yes":"no" );
  fprintf( stdout, "\tDirect pointers mode ____ : %s\n", info.mco_direct_pointers      ? "yes":"no" );  
}

#if (EXTREMEDB_VERSION>=41)
void show_device_info(const char * lead_line, mco_device_t dev[], int nDev)
{
  const char * memtype[]    = { "MCO_MEMORY_NULL", "MCO_MEMORY_CONV", "MCO_MEMORY_NAMED", "MCO_MEMORY_FILE", 
                          "MCO_MEMORY_MULTIFILE", "MCO_MEMORY_RAID", "MCO_MEMORY_INT_DESC" };
  const char * assigntype[] = { "MCO_MEMORY_ASSIGN_DATABASE", "MCO_MEMORY_ASSIGN_CACHE", 
                          "MCO_MEMORY_ASSIGN_PERSISTENT", "MCO_MEMORY_ASSIGN_LOG" };
  const char * flagtype[]   = { "MCO_FILE_OPEN_DEFAULT", "MCO_FILE_OPEN_READ_ONLY", "MCO_FILE_OPEN_TRUNCATE",
                          "MCO_FILE_OPEN_NO_BUFFERING", "MCO_FILE_OPEN_EXISTING" };
  int i, nflags;

  if ( *lead_line )
    printf( lead_line );
  printf("\n");
  for ( i=0; i < nDev; i++ ) {
    printf("\n\tdev[%d]:\tMemory type: %s\n", i, memtype[dev[i].type] );
    printf("\t\tAssignment : %s\n", assigntype[dev[i].assignment] );
    if ( MCO_MEMORY_FILE == dev[i].type || MCO_MEMORY_MULTIFILE == dev[i].type || MCO_MEMORY_RAID == dev[i].type ) 
       printf("\t\tFilename   : %s\n", dev[i].dev.file.name );
    else
       printf("\t\tSize       : %llu\n", (uint8)dev[i].size );
    printf("\t\tFlags      : " );
    if ( MCO_FILE_OPEN_DEFAULT == ( dev[i].dev.file.flags & MCO_FILE_OPEN_DEFAULT ) )
    {
      printf("%s", flagtype[0] );
    } else {
      nflags = 0;
      if ( MCO_FILE_OPEN_READ_ONLY == ( dev[i].dev.file.flags & MCO_FILE_OPEN_READ_ONLY ) )
      {
        printf("%s", flagtype[1] );
        nflags++;
      }
      if ( MCO_FILE_OPEN_TRUNCATE == ( dev[i].dev.file.flags & MCO_FILE_OPEN_TRUNCATE ) )
      {
        if ( nflags > 0 ) printf(" | ");
        printf("%s", flagtype[2] );
        nflags++;
      }
      if ( MCO_FILE_OPEN_NO_BUFFERING == ( dev[i].dev.file.flags & MCO_FILE_OPEN_NO_BUFFERING ) )
      {
        if ( nflags > 0 ) printf(" | ");
        printf("%s", flagtype[3] );
        nflags++;
      }
      if ( MCO_FILE_OPEN_EXISTING == ( dev[i].dev.file.flags & MCO_FILE_OPEN_EXISTING ) )
      {
        if ( nflags > 0 ) printf(" | ");
        printf("%s", flagtype[4] );
        nflags++;
      }
    }
    printf("\n" );
  }
}
#endif

