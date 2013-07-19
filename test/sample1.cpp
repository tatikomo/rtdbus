#include <string>

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
int64_t service1_id;
int64_t service2_id;

int main(int argc, char **argv)
{
  bool status = false;
  XDBDatabase::DBState state;

  database = new XDBDatabaseBroker();
  if (NULL != database)
  {
    status = true;
  }

  delete database;
  //mco_db_kill("BrokerDB");

  return (status == true)? 0 : -1;
}
