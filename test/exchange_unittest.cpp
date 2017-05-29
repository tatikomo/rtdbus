// Общесистемные заголовочные файлы
#include <string>
#include <list>
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
#include "mdp_common.h"
#include "mdp_zmsg.hpp"
#include "exchange_config.hpp"
#include "exchange_config_sac.hpp"
#include "exchange_config_egsa.hpp"
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_request.hpp"

#include "proto/rtdbus.pb.h"

AcquisitionSystemConfig* g_sa_config = NULL;
EGSA* g_egsa_instance = NULL;
AcqSiteEntry *g_sac_entry = NULL;

const char* g_sa_config_filename = "BI4500.json";

// =======================================================================================================================
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

// =======================================================================================================================
TEST(TestEXCHANGE, EGSA_CREATE)
{
  std::string broker_endpoint = ENDPOINT_BROKER;
  std::string service_name = EXCHANGE_NAME;

  g_egsa_instance = new EGSA(broker_endpoint, service_name);
  ASSERT_TRUE(g_egsa_instance != NULL);

  g_sa_config = new AcquisitionSystemConfig(g_sa_config_filename);
  ASSERT_TRUE(g_sa_config != NULL);
}

// =======================================================================================================================
TEST(TestEXCHANGE, SA_CONFIG)
{
  int rc = g_sa_config->load();
  EXPECT_TRUE(OK == rc);
}

// =======================================================================================================================
TEST(TestEXCHANGE, EGSA_CONFIG)
{
  int rc = g_egsa_instance->load_config();
  EXPECT_TRUE(OK == rc);

  ASSERT_TRUE(g_egsa_instance->config());

  LOG(INFO) << "load " << g_egsa_instance->config()->cycles().size() << " cycles";
  EXPECT_TRUE(g_egsa_instance->config()->cycles().size() == 7);

  LOG(INFO) << "load " << g_egsa_instance->config()->sites().size() << " sites";
  EXPECT_TRUE(g_egsa_instance->config()->sites().size() == 3);

  LOG(INFO) << "load " << g_egsa_instance->config()->requests().size() << " requests";
  EXPECT_TRUE(g_egsa_instance->config()->requests().size() == 41);
}

