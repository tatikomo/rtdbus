#include <iostream>
#include <string>

#include <xercesc/util/PlatformUtils.hpp>

#include "glog/logging.h"
#include "gtest/gtest.h"

#include "xdb_broker_impl.hpp"
#include "xdb_broker.hpp"
#include "xdb_broker_service.hpp"
#include "xdb_broker_worker.hpp"
#include "dat/broker_db.hpp"
#include "dat/rtap_db-pimpl.hxx"
#include "dat/rtap_db-pskel.hxx"
#include "proto/common.pb.h"

#include "xdb_rtap_application.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"

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
xdb::Database::DBState state;
xdb::RtApplication* app = NULL;
xdb::RtEnvironment* env = NULL;
xdb::RtDbConnection* connection = NULL;

void wait()
{
//  puts("\nNext...");
//  getchar();
}

void show_runtime_info(const char * lead_line)
{
  mco_runtime_info_t info;
  
  /* get runtime info */
  //mco_get_runtime_info(&info);

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

    state = database->State();
    EXPECT_EQ(state, xdb::Database::UNINITIALIZED);

    status = database->Connect();
    ASSERT_TRUE(status == true);

    state = database->State();
    ASSERT_TRUE(state == xdb::Database::CONNECTED);

    show_runtime_info("Database runtime information:\n=======================================");
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
  pb_header.set_source_pid(getpid());
  pb_header.set_proc_dest("dest");
  pb_header.set_proc_origin("src");
  pb_header.set_sys_msg_type(100);
  pb_header.set_usr_msg_type(ADG_D_MSG_EXECRESULT);

  pb_exec_result_request.set_exec_result(0);
  pb_exec_result_request.set_failure_cause(1);

  for (int i=1; i<10; i++)
  {
    sprintf(reply, "@C0000000%02d", i);
    pb_header.set_exchange_id(i);
    pb_header.SerializeToString(&pb_serialized_header);

    pb_exec_result_request.set_user_exchange_id(i);
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

    ASSERT_TRUE (service1 != NULL);
    delete service1;

    ASSERT_TRUE (service2 != NULL);
    delete service2;

#if 0
    ASSERT_TRUE (database != NULL);
    database->Disconnect();

    state = database->State();
    EXPECT_EQ(state, xdb::Database::DISCONNECTED);
#endif

    delete database;
}

TEST(TestLurkerDATABASE, CREATION)
{
  bool status = false;
  xdb::RtEnvironment *env1 = NULL;
  const int argc = 3;
  char *argv[argc] = {
                    "test",
                    "параметр_1",
                    "параметр_2"
                    };

  app = new xdb::RtApplication("test");
  ASSERT_TRUE(app != NULL);

  app->setOption("OF_CREATE", 1);
  app->setOption("OF_RDWR", 1);
  EXPECT_EQ(app->getLastError().getCode(), xdb::rtE_NONE);

//  app->setEnvName("RTAP");

  LOG(INFO) << "Initialize: " << app->initialize().getCode();
  LOG(INFO) << "Operation mode: " << app->getOperationMode();
  LOG(INFO) << "Operation state: " << app->getOperationState();

  env = app->getEnvironment("SINF");
  EXPECT_TRUE(env != NULL);

  env1 = app->getEnvironment("SINF");
  EXPECT_TRUE(env1 != NULL);
  // Это должен быть один и тот же объект SINF
  // с одинаковым именем
  EXPECT_TRUE(0 == strcmp(env->getName(), env1->getName()));
  // и аресом
  EXPECT_TRUE(env == env1);

  connection = env->createDbConnection();
  EXPECT_TRUE(connection != NULL);
}

TEST(TestLurkerDATABASE, DESTROY)
{
  delete connection;
  delete env;
  delete app;
}

using namespace xercesc;
TEST(TestTools, INIT)
{
  const int argc = 2;
  char *argv[argc] = {
                    "TestTools",
                    "classes.xml"
                    };

  try
  {
    XMLPlatformUtils::Initialize("UTF-8");
  }
  catch (const XMLException& toCatch)
  {
    // Do your failure processing here
    std::cerr << "Error during initialization! :\n"
              << toCatch.getMessage() << std::endl;
    return;
  }

  try
  {
    // Do your actual work with Xerces-C++ here.
    std::cout << "Hello, World!"<<std::endl;

    // Instantiate individual parsers.
    //
    ::rtap_db::RTDB_STRUCT_pimpl RTDB_STRUCT_p;
    ::rtap_db::Class_pimpl Class_p;
    ::rtap_db::Code_pimpl Code_p;
    ::rtap_db::ClassType_pimpl ClassType_p;
    ::rtap_db::Attr_pimpl Attr_p;
    ::rtap_db::PointKind_pimpl PointKind_p;
    ::rtap_db::Accessibility_pimpl Accessibility_p;
    ::rtap_db::AttributeType_pimpl AttributeType_p;
    ::rtap_db::AttrNameType_pimpl AttrNameType_p;
    ::xml_schema::string_pimpl string_p;

    // Connect the parsers together.
    //
    RTDB_STRUCT_p.parsers (Class_p);

    Class_p.parsers (Code_p,
                     ClassType_p,
                     Attr_p);

    Attr_p.parsers (PointKind_p,
                    Accessibility_p,
                    AttributeType_p,
                    AttrNameType_p,
                    string_p);

    // Parse the XML document.
    //
    ::xml_schema::document doc_p (
      RTDB_STRUCT_p,
      "http://www.example.com/rtap_db",
      "RTDB_STRUCT");

    RTDB_STRUCT_p.pre ();
    doc_p.parse (argv[1]);
    RTDB_STRUCT_p.post_RTDB_STRUCT ();












#if 0
    // Print the object model.
    //
    for (::rtap_db::Class_pimpl::iterator i (rtdb.begin ()); i != rtdb.end (); ++i)
    {
      std::cout << "code:  " << i->Code () << std::endl
         << "name:   " << i->Name () << std::endl
         << endl;
    }
#endif






    XMLPlatformUtils::Terminate();
  }
  catch (const ::xml_schema::exception& e)
  {
    std::cerr << e << std::endl;
    return 1;
  }
  catch (const std::ios_base::failure&)
  {
    std::cerr << argv[1] << ": error: io failure" << std::endl;
    return 1;
  }
}

int main(int argc, char** argv)
{
  ::google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InstallFailureSignalHandler();
  int retval = RUN_ALL_TESTS();
  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();
  return retval;
}

