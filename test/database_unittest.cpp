#include <string>
#include "gtest/gtest.h"

#include "xdb_database_broker.hpp"
#include "xdb_database_service.hpp"
#include "dat/xdb_broker.hpp"

char *service_name = (char*)"service_test_1";
XDBDatabaseBroker *database;

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

TEST(TestBrokerDATABASE, INSERT)
{
    bool status;

    status = database->IsServiceExist(service_name);
    EXPECT_EQ(status, false);

    status = database->AddService(service_name);
    EXPECT_EQ(status, true);

    status = database->IsServiceExist(service_name);
    EXPECT_EQ(status, true);
}

TEST(TestBrokerDATABASE, CHECK_EXIST)
{
    bool status;
    char *unbelievable_service_name = (char*)"unbelievable_service";
    XDBService *service = NULL;
    
    status = database->IsServiceExist(service_name);
    EXPECT_EQ(status, true);

    service = database->GetServiceByName(service_name);
    ASSERT_TRUE (service != NULL);
    ASSERT_TRUE(strcmp(service->GetNAME(), service_name) == 0);
    printf("%s.auto_id = %lld\n", service->GetNAME(), service->GetID());
    if (service)
      delete service;

    status = database->IsServiceExist(unbelievable_service_name);
    EXPECT_EQ(status, false);
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
    ASSERT_TRUE (database != NULL);
    delete database;
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

