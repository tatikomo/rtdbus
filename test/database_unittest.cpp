#include <iostream>
#include <string>

#include <stdlib.h> // putenv
#include <stdio.h>  // fprintf

#include <xercesc/util/PlatformUtils.hpp>

#include "glog/logging.h"
#include "gtest/gtest.h"

#include "xdb_common.hpp"

#include "xdb_impl_db_broker.hpp"
#include "xdb_broker.hpp"
#include "xdb_broker_service.hpp"
#include "xdb_broker_worker.hpp"
#include "dat/broker_db.hpp"
// Включение поддержки XML формата хранения данных instance_total
#include "dat/rtap_db.hxx"
#include "dat/rtap_db-pimpl.hxx"
#include "dat/rtap_db-pskel.hxx"
// Включение поддержки XML формата хранения словарей БДРВ
#include "dat/rtap_db_dict.hxx"
#include "dat/rtap_db_dict-pskel.hxx"
#include "dat/rtap_db_dict-pimpl.hxx"

#include "proto/common.pb.h"
#include "msg/msg_common.h"

#include "xdb_rtap_application.hpp"
#include "xdb_rtap_database.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"
#include "xdb_rtap_snap.hpp"

using namespace xdb;

const char *service_name_1 = "service_test_1";
const char *service_name_2 = "service_test_2";
const char *unbelievable_service_name = "unbelievable_service";
const char *worker_identity_1 = "SN1_AAAAAAA";
const char *worker_identity_2 = "SN1_WRK2";
const char *worker_identity_3 = "WRK3";
std::string group_name_1 = "Группа подписки #1";
std::string group_name_2 = "Группа подписки #2";
std::string group_name_unexistant = "Несуществующая";
const char *sbs_points[] = {
    "/KD4001/GOV022",       // GOFVALTSC
    "/KO4016/GOV171/PT01",  // GOFVALTI
    "/KA4003/XA01"          // GOFVALAL
};
xdb::DatabaseBroker *database = NULL;
xdb::Service *service1 = NULL;
xdb::Service *service2 = NULL;
xdb::Letter  *letter   = NULL;
int64_t service1_id;
int64_t service2_id;
DBState_t state;
xdb::RtApplication* app = NULL;
xdb::RtEnvironment* env = NULL;
xdb::RtConnection* connection = NULL;

/* 
 * Содержимое базы данных RTDB после чтения из файла classes.xml
 * Инициализируется в функции TestTools.LOAD_XML
 * Используется в функции TestRtapDATABASE.CREATE
 */
rtap_db::Points point_list;
/*
 * Экземпляр НСИ БДРВ RTAP
 */
rtap_db_dict::Dictionaries_t dict;

