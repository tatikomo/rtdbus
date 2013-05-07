#include <string>
#include "gtest/gtest.h"

#include "xdb_database_broker.hpp"
#include "xdb_database_service.hpp"
#include "xdb_database_worker.hpp"
#include "dat/xdb_broker.hpp"

char *service_name = (char*)"service_test_1";
char *worker_identity = (char*)"SN1_AAAAAAA";
XDBDatabaseBroker *database = NULL;
XDBService *service = NULL;

TEST(TestBrokerDATABASE, OPEN)
{
    bool status;
    XDBDatabase::DBState state;

    database = new XDBDatabaseBroker();
    ASSERT_TRUE (database != NULL);

    state = database->State();
    EXPECT_EQ(state, XDBDatabase::DISCONNECTED);

    status = database->Connect();
    EXPECT_EQ(status, true);

    state = database->State();
    EXPECT_EQ(state, XDBDatabase::CONNECTED);

    status = database->Open();
    EXPECT_EQ(status, true);

    state = database->State();
    EXPECT_EQ(state, XDBDatabase::OPENED);
}

TEST(TestBrokerDATABASE, INSERT_SERVICE)
{
    bool status;
    int64_t id;

    status = database->IsServiceExist(service_name);
    EXPECT_EQ(status, false);

    status = database->AddService(service_name);
    EXPECT_EQ(status, true);

    status = database->IsServiceExist(service_name);
    EXPECT_EQ(status, true);

    service = database->GetServiceByName(service_name);
    ASSERT_TRUE (service != NULL);
    id = service->GetID();
    EXPECT_EQ(id, 1);
}

TEST(TestBrokerDATABASE, INSERT_WORKER)
{
    XDBWorker  *worker  = NULL;
    bool status;

    worker = database->PopWorkerForService(service);
    /* Обработчиков еще нет - worker д.б. = NULL */
    ASSERT_TRUE (worker == NULL);

    worker = new XDBWorker(worker_identity);
    status = database->PushWorkerForService(service, worker);
    EXPECT_EQ(status, true);

    worker = database->PopWorkerForService(service);
    ASSERT_TRUE (worker != NULL);
    //std::cout <<  worker->GetIDENTITY() << std::endl;
    ASSERT_TRUE (0 == (strcmp(worker->GetIDENTITY(), worker_identity)));
    delete worker;
}

TEST(TestBrokerDATABASE, CHECK_EXIST_SERVICE)
{
    bool status;
    char *unbelievable_service_name = (char*)"unbelievable_service";
    XDBService *service = NULL;
    
    status = database->IsServiceExist(service_name);
    EXPECT_EQ(status, true);

    service = database->GetServiceByName(service_name);
    ASSERT_TRUE (service != NULL);
    ASSERT_TRUE(strcmp(service->GetNAME(), service_name) == 0);
    //printf("%s.auto_id = %lld\n", service->GetNAME(), service->GetID());
    delete service;

    status = database->IsServiceExist(unbelievable_service_name);
    EXPECT_EQ(status, false);
}

TEST(TestBrokerDATABASE, CHECK_EXIST_WORKER)
{
    bool status;
}

TEST(TestBrokerDATABASE, REMOVE)
{
    bool status;

    status = database->IsServiceExist(service_name);
    EXPECT_EQ(status, true);

    status = database->RemoveService(service_name);
    EXPECT_EQ(status, true);

    status = database->IsServiceExist(service_name);
    EXPECT_EQ(status, false);
}

TEST(TestBrokerDATABASE, DESTROY)
{
    ASSERT_TRUE (service != NULL);
    delete service;

    ASSERT_TRUE (database != NULL);
    database->Disconnect();
    delete database;
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