// =======================================================================================================================
// Подготовить полную информацию по циклам, включая связанные с этими циклами сайты
// ГТП:
// GCP_PGACQ_DIPL   =   основной цикл сбора информации (1 час)
//      Запросы:
//          A_GCPRIMARY
//
// GCS_SGACQ_DIPL   =   вторичный цикл сбора (8 часов)
//      Запросы:
//          A_GCSECOND
//
// INFO_DACQ_DIPL   =   ? (15 минут)
//      Запросы:
//          A_INFOSACQ
//
// ЛПУ:
// GENCONTROL       =   Общий сбор данных (1 час)
//      Запросы:
//          A_GENCONTROL
//
// URGINFOS         =   Опрос срочными данными (тревоги?) (10 минут)
//      Запросы:
//          A_URGINFOS
//
// INFOSACQ         =   Опрос состояния Системы Сбора (1 минута)
//      Запросы:
//          A_INFOSACQ
//
// INFO_DACQ_DIPADJ =   Опрос данных смежных ЛПУ (10 секунд)
//      Запросы:
//          A_INFOSACQ
//
//
// Циклы согласно отчету Конфигуратора:
//   'GENCONTROL'       DIPL general
//   'URGINFOS'         DIPL urgency
//   'INFOSACQ_URG'     DIPL urgency
//   'INFOSACQ'         infosacq
//   'INFO_DACQ_DIPADJ' DIPL differ
//   'GEN_GACQ_DIPADJ'  DIPL globale
//   'GCP_PGACQ_DIPL'   DIR principal globale
//   'GCS_SGACQ_DIPL'   DIPL secondary globale
//   'INFO_DACQ_DIPL'   DIR differ
//   'IAPRIMARY'        iaprimary (also SA)
//   'IASECOND'         iasecondary (also SA)
//   'SERVCMD'          service command (also SA)
//   'INFODIFF'         info diffusion (also SA)
//   'ACQSYSACQ'        acquise system acquisition (also SA)
//   'GCT_TGACQ_DIPL'   tertiary
//
// 2 SA : SAs sending binary informations (cycle 3)
// 3 SA : SAs sending serious binary informations (cycle 1 and 2)
// 4 SA : SAs sending not serious binary_informations (cycle 2)
// 5 : adjacent DIPLs sending informations (cycle 5,7)
// 6 DIR  : DIPLs sending primary informations to the DIR (cycle 11)
// 7 DIR  : DIPLs sending secondary informations to the DIR (cycle 12)
// 8 DIR  : DIPLs sending (in differential mode) primary/secondary informations to the DIR
// 9 DIR  : acquisition (in tertiaire mode) informations the DIPLs
// 1 DIPL : adjacent DIPLs Diffusion des informations en mode differentiel des DIPLs_adjacent
// 2 DIPL : DIPLs sending primary informations in differential mode to the DIR
// 3 DIPL : DIPLs sending secondary informations in differential mode to the DIR
// 4 DIPL : DIPLs sending tertiary informations in differential mode to the DIR
//
TEST(TestEXCHANGE, EGSA_CYCLES_CONFIG)
{
  // TODO Собрать для каждой СС информацию в виде, пригодном к выдаче Запросов в Циклах
  CycleList &cycles = g_egsa_instance->cycles();

  for(size_t cidx = 0; cidx < cycles.size(); cidx++) {
    Cycle* cycle = cycles[cidx];
    if (cycle) {
      LOG(INFO) << cycle->name();

      AcqSiteList* sites = cycle->sites();
      if (sites) {

        for (size_t sidx = 0; sidx < sites->count(); sidx++) {

          AcqSiteEntry* sa = (*sites)[sidx];
          LOG(INFO) << "\t" << "SA " << sa->name();

          // Получить для данной СС актуальный Запрос из конфигурации
          LOG(INFO) << "\t\t" << "REQ #" << cycle->req_id() << " " << Request::name(cycle->req_id());

          // Для актуального Запроса найти все вложенные в него подзапросы
          RequestDictionary& d_requests = g_egsa_instance->dictionary_requests();

          Request *main_req = d_requests.query_by_id(cycle->req_id());

          if (main_req) { // Есть вложенные Запросы
#if 0
            const ech_t_ReqId* included_requests = main_req->included();
            int ir = 0;

            while (NOT_EXISTENT != included_requests[ir]) {

              LOG(INFO) << "\t\t\t" << "IR #" << included_requests[ir] << " " << Request::name(included_requests[ir]);

              // Запомнить для этой СС связку "Цикл" - "Связанные Запросы"
              // TODO: некоторые вложенные Запросы можно пропускать, если для них нет
              sa->push_request_for_cycle(cycle, main_req->included());

              ir++;
            } // while по всем подзапросам
#else

            // Запомнить для этой СС связку "Цикл" - "Связанные Запросы"
            // TODO: некоторые вложенные Запросы можно пропускать, если для них нет
            sa->push_request_for_cycle(cycle, main_req->included());
#endif

          } // if есть вложенные подзапросы
        } // for по всем системам сбора
      } // if если для этого Цикла есть системы сбора
      else {
        LOG(WARNING) << "\t" << "Cycle #" << cidx << " hasn't any sites";
      }
    } // if если данный Цикл используется
    else {
      LOG(WARNING) << "Skip empty cycle #" << cidx;
    }
  } // for проверить все известные Циклы
}

#if 0
TEST(TestEXCHANGE, EGSA_CYCLES)
{
//  RequestEntry* req_entry_dict = NULL;
  int rc;

  LOG(INFO) << "BEGIN cycles activates testing";
  rc = g_egsa_instance->activate_cycles();
  EXPECT_TRUE(rc == OK);

  // > 36 секунд, чтобы GENCONTROL успел выполниться 2 раза
  g_egsa_instance->wait(10);
  /*
  for (int i=0; i<20; i++) {
    std::cout << ".";
    fflush(stdout);
    g_egsa_instance->tick_tack();
    sleep(1);
  }
  std::cout << std::endl;*/
  rc = g_egsa_instance->deactivate_cycles();
  LOG(INFO) << "END cycles activates testing";
}
#else
#warning "Suppress EGSA_CYCLES test for a while"
#endif

