#include <string>
#include "gtest/gtest.h"

#include "xdb_database_broker.hpp"
#include "xdb_database_service.hpp"
#include "xdb_database_worker.hpp"
#include "dat/xdb_broker.hpp"

char *service_name = (char*)"service_test_1";
char *worker_identity = (char*)"SN1_AAAAAAA";
XDBDatabaseBroker *database = NULL;
Service *service = NULL;
int64_t service_id;

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

    status = database->IsServiceExist(service_name);
    EXPECT_EQ(status, false);

    status = database->AddService(service_name);
    EXPECT_EQ(status, true);

    status = database->IsServiceExist(service_name);
    EXPECT_EQ(status, true);

    service = database->GetServiceByName(service_name);
    ASSERT_TRUE (service != NULL);
    service_id = service->GetID();
    // В пустой базе индексы начинаются с 1, поэтому service id=1
    EXPECT_EQ(service_id, 1);
}

TEST(TestBrokerDATABASE, INSERT_WORKER)
{
    Worker  *worker  = NULL;
    bool status;

    worker = database->PopWorker(service);
    /* Обработчиков еще нет - worker д.б. = NULL */
    ASSERT_TRUE (worker == NULL);
    delete worker;

    worker = new Worker(worker_identity, service_id);
    status = database->PushWorker(worker);
    EXPECT_EQ(status, true);
    delete worker;

    worker = database->PopWorker(service);
    ASSERT_TRUE (worker != NULL);
    ASSERT_TRUE (0 == (strcmp(worker->GetIDENTITY(), worker_identity)));
    const timer_mark_t expiration_time_mark = worker->GetEXPIRATION();
    std::cout << "worker: '" << worker->GetIDENTITY() << "' "
              << "state: " << worker->GetSTATE() << " "
              << "expiration: " 
              << expiration_time_mark.tv_sec << "."
              << expiration_time_mark.tv_nsec << std::endl;
    delete worker;

    /*
     * У Сервиса был зарегистрирован только один Обработчик,
     * Вторая попытка получения Обработчика из спула не должна
     * ничего возвращать
     */
    worker = database->PopWorker(service);
    ASSERT_TRUE (worker == NULL);
}

TEST(TestBrokerDATABASE, REMOVE_WORKER)
{
    Worker  *worker  = NULL;
    bool status;

    worker = database->PopWorker(service);
    ASSERT_TRUE (worker != NULL);
    EXPECT_EQ(worker->GetSTATE(), Worker::ARMED);
    status = database->RemoveWorker(worker);
    EXPECT_EQ(status, true);

    worker = database->PopWorker(service);
    ASSERT_TRUE (worker == NULL);
}

TEST(TestBrokerDATABASE, CHECK_EXIST_SERVICE)
{
    bool status;
    char *unbelievable_service_name = (char*)"unbelievable_service";
    Service *service = NULL;
    
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

