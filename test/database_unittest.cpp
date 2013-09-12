#include <string>
#include <glog/logging.h>
#include "gtest/gtest.h"

#include "xdb_database_broker_impl.hpp"
#include "xdb_database_broker.hpp"
#include "xdb_database_service.hpp"
#include "xdb_database_worker.hpp"
#include "dat/xdb_broker.hpp"

const char *service_name_1 = "service_test_1";
const char *service_name_2 = "service_test_2";
const char *unbelievable_service_name = "unbelievable_service";
const char *worker_identity_1 = "SN1_AAAAAAA";
const char *worker_identity_2 = "SN1_WRK2";
const char *worker_identity_3 = "WRK3";
XDBDatabaseBroker *database = NULL;
Service *service1 = NULL;
Service *service2 = NULL;
int64_t service1_id;
int64_t service2_id;
XDBDatabase::DBState state;

void wait()
{
//  puts("\nNext...");
//  getchar();
}

TEST(TestBrokerDATABASE, OPEN)
{
    bool status;

    database = new XDBDatabaseBroker();
    ASSERT_TRUE (database != NULL);

    state = database->State();
    EXPECT_EQ(state, XDBDatabase::UNINITIALIZED);

    status = database->Connect();
    ASSERT_TRUE(status == true);

    state = database->State();
    ASSERT_TRUE(state == XDBDatabase::CONNECTED);
}

TEST(TestBrokerDATABASE, INSERT_SERVICE)
{
    bool status;
    Service *srv;

    /* В пустой базе нет заранее предопределенных Сервисов - ошибка */
    status = database->IsServiceExist(service_name_1);
    EXPECT_EQ(status, false);

    /* В пустой базе нет заранее предопределенных Сервисов - ошибка */
    status = database->IsServiceExist(service_name_2);
    EXPECT_EQ(status, false);

    /* Добавим первый Сервис */
    srv = database->AddService(service_name_1);
    EXPECT_NE(srv, (Service*)NULL);
    delete srv;

    status = database->IsServiceExist(service_name_1);
    EXPECT_EQ(status, true);

    service1 = database->GetServiceByName(service_name_1);
    ASSERT_TRUE (service1 != NULL);
    service1_id = service1->GetID();
    // В пустой базе индексы начинаются с 1, поэтому у первого Сервиса id=1
    EXPECT_EQ(service1_id, 1);
    LOG(INFO) << "Service "<<service1->GetNAME()<<":"<<service1->GetID()<<" is added";

    /* Добавим второй Сервис */
    srv = database->AddService(service_name_2);
    EXPECT_NE(srv, (Service*)NULL);
    delete srv;

    status = database->IsServiceExist(service_name_2);
    EXPECT_EQ(status, true);

    service2 = database->GetServiceByName(service_name_2);
    ASSERT_TRUE (service2 != NULL);
    service2_id = service2->GetID();
    EXPECT_EQ(service2_id, 2);
    LOG(INFO) << "Service "<<service2->GetNAME()<<":"<<service2->GetID()<<" is added";

    database->MakeSnapshot("ONLY_SRV");
}