// =======================================================================================================================
// Проверка работы класса Системы Сбора
TEST(TestEXCHANGE, SAC_CREATE)
{
  egsa_config_site_item_t new_item;

  // Есть хотя бы один хост для тестов?
  ASSERT_TRUE(g_egsa_instance->config()->sites().size() > 0);

  // да - его и возьмём
  new_item.name.assign(g_egsa_instance->config()->sites().begin()->first);
  new_item.nature   = g_egsa_instance->config()->sites().begin()->second->nature;
  new_item.level    = g_egsa_instance->config()->sites().begin()->second->level;
  new_item.auto_init= g_egsa_instance->config()->sites().begin()->second->auto_init;
  new_item.auto_gencontrol = g_egsa_instance->config()->sites().begin()->second->auto_gencontrol;

  g_sac_entry = new AcqSiteEntry(g_egsa_instance, &new_item);
  // Начальное состояние СС
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_NO);
  // Послать системе команду инициализации
  g_sac_entry->send(ADG_D_MSG_ENDALLINIT);
  // Поскольку это имитация, состояние системы не изменится,
  // и должен сработать таймер по завершению таймаута
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_NO);
}

// =======================================================================================================================
// Проверка работы класса Системы Сбора
TEST(TestEXCHANGE, SAC_STATES)
{
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_NO);

#if 0
  LOG(INFO) << "Set state to PRE_OPER";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, 2);
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_NO);
  LOG(INFO) << "Set state to CONNECTED";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, 1);
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_O);
  LOG(INFO) << "Set state to incorrect value(3)";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, 3);
  // Состояние не изменилось
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_O);

  // Сбросить Запрет
  LOG(INFO) << "Clear attr \"INHIBITED\"";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_INHIB, 0);
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_O);
  // Установить Запрет
  LOG(INFO) << "Set attr \"INHIBITED\"";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_INHIB, 1);
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_I_NM_O);
  // Сбросить Запрет
  LOG(INFO) << "Clear attr \"INHIBITED\"";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_INHIB, 0);
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_O);

  // Сбросить Техобслуживание
  LOG(INFO) << "Clear attr \"EXPMODE\"";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_EXPMODE, 0);
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_O);
  // Установить Техобслуживание
  LOG(INFO) << "Set attr \"EXPMODE\"";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_EXPMODE, 1);
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_M_O);
  // Сбросить Техобслуживание
  LOG(INFO) << "Clear attr \"EXPMODE\"";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_EXPMODE, 0);
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_O);

  // Установить "В процессе инициализации связи"
  LOG(INFO) << "Set state to PRE_OPER";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, 2);
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_NO);
  // Установить "Обрыв связи"
  LOG(INFO) << "Set state to DISCONNECTED";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, 0);
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_NO);
  // Установить "В процессе инициализации связи"
  LOG(INFO) << "Set state to PRE_OPER";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, 2);
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_NO);
  // Установить "Связь установлена"
  LOG(INFO) << "Set state to CONNECTED";
  g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, 1);
  EXPECT_TRUE(g_sac_entry->state() == EGA_EGA_AUT_D_STATE_NI_NM_O);
#else
  for (int inhib = 0; inhib < 2; inhib++)
  {
    g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_INHIB, inhib);

    for (int expmode = 0; expmode < 2; expmode++)
    {
      g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_EXPMODE, expmode);

      // 0 -> 2 -> 1 : последовательность состояний "Недоступна" -> "Инициализация" -> "Оперативна"
      g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, 0);
      g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, 2);
      g_sac_entry->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, 1);
    }
  }

  while (!g_sac_entry->requests().empty()) {
    Request* finished = g_sac_entry->requests().front();
    LOG(INFO) << g_sac_entry->name() << ": release request " << finished->name() << ", state " << finished->str_state();
    delete finished;
    g_sac_entry->requests().pop_front();
  }

#endif

}

