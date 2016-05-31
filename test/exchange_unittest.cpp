#include "glog/logging.h"
#include "gtest/gtest.h"

#include <string>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_config_sac.hpp"
#include "exchange_config_egsa.hpp"
#include "exchange_egsa_init.hpp"
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_request_cyclic.hpp"

AcquisitionSystemConfig* g_sa_config = NULL;
EgsaConfig* g_egsa_config = NULL;
EGSA* g_egsa_instance = NULL;
msg::MessageFactory* g_message_factory = NULL;
zmq::context_t g_ctx(1);

const char* g_sa_config_filename = "BI4500.json";
const char* g_egsa_config_filename = "egsa.json";
extern ega_ega_odm_t_RequestEntry g_requests_table[]; // declared in exchange_egsa_impl.cpp

TEST(TestEXCHANGE, EGSA_CREATE)
{
  std::string broker_endpoint = ENDPOINT_BROKER;

  g_message_factory = new msg::MessageFactory(EXCHANGE_NAME);
  ASSERT_TRUE(g_message_factory != NULL);

  g_egsa_instance = new EGSA(broker_endpoint, g_ctx, g_message_factory);
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
  LOG(INFO) << "load " << g_egsa_config->sites().size() << " sites";
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

#if 0
TEST(TestEXCHANGE, EGSA_CYCLES)
{
  ega_ega_odm_t_RequestEntry* req_entry_dict = NULL;
  int rc;

  for(egsa_config_cycles_t::iterator it = g_egsa_config->cycles().begin();
      it != g_egsa_config->cycles().end();
      it++)
  {
    // Найти запрос с именем (*it).second->request_name в m_requests_table[]
    if (OK == get_request_by_name((*it).second->request_name, req_entry_dict)) {
      // Создадим экземпляр Цикла, удалиться он в деструкторе EGSA
      Cycle *cycle = new Cycle((*it).first.c_str(),
                               (*it).second->period,
                               req_entry_dict->e_RequestId,
                               CYCLE_NORMAL);

      cycle->dump();
      // Передать объект Цикл в подчинение EGSA
      g_egsa_instance->push_cycle(cycle);
    }
  }

  rc = g_egsa_instance->activate_cycles();
  EXPECT_TRUE(rc == OK);

  g_egsa_instance->wait(20);
  rc = g_egsa_instance->deactivate_cycles();
}
#endif

TEST(TestEXCHANGE, EGSA_RUN)
{
  g_egsa_instance->run();
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

  //::google::protobuf::ShutdownProtobufLibrary();
//  ::google::ShutdownGoogleLogging();
  return retval;
}

