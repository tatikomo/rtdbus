#include <string>
#include "gtest/gtest.h"

#include "xdb_database_broker.hpp"
#include "dat/xdb_broker.hpp"

char *service_name = (char*)"service_test_1";
XDBDatabaseBroker *database;

TEST(TestBrokerDATABASE, CREATE)
{
    bool status;

    database = new XDBDatabaseBroker();
    ASSERT_TRUE (database != NULL);

    status = database->Connect();
    EXPECT_EQ(status, true);
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
    
    status = database->IsServiceExist(service_name);
    EXPECT_EQ(status, true);

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