// =======================================================================================================================
TEST(TestEXCHANGE, EGSA_SITES)
{
  const egsa_config_site_item_t config_item[] = {
    // Имя
    // |
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

  AcqSiteList& sites_list = g_egsa_instance->sites();
  LOG(INFO) << "EGSA_SITES loads " << sites_list.count() << " sites";

  for (size_t i=0; i < sites_list.count(); i++) {
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
        LOG(FATAL) << "Loaded SITES more than 3: " << i;
        ASSERT_TRUE(0 == 1);
    }
  }

  delete check_data[0];
  delete check_data[1];
  delete check_data[2];
}

// =======================================================================================================================
TEST(TestEXCHANGE, EGSA_DICT_REQUESTS)
{
  RequestDictionary &rl = g_egsa_instance->dictionary_requests();

  for (size_t id = 0; id < rl.size() /*NBREQUESTS*/; id++) {

   // LOG(INFO) << "GEV: RequestDictionary idx=" << id << " size=" << rl.size();

    ech_t_ReqId rid = static_cast<ech_t_ReqId>(id);

    Request *r = rl.query_by_id(rid);
    // Запроса EGA_GAZPROCOP нет в конфигурации
    if (EGA_GAZPROCOP != id)
      EXPECT_TRUE(NULL != r);
    else
      EXPECT_TRUE(NULL == r);
#if 0
    if (r)
      LOG(INFO) << id << " req_id=" << r->id()
                << " name=" << Request::name(r->id()) /* << ", " << EgsaConfig::g_request_dictionary[r->id()].s_RequestName*/
                << " objclass=" << r->objclass()
                << " exchange_id=" << r->exchange_id()
                << " prio=" << r->priority();
#endif
  }
  // req_list.insert()

  // Локальный запрос IL_INITCOMD является композитным,
  // состоящим из AB_STATESCMD,AB_HISTALARM,AB_HHISTINFSACQ,AB_GENCONTROL
  Request *ric = rl.query_by_id(ESG_LOCID_INITCOMD);
  ASSERT_TRUE(NULL != ric);
  LOG(INFO) << ESG_ESG_D_LOCSTR_INITCOMD << ": composed() = " << ric->composed();
  EXPECT_TRUE(ric->composed() == 4);
  EXPECT_TRUE(ric->included()[ESG_BASID_STATECMD] == 1);
  EXPECT_TRUE(ric->included()[ESG_BASID_HISTALARM] == 2);
  EXPECT_TRUE(ric->included()[ESG_BASID_HHISTINFSACQ] == 3);
  EXPECT_TRUE(ric->included()[ESG_BASID_GENCONTROL] == 4);

  // Запрос URGINFOS не является композтным
  Request *ru = rl.query_by_id(EGA_URGINFOS);
  ASSERT_TRUE(NULL != ru);
  LOG(INFO) << EGA_EGA_D_STRURGINFOS << ": composed() = " << ru->composed();
  EXPECT_TRUE(ru->composed() == 0);
  EXPECT_TRUE(ru->included()[ESG_BASID_STATECMD] == 0);
}

