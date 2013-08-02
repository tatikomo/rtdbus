#include <string>
#include <stdio.h>

#include "sample1.hpp"
#include "xdb_database_broker.hpp"
#include "xdb_database_service.hpp"
#include "xdb_database_worker.hpp"
#include "dat/xdb_broker.hpp"

char *service_name_1 = (char*)"service_test_1";
char *service_name_2 = (char*)"service_test_2";
char *unbelievable_service_name = (char*)"unbelievable_service";
char *worker_identity_1 = (char*)"SN1_AAAAAAA";
char *worker_identity_2 = (char*)"SN1_WRK2";
char *worker_identity_3 = (char*)"SN1_WRK3";
XDBDatabaseBroker *database = NULL;
Service *service1 = NULL;
Service *service2 = NULL;
Worker  *worker  = NULL;
int64_t service1_id;
int64_t service2_id;

int main(int argc, char **argv)
{
  bool status = false;
  XDBDatabase::DBState state;

  database = new XDBDatabaseBroker();
  while (true)
  {
    if (!database)
    {
      fprintf(stderr, "\nE1: database creation failed\n");
      break;
    }
    fprintf(stderr, "\nI1: creation OK\n"); fflush(stderr);

    if (false == (status = database->Connect()))
    {
      fprintf(stderr, "\nE2: connection failed\n");
      break;
    }
    fprintf(stderr, "\nI2: connection OK\n"); fflush(stderr);

    if (XDBDatabase::CONNECTED != (state = database->State()))
    {
      fprintf(stderr, "\nE3: status is 'not connected'\n");
      break;
    }
    fprintf(stderr, "\nI3: status is 'connected'\n"); fflush(stderr);

/* ########################################################### */
/* ADD SERVICES                                                */
/* ########################################################### */
    if (NULL == (service1 = database->AddService(service_name_1)))
    {
      fprintf(stderr, "\nE4: service '%s' adding failed\n", service_name_1);
      break;
    }
    fprintf(stderr, "\nI4: add service '%s' (id=%ld)\n", 
            service_name_1, service1->GetID()); fflush(stderr);
    service1_id = service1->GetID();

    if (NULL == (service2 = database->AddService(service_name_2)))
    {
      fprintf(stderr, "\nE5: service adding '%s' failed\n", service_name_2);
      break;
    }
    fprintf(stderr, "\nI5: add service '%s' (id=%ld)\n", 
            service_name_2, service2->GetID()); fflush(stderr);
    service2_id = service2->GetID();
    
/* ########################################################### */
/* ADD WORKERS                                                 */
/* ########################################################### */
    worker = new Worker(worker_identity_1, service1_id);
    status = database->PushWorker(worker);
    if (false == status)
    {
      fprintf(stderr, "\nE6: adding worker '%s' for service '%s' failed\n",
              worker_identity_1, service_name_1);
      break;
    }
    fprintf(stderr, "\nI6: push worker '%s'\n", worker_identity_1); fflush(stderr);
    delete worker;

    worker = new Worker(worker_identity_2, service1_id);
    status = database->PushWorker(worker);
    if (false == status)
    {
      fprintf(stderr, "\nE7: adding worker '%s' for service '%s' failed\n",
              worker_identity_2, service_name_1);
      break;
    }
    fprintf(stderr, "\nI7: push worker '%s'\n", worker_identity_2); fflush(stderr);
    delete worker;

#if 1
    worker = new Worker(worker_identity_3, service2_id);
    status = database->PushWorker(worker);
    if (false == status)
    {
      fprintf(stderr, "\nE8: adding worker '%s' for service '%s' failed\n",
              worker_identity_3, service_name_2);
      break;
    }
    fprintf(stderr, "\nI8: push worker '%s'\n", worker_identity_3); fflush(stderr);
    delete worker;
#else
    fprintf(stderr, "\nI8: SKIP PUSHING WORKER '%s' FOR SERVICE2\n", worker_identity_3); fflush(stderr);
#endif

/* ########################################################### */
/* REMOVE WORKERS OF SERVICE1                                  */
/* ########################################################### */
    worker = database->PopWorker(service1);
    if (!worker)
    {
      fprintf(stderr, "\nE9: pop worker from service '%s' is failed\n",
              service_name_1);
      break;
    }
    fprintf(stderr, "\nI9: pop worker '%s' from '%s'\n", 
        worker->GetIDENTITY(), service1->GetNAME()); fflush(stderr);

    database->MakeSnapshot("I9");
#if 1
    if (false == (status = database->RemoveWorker(worker)))
    {
      fprintf(stderr, "\nE9-1: removing worker '%s' from '%s' failed\n",
            worker->GetIDENTITY(), service1->GetNAME());
    }
    else
    {
      fprintf(stderr, "\nE9-1: remove worker '%s' from '%s'\n",
              worker->GetIDENTITY(), service1->GetNAME()); fflush(stderr);
    }
    database->MakeSnapshot("I9-1");
#else
    fprintf(stderr, "\nI9: SKIP REMOVING WORKER '%s'\n", worker->GetIDENTITY()); fflush(stderr);
#endif
    delete worker;

    worker = database->PopWorker(service1);
    if (!worker)
    {
      fprintf(stderr, "\nE10: pop worker from service '%s' is failed\n",
              service_name_1);
      break;
    }
    fprintf(stderr, "\nI10: pop worker '%s' from '%s'\n", 
        worker->GetIDENTITY(), service1->GetNAME()); fflush(stderr);

    database->MakeSnapshot("I10");
#if 1
    if (false == (status = database->RemoveWorker(worker)))
    {
      fprintf(stderr, "\nE10-1: removing worker '%s' from '%s' failed\n", 
            worker->GetIDENTITY(), service1->GetNAME());
    }
    else
    {
      fprintf(stderr, "\nI10-1: remove worker '%s' from '%s'\n", 
              worker->GetIDENTITY(), service1->GetNAME());
    }
    database->MakeSnapshot("I10-1");
#else
    fprintf(stderr, "\nI10: SKIP REMOVING WORKER '%s'\n", worker->GetIDENTITY()); fflush(stderr);
#endif
    delete worker;
    

/* ########################################################### */
/* REMOVE SERVICE1                                             */
/* ########################################################### */
#if 1
    if (false == (status = database->RemoveService(service_name_1)))
    {
      fprintf(stderr, "\nE11: remove service '%s' failed\n",
              service_name_1);
      break;
    }
    fprintf(stderr, "\nI11: removing service '%s'\n", service_name_1); fflush(stderr);
#else
    fprintf(stderr, "\nI11: SKIP REMOVING SERVICE '%s'\n", service_name_1); fflush(stderr);
#endif
    delete service1;

/* ########################################################### */
/* REMOVE WORKERS OF SERVICE2                                  */
/* ########################################################### */
    worker = database->PopWorker(service2);
    if (!worker)
    {
      fprintf(stderr, "\nE12: pop worker from service '%s' is failed\n",
              service_name_2);
      break;
    }
    fprintf(stderr, "\nI12: pop worker '%s' from '%s'\n", 
        worker->GetIDENTITY(), service_name_2); fflush(stderr);
#if 1
    if (false == (status = database->RemoveWorker(worker)))
    {
      fprintf(stderr, "\nE12-1: removing worker '%s' from '%s' failed\n", 
            worker->GetIDENTITY(), service2->GetNAME());
    }
    else
    {
      fprintf(stderr, "\nI12-1: remove worker '%s' from '%s'\n", 
              worker->GetIDENTITY(), service2->GetNAME());
    }
#else 
    fprintf(stderr, "\nI12: SKIP POPING WORKER FROM SERVICE '%s'\n", service_name_2); fflush(stderr);
#endif
    delete worker;

/* ########################################################### */
/* REMOVE SERVICE2                                             */
/* ########################################################### */
#if 1
    if (false == (status = database->RemoveService(service_name_2)))
    {
      fprintf(stderr, "\nE13: remove service '%s' failed\n",
              service_name_2);
      break;
    }
    fprintf(stderr, "\nI13: removing service '%s'\n", service_name_2); fflush(stderr);
#else
    fprintf(stderr, "\nI13: SKIP REMOVING SERVICE '%s'\n", service_name_2); fflush(stderr);
#endif
    delete service2;

    fprintf(stderr, "\nALL DONE\n"); fflush(stderr);
    status = true;
    break;
  }

  delete database;
  //mco_db_kill("BrokerDB");

  return (status == true)? 0 : -1;
}