void wait()
{
#ifdef USE_EXTREMEDB_HTTP_SERVER
//  puts("\nNext...");
//  getchar();
#endif
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

TEST(TestBrokerDATABASE, OPEN)
{
    bool status;

    database = new xdb::DatabaseBroker();
    ASSERT_TRUE (database != NULL);

    state = static_cast<DBState_t>(database->State());
    ASSERT_TRUE(state == DB_STATE_UNINITIALIZED);

    status = (database->Connect());
    ASSERT_TRUE(status == true);

    state = static_cast<DBState_t>(database->State());
    ASSERT_TRUE(state == DB_STATE_CONNECTED);

#if VERBOSE
    show_runtime_info("Database runtime information:\n=======================================");
#endif
}

TEST(TestBrokerDATABASE, INSERT_SERVICE)
{
    bool status;
    xdb::Service *srv;

    /* В пустой базе нет заранее предопределенных Сервисов - ошибка */
    status = database->IsServiceExist(service_name_1);
    EXPECT_EQ(status, false);

    /* В пустой базе нет заранее предопределенных Сервисов - ошибка */
    status = database->IsServiceExist(service_name_2);
    EXPECT_EQ(status, false);

    /* Добавим первый Сервис */
    srv = database->AddService(service_name_1);
    EXPECT_NE(srv, (xdb::Service*)NULL);
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
    EXPECT_NE(srv, (xdb::Service*)NULL);
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
    xdb::Worker  *worker0  = NULL;
    xdb::Worker  *worker1_1= NULL;
    xdb::Worker  *worker1_2= NULL;
    xdb::Worker  *worker2  = NULL;
    xdb::Worker  *worker3  = NULL;
    bool status;

    worker0 = database->PopWorker(service1);
    /* Обработчиков еще нет - worker д.б. = NULL */
    EXPECT_TRUE (worker0 == NULL);
    delete worker0;

    /*
     * УСПЕШНО 
     * Поместить первого Обработчика в спул Службы
     */
    worker1_1 = new xdb::Worker(service1_id, worker_identity_1);
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
    worker1_2 = new xdb::Worker(service1_id, worker_identity_1);
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
    worker2 = new xdb::Worker(service1_id, worker_identity_2);
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
    worker3 = new xdb::Worker(service2_id, worker_identity_3);
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
    xdb::Worker  *worker  = NULL;
    timer_mark_t expiration_time_mark;
    bool status;

    worker = database->PopWorker(service1);
    ASSERT_TRUE (worker != NULL);
    EXPECT_EQ(worker->GetSERVICE_ID(), service1_id);
    EXPECT_EQ(worker->GetSTATE(), xdb::Worker::ARMED);
    // Проверка пока не возможна, поскольку порядок возвращения экземпляров
    // может не совпадать с порядком их помещения в базу
    //EXPECT_STREQ(worker->GetIDENTITY(), worker_identity_2);
    expiration_time_mark = worker->GetEXPIRATION();
#if defined VERBOSE
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
    EXPECT_EQ(worker->GetSTATE(), xdb::Worker::DISARMED);
    LOG(INFO) << "Worker "<<worker->GetIDENTITY()<<" removed";
    database->MakeSnapshot("DIS_WRK_1");
    wait();
    delete worker;

    worker = database->PopWorker(service1);
    ASSERT_TRUE (worker != NULL);
    EXPECT_EQ(worker->GetSERVICE_ID(), service1_id);
    EXPECT_EQ(worker->GetSTATE(), xdb::Worker::ARMED);
    //EXPECT_STREQ(worker->GetIDENTITY(), worker_identity_1);
    expiration_time_mark = worker->GetEXPIRATION();
#if defined VERBOSE
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
    EXPECT_EQ(worker->GetSTATE(), xdb::Worker::DISARMED);
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
    EXPECT_EQ(worker->GetSTATE(), xdb::Worker::ARMED);
    database->MakeSnapshot("POP_WRK_3");

    status = database->RemoveWorker(worker);
    EXPECT_EQ(status, true);
    EXPECT_EQ(worker->GetSTATE(), xdb::Worker::DISARMED);
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
    xdb::Service *service = NULL;
    
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
  xdb::Service*  srv = NULL;
  xdb::ServiceList *services_list = database->GetServiceList();
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
  char reply[IDENTITY_MAXLEN+1];
  char cause[IDENTITY_MAXLEN+1];
  RTDBM::Header     pb_header;
  RTDBM::ExecResult pb_exec_result_request;
  ::RTDBM::gof_t_ExecResult pb_exec_result_val;
  std::string       pb_serialized_header;
  std::string       pb_serialized_request;

  pb_header.set_protocol_version(1);
  pb_header.set_interest_id(1);
  pb_header.set_source_pid(getpid());
  pb_header.set_proc_dest("dest");
  pb_header.set_proc_origin("src");
  pb_header.set_sys_msg_type(USER_MESSAGE_TYPE);
  pb_header.set_usr_msg_type(ADG_D_MSG_EXECRESULT);
  pb_header.set_time_mark(time(0));

//  pb_exec_result_request.set_exec_result(0);
//  pb_exec_result_request.set_failure_cause(1);

  for (int i=1; i<10; i++)
  {
    sprintf(reply, "@C0000000%02d", i);
    sprintf(cause, "F_CAUSE_%02d", i);
    pb_header.set_exchange_id(i);
    pb_header.SerializeToString(&pb_serialized_header);

    pb_exec_result_val = static_cast<RTDBM::gof_t_ExecResult>(i % 2);
    pb_exec_result_request.set_exec_result(pb_exec_result_val);

    pb_exec_result_request.mutable_failure_cause()->set_error_code(i * 2);
    pb_exec_result_request.mutable_failure_cause()->set_error_text(cause);

    pb_exec_result_request.SerializeToString(&pb_serialized_request);

    letter = new xdb::Letter(reply, pb_serialized_header, pb_serialized_request);
    letter->Dump();
    delete letter;
  }
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
//    ASSERT_TRUE(letter != NULL);
//    delete letter;

    // NB: удаление этих двух объектов Сервиса приведет
    // к выводу ошибки в консоль, поскольку они уже удалены
    // ранее в тесте REMOVE_SERVICE.
    // Сохранился только класс, без своего представления в БДРВ.
    ASSERT_TRUE (service1 != NULL);
    delete service1;

    ASSERT_TRUE (service2 != NULL);
    delete service2;

#if 0
    ASSERT_TRUE (database != NULL);
    database->Disconnect();

    state = database->State();
    EXPECT_EQ(state, DB_STATE_DISCONNECTED);
#endif

#ifdef USE_EXTREMEDB_HTTP_SERVER
    printf("Press any key to continue...");
    int ch = getchar();
#endif

    delete database;
}

TEST(TestDiggerDATABASE, CREATION)
{
//  bool status = false;
  xdb::Error err;
  RtEnvironment *env1 = NULL;
//  const int argc = 3;
//  char *argv[argc] = {
//                    (char*)"test",
//                    (char*)"параметр_1",
//                    (char*)"параметр_2"
//                    };

  LOG(INFO) << "BEGIN CREATION TestDiggerDATABASE";

  app = new RtApplication("RTAP");
  ASSERT_TRUE(app != NULL);

  // Если снимок БД есть - прочитать его при старте,
  // Если снимка нет - подключения не будет
  app->setOption("OF_CREATE", 0);
  app->setOption("OF_LOAD_SNAP", 1);
  app->setOption("OF_RDWR",   1);
  app->setOption("OF_REGISTER_EVENT", 1); // Взвести CE
  app->setOption("OF_DATABASE_SIZE", 1024 * 1024 * 10);
  app->setOption("OF_MEMORYPAGE_SIZE", 1024); // 0..65535
  app->setOption("OF_MAP_ADDRESS",   0x30000000);
  EXPECT_EQ(app->getLastError().getCode(), xdb::rtE_NONE);

  // Завершить инициализацию
  LOG(INFO) << "Initialize: " << app->initialize().getCode();
  LOG(INFO) << "Operation mode: " << app->getOperationMode();
  LOG(INFO) << "Operation state: " << app->getOperationState();

  // Загрузить данные сохраненной среды RTAP
  // Экземпляр env принадлежит app и будет им удален
  env = app->loadEnvironment("SINF");
  // Проверим успешность создания экземпляра Среды БДРВ
  ASSERT_TRUE(env != NULL);
  // Проверим успешность восстановления данных БДРВ из снимка
  // При ошибке восстановления будет код ошибки rtE_XML_IMPORT
  EXPECT_EQ(env->getLastError().getCode(), rtE_NONE);

  // На данном этапе БД загружена из снимка, но еще нет подключения
  connection = env->getConnection();
  ASSERT_TRUE(connection != NULL);

#if 0
  // TODO Реализация
  env->Start();
  EXPECT_EQ(env->getLastError().getCode(), xdb::rtE_NONE /*rtE_NOT_IMPLEMENTED*/);
#endif

  err = env->MakeSnapshot(NULL);
  EXPECT_EQ(env->getLastError().getCode(), xdb::rtE_NONE);
  EXPECT_EQ(env->getLastError().getCode(), err.getCode());

  // Проверка корректности получения экземпляра Среды с одним 
  // названием и невозможности появления её дубликата
  env1 = app->loadEnvironment("SINF");
  ASSERT_TRUE(env1 != NULL);

  if (env && env1)
  {
    // Это должен быть один и тот же экземпляр с одинаковым именем
    EXPECT_TRUE(0 == strcmp(env->getName(), env1->getName()));
    // и адресом
    EXPECT_TRUE(env == env1);

    err = env->MakeSnapshot("DIGGER");
    EXPECT_EQ(env->getLastError().getCode(), xdb::rtE_NONE);
    EXPECT_EQ(env->getLastError().getCode(), err.getCode());
  }
  else
  {
    LOG(INFO) << "There is no existing environment 'SINF' for application "
              << app->getAppName();
  }

  LOG(INFO) << "END CREATION TestDiggerDATABASE";
}

TEST(TestDiggerDATABASE, READ_WRITE)
{
  AttributeInfo_t info;
  Error status;

  memset((void*)&info.value, '\0', sizeof(info.value));
  // Проверка чтения дискретного параметра
  // ================================
  info.name = "/KD4001/GOV022.STATUS";
  info.type = DB_TYPE_UNDEF;

  status = connection->read(&info);
  // Проверка успешности чтения данных
  EXPECT_EQ(status.code(), rtE_NONE);
  // После чтения атрибута должен определиться его тип
  EXPECT_EQ(info.type, DB_TYPE_UINT8);
  // Значение по-умолчанию = DISABLED
  // EXPECT_EQ(info.value.fixed.val_int8, /* DISABLED */ 2);
  if (info.value.fixed.val_int8 == /* DISABLED */ 2)
  {
    // Сохраненный снимок БДРВ имеет первоначальные значения
  }

  // Проверка записи - неуспешно
  // ================================
  info.type = DB_TYPE_UINT8; // не обязательно
  // Попытка записать запрещенное значение, допустимы только 0,1,2
  info.value.fixed.val_int32 = 101;
  status = connection->write(&info);
  // Недопустимое значение не должно писаться в БДРВ
  EXPECT_EQ(status.code(), rtE_ILLEGAL_PARAMETER_VALUE);

  // 0 = PLANNED
  // 1 = WORKED
  // 2 = DISABLED
  for (int stat = 0; stat < 3; stat++)
  {
      // Проверка записи - успешно
      // ================================
      info.value.fixed.val_int32 = stat;
      status = connection->write(&info);
      // Успешность записи корректного значения
      EXPECT_EQ(status.code(), rtE_NONE);

      // Проверка чтения - успешно
      // ================================
      status = connection->read(&info);
      // Проверка успешности чтения данных
      EXPECT_EQ(status.code(), rtE_NONE);
      // После чтения атрибута должен определиться его тип
      EXPECT_EQ(info.type, DB_TYPE_UINT8);
      EXPECT_EQ(info.value.fixed.val_int8, stat);
  }

  // Проверка чтения строкового параметра
  // ================================
  info.name = "/KD4001/GOV022.SHORTLABEL";
  info.type = DB_TYPE_UNDEF;
  info.value.dynamic.varchar = NULL;
  status = connection->read(&info);
  // Проверка успешности чтения данных
  EXPECT_EQ(status.code(), rtE_NONE);
  // После чтения атрибута должен определиться его тип
  EXPECT_EQ(info.type, DB_TYPE_BYTES16);
  std::cout << info.name << " = [" << info.value.dynamic.size << "] '"
            << info.value.dynamic.varchar << "'" << std::endl;
  delete [] info.value.dynamic.varchar;
}

TEST(TestDiggerDATABASE, UPDATE_EVENTS)
{
  AttributeInfo_t info_write;
  AttributeInfo_t info_read;
  Error status;

  memset((void*)&info_read.value, '\0', sizeof(info_read.value));
  memset((void*)&info_write.value, '\0', sizeof(info_write.value));
  info_write.type = DB_TYPE_UINT8;

  for (int att_idx=1; att_idx<2; att_idx++)
  {
    // Проверка срабатывания события при изменении VALIDCHANGE
    for(int /*ValidChange*/ code = 0/*VALIDCHANGE_FAULT*/; code <= 12/*VALIDCHANGE_END_INHIB_SA*/; code++)
    {
      std::cout << std::endl << "Iteration VALIDCHANGE=" << code << std::endl;
      // Изменить значения атрибутов VAL, VALACQ, VALMANUAL, VALID, VALIDACQ
      info_write.name = std::string(sbs_points[att_idx]) + "." + RTDB_ATT_VAL;
      info_write.value.fixed.val_double = 4321.5678;
      status = connection->write(&info_write);
      if (status.code() == rtE_NONE)
        std::cout << "Set "<< info_write.name <<"="<< info_write.value.fixed.val_double << std::endl;

      info_write.name = std::string(sbs_points[att_idx]) + "." + RTDB_ATT_VALACQ;
      info_write.value.fixed.val_double = 1234.5678;
      status = connection->write(&info_write);
      if (status.code() == rtE_NONE)
        std::cout << "Set "<< info_write.name <<"="<< info_write.value.fixed.val_double << std::endl;

      info_write.name = std::string(sbs_points[att_idx]) + "." + RTDB_ATT_VALMANUAL;
      info_write.value.fixed.val_double = 5678.1234;
      status = connection->write(&info_write);
      if (status.code() == rtE_NONE)
        std::cout << "Set "<< info_write.name <<"="<< info_write.value.fixed.val_double << std::endl;

      info_write.name = std::string(sbs_points[att_idx]) + "." + RTDB_ATT_VALID;
      info_write.value.fixed.val_uint8 = 1;
      status = connection->write(&info_write);
      if (status.code() == rtE_NONE)
        std::cout << "Set "<<info_write.name <<"="<< (unsigned int)info_write.value.fixed.val_uint8 << std::endl;

      info_write.name = std::string(sbs_points[att_idx]) + "." + RTDB_ATT_VALIDACQ;
      info_write.value.fixed.val_uint8 = 1;
      status = connection->write(&info_write);
      if (status.code() == rtE_NONE)
        std::cout << "Set "<<info_write.name <<"="<< (unsigned int)info_write.value.fixed.val_uint8 << std::endl;

      // Изменив значение VALIDCHANGE, инициировать выполнение CE
      info_write.name = std::string(sbs_points[att_idx]) + "." + RTDB_ATT_VALIDCHANGE;
      info_write.value.fixed.val_uint8 = code;
      status = connection->write(&info_write);
      // Проверка успешности записи данных
      EXPECT_EQ(status.code(), rtE_NONE);
      if (status.code() != rtE_NONE)
        std::cout << "Error '" << info_write.name << "' update: " << status.what() << std::endl;
      else
        std::cout << "Set "<<info_write.name << "=" << (unsigned int)info_write.value.fixed.val_uint8 << std::endl;

      // Прочесть рассчитанные CE значения атрибутов VAL, VALACQ, VALID, VALIDACQ
      info_read.name = std::string(sbs_points[att_idx]) + "." + RTDB_ATT_VAL;
      status = connection->read(&info_read);
      if (status.code() == rtE_NONE)
        std::cout << "Get "<< info_read.name <<"="<< info_read.value.fixed.val_double << std::endl;

      info_read.name = std::string(sbs_points[att_idx]) + "." + RTDB_ATT_VALACQ;
      status = connection->read(&info_read);
      if (status.code() == rtE_NONE)
        std::cout << "Get "<< info_read.name <<"="<< info_read.value.fixed.val_double << std::endl;

      info_read.name = std::string(sbs_points[att_idx]) + "." + RTDB_ATT_VALID;
      status = connection->read(&info_read);
      if (status.code() == rtE_NONE)
        std::cout << "Get "<<info_read.name <<"="<< (unsigned int)info_read.value.fixed.val_uint8 << std::endl;

      info_read.name = std::string(sbs_points[att_idx]) + "." + RTDB_ATT_VALIDACQ;
      status = connection->read(&info_read);
      if (status.code() == rtE_NONE)
        std::cout << "Get "<<info_read.name <<"="<< (unsigned int)info_read.value.fixed.val_uint8 << std::endl;

      info_read.name = std::string(sbs_points[att_idx]) + "." + RTDB_ATT_VALIDCHANGE;
      status = connection->read(&info_read);
      if (status.code() == rtE_NONE)
        std::cout << "Get "<<info_read.name <<"="<< (unsigned int)info_read.value.fixed.val_uint8 << std::endl;

    }
  }
}

// Удаление ранее созданной группы
TEST(TestDiggerDATABASE, INIT_SBS)
{
  rtDbCq operation;

//  env->MakeSnapshot("BEFORE_INIT_SBS");
  // Название группы подписки
  operation.buffer = static_cast<void*>(&group_name_1);
  // Сначала удалить возможно ранее созданную группу.
  // Код удаления групп group_name_1 и group_name_2 может быть ошибочным,
  // это допустимо для свежих снимков БДРВ.
  operation.action.config = rtCONFIG_DEL_GROUP_SBS;
  const Error& err_1 = connection->ConfigDatabase(operation);
  std::cout << "Delete subscription group " << group_name_1 << " was: " << err_1.what() << std::endl;

  operation.buffer = static_cast<void*>(&group_name_2);
  const Error& err_2 = connection->ConfigDatabase(operation);
  std::cout << "Delete subscription group " << group_name_2 << " was: " << err_2.what() << std::endl;

  // Попытка удаления группы group_name_unexistant должна быть ошибочной
  operation.buffer = static_cast<void*>(&group_name_unexistant);
  const Error& err_3 = connection->ConfigDatabase(operation);
  std::cout << "Delete subscription group " << group_name_unexistant << " was: " << err_3.what() << std::endl;
  EXPECT_EQ(err_3.code(), rtE_RUNTIME_ERROR);
}

// Тест создания группы подписки с заданными элементами
// Одна и та же точка может входить в несколько разных групп
TEST(TestDiggerDATABASE, CREATE_SBS)
{
  rtDbCq operation;
  std::vector<std::string> tags_list;

//  env->MakeSnapshot("BEFORE_CREATE_SBS");
  // Название группы подписки
  operation.buffer = static_cast<void*>(&group_name_1);
  // Тип конфигурирования - создание группы подписки
  operation.action.config = rtCONFIG_ADD_GROUP_SBS;

  tags_list.push_back("/KD4001/GOV022"); // Эта точка участвует в двух группах
  tags_list.push_back("/KD4001/GOV023");
  tags_list.push_back("/KD4001/GOV024");
  tags_list.push_back("/KD4001/GOV025");
  tags_list.push_back("/KD4001/GOV026");
  tags_list.push_back("/KD4001/GOV027");
  tags_list.push_back("/KD4001/GOV028");
  tags_list.push_back("/KD4001/GOV029");
  tags_list.push_back("/KD4001/GOV030");
  tags_list.push_back("/KD4001/GOV031");
  tags_list.push_back("/KD4001/GOV033");
  // Название точек, входящих в набор создаваемой группы
  operation.tags = &tags_list;
  operation.addrCnt = tags_list.size();
  
  const Error& status_1 = connection->ConfigDatabase(operation);
  EXPECT_EQ(status_1.code(), rtE_NONE);
  std::cout << "Create subscription group was: " << status_1.what() << std::endl;

  tags_list.clear();
  tags_list.push_back("/KD4001/GOV022");       // GOFVALTSC
  tags_list.push_back("/KO4016/GOV171/PT01");  // GOFVALTI
  tags_list.push_back("/KA4003/XA01");         // GOFVALAL
  operation.addrCnt = tags_list.size();
  operation.buffer = static_cast<void*>(&group_name_2);
  const Error& status_2 = connection->ConfigDatabase(operation);
  EXPECT_EQ(status_2.code(), rtE_NONE);
  std::cout << "Create subscription group was: " << status_2.what() << std::endl;
//  env->MakeSnapshot("AFTER_CREATE_SBS");
}

// Тест чтения всех значений точек из заданной группы подписки
TEST(TestDiggerDATABASE, READ_SBS)
{
  AttributeInfo_t info;
//  rtDbCq operation;
  xdb::SubscriptionPoints_t points_list;
  int list_size = 0;
  char s_date [D_DATE_FORMAT_LEN + 1];
  time_t given_time;
  char s_val[100];
  uint16_t  update_val = 0;
  unsigned int iteration, item;
  AttributeMapIterator_t it;
  Error status;
  std::string kd4005="KD4005_SS";

  status = connection->read(group_name_2, &list_size, NULL);
  EXPECT_EQ(status.code(), rtE_NONE);
  EXPECT_EQ(list_size, 3); // кол-во строк из group_name_2

  for (iteration = 0; iteration < 3; iteration++)
  {
      // Проверка чтения группы после изменения значения дискретного параметра
      // ================================
      memset((void*)&info.value, '\0', sizeof(info.value));
      info.name = "/KD4001/GOV035.VAL";
      info.type = DB_TYPE_UINT16;
      info.value.fixed.val_uint16 = ++update_val;
      status = connection->write(&info);
      EXPECT_EQ(status.code(), rtE_NONE);

      points_list.clear();

      list_size = 2;
      status = connection->read(kd4005/*group_name_2*/, &list_size, &points_list);
      EXPECT_EQ(status.code(), rtE_NONE);

      for(item=0; item < points_list.size(); item++)
      {
        std::cout << "#" << iteration << " read sbs tag:" << points_list.at(item)->tag
            << " objclass:" << points_list.at(item)->objclass
            << " attr#:" << points_list.at(item)->attributes.size()
            << std::endl;
        for(it = points_list.at(item)->attributes.begin();
            it != points_list.at(item)->attributes.end();
            it++ )
        {
          switch((*it).second.type)
          {
            case DB_TYPE_LOGICAL:   /* 1 */
              sprintf(s_val, "%d", (*it).second.value.fixed.val_bool);
            break;
            case DB_TYPE_INT8:      /* 2 */
              sprintf(s_val, "%+02d", (signed int)(*it).second.value.fixed.val_int8);
            break;
            case DB_TYPE_UINT8:     /* 3 */
              sprintf(s_val, "%02d", (unsigned int)(*it).second.value.fixed.val_uint8);
            break;
            case DB_TYPE_INT16:     /* 4 */
              sprintf(s_val, "%+05d", (*it).second.value.fixed.val_int16);
            break;
            case DB_TYPE_UINT16:    /* 5 */
              sprintf(s_val, "%05d", (*it).second.value.fixed.val_uint16);
            break;
            case DB_TYPE_INT32:     /* 6 */
              sprintf(s_val, "%+d", (*it).second.value.fixed.val_int32);
            break;
            case DB_TYPE_UINT32:    /* 7 */
              sprintf(s_val, "%d", (*it).second.value.fixed.val_uint32);
            break;
            case DB_TYPE_INT64:     /* 8 */
              sprintf(s_val, "%+lld", (*it).second.value.fixed.val_int64);
            break;
            case DB_TYPE_UINT64:    /* 9 */
              sprintf(s_val, "%lld", (*it).second.value.fixed.val_uint64);
            break;
            case DB_TYPE_FLOAT:     /* 10 */
              sprintf(s_val, "%f", (*it).second.value.fixed.val_float);
            break;
            case DB_TYPE_DOUBLE:    /* 11 */
              sprintf(s_val, "%g", (*it).second.value.fixed.val_double);
            break;
            case DB_TYPE_BYTES:     /* 12 */ // переменная длина строки
              sprintf(s_val, "%s", (*it).second.value.dynamic.val_string->c_str());
              delete (*it).second.value.dynamic.val_string;
            break;
            case DB_TYPE_BYTES4:    /* 13 */
            case DB_TYPE_BYTES8:    /* 14 */
            case DB_TYPE_BYTES12:   /* 15 */
            case DB_TYPE_BYTES16:   /* 16 */
            case DB_TYPE_BYTES20:   /* 17 */
            case DB_TYPE_BYTES32:   /* 18 */
            case DB_TYPE_BYTES48:   /* 19 */
            case DB_TYPE_BYTES64:   /* 20 */
            case DB_TYPE_BYTES80:   /* 21 */
            case DB_TYPE_BYTES128:  /* 22 */
            case DB_TYPE_BYTES256:  /* 23 */
              sprintf(s_val, "%s", (*it).second.value.dynamic.varchar);
              delete [] (*it).second.value.dynamic.varchar;
            break;
            case DB_TYPE_ABSTIME:   /* 24 */
              given_time = (*it).second.value.fixed.val_time.tv_sec;
              strftime(s_date, D_DATE_FORMAT_LEN, D_DATE_FORMAT_STR, localtime(&given_time));
              snprintf(s_val, D_DATE_FORMAT_W_MSEC_LEN, "%s.%06ld", s_date, (*it).second.value.fixed.val_time.tv_usec);
            break;
            case DB_TYPE_UNDEF:
              sprintf(s_val, "<undefined type: %d>", (*it).second.type);
            break;
            default:
              sprintf(s_val, "<unsupported type: %d>", (*it).second.type);
          }
          std::cout << "\t[t:" << (*it).second.type << "] " << (*it).first << ":\t" << s_val << std::endl;
        }

       delete points_list.at(item);
      }
  }
//  env->MakeSnapshot("AFTER_READ_SBS");
}

// Тест обнаружения изменений значений атрибутов точек из заданной группы подписки
// К этому тесту (если в группу 1 был включен "/KD4001/GOV022") изменения коснулись трех точек:
// /KD4001/GOV022       [aid:0x34A] [sbs_item aid:0x109D] [sbs_stat aid:0x109C "Группа подписки #1"]
// /KD4001/GOV035       [aid:0x362] [нет группы]
// /KO4016/GOV171/PT01  [aid:0xC30] [sbs_item aid:0x10AA] [sbs_stat aid:0x10A8 "Группа подписки #2"]
//
// План теста:
// 1. Изменить неск. атрибутов только одной точки из группы подписки
// 2. Найти группу, в которой произошло изменение значений одной из точек
// 3. Найти точку в этой группе, для которой произошли изменения
TEST(TestDiggerDATABASE, CATCH_SBS)
{
  AttributeInfo_t info_write;
  AttributeInfo_t info_read;
  // Запрос на изменившиеся SBS группы
  rtDbCq operation;
  std::vector<std::string> tags;
  map_id_name_t sbs_map;
  map_id_name_t points_map;
  Error status;

  // 1) Занести в /KO4016/GOV171/PT01.VALACQ = 3.1415
  // ====================================================
  info_write.name = std::string(sbs_points[1]) + "." + RTDB_ATT_VALACQ;
  info_write.value.fixed.val_double = 3.1415;
  status = connection->write(&info_write);
  EXPECT_EQ(status.code(), rtE_NONE);

//  env->MakeSnapshot("BEFORE_CATCH_SBS");
  // 2) Найти все группы, в которых изменились точки
  // ====================================================
  // Тип конфигурирования - получение информации о группах подписки,
  // имеющих модифицированные Точки
  operation.action.query = rtQUERY_SBS_LIST_ARMED;
  operation.buffer = &sbs_map;
  status = connection->QueryDatabase(operation);
  EXPECT_EQ(status.code(), rtE_NONE);
  // Изменились элементы в двух группах, т.к. одна из модифицированных
  // точек (/KD4001/GOV022) входит в две группы сразу
  EXPECT_EQ(sbs_map.size(), 2);

  for (map_id_name_t::iterator it = sbs_map.begin();
       it != sbs_map.end();
       it++)
  {
    LOG(INFO) << it->first << ":'" << it->second << "'";

    // 3) Получить id и теги всех изменившихся точек указанной группы
    // ====================================================
    operation.action.query = rtQUERY_SBS_POINTS_ARMED;
    // Название искомой группы подписки хранится в it->second
    tags.clear();
    tags.push_back(it->second);
    operation.tags = &tags;
    operation.buffer = &points_map;
    status = connection->QueryDatabase(operation);
    EXPECT_EQ(status.code(), rtE_NONE);
    //EXPECT_EQ(points_map.size(), 2);
  }

#if 0
  // 4) Получить id и теги всех изменившихся точек всех групп, вместе с id групп
  // ====================================================
  operation.action.query = rtQUERY_SBS_ALL_POINTS_ARMED;
  operation.buffer = &sbs_map;
  status = connection->QueryDatabase(operation);
  EXPECT_EQ(status.code(), rtE_NONE);
#endif

//  env->MakeSnapshot("AFTER_CATCH_SBS");
}

// Тест на сброс флага модификации для одной из групп
TEST(TestDiggerDATABASE, DISARM_SBS)
{
  rtDbCq operation;
  std::vector<std::string> tags;
  map_id_name_t points_map;
  Error status;
  int sbs_items_old_count = 0;
  int sbs_items_new_count = 0;
 
  // Получим список модифицированных точек этой группы
  tags.push_back(group_name_2);
  operation.action.query = rtQUERY_SBS_POINTS_ARMED;
  operation.tags = &tags;
  operation.buffer = &points_map;
  status = connection->QueryDatabase(operation);
  EXPECT_EQ(status.code(), rtE_NONE);

  // Должно быть ненулевое количество Точек
  sbs_items_old_count = points_map.size();
  LOG(INFO) << "SBS '"<<group_name_2<<"' size="<<sbs_items_old_count;
  EXPECT_TRUE(sbs_items_old_count > 0);

  LOG(INFO) << "Clear SBS '"<<group_name_2<<"' modifications";
  // Запрос на сброс модификации группы group_name_2
  operation.action.query = rtQUERY_SBS_POINTS_DISARM;
  operation.buffer = NULL;
  status = connection->QueryDatabase(operation);
  EXPECT_EQ(status.code(), rtE_NONE);

  LOG(INFO) << "Get new SBS '"<<group_name_2<<"' size";
  operation.action.query = rtQUERY_SBS_POINTS_ARMED;
  operation.buffer = &points_map;
  // Ожидается, что список модифицированных точек этой группы будет пустым
  status = connection->QueryDatabase(operation);
  EXPECT_EQ(status.code(), rtE_NONE);

  // Должно быть нулевое количество Точек
  sbs_items_new_count = points_map.size();
  EXPECT_TRUE(sbs_items_new_count == 0);
}

// Тест чтения всех значений точек из заданной группы подписки
TEST(TestDiggerDATABASE, DELETE_SBS)
{
  Error status;
  rtDbCq operation;

//  env->MakeSnapshot("BEFORE_DELETE_SBS");
  // Проверить удаление действительно существующей группы
  // ====================================================
  // Удалить ранее созданную группу
  operation.action.config = rtCONFIG_DEL_GROUP_SBS;
  // Название группы подписки
  operation.buffer = static_cast<void*>(&group_name_2);

  status = connection->ConfigDatabase(operation);
  // Успешность записи корректного значения
  EXPECT_EQ(status.code(), rtE_NONE);

  // Проверить удаление несуществующей группы подписки
  // ====================================================
  operation.buffer = static_cast<void*>(&group_name_unexistant);

  status = connection->ConfigDatabase(operation);
  EXPECT_EQ(status.code(), rtE_RUNTIME_ERROR);

//  env->MakeSnapshot("AFTER_DELETE_SBS");
}

TEST(TestDiggerDATABASE, DESTROY)
{
  LOG(INFO) << "BEGIN DESTROY TestDiggerDATABASE";
  delete connection;
  delete app;
  LOG(INFO) << "END DESTROY TestDiggerDATABASE";
}

using namespace xercesc;
TEST(TestTools, LOAD_DICT_XML)
{
  const int argc = 2;
  char *argv[argc] = {
                    (char*)"LOAD_DICT_XML",
                    (char*)"rtap_dict.xml"
                    };

  LOG(INFO) << "BEGIN LOAD_DICT_XML TestTools";

  try
  {
    ::rtap_db_dict::Dictionaries_pimpl Dictionaries_p;
    ::rtap_db_dict::UNITY_LABEL_pimpl UNITY_LABEL_p;
    ::rtap_db_dict::UnityDimensionType_pimpl UnityDimensionType_p;
    ::rtap_db_dict::UnityDimensionEntry_pimpl UnityDimensionEntry_p;
    ::rtap_db_dict::UnityIdEntry_pimpl UnityIdEntry_p;
    ::rtap_db_dict::UnityLabelEntry_pimpl UnityLabelEntry_p;
    ::rtap_db_dict::UnityDesignationEntry_pimpl UnityDesignationEntry_p;
    ::rtap_db_dict::VAL_LABEL_pimpl VAL_LABEL_p;
    ::rtap_db_dict::ObjClassEntry_pimpl ObjClassEntry_p;
    ::rtap_db_dict::ValueEntry_pimpl ValueEntry_p;
    ::rtap_db_dict::ValueLabelEntry_pimpl ValueLabelEntry_p;
    ::rtap_db_dict::CE_ITEM_pimpl CE_ITEM_p;
    ::rtap_db_dict::Identifier_pimpl Identifier_p;
    ::rtap_db_dict::ActionScript_pimpl ActionScript_p;
    ::rtap_db_dict::INFO_TYPES_pimpl INFO_TYPES_p;
    ::rtap_db_dict::InfoTypeEntry_pimpl InfoTypeEntry_p;
    ::xml_schema::string_pimpl string_p;

    // Connect the parsers together.
    //
    Dictionaries_p.parsers (UNITY_LABEL_p,
                            VAL_LABEL_p,
                            CE_ITEM_p,
                            INFO_TYPES_p);

    UNITY_LABEL_p.parsers (UnityDimensionType_p,
                           UnityDimensionEntry_p,
                           UnityIdEntry_p,
                           UnityLabelEntry_p,
                           UnityDesignationEntry_p);

    VAL_LABEL_p.parsers (ObjClassEntry_p,
                         ValueEntry_p,
                         ValueLabelEntry_p);

    CE_ITEM_p.parsers (Identifier_p,
                  ActionScript_p);
                  
    INFO_TYPES_p.parsers (ObjClassEntry_p,
                          InfoTypeEntry_p,
                          string_p);

    // Parse the XML document.
    //
    ::xml_schema::document doc_p (
      Dictionaries_p,
      "http://www.example.com/rtap_db_dict",
      "Dictionaries");

    // TODO: передать внутрь пустые списки НСИ для их инициализации
    Dictionaries_p.pre (dict);
    doc_p.parse (argv[1]);
    Dictionaries_p.post_Dictionaries ();

    EXPECT_EQ(dict.unity_dict.size(), 51);
    EXPECT_EQ(dict.values_dict.size(), 26);
    EXPECT_EQ(dict.macros_dict.size(), 0);
    EXPECT_EQ(dict.infotypes_dict.size(), 0);

    //applyDICT(dict);
  }
  catch (const ::xml_schema::exception& e)
  {
    std::cerr << e << std::endl;
    return;
  }
  catch (const std::ios_base::failure&)
  {
    std::cerr << argv[0] << " error: i/o failure" << std::endl;
    return;
  }

  LOG(INFO) << "END LOAD_DICT_XML TestTools";
}

TEST(TestTools, LOAD_DATA_XML)
{
  unsigned int class_item;
//  unsigned int attribute_item;
  const int argc = 2;
  char *argv[argc] = {
                    (char*)"LOAD_DATA_XML",
                    (char*)"classes.xml"
                    };

  LOG(INFO) << "BEGIN LOAD_DATA_XML TestTools";

  try
  {
    // Instantiate individual parsers.
    //
    ::rtap_db::RTDB_STRUCT_pimpl RTDB_STRUCT_p;
    ::rtap_db::Point_pimpl       Point_p;
    ::rtap_db::Objclass_pimpl    Objclass_p;
    ::rtap_db::Tag_pimpl         Tag_p;
    ::rtap_db::Attr_pimpl        Attr_p;
    ::rtap_db::PointKind_pimpl   PointKind_p;
    ::rtap_db::Accessibility_pimpl Accessibility_p;
    ::rtap_db::AttributeType_pimpl AttributeType_p;
    ::rtap_db::AttrNameType_pimpl  AttrNameType_p;
    ::xml_schema::string_pimpl     string_p;

    // Connect the parsers together.
    //
    RTDB_STRUCT_p.parsers (Point_p);

    Point_p.parsers (Objclass_p,
                     Tag_p,
                     Attr_p);

    Attr_p.parsers (AttrNameType_p,
                    PointKind_p,
                    Accessibility_p,
                    AttributeType_p,
                    string_p);

    // Parse the XML document.
    //
    ::xml_schema::document doc_p (
      RTDB_STRUCT_p,
      "http://www.example.com/rtap_db",
      "RTDB_STRUCT");

    RTDB_STRUCT_p.pre (point_list);
    doc_p.parse (argv[1]);
    RTDB_STRUCT_p.post_RTDB_STRUCT ();

#if defined VERBOSE
    std::cout << "Parsing XML is over, processed " << point_list.size() << " element(s)" << std::endl;
#endif
    /* В cmake/classes.xml есть 3 точки */
    EXPECT_EQ(point_list.size(), 3);

    for (class_item=0; class_item<point_list.size(); class_item++)
    {
#if defined VERBOSE
      std::cout << "\tOBJCLASS:  " << point_list[class_item].objclass() << std::endl;
      std::cout << "\tTAG:  '" << point_list[class_item].tag() << "'" << std::endl;
      std::cout << "\t#ATTR: " << point_list[class_item].m_attributes.size() << std::endl;
      if (point_list[class_item].m_attributes.size())
      {
        for (attribute_item=0;
             attribute_item<point_list[class_item].m_attributes.size();
             attribute_item++)
        {
          std::cout << "\t\t" << point_list[class_item].m_attributes[attribute_item].name()
                    << " : "  << point_list[class_item].m_attributes[attribute_item].value()
                    << " : "  << point_list[class_item].m_attributes[attribute_item].kind()
                    << " : "  << point_list[class_item].m_attributes[attribute_item].type()
                    << " : "  << point_list[class_item].m_attributes[attribute_item].accessibility()
                    << std::endl;
        }
      }
#endif

      switch(class_item)
      {
        case GOF_D_BDR_OBJCLASS_TS:
            EXPECT_TRUE(point_list[class_item].objclass() == class_item);
            EXPECT_EQ(point_list[class_item].m_attributes.size(), 1);
            break;

        case GOF_D_BDR_OBJCLASS_TM:
            EXPECT_TRUE(point_list[class_item].objclass() == class_item);
            EXPECT_EQ(point_list[class_item].m_attributes.size(), 2);
            break;

        case GOF_D_BDR_OBJCLASS_TSA:
            EXPECT_TRUE(point_list[class_item].objclass() == class_item);
            EXPECT_EQ(point_list[class_item].m_attributes.size(), 1);
            break;
      }

#if defined VERBOSE
      std::cout << std::endl;
#endif
    }
  }
  catch (const ::xml_schema::exception& e)
  {
    std::cerr << e << std::endl;
    return;
  }
  catch (const std::ios_base::failure&)
  {
    std::cerr << argv[0] << " error: i/o failure" << std::endl;
    return;
  }
  LOG(INFO) << "END LOAD_DATA_XML TestTools";
}

TEST(TestTools, LOAD_CLASSES)
{
  int  objclass_idx;
  int  loaded;
  char fpath[255];
  char msg_info[255];
  char msg_val[255];
  char s_date [D_DATE_FORMAT_LEN + 1];
  time_t given_time;
  xdb::AttributeMap_t *pool;

  LOG(INFO) << "BEGIN LOAD_CLASSES TestTools";

  getcwd(fpath, 255);
  loaded = xdb::processClassFile(fpath);
  EXPECT_TRUE(loaded > 0); // загружен хотя бы один класс

  for (objclass_idx=0; objclass_idx <= GOF_D_BDR_OBJCLASS_LASTUSED; objclass_idx++)
  {
    pool = xdb::ClassDescriptionTable[objclass_idx].attributes_pool;
    if (!strncmp(xdb::ClassDescriptionTable[objclass_idx].name,
                D_MISSING_OBJCODE,
                TAG_NAME_MAXLEN))
    {
      continue;
    }

    if (pool)
    {
#if defined VERBOSE
        std::cout << "#" << objclass_idx << " : " 
            << xdb::ClassDescriptionTable[objclass_idx].name 
            << "(" << pool->size() << ")" << std::endl;
#endif

        for (xdb::AttributeMapIterator_t it=pool->begin(); it!=pool->end(); ++it)
        {
            sprintf(msg_info, "\"%s\" : %02d", 
                it->second.name.c_str(), it->second.type);

            switch(it->second.type)
            {
              case xdb::DB_TYPE_LOGICAL:
                  sprintf(msg_val, "%d", it->second.value.fixed.val_bool);
                  break;
              case xdb::DB_TYPE_INT8:
                  sprintf(msg_val, "%02X", it->second.value.fixed.val_int8);
                  break;
              case xdb::DB_TYPE_UINT8:
                  sprintf(msg_val, "%02X", it->second.value.fixed.val_uint8);
                  break;
              case xdb::DB_TYPE_INT16:
                  sprintf(msg_val, "%04X", it->second.value.fixed.val_int16);
                  break;
              case xdb::DB_TYPE_UINT16:
                  sprintf(msg_val, "%04X", it->second.value.fixed.val_uint16);
                  break;
              case xdb::DB_TYPE_INT32:
                  sprintf(msg_val, "%08X", it->second.value.fixed.val_int32);
                  break;
              case xdb::DB_TYPE_UINT32:
                  sprintf(msg_val, "%08X", it->second.value.fixed.val_uint32);
                  break;
              case xdb::DB_TYPE_INT64:
                  sprintf(msg_val, "%lld", it->second.value.fixed.val_int64);
                  break;
              case xdb::DB_TYPE_UINT64:
                  sprintf(msg_val, "%llu", it->second.value.fixed.val_uint64);
                  break;
              case xdb::DB_TYPE_FLOAT:
                  sprintf(msg_val, "%f", it->second.value.fixed.val_float);
                  break;
              case xdb::DB_TYPE_DOUBLE:
                  sprintf(msg_val, "%g", it->second.value.fixed.val_double);
                  break;

              case xdb::DB_TYPE_BYTES:
              case xdb::DB_TYPE_BYTES4:
              case xdb::DB_TYPE_BYTES8:
              case xdb::DB_TYPE_BYTES12:
              case xdb::DB_TYPE_BYTES16:
              case xdb::DB_TYPE_BYTES20:
              case xdb::DB_TYPE_BYTES32:
              case xdb::DB_TYPE_BYTES48:
              case xdb::DB_TYPE_BYTES64:
              case xdb::DB_TYPE_BYTES80:
              case xdb::DB_TYPE_BYTES128:
              case xdb::DB_TYPE_BYTES256:
                  sprintf(msg_val, "[%02X] \"%s\"", 
                    it->second.value.dynamic.size,
                    it->second.value.dynamic.varchar);
                  break;

              case xdb::DB_TYPE_UNDEF:
                  sprintf(msg_val, ": undef %02d", xdb::DB_TYPE_UNDEF);
                  break;

              case xdb::DB_TYPE_ABSTIME:
                  given_time = it->second.value.fixed.val_time.tv_sec;
                  strftime(s_date, D_DATE_FORMAT_LEN, D_DATE_FORMAT_STR, localtime(&given_time));
                  snprintf(msg_val, D_DATE_FORMAT_W_MSEC_LEN, "%s.%06ld", s_date, it->second.value.fixed.val_time.tv_usec);
                  break;

              default:
                  LOG(ERROR) << ": <error>=" << it->second.type;
            }
#if defined VERBOSE
            std::cout << msg_info << " | " << msg_val << std::endl;
#endif
        }
    }
  }
  LOG(INFO) << "END LOAD_CLASSES TestTools";
}

TEST(TestTools, LOAD_INSTANCE)
{
  bool status = false;
  char fpath[255];

  LOG(INFO) << "BEGIN LOAD_INSTANCE TestTools";
  getcwd(fpath, 255);
  status = xdb::processInstanceFile(fpath);
  EXPECT_TRUE(status);
  LOG(INFO) << "END LOAD_INSTANCE TestTools";
}

int main(int argc, char** argv)
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InstallFailureSignalHandler();

  // Инициировать XML-движок единственный раз в рамках одного процесса
  try
  {
    XMLPlatformUtils::Initialize("UTF-8");
  }
  catch (const XMLException& toCatch)
  {
    std::cerr << "Error during initialization! :\n"
              << toCatch.getMessage() << std::endl;
    return -1;
  }

  setenv("MCO_RUNTIME_STOP", "1", 0);
  int retval = RUN_ALL_TESTS();

  try
  {
    XMLPlatformUtils::Terminate();
  }
  catch (const XMLException& toCatch)
  {
    std::cerr << "Error during finalization! :\n"
              << toCatch.getMessage() << std::endl;
  }

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();
  return retval;
}