// =======================================================================================================================
TEST(TestEXCHANGE, EGSA_RT_REQUESTS)
{
#if 0
  RequestRuntimeList rt_list;
  Request* dict_req = NULL;
  RequestDictionary& dict_requests = g_egsa_instance->dictionary_requests();
  AcqSiteList& sites_list = g_egsa_instance->sites();
  CycleList &cycles = g_egsa_instance->cycles();
  std::list<Request*>::iterator it;
  AcqSiteEntry* site1 = sites_list["BI4001"];
  AcqSiteEntry* site2 = sites_list["BI4002"];
  AcqSiteEntry* site3 = sites_list["K42001"];
  Cycle* cycle1 = cycles["GENCONTROL"];
  Cycle* cycle2 = cycles["INFOSACQ"];

  ASSERT_TRUE(site1);
  ASSERT_TRUE(cycle1);

  dict_req = dict_requests.query_by_id(EGA_INITCMD);
  ASSERT_TRUE(dict_req);
  Request* req1 = new Request(dict_req, site1, cycle1);
  dict_req = dict_requests.query_by_id(ESG_BASID_STATEACQ);
  ASSERT_TRUE(dict_req);
  Request* req2 = new Request(dict_req, site3, cycle1);
  dict_req = dict_requests.query_by_id(EGA_DELEGATION);
  ASSERT_TRUE(dict_req);
  Request* req3 = new Request(dict_req, site2, cycle2);
  dict_req = dict_requests.query_by_id(ESG_BASID_INFOSACQ);
  ASSERT_TRUE(dict_req);
  Request* req4 = new Request(dict_req, site3, cycle2);

  // Взвести колбек для запроса на 2 секунды
  rt_list.add(req1, 1);
  rt_list.add(req2, 1);
  rt_list.add(req3, 2);
  rt_list.add(req4, 2);
#else

  AcqSiteEntry* site1 = g_egsa_instance->sites()["BI4001"];
  AcqSiteEntry* site2 = g_egsa_instance->sites()["BI4002"];
  AcqSiteEntry* site3 = g_egsa_instance->sites()["K42001"];

#endif

  // ///////////////////////////////////////////////////////////////////////////////////////
  // Проверка работы обработки Запросов при изменении состояния СС
  // Для каждого Цикла должна быть своя очередь Запросов? Или все Запросы в одном списке?
  int iter=0;
  const int msec_delay = 250000;
  const int limit_sec = 25;
  // количество итераций в limit_sec секундах
  const int limit_iter = (1000000 / msec_delay) * limit_sec;

  while (iter++ < limit_iter)
  {
    LOG(INFO) << "iter " << iter << "/" << limit_iter;
    switch(iter) {
      case 2:
        site1->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, SYNTHSTATE_PRE_OPER);
        site1->esg_esg_aut_StateManage(RTDB_ATT_IDX_EXPMODE, 1);
        site1->esg_esg_aut_StateManage(RTDB_ATT_IDX_INHIB, 0);

        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, SYNTHSTATE_PRE_OPER);
        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_EXPMODE, 1);
        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_INHIB, 0);
        break;

      case 5:
        site2->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, SYNTHSTATE_PRE_OPER);
        site2->esg_esg_aut_StateManage(RTDB_ATT_IDX_EXPMODE, 1);
        site2->esg_esg_aut_StateManage(RTDB_ATT_IDX_INHIB, 0);
        break;

      case 8:
        site1->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, SYNTHSTATE_OPER);
        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, SYNTHSTATE_OPER);
        break;

      case 15:
        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, SYNTHSTATE_PRE_OPER);
        break;

      case 16:
        site2->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, SYNTHSTATE_OPER);
        break;

      case 20:
        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, SYNTHSTATE_OPER);
        break;

      case 45:
        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_EXPMODE, 0);
        break;

      case 53:
        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_EXPMODE, 1);
        break;

      case 60:
        site2->esg_esg_aut_StateManage(RTDB_ATT_IDX_EXPMODE, 0);
        break;

      case 70:
        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_INHIB, 1);
        break;

      case 78:
        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_INHIB, 0);
        break;

      case 82:
        site2->esg_esg_aut_StateManage(RTDB_ATT_IDX_EXPMODE, 1);

        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, SYNTHSTATE_UNREACH);
        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_EXPMODE, 1);
        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_INHIB, 0);
        break;

      case 85:
        site1->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, SYNTHSTATE_UNREACH);

        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, SYNTHSTATE_PRE_OPER);
        break;

      case 90:
        site3->esg_esg_aut_StateManage(RTDB_ATT_IDX_SYNTHSTATE, SYNTHSTATE_OPER);
        break;
    }

#if 0
    // NB: Запрос EGA_INITCMD с приоритетом 127 должен отобразиться
    // раньше, чем EGA_DELEGATION с приоритетом 99
    rt_list.timer();
