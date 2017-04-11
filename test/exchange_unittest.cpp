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
// NB: Удалить возвращаемый объект после использования
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
  rc =  get_request_by_name("unexistant_request", req_entry_dict);
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

#if 1
  LOG(INFO) << "BEGIN cycles activates testing";
  rc = g_egsa_instance->activate_cycles();
  EXPECT_TRUE(rc == OK);

  g_egsa_instance->wait(16);
  rc = g_egsa_instance->deactivate_cycles();
  LOG(INFO) << "END cycles activates testing";
#endif
}

// Проверка работы класса Системы Сбора
TEST(TestEXCHANGE, SAC_CREATE)
{
  egsa_config_site_item_t new_item;

  // Есть хотя бы один хост для тестов?
  ASSERT_TRUE(g_egsa_config->sites().size() > 0);

  // да - его и возьмём
  new_item.name.assign(g_egsa_config->sites().begin()->first);
  new_item.nature   = g_egsa_config->sites().begin()->second->nature;
  new_item.level    = g_egsa_config->sites().begin()->second->level;
  new_item.auto_init= g_egsa_config->sites().begin()->second->auto_init;
  new_item.auto_gencontrol = g_egsa_config->sites().begin()->second->auto_gencontrol;
  AcqSiteEntry new_sac_info(g_egsa_instance, &new_item);

  g_sa = new SystemAcquisition(g_egsa_instance, &new_sac_info);
  // Начальное состояние СС
  EXPECT_TRUE(g_sa->state() == SA_STATE_UNKNOWN);
  // Послать системе команду инициализации
  g_sa->send(ADG_D_MSG_ENDALLINIT);
  // Поскольку это имитация, состояние системы не изменится,
  // и должен сработать таймер по завершению таймаута
  EXPECT_TRUE(g_sa->state() == SA_STATE_UNKNOWN);
}

#if 1
TEST(TestEXCHANGE, EGSA_SITES)
{
  const egsa_config_site_item_t config_item[] = {
    // Имя
    // |        Уровень
    // |        |            Тип
    // |        |            |                      AUTO_INIT
    // |        |            |                      |     AUTO_GENCONTROL
    // |        |            |                      |     |
    { "BI4001", LEVEL_LOCAL, GOF_D_SAC_NATURE_EELE, true, true  },
    { "BI4002", LEVEL_LOCAL, GOF_D_SAC_NATURE_EELE, true, true  },
    { "K42001", LEVEL_UPPER, GOF_D_SAC_NATURE_DIR,  true, false }
  };
  // В тестовом файле только три записи
  const AcqSiteEntry* check_data[3];
  check_data[0] = new AcqSiteEntry(g_egsa_instance, &config_item[0]);
  check_data[1] = new AcqSiteEntry(g_egsa_instance, &config_item[1]);
  check_data[2] = new AcqSiteEntry(g_egsa_instance, &config_item[2]);

  g_egsa_instance->init_sites();

  AcqSiteList& sites_list = g_egsa_instance->get_sites();

  for (size_t i=0; i < sites_list.size(); i++) {
    const AcqSiteEntry* entry = sites_list[i];

    switch(i) {
      case 0:
      case 1:
      case 2:
        EXPECT_TRUE(0 == strcmp(entry->name(), check_data[i]->name()));
        entry = sites_list[check_data[i]->name()];
        ASSERT_TRUE(NULL != entry);

        LOG(INFO) << entry->name()
                  << "; nature=" << entry->nature()
                  << "; auto_init=" << entry->auto_i()
                  << "; auto_gc=" << entry->auto_gc();

        EXPECT_TRUE(0 == strcmp(entry->name(), check_data[i]->name()));
        EXPECT_TRUE(check_data[i]->nature()  == entry->nature());
        EXPECT_TRUE(check_data[i]->auto_i()  == entry->auto_i());
        EXPECT_TRUE(check_data[i]->auto_gc() == entry->auto_gc());
        break;

      default:
        LOG(FATAL) << "Loaded SITES more than 3";
        ASSERT_TRUE(0 == 1);
    }
  }

  delete check_data[0];
  delete check_data[1];
  delete check_data[2];
}
#endif

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
 
#if 0
//  g_egsa_instance->handle_end_all_init(message_end_all_init, identity);
  LOG(INFO) << "\t:start waiting events (10 sec)";
  int i=1000;
  while (--i) {
    usleep(10000);
    g_egsa_instance->tick_tack();
  }
  LOG(INFO) << "\t:finish waiting events";
#endif

  delete message_end_all_init;
}

#if 0
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

  delete message_stop;
}
#endif

TEST(TestEXCHANGE, EGSA_FREE)
{
  delete g_egsa_config;
  delete g_sa_config;
  delete g_sa;

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

