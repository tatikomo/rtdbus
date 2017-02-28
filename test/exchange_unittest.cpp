// Общесистемные заголовочные файлы
#include <string>
#include <vector>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "gtest/gtest.h"
#include "google/protobuf/stubs/common.h"
#include "zmq.hpp"

// Служебные файлы RTDBUS
#include "mdp_zmsg.hpp"
#include "exchange_config.hpp"
#include "exchange_config_sac.hpp"
#include "exchange_config_egsa.hpp"
#include "exchange_egsa_init.hpp"
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_sa.hpp"
#include "exchange_egsa_request_cyclic.hpp"

#include "proto/rtdbus.pb.h"

AcquisitionSystemConfig* g_sa_config = NULL;
EgsaConfig* g_egsa_config = NULL;
EGSA* g_egsa_instance = NULL;
SystemAcquisition* g_sa = NULL;

const char* g_sa_config_filename = "BI4500.json";
const char* g_egsa_config_filename = "egsa.json";
extern ega_ega_odm_t_RequestEntry g_requests_table[]; // declared in exchange_egsa_impl.cpp

// Создать корректно заполненные сообщения нужного типа для тестирования реакции EGSA
mdp::zmsg* create_message_by_type(const std::string& dest,
                            const std::string& origin,
                            int msg_type,
                            int exch_id = 1000,
                            int interest_id = 0)
{
  mdp::zmsg*         message = NULL;
  RTDBM::Header      pb_header;
  RTDBM::ExecResult  pb_exec_result_request;
  RTDBM::SimpleRequest  pb_simple_request;
  std::string        pb_serialized_header;
  std::string        pb_serialized_request;
  std::string        error_text("пусто");
  bool               ready = true;

  // Постоянные поля заголовка сообщения
  pb_header.set_protocol_version(1);
  pb_header.set_exchange_id(exch_id);
  pb_header.set_interest_id(interest_id);
  pb_header.set_source_pid(9999);
  pb_header.set_proc_dest(dest.c_str());
  pb_header.set_proc_origin(origin);
  pb_header.set_sys_msg_type(100);
  pb_header.set_time_mark(time(0));

  switch(msg_type) {
    case ADG_D_MSG_EXECRESULT:
      pb_header.set_usr_msg_type(ADG_D_MSG_EXECRESULT);
      pb_exec_result_request.set_exec_result(RTDBM::GOF_D_PARTSUCCESS);
      pb_exec_result_request.mutable_failure_cause()->set_error_code(0);
      pb_exec_result_request.mutable_failure_cause()->set_error_text(error_text);

      pb_header.SerializeToString(&pb_serialized_header);
      pb_exec_result_request.SerializeToString(&pb_serialized_request);
      break;

    case ADG_D_MSG_ENDALLINIT:
      pb_header.set_usr_msg_type(ADG_D_MSG_ENDALLINIT);
      pb_simple_request.set_exec_result(RTDBM::GOF_D_PARTSUCCESS);

      pb_header.SerializeToString(&pb_serialized_header);
      pb_simple_request.SerializeToString(&pb_serialized_request);
      break;

    case ADG_D_MSG_STOP:
      pb_header.set_usr_msg_type(ADG_D_MSG_STOP);
      pb_simple_request.set_exec_result(RTDBM::GOF_D_PARTSUCCESS);

      pb_header.SerializeToString(&pb_serialized_header);
      pb_simple_request.SerializeToString(&pb_serialized_request);
      break;

    default:
      LOG(ERROR) << "Unable to create message type: " << msg_type;
      ready = false;
  }

  if (ready) {
    message = new mdp::zmsg();

    message->push_front(pb_serialized_request);
    message->push_front(pb_serialized_header);
    message->push_front(const_cast<char*>(EMPTY_FRAME));
    message->push_front(const_cast<char*>(origin.c_str()));
  }

  return message;
}

TEST(TestEXCHANGE, EGSA_CREATE)
{
  std::string broker_endpoint = ENDPOINT_BROKER;
  std::string service_name = EXCHANGE_NAME;

  g_egsa_instance = new EGSA(broker_endpoint, service_name);
  ASSERT_TRUE(g_egsa_instance != NULL);

  g_sa_config = new AcquisitionSystemConfig(g_sa_config_filename);
  ASSERT_TRUE(g_sa_config != NULL);

  g_egsa_config = new EgsaConfig(g_egsa_config_filename);
  ASSERT_TRUE(g_egsa_config != NULL);
}

TEST(TestEXCHANGE, SA_CONFIG)
{
  int rc = g_sa_config->load();
  EXPECT_TRUE(OK == rc);
}

TEST(TestEXCHANGE, EGSA_CONFIG)
{
  int rc = g_egsa_config->load();
  EXPECT_TRUE(OK == rc);

  LOG(INFO) << "load " << g_egsa_config->cycles().size() << " cycles";
  EXPECT_TRUE(g_egsa_config->cycles().size() == 4);

  LOG(INFO) << "load " << g_egsa_config->sites().size() << " sites";
  EXPECT_TRUE(g_egsa_config->sites().size() == 3);
}