TEST(TestBrokerDATABASE, INSERT_WORKER)
{
    Worker  *worker0  = NULL;
    Worker  *worker1_1= NULL;
    Worker  *worker1_2= NULL;
    Worker  *worker2  = NULL;
    Worker  *worker3  = NULL;
    bool status;

    worker0 = database->PopWorker(service1);
    /* Обработчиков еще нет - worker д.б. = NULL */
    EXPECT_TRUE (worker0 == NULL);
    delete worker0;

    /*
     * УСПЕШНО 
     * Поместить первого Обработчика в спул Службы
     */
    worker1_1 = new Worker(service1_id, worker_identity_1);
    status = database->PushWorker(worker1_1);
    EXPECT_EQ(status, true);
    LOG(INFO) << "Worker "<<worker1_1->GetIDENTITY()<<":"<<worker1_1->GetID()
              <<" is added for service id="<<service1_id;
    database->MakeSnapshot("INS_WRK_1_A");
    EXPECT_NE(worker1_1->GetID(), 0);
    wait();

    /*
     * УСПЕШНО
     * Повторная регистрация Обработчика с идентификатором, который там уже 
     * присутствует.
     * Должно призойти замещение предыдущего экземпляра Обработчика worker_identity_1
     */
    worker1_2 = new Worker(service1_id, worker_identity_1);
    status = database->PushWorker(worker1_2);
    EXPECT_EQ(status, true);
    LOG(INFO) << "Worker "<<worker1_2->GetIDENTITY()<<":"<<worker1_2->GetID()
              <<" is added for service id="<<service1_id;
    database->MakeSnapshot("INS_WRK_1_B");
    EXPECT_EQ(worker1_1->GetID(), worker1_2->GetID());
    wait();

    /* 
     * УСПЕШНО
     * Можно поместить нескольких Обработчиков с разными идентификаторами
     */
    worker2 = new Worker(service1_id, worker_identity_2);
    status = database->PushWorker(worker2);
    EXPECT_EQ(status, true);
    LOG(INFO) << "Worker "<<worker2->GetIDENTITY()<<":"<<worker2->GetID()
              << " is added for service id="<<service1_id;
    database->MakeSnapshot("INS_WRK_2");
    EXPECT_NE(worker1_2->GetID(), worker2->GetID());
    wait();

    /* 
     * УСПЕШНО
     * Помещение экземпляра в спул второго Сервиса 
     */
    worker3 = new Worker(service2_id, worker_identity_3);
    status = database->PushWorker(worker3);
    EXPECT_EQ(status, true);
    LOG(INFO) << "Worker "<<worker3->GetIDENTITY()<<":"<<worker3->GetID()
              << " is added for service id="<<service2_id;
    database->MakeSnapshot("INS_WRK_3");
    EXPECT_NE(worker2->GetID(),   worker3->GetID());
    wait();

    delete worker1_1;
    delete worker1_2;
    delete worker2;
    delete worker3;
}

TEST(TestBrokerDATABASE, REMOVE_WORKER)
{
    Worker  *worker  = NULL;
    timer_mark_t expiration_time_mark;
    bool status;

    worker = database->PopWorker(service1);
    ASSERT_TRUE (worker != NULL);
    EXPECT_EQ(worker->GetSERVICE_ID(), service1_id);
    EXPECT_EQ(worker->GetSTATE(), Worker::ARMED);
    // Проверка пока не возможна, поскольку порядок возвращения экземпляров
    // может не совпадать с порядком их помещения в базу
    //EXPECT_STREQ(worker->GetIDENTITY(), worker_identity_2);
    expiration_time_mark = worker->GetEXPIRATION();
#if 0
    std::cout << "worker: '" << worker->GetIDENTITY() << "' "
              << "state: " << worker->GetSTATE() << " "
              << "expiration: " 
              << expiration_time_mark.tv_sec << "."
              << expiration_time_mark.tv_nsec << std::endl;
#endif

    database->MakeSnapshot("POP_WRK_1");
    wait();

    status = database->RemoveWorker(worker);
    EXPECT_EQ(status, true);
    EXPECT_EQ(worker->GetSTATE(), Worker::DISARMED);
    LOG(INFO) << "Worker "<<worker->GetIDENTITY()<<" removed";
    database->MakeSnapshot("DIS_WRK_1");
    wait();
    delete worker;

    worker = database->PopWorker(service1);
    ASSERT_TRUE (worker != NULL);
    EXPECT_EQ(worker->GetSERVICE_ID(), service1_id);
    EXPECT_EQ(worker->GetSTATE(), Worker::ARMED);
    //EXPECT_STREQ(worker->GetIDENTITY(), worker_identity_1);
    expiration_time_mark = worker->GetEXPIRATION();
#if 0
    std::cout << "worker: '" << worker->GetIDENTITY() << "' "
              << "state: " << worker->GetSTATE() << " "
              << "expiration: " 
              << expiration_time_mark.tv_sec << "."
              << expiration_time_mark.tv_nsec << std::endl;
#endif

    database->MakeSnapshot("POP_WRK_2");
    wait();

    status = database->RemoveWorker(worker);
    EXPECT_EQ(status, true);
    EXPECT_EQ(worker->GetSTATE(), Worker::DISARMED);
    LOG(INFO) << "Worker "<<worker->GetIDENTITY()<<" removed";
    database->MakeSnapshot("DIS_WRK_2");
    wait();
    delete worker;
    /*
     * У Сервиса-1 было зарегистрировано только два Обработчика,
     * и они все только что были удалены RemoveWorker-ом.
     * Эта попытка получения Обработчика из спула не должна ничего 
     * возвращать, т.к. у Сервиса1 нет Обработчиков в состоянии ARMED
     */
    worker = database->PopWorker(service1);
    ASSERT_TRUE (worker == NULL);
    delete worker;

    /*
     * Получить и деактивировать единственный у Сервиса-2 Обработчик
     */
    worker = database->PopWorker(service2);
    ASSERT_TRUE (worker != NULL);
    EXPECT_EQ(worker->GetSERVICE_ID(), service2_id);
    EXPECT_EQ(worker->GetSTATE(), Worker::ARMED);
    database->MakeSnapshot("POP_WRK_3");

    status = database->RemoveWorker(worker);
    EXPECT_EQ(status, true);
    EXPECT_EQ(worker->GetSTATE(), Worker::DISARMED);
    LOG(INFO) << "Worker "<<worker->GetIDENTITY()<<" removed";
    delete worker;
    database->MakeSnapshot("DIS_WRK_3");
    /*
     * У Сервиса-2 был зарегистрирован только один Обработчик,
     * Вторая попытка получения Обработчика из спула не должна
     * вернуть Обработчика, т.к. оставшиеся находятся в DISARMED
     */
    worker = database->PopWorker(service2);
    ASSERT_TRUE (worker == NULL);
}