#endif

    if (!site3->requests().empty()) {

        // Всегда исполнять запросы по очереди.
        // Пока не выполнится текущий, не переходить к исполнению следующих
        Request *executed = site3->requests().front();

        LOG(INFO) << site3->name() << ": executed " << executed->name() << " " << executed->str_state();

        switch(executed->state()) {
          // -----------------------------------------------------------------
          case Request::STATE_INPROGRESS:  // На исполнении
            executed->state(Request::STATE_EXECUTED);
            break;

          // -----------------------------------------------------------------
          case Request::STATE_ACCEPTED:    // Принят к исполнению
            // в режим ожидания, если не подразумевается немедленного ответа
            executed->state(Request::STATE_WAIT_N);
            break;

          // -----------------------------------------------------------------
          case Request::STATE_SENT:        // Отправлен
            // после получения подтверждения о приеме
            executed->state(Request::STATE_ACCEPTED);
            break;

          // -----------------------------------------------------------------
          case Request::STATE_EXECUTED:    // Исполнен
            // Удалить запрос из списка
            LOG(INFO) << "Done request " << executed->name();
            delete executed;
            site3->requests().pop_front();
            break;

          // -----------------------------------------------------------------
          case Request::STATE_ERROR:       // При выполнении возникла ошибка
            LOG(INFO) << "Error request " << executed->name();
            break;

          // -----------------------------------------------------------------
          case Request::STATE_NOTSENT:     // Еще не отправлен
            // TODO: отправить в адрес СС
            executed->state(Request::STATE_SENT);
            break;

          // -----------------------------------------------------------------
          case Request::STATE_WAIT_N:      // Ожидает (нормальный приоритет)
          case Request::STATE_WAIT_U:      // Ожидает (высокий приоритет)
            // ожидать ответа на запрос от СС, может быть:
            // ERROR = Ошибка
            // EXECUTED = Исполнено
            // INPROGRESS = Промежуточное состояние - ещё в процессе выполнения
            executed->state(Request::STATE_INPROGRESS);
            break;

          // -----------------------------------------------------------------
          case Request::STATE_SENT_N:      // Отправлен (нормальный приоритет)
          case Request::STATE_SENT_U:      // Отправлен (высокий приоритет)
            executed->state(Request::STATE_EXECUTED);
            break;

          // -----------------------------------------------------------------
          case Request::STATE_UNKNOWN:
            LOG(ERROR) << site3->name() << ": unsupported request " << executed->name() << " state: " << executed->state();
        }
    }

    usleep(msec_delay);
  }

  // Возможно остались неудаленные/невыполненные запросы
  LOG(INFO) << "Clear resources for SA " << site3->name();
  while (!site3->requests().empty()) {
    Request* finished = site3->requests().front();
    LOG(INFO) << site3->name() << ": release request=" << finished->name() << ", state=" << finished->str_state();
    delete finished;
    site3->requests().pop_front();
  }

#if 0
  delete req1;
  delete req2;
  delete req3;
  delete req4;
#endif
}

// =======================================================================================================================
TEST(TestEXCHANGE, EGSA_RUN)
{
#if 0
  g_egsa_instance->implementation();
#else
#warning "Отключена проверка EGSA::implementation"
  g_egsa_instance->run();
#endif
}

// =======================================================================================================================
/*
TEST(TestEXCHANGE, EGSA_ENDALLINIT)
{
  const std::string identity = "RTDBUS_BROKER";
  mdp::zmsg * message_end_all_init = create_message_by_type("EGSA", "TEST", ADG_D_MSG_ENDALLINIT);
  bool is_stop = false;

  EXPECT_TRUE(message_end_all_init != NULL);
  EXPECT_TRUE(is_stop == false);

#if 1
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
*/

// =======================================================================================================================
TEST(TestEXCHANGE, EGSA_FREE)
{
  delete g_sa_config;
  delete g_sac_entry;

  delete g_egsa_instance;
}

// =======================================================================================================================
#if 0
TEST(TestEXCHANGE, EGSA_STOP)
{
  std::string broker_endpoint = ENDPOINT_BROKER;
  std::string service_name = EXCHANGE_NAME;

  //msg::Letter* message_stop = NULL;
  const std::string identity = "RTDBUS_BROKER";
  mdp::zmsg * message_stop = create_message_by_type("EGSA", "TEST", ADG_D_MSG_STOP);
  bool is_stop = false;

  EXPECT_TRUE(message_stop != NULL);

  g_egsa_instance = new EGSA(broker_endpoint, service_name);
  ASSERT_TRUE(g_egsa_instance != NULL);

  g_egsa_instance->run();

  g_egsa_instance->processing(message_stop, identity, is_stop);

  //g_egsa_instance->handle_stop(message_stop, identity);
  EXPECT_TRUE(is_stop == true);

  delete message_stop;
}
#endif

// =======================================================================================================================
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

