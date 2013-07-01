#include <string>
#include "gtest/gtest.h"

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

    /* В пустой базе нет заранее предопределенных Сервисов - ошибка */
    status = database->IsServiceExist(service_name_1);
    EXPECT_EQ(status, false);

    /* В пустой базе нет заранее предопределенных Сервисов - ошибка */
    status = database->IsServiceExist(service_name_2);
    EXPECT_EQ(status, false);

    /* Добавим первый Сервис */
    status = database->AddService(service_name_1);
    EXPECT_EQ(status, true);

    status = database->IsServiceExist(service_name_1);
    EXPECT_EQ(status, true);

#if 0
    /* Добавим второй Сервис */
    status = database->AddService(service_name_2);
    EXPECT_EQ(status, true);

    status = database->IsServiceExist(service_name_2);
    EXPECT_EQ(status, true);
#endif

    service1 = database->GetServiceByName(service_name_1);
    ASSERT_TRUE (service1 != NULL);
    service1_id = service1->GetID();
    // В пустой базе индексы начинаются с 1, поэтому у первого Сервиса id=1
    EXPECT_EQ(service1_id, 1);

    
#if 0
    service2 = database->GetServiceByName(service_name_2);
    ASSERT_TRUE (service2 != NULL);
    service2_id = service2->GetID();
#endif

    database->MakeSnapshot();
}

TEST(TestBrokerDATABASE, INSERT_WORKER)
{
    Worker  *worker  = NULL;
    bool status;

    worker = database->PopWorker(service1);
    /* Обработчиков еще нет - worker д.б. = NULL */
    ASSERT_TRUE (worker == NULL);
    delete worker;

    worker = new Worker(worker_identity_1, service1_id);
    status = database->PushWorker(worker);
    EXPECT_EQ(status, true);
    delete worker;
    database->MakeSnapshot();

    /*
     * Повторная регистрация Обработчика с идентификатором, который там уже 
     * присутствует, приводит к замещению предыдущего экземпляра 
     */
    worker = new Worker(worker_identity_1, service1_id);
    status = database->PushWorker(worker);
    EXPECT_EQ(status, true);
    delete worker;
    database->MakeSnapshot();

    /* Можно поместить не более двух Обработчиков с разными идентификаторами */
    worker = new Worker(worker_identity_2, service1_id);
    status = database->PushWorker(worker);
    EXPECT_EQ(status, true);
    delete worker;
    database->MakeSnapshot();

    /* Ошибка помещения третьего экземпляра в спул */
    worker = new Worker(worker_identity_3, service1_id);
    status = database->PushWorker(worker);
    EXPECT_EQ(status, false);
    delete worker;

#if 0
    /* Помещение экземпляра в спул второго Сервиса */
    worker = new Worker(worker_identity_3, service2_id);
    status = database->PushWorker(worker);
    EXPECT_EQ(status, true);
    delete worker;
    database->MakeSnapshot();
#endif
}

TEST(TestBrokerDATABASE, REMOVE_WORKER)
{
    Worker  *worker  = NULL;
    timer_mark_t expiration_time_mark;
    bool status;

    worker = database->PopWorker(service1);
    ASSERT_TRUE (worker != NULL);
    ASSERT_TRUE (0 == (strcmp(worker->GetIDENTITY(), worker_identity_1)));
    expiration_time_mark = worker->GetEXPIRATION();
    std::cout << "worker: '" << worker->GetIDENTITY() << "' "
              << "state: " << worker->GetSTATE() << " "
              << "expiration: " 
              << expiration_time_mark.tv_sec << "."
              << expiration_time_mark.tv_nsec << std::endl;
    delete worker;
    database->MakeSnapshot();

    worker = database->PopWorker(service1);
    ASSERT_TRUE (worker != NULL);
    ASSERT_TRUE (0 == (strcmp(worker->GetIDENTITY(), worker_identity_2)));
    expiration_time_mark = worker->GetEXPIRATION();
    std::cout << "worker: '" << worker->GetIDENTITY() << "' "
              << "state: " << worker->GetSTATE() << " "
              << "expiration: " 
              << expiration_time_mark.tv_sec << "."
              << expiration_time_mark.tv_nsec << std::endl;
    delete worker;
    database->MakeSnapshot();

    /*
     * У Сервиса было зарегистрировано только два Обработчика,
     * Третья попытка получения Обработчика из спула не должна
     * ничего возвращать
     */
    worker = database->PopWorker(service1);
    ASSERT_TRUE (worker == NULL);


#if 0

    worker = database->PopWorker(service);
    ASSERT_TRUE (worker != NULL);
    EXPECT_EQ(worker->GetSTATE(), Worker::ARMED);
    database->MakeSnapshot();

    status = database->RemoveWorker(worker);
    EXPECT_EQ(status, true);
    database->MakeSnapshot();

    worker = database->PopWorker(service);
    ASSERT_TRUE (worker == NULL);
    database->MakeSnapshot();

#endif
}

TEST(TestBrokerDATABASE, CHECK_EXIST_SERVICE)
{
    bool status;
    Service *service = NULL;
    
    status = database->IsServiceExist(service_name_1);
    EXPECT_EQ(status, true);

    service = database->GetServiceByName(service_name_1);
    ASSERT_TRUE (service != NULL);
    ASSERT_TRUE(strcmp(service->GetNAME(), service_name_1) == 0);
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
    Worker  *worker  = NULL;

    status = database->RemoveService(unbelievable_service_name);
    EXPECT_EQ(status, false);

    status = database->RemoveService(service_name_1);
    EXPECT_EQ(status, true);
    database->MakeSnapshot();

#if 0
    worker = database->PopWorker(service2);
    ASSERT_TRUE (worker != NULL);
    database->MakeSnapshot();

    status = database->RemoveWorker(worker);
    EXPECT_EQ(status, true);
    database->MakeSnapshot();

    status = database->RemoveService(service_name_2);
    EXPECT_EQ(status, true);
    database->MakeSnapshot();
#endif
}

TEST(TestBrokerDATABASE, DESTROY)
{
    ASSERT_TRUE (service1 != NULL);
    delete service1;

#if 0
    ASSERT_TRUE (service2 != NULL);
    delete service2;
#endif

    ASSERT_TRUE (database != NULL);
    database->Disconnect();
    delete database;
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