TEST(TestEXCHANGE, EGSA_REQUESTS)
{
  ega_ega_odm_t_RequestEntry* req_entry_dict = NULL;
  int rc;

  // Проверка поиска несуществующего запроса
  rc =  get_request_by_name("unexistant_cycle", req_entry_dict);
  EXPECT_TRUE(rc == NOK);
  EXPECT_TRUE(req_entry_dict == NULL);

  // Проверка поиска существующего запроса
  rc =  get_request_by_name(EGA_EGA_D_STRGENCONTROL, req_entry_dict);
  EXPECT_TRUE(rc == OK);
  EXPECT_TRUE(req_entry_dict != NULL);
  EXPECT_TRUE(req_entry_dict->e_RequestId == ECH_D_GENCONTROL);
}

// Подготовить полную информацию по циклам, включая связанные с этими циклами сайты
TEST(TestEXCHANGE, EGSA_CYCLES)
{
  ega_ega_odm_t_RequestEntry* req_entry_dict = NULL;
  int rc;

  for(egsa_config_cycles_t::const_iterator it = g_egsa_config->cycles().begin();
      it != g_egsa_config->cycles().end();
      ++it)
  {
    // Найти запрос с именем (*it).second->request_name в m_requests_table[]
    if (OK == get_request_by_name((*it).second->request_name, req_entry_dict)) {
      // Создадим экземпляр Цикла, удалится он в деструкторе EGSA
      Cycle *cycle = new Cycle((*it).first.c_str(),
                               (*it).second->period,
                               req_entry_dict->e_RequestId,
                               CYCLE_NORMAL);

      // Для данного цикла получить все использующие его сайты
      for(std::vector <std::string>::const_iterator its = (*it).second->sites.begin();
          its != (*it).second->sites.end();
          ++its) {
        cycle->register_SA((*its));
      }
      //
      //
      //
      cycle->dump();
      // Передать объект Цикл в подчинение EGSA
      g_egsa_instance->push_cycle(cycle);
    }
  }

#if 0
  LOG(INFO) << "BEGIN cycles activates testing";
  rc = g_egsa_instance->activate_cycles();
  EXPECT_TRUE(rc == OK);

  g_egsa_instance->wait(10);
  rc = g_egsa_instance->deactivate_cycles();
  LOG(INFO) << "END cycles activates testing";
#endif
}

// Проверка работы класса Системы Сбора
TEST(TestEXCHANGE, SAC_CREATE)
{
  sa_object_level_t level = LEVEL_UNKNOWN;
  gof_t_SacNature nature = GOF_D_SAC_NATURE_EUNK;

  // Есть хотя бы один хост для тестов
  ASSERT_TRUE(g_egsa_config->sites().size() > 0);

  const std::string tag = g_egsa_config->sites().begin()->first;
  int raw_level  = g_egsa_config->sites().begin()->second->level;
  int raw_nature = g_egsa_config->sites().begin()->second->nature;

  if ((GOF_D_SAC_NATURE_DIR >= raw_nature) && (raw_nature <= GOF_D_SAC_NATURE_EUNK)) {
    nature = static_cast<gof_t_SacNature>(raw_nature);
  }

  if ((LEVEL_UNKNOWN >= raw_level) && (raw_level <= LEVEL_UPPER)) {
    level = static_cast<sa_object_level_t>(raw_level);
  }

  g_sa = new SystemAcquisition(g_egsa_instance, level, nature, tag);
  // Начальное состояние СС
  EXPECT_TRUE(g_sa->state() == SA_STATE_DISCONNECTED);
  // Послать системе команду инициализации
  g_sa->send(ADG_D_MSG_ENDALLINIT);
  // Поскольку это имитация, состояние системы не изменится,
  // и должен сработать таймер по завершению таймаута
  EXPECT_TRUE(g_sa->state() == SA_STATE_DISCONNECTED);
}

#if 0
TEST(TestEXCHANGE, EGSA_RUN)
{
  g_egsa_instance->run();
}
#endif

TEST(TestEXCHANGE, EGSA_ENDALLINIT)
{
//  msg::Letter* message_end_all_init = NULL;
  const std::string identity = "RTDBUS_BROKER";
  mdp::zmsg * message_end_all_init = create_message_by_type("EGSA", "TEST", ADG_D_MSG_ENDALLINIT);
  bool is_stop = false;

  EXPECT_TRUE(message_end_all_init != NULL);
  g_egsa_instance->processing(message_end_all_init, identity, is_stop);

  EXPECT_TRUE(is_stop == false);
//  g_egsa_instance->handle_end_all_init(message_end_all_init, identity);
}

TEST(TestEXCHANGE, EGSA_STOP)
{
  //msg::Letter* message_stop = NULL;
  const std::string identity = "RTDBUS_BROKER";
  mdp::zmsg * message_stop = create_message_by_type("EGSA", "TEST", ADG_D_MSG_STOP);
  bool is_stop = false;

  EXPECT_TRUE(message_stop != NULL);
  g_egsa_instance->processing(message_stop, identity, is_stop);

  //g_egsa_instance->handle_stop(message_stop, identity);
  EXPECT_TRUE(is_stop == true);
}

TEST(TestEXCHANGE, EGSA_FREE)
{
  delete g_egsa_config;
  delete g_sa_config;

  delete g_egsa_instance;
}

int main(int argc, char** argv)
{
  //GOOGLE_PROTOBUF_VERIFY_VERSION;

//  ::google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InstallFailureSignalHandler();

  int retval = RUN_ALL_TESTS();

  ::google::protobuf::ShutdownProtobufLibrary();
//  ::google::ShutdownGoogleLogging();
  return retval;
}

