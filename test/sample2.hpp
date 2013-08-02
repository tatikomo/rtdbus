#if !defined SAMPLE2_XDB
#define SAMPLE2_XDB

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

#define EXTREMEDB_VERSION 50
#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"

void extended_errhandler(MCO_RET, const char*, int);
void show_runtime_info(const char *);

# if (EXTREMEDB_VERSION<=40)
mco_size_t file_writer(void*, const void*, mco_size_t);
# else
mco_size_sig_t file_writer(void*, const void*, mco_size_t);
# endif

#ifdef __cplusplus
}
#endif

#include <dat/xdb_broker.hpp>

const int   DATABASE_SIZE           = 1024 * 1024 * 1;
const int   WORKERS_SPOOL_MAXLEN    = 2;
const int   IDENTITY_MAXLEN         = 11;
const int   SERVICENAME_MAXLEN      = 32;
const int   HEARTBEAT_PERIOD_VALUE  = 10;
const char* DATABASE_NAME           = "XDB_SAMPLE1";

/* stuff for database snapshot producing */
char        m_snapshot_file_prefix[100];
char        snapshot_file_name[50];
bool        m_save_to_xml_feature;
int         m_snapshot_counter;
bool        m_initialized;

/* stuff for storaging delta of time */
typedef struct timespec timer_mark_t;

/* reduced substitute for class Service */
typedef struct
{
    int64_t  m_id;
    char     m_name[SERVICENAME_MAXLEN+1];
    ServiceState    m_state;
} Service;

/* reduced substitute for class Worker */
typedef struct
{
    int64_t  m_service_id;
    char     m_identity[IDENTITY_MAXLEN+1];
    WorkerState    m_state;
    timer_mark_t m_expiration;
} Worker;


const char *empty_str = "";
char *service_name_1 = (char*)"service_test_1";
char *service_name_2 = (char*)"service_test_2";
char *unbelievable_service_name = (char*)"unbelievable_service";
char *worker_identity_1 = (char*)"SN1_AAAAAAA";
char *worker_identity_2 = (char*)"SN1_WRK2";
char *worker_identity_3 = (char*)"SN1_WRK3";

Service *service1 = NULL;
Service *service2 = NULL;
Worker  *worker  = NULL;

int64_t service1_id;
int64_t service2_id;

/* ================================================= */

void LogError(MCO_RET, const char*, const char*, ...);
void LogWarn(const char*, const char*, ...);
bool Init();
bool Connect();
void Disconnect();
Service* AddService(const char*);
Service *GetServiceById(int64_t);
bool PushWorker(Worker*);
Service* LoadService(mco_trans_h, autoid_t&, xdb_broker::XDBService&);
uint2 LocatingFirstOccurence(xdb_broker::XDBService&, WorkerState);
void MakeSnapshot(const char*);
MCO_RET SaveDbToFile(const char*);
Worker* PopWorker(Service*);
bool RemoveWorker(Worker*);


#endif