TEST(TestBrokerDATABASE, CHECK_SERVICE)
{
    bool status;
    Service *service = NULL;
    
    status = database->IsServiceExist(service_name_1);
    EXPECT_EQ(status, true);

    service = database->GetServiceByName(service_name_1);
    ASSERT_TRUE (service != NULL);
    EXPECT_STREQ(service->GetNAME(), service_name_1);
    //printf("%s.auto_id = %lld\n", service->GetNAME(), service->GetID());
    delete service;

    status = database->IsServiceExist(unbelievable_service_name);
    EXPECT_EQ(status, false);

    // TODO: Получить список имеющихся в БД Служб
}


TEST(TestBrokerDATABASE, SERVICE_LIST)
{
  bool status = false;
  Service*  srv = NULL;
  ServiceList *services_list = database->GetServiceList();
  ASSERT_TRUE (services_list != NULL);

  int services_count = services_list->size();
  EXPECT_EQ(services_count, 2); // service_test_1 + service_test_2

  srv = services_list->last();
  ASSERT_TRUE(srv != NULL);

  srv = services_list->first();
  while (srv)
  {
    ASSERT_TRUE(srv != NULL);
    LOG(INFO) << "FIRST SERVICE_ITERATOR " << srv->GetNAME() << ":" << srv->GetID();
    srv = services_list->next();
  }

  // Перечитать список Сервисов из базы данных
  status = services_list->refresh();
  EXPECT_EQ(status, true);

  srv = services_list->first();
  while (srv)
  {
    ASSERT_TRUE(srv != NULL);
    LOG(INFO) << "REFRESHED SERVICE_ITERATOR " << srv->GetNAME() << ":" << srv->GetID();
    srv = services_list->next();
  }

  // Создать список из Сервисов и инициировать его из БД. 
//  srv_array = services_list->getList();
//  ASSERT_TRUE(srv_array != NULL);
}

TEST(TestBrokerDATABASE, CHECK_LETTER)
{
  Letter *letter = new Letter();

}

TEST(TestBrokerDATABASE, CHECK_EXIST_WORKER)
{
}

TEST(TestBrokerDATABASE, REMOVE_SERVICE)
{
    bool status;

    status = database->RemoveService(unbelievable_service_name);
    EXPECT_EQ(status, false);

    status = database->RemoveService(service_name_1);
    EXPECT_EQ(status, true);
    database->MakeSnapshot("WO_SRV_1");
    wait();

    status = database->RemoveService(service_name_2);
    EXPECT_EQ(status, true);
    database->MakeSnapshot("WO_ALL_SRV");
    wait();
}

TEST(TestBrokerDATABASE, DESTROY)
{
    ASSERT_TRUE (service1 != NULL);
    delete service1;

    ASSERT_TRUE (service2 != NULL);
    delete service2;

#if 0
    ASSERT_TRUE (database != NULL);
    database->Disconnect();

    state = database->State();
    EXPECT_EQ(state, XDBDatabase::DISCONNECTED);
#endif

    delete database;
}

int main(int argc, char** argv)
{
  ::google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InstallFailureSignalHandler();
  int retval = RUN_ALL_TESTS();
  ::google::ShutdownGoogleLogging();
  return retval;
}

