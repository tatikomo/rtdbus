#include <iostream>
#include <string>

#include <stdlib.h> // putenv
#include <stdio.h>  // fprintf

#include <xercesc/util/PlatformUtils.hpp>

#include "glog/logging.h"
#include "gtest/gtest.h"

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

#include "xdb_rtap_const.hpp"
#include "xdb_rtap_application.hpp"
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
  RTDBM::Header     pb_header;
  RTDBM::ExecResult pb_exec_result_request;
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
    pb_header.set_exchange_id(i);
    pb_header.SerializeToString(&pb_serialized_header);

    pb_exec_result_request.set_exec_result(i % 2);
    pb_exec_result_request.set_failure_cause(i * 2);
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

  app->setOption("OF_CREATE", 1);
  app->setOption("OF_RDWR",   1);
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
  ASSERT_TRUE(env != NULL);

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

#if 1
    connection = env->getConnection();
    EXPECT_TRUE(connection != NULL);
#endif

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

#if VERBOSE
    std::cout << "Parsing XML is over, processed " << point_list.size() << " element(s)" << std::endl;
#endif
    /* В cmake/classes.xml есть 3 точки */
    EXPECT_EQ(point_list.size(), 3);

    for (class_item=0; class_item<point_list.size(); class_item++)
    {
#if VERBOSE
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

#if VERBOSE
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
                UNIVNAME_LENGTH))
    {
      continue;
    }

    if (pool)
    {
#if VERBOSE
        std::cout << "#" << objclass_idx << " : " 
            << xdb::ClassDescriptionTable[objclass_idx].name 
            << "(" << pool->size() << ")" << std::endl;
#endif

        for (xdb::AttributeMapIterator_t it=pool->begin(); it!=pool->end(); ++it)
        {
            sprintf(msg_info, "\"%s\" : %02d", 
                it->second.name.c_str(), it->second.db_type);

            switch(it->second.db_type)
            {
              case xdb::DB_TYPE_INT8:
                  sprintf(msg_val, "%02X", it->second.value.val_int8);
                  break;
              case xdb::DB_TYPE_UINT8:
                  sprintf(msg_val, "%02X", it->second.value.val_uint8);
                  break;

              case xdb::DB_TYPE_INT16:
                  sprintf(msg_val, "%04X", it->second.value.val_int16);
                  break;
              case xdb::DB_TYPE_UINT16:
                  sprintf(msg_val, "%04X", it->second.value.val_uint16);
                  break;

              case xdb::DB_TYPE_INT32:
                  sprintf(msg_val, "%08X", it->second.value.val_int32);
                  break;
              case xdb::DB_TYPE_UINT32:
                  sprintf(msg_val, "%08X", it->second.value.val_uint32);
                  break;

              case xdb::DB_TYPE_INT64:
                  sprintf(msg_val, "%lld", it->second.value.val_int64);
                  break;
              case xdb::DB_TYPE_UINT64:
                  sprintf(msg_val, "%llu", it->second.value.val_uint64);
                  break;

              case xdb::DB_TYPE_FLOAT:
                  sprintf(msg_val, "%f", it->second.value.val_float);
                  break;

              case xdb::DB_TYPE_DOUBLE:
                  sprintf(msg_val, "%g", it->second.value.val_double);
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
                    it->second.value.val_bytes.size,
                    it->second.value.val_bytes.data);
                  break;

              case xdb::DB_TYPE_UNDEF:
                  sprintf(msg_val, ": undef %02d", xdb::DB_TYPE_UNDEF);
                  break;

              default:
                  LOG(ERROR) << ": <error>=" << it->second.db_type;
            }
#if VERBOSE
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

#if 0
/* Заполнить контент RTDB на основе прочитанных из XML данных */
TEST(TestRtapDATABASE, CREATE)
{
  LOG(INFO) << "BEGIN CREATE TestRtapDATABASE";

  app = new xdb::RtApplication("RTDB_TEST");
  ASSERT_TRUE(app != NULL);

  // Прочитать данные из ранее сохраненного XML
  app->setOption("OF_LOAD_SNAP", 1);
  EXPECT_EQ(app->getLastError().getCode(), xdb::rtE_NONE);

//  app->setEnvName("RTAP");

  LOG(INFO) << "Initialize: " << app->initialize().getCode();
  LOG(INFO) << "Operation mode: " << app->getOperationMode();
  LOG(INFO) << "Operation state: " << app->getOperationState();

  env = app->loadEnvironment("SINF");
  EXPECT_TRUE(env != NULL);

//  connection = env->createConnection();
//  EXPECT_TRUE(connection != NULL);
  LOG(INFO) << "END CREATE TestRtapDATABASE";
}

TEST(TestRtapDATABASE, SHOW_CONTENT)
{
//  env->Dump();
}

TEST(TestRtapDATABASE, TERMINATE)
{
  LOG(INFO) << "BEGIN TERMINATE TestRtapDATABASE";
//  delete env;
  delete app;
  LOG(INFO) << "END TERMINATE TestRtapDATABASE";
}
#endif

int main(int argc, char** argv)
{
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

