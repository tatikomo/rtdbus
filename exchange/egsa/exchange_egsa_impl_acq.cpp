#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "msg_common.h"
#include "msg_message.hpp"
#include "exchange_config_site.hpp"
#include "exchange_config_request.hpp"
#include "exchange_egsa_impl.hpp"

extern int interrupt_worker;

// ==========================================================================================================
// Аналог esg_acq_inm_InputWait в ГОФО
//
// Обменивается данными только с верхним уровнем EGSA::implementation
// В состоянии INIT допустимы сообщения:
//   INIT
//   STOP
//   ENDALLINIT
//   ASKLIFE
//   TK_MESSAGE
// В рабочем состоянии допустимы сообщения:
//   ECH_D_MSG_INT_REQUEST
//   ESG_D_RECEIPT_INDIC
//   ESG_D_TRANSMIT_REPORT
//   ESG_D_STATUS_INDIC
//   SIG_D_MSG_ENDHISTFILE
//   SIG_D_MSG_EMERGENCY
//   ESG_D_INIT_PHASE
int EGSA::implementation_acq()
{
  static const char *fname = "EGSA::implementation_acq";
  //bool changed = true;
  int linger = 0;
  int send_timeout_msec = SEND_TIMEOUT_MSEC;
  int recv_timeout_msec = RECV_TIMEOUT_MSEC;

  mdp::zmsg * request = NULL;
  zmq::pollitem_t socket_items[2];
  // Сокет для передачи данных в смежные системы.
  // Он должен быть подключён ко всем, с кем планируется обмен.
  zmq::socket_t es_acq(m_context, ZMQ_PUSH);
  const int PollingTimeout = 1000;
  int rc = OK;

#ifdef _FUNCTIONAL_TEST
  LOG(INFO) << fname << ": will use FUNCTIONAL_TEST";
#endif

  try
  {
    // Сокет для обмена командами с нитью EGSA::implementation
    m_acq_control = new zmq::socket_t(m_context, ZMQ_PAIR);

    m_acq_control->connect(ENDPOINT_EGSA_ACQ_BACKEND);
    LOG(INFO) << fname << ": connects to EGSA::implementation " << ENDPOINT_EGSA_ACQ_BACKEND;
    m_acq_control->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    m_acq_control->setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    m_acq_control->setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

    socket_items[0].socket = (void *) *m_acq_control;  // NB: (void*) обязателен
    socket_items[0].fd = 0;
    socket_items[0].events = ZMQ_POLLIN;
    socket_items[0].revents = 0;

    socket_items[1].socket = (void *) es_acq;  // NB: (void*) обязателен
    socket_items[1].fd = 0;
    socket_items[1].events = ZMQ_POLLIN;
    socket_items[1].revents = 0;

    init_acq();

    while (!interrupt_worker && (acq_state()!=STATE_STOP))
    {
      LOG(INFO) << fname << ": current state " << acq_state();

      zmq::poll(socket_items, 2, PollingTimeout);

      if (socket_items[0].revents & ZMQ_POLLIN) { // от EGSA::implementation
        request = new mdp::zmsg(*m_acq_control);
        assert(request);
        process_acq_command(request);
        delete request;
      }
      else if (socket_items[1].revents & ZMQ_POLLIN) { // получен запрос от внешней системы
        request = new mdp::zmsg(es_acq);
        assert(request);

        // replay address
        const std::string identity  = request->pop_front();
        const std::string empty = request->pop_front();

        rc = process_acq_request(request, identity);
        delete request;
      }
    }

    m_acq_control->close();
    delete m_acq_control;
    es_acq.close();

  } // конец блока try-catch
  catch(zmq::error_t error)
  {
    LOG(ERROR) << fname << ": catch: " << error.what();
    rc = NOK;
  }
  catch(std::exception & e)
  {
    LOG(ERROR) << fname << ": catch the signal: " << e.what();
    rc = NOK;
  }

  fini_acq();

  LOG(INFO) << fname << ": done";

  pthread_exit(NULL);

  return rc;
}

// ==========================================================================================================
int EGSA::init_acq()
{
  int rc = OK;

  LOG(INFO) << "Start ES_ACQ thread";

  // TODO: подключиться к XDB

  change_acq_state_to(STATE_INITIAL);

  return rc;
}

// ==========================================================================================================
int EGSA::fini_acq()
{
  int rc = OK;

  LOG(INFO) << "Finish ES_ACQ thread";

  // TODO: отключиться от БД XDB

  change_acq_state_to(STATE_STOP);

  return rc;
}

// ==========================================================================================================
// Обработка запросов от смежных Систем
int EGSA::process_acq_request(mdp::zmsg* request, const std::string& identity)
{
  static const char* fname = "EGSA::process_acq_request";
  int rc = NOK;
  rtdbMsgType msgType;
  rtdbExchangeId message_type;

  msg::Letter *letter = m_message_factory->create(request);
  if (letter->valid())
  {
    msgType = letter->header()->usr_msg_type();
    LOG(INFO) << fname << ": Receive type:" <<  msgType;

    switch (msgType)
    {
        // CASE input type is EGSA request
        // Manage the request
        //----------------------------------------------------------------------------
        case ECH_D_MSG_INT_REQUEST:
          rc = esg_acq_inm_ReqMan(letter);
          break;

        // CASE Broadcast receipt coming from COM
        //----------------------------------------------------------------------------
        case ESG_D_RECEIPT_INDIC:
          rc = esg_acq_inm_ReceiptIndic(letter);
          break;

        // CASE Broadcast report coming from COM
        // Delete the corresponding file
        //----------------------------------------------------------------------------
        case ESG_D_TRANSMIT_REPORT:
          rc = esg_acq_inm_TransmReport(letter);
          break;

        // CASE input type is a new state value for a SATO
        // Store this new state in memory (esg_acq_inm_StateManage)
        //----------------------------------------------------------------------------
        case ESG_D_STATUS_INDIC:
          rc = esg_acq_inm_StatesMan(letter);
          break;

        // CASE send basic request HHISTINFOSAQ
        // Store this new state in memory (esg_acq_inm_StateManage)
        //----------------------------------------------------------------------------
        case SIG_D_MSG_ENDHISTFILE:
          rc = esg_acq_inm_SendHhist(letter);
          break;

        // On receiving from SINF a request for emergency cycle
        // CASE Request for emergency acquisition cycle - send the request to the distant site
        //----------------------------------------------------------------------------
        case SIG_D_MSG_EMERGENCY:
          rc = esg_acq_inm_EmergenCycReq(letter);
          break;

        // CASE input type is a message from local ES_SENDing task which notify ACQ process
        // that distant init. phase is finished - treat the synthetic state of this site
        // (esg_acq_inm_PhaseChgMan)
        //----------------------------------------------------------------------------
        case ESG_D_INIT_PHASE:
          rc = esg_acq_inm_PhaseChgMan(letter);
          break;

        // CASE input type is unknown
        // Send an error
        //----------------------------------------------------------------------------
        default:
          LOG(ERROR) << fname << ": ESG_ESG_D_ERR_UNKNOWNMESS: " << msgType;
          break;
    } // end switch message type

  } // end message is valid

  return rc;
}

// ==========================================================================================================
int EGSA::process_acq_command(mdp::zmsg* request)
{
  static const char *fname = "EGSA::process_acq";
  int rc = OK;
  const char* report = internal_report_string[UNWILLING];
  const std::string command_identity = request->pop_front();

  LOG(INFO) << fname << ": read command " << command_identity;

  if (0 == command_identity.compare(TK_MESSAGE))        // <<<========================== Циклическое сообщение
  {
    // CASE input type is waken-up to delete old requests
    // Manage the deletion of too old requests (esg_acq_rsh_ReqsDelMan)
    const std::string cycle_identity = request->pop_front();
    LOG(INFO) << fname << ": timekeeper message " << cycle_identity;

    if (0 == cycle_identity.compare(OLDREQUEST)) {
      rc = esg_acq_rsh_ReqsDelMan();
    }
#if 0
    else if (0 == cycle_identity.compare(ESG_ACQ_INM_D_HISTDELAY)) {
      // new delay added for historic regulation
      rc = esg_acq_inm_NextHistTreat();
    }
    else { // other wakeups
      // Stop the wakeups which are not used in the process (esg_ine_tim_CancelTimer)
      rc = esg_ine_tim_CancelTimer(cycle_identity);
    }
#endif
  }
  else if (0 == command_identity.compare(ASKLIFE))      // <<========================== Запрос состояния
  {
    report = internal_report_string[GOOD];
    rc = OK;
  }
  else if (0 == command_identity.compare(INIT))         // <<========================== Приказ о начале инициализации
  {
    if (acq_state() == STATE_INITIAL) { // Процесс ждал инициализацию?
      // TODO: Выполнить подготовительные работы
      rc = OK;
      report = internal_report_string[GOOD];
      change_acq_state_to(STATE_WAITING_INIT);
    }
    else { // Не ожидалось получение команды об инициализации
      report = internal_report_string[ALREADY];
      LOG(WARNING) << fname << ": unwilling INIT message in state " << acq_state();
    }
  }
  else if (0 == command_identity.compare(ENDALLINIT))   // <========================== Все соседние процессы инициализировались
  {
    if (acq_state() == STATE_WAITING_INIT) { // Процесс ждал завершения инициализации?
      report = internal_report_string[GOOD];
      change_acq_state_to(STATE_END_INIT);
      // Поделаем чего-нибудь полезного...
      change_acq_state_to(STATE_RUN); // Переход в рабочее состояние
    }
    else { // Не ожидалось получение команды о завершении инициализации
      report = internal_report_string[ALREADY];
      LOG(WARNING) << fname << ": unwilling ENDALLINIT message in state " << acq_state();
    }
  }
  else if (0 == command_identity.compare(SHUTTINGDOWN)) // <<========================== Приказ о завершении работы
  {
      report = internal_report_string[GOOD];
      change_acq_state_to(STATE_SHUTTINGDOWN);
      change_acq_state_to(STATE_STOP);
  }
  else                                                  // <=========================== Необслуживаемая (еще?) команда
  {
      report = internal_report_string[UNWILLING];
      LOG(ERROR) << fname << ": get unsupported command: " << command_identity;
      rc = NOK;
  }

  // Ответить на сообщение о завершении инициализации
  m_acq_control->send(report, strlen(report));
  LOG(INFO) << fname << ": send replay " << report;

  return rc;
}

// ==========================================================================================================
// Проверка ответа указанного Сайта на Составной (?) запрос
int EGSA::esg_acq_rsh_CheckBreq(AcqSiteEntry *site) //, void * /*esg_esg_t_ReqResp */ r_ReqResp)
{
  static const char *fname = "esg_acq_rsh_CheckBreq";
  int rc = OK;
#if 1
#else
  esg_esg_lis_t_ProgListEntry r_IConsultEntry;
  esg_esg_lis_t_ProgListEntry r_OResultEntry;
  esg_esg_lis_t_ProgListEntry r_ONextEntry;
  esg_esg_t_DescRequest r_IReadRequest;
  esg_esg_odm_t_AcqSiteEntry r_AcqSite;
  gof_t_Cause r_FailCause;
  int i_StateStatus;
  int i_DelReqStatus;
  bool b_ResFlag;

  // the state has now the ExecResult type
  gof_t_ExecResult e_State;
  int i_ReqState;
  esg_t_DataCharact e_ReqPriority;
  int h_RankBas;
  int i_StateValue;
  int i_SiteStateValue;
  bool b_DeleteReq;
  bool b_Result;
  esg_esg_t_DescRequest r_ProviderRequestDesc;
  esg_esg_odm_t_LocalReq r_ResultLocalReq;
  esg_esg_t_ReqResp r_ReqInv;
  int j_j;
  bool b_Period_Pres;
  gof_t_PeriodType e_Hist_Period;
  bool b_PeriodFound;
  bool b_HasData;
  esg_esg_t_HHistTiPeriods *pr_IHHistTiPeriods;
  esg_esg_odm_t_GeneralData r_LocalGenData;
  gof_t_StructAtt r_RecAttr;    // Attribut of a smad record
  bool b_RecAttrChanged;        // Changed indicator of smad record
  int i_SynthState;             // value of state

  // next basic request to treat (may be the same) as previous one for an HHist TI request
  int h_NextRankBas;

  // indicator of still at least one period to treat in HHistTi
  bool b_PeriodToTreat;

  // number of basic requests
  int h_NbBasicReq;

  // composed request structure
  esg_esg_odm_t_ComposedReq r_ResultCompReq;

  // ..............................................................................
  b_DeleteReq = false;
  b_ResFlag = false;
  e_State = GOF_D_FAILURE;
  b_HasData = false;
  i_error_code = 0;
  s_error_text[0] = '\0';

  LOG(ERROR) << fname << ": site=" << site->name();

  // Try to find the local request - r_OResultEntry - corresponding to the
  // exchange identifier of the response received. (it means containing the
  // basic request corresponding to the entry.
  // -------------------------------------------------------------------------
  if (rc == OK)
  {
    r_IReadRequest.i_IdExchange = r_ReqResp.Request_Id.i_IdExchange;
    r_IReadRequest.r_ExchangedRequest.h_ReqId = r_ReqResp.Request_Id.r_ExchangedRequest.h_ReqId;
    r_IReadRequest.r_ExchangedRequest.h_ReqType = r_ReqResp.Request_Id.r_ExchangedRequest.h_ReqType;

    rc = esg_esg_lis_ConsultBasic(site
                                        r_IReadRequest,
                                        &h_RankBas,
                                        &b_ResFlag,
                                        &r_OResultEntry);
    if (rc != OK)
    {
      LOG(ERROR) << fname << ": esg_esg_lis_ConsultBasic  site " << site->name()
    }
  }


  // If the basic request is not found in the progress list the routine
  // an error message is stored in a message file
  // ---------------------------------------------------------------------
  if ((rc == OK) && (b_ResFlag == false)) {
    rc = NOK;
    LOG(ERROR) << fname << ": ESG_ESG_D_ERR_INEXISTINGREQ " << site->name();
  }
  else {
    r_ProviderRequestDesc.i_IdExchange = r_OResultEntry.i_ProviderRequestDesc;
    r_ProviderRequestDesc.r_ExchangedRequest.h_ReqId = r_OResultEntry.e_EgaReqId;
  }

  // If the basic request is found, check wich type of response has been received.
  // -----------------------------------------------------------------------------
  if (rc == OK)
  {
    // the state has now the ExecResult type
    rc = esg_acq_rsh_CheckStateResp(r_ReqResp,
                                          &e_State,
                                          &b_HasData,
                                          &r_FailCause,
                                          &b_Period_Pres,
                                          &e_Hist_Period);

    LOG(INFO) << fname << ": site=" << site->name()
              << " After CheckStateResp Result=" << e_State
              << " ErrCause=" << i_error_code
              << " HPeriod=" << e_Hist_Period
              << " rc=" << rc;
  }

  // Check if the basic request is HHISTINFSACQ. In this case update the
  // array and stop the treatment of the last request.
  // -------------------------------------------------------------------
  if (rc == OK)
  {
    if (r_OResultEntry.ar_TabReqBas[h_RankBas].i_IdExchange.r_ExchangedRequest.h_ReqId == ESG_ESG_D_BASID_HHISTINFSACQ)
    {
      // set the period flag with the value treated
      if (b_Period_Pres == true)
      {
        pr_IHHistTiPeriods = r_OResultEntry.r_RequestContent.pc_Content;
        j_j = 0;
        b_PeriodFound = false;

        // the received period is set to terminated
        while ((j_j < pr_IHHistTiPeriods->i_NbPeriod) && (b_PeriodFound == false))
        {
          if (pr_IHHistTiPeriods->ar_AcqHHistTi[j_j].i_Period == e_Hist_Period)
          {
            pr_IHHistTiPeriods->ar_AcqHHistTi[j_j].b_Terminated = true;
            b_PeriodFound = true;
            LOG(INFO) << fname << "Terminated Period " << e_Hist_Period << ", j_j=" << j_j;
          }
          else
          {
            j_j++;
          }
        }

        if (b_PeriodFound)
        {
          // update the request
          rc = esg_esg_lis_UpdateEntry(site,
                                             ESG_ESG_LIS_D_UPSTATEREQUEST,
                                             &r_OResultEntry);
          if (rc != OK)
          {
            LOG(INFO) << fname << ": esg_esg_lis_UpdateEntry : site=", site->name();
          }
        }
        else
        {
          rc = NOK;
          LOG(ERROR) << fname << ": ESG_ESG_D_ERR_PERIODNFOUND " << site->name();
        }
      }
      else
      {
        rc = NOK;
        LOG(ERROR) << fname << ": ESG_ESG_D_ERR_NOPERIODIHHISTFILE " << site->name();
      }
    }
  }

  // Calls the esg_acq_dac_Switch to update share memory ESDx-EGSA
  // -------------------------------------------------------------
  if (rc == OK)
  {
    if (r_IReadRequest.r_ExchangedRequest.h_ReqId != ESG_ESG_D_BASID_STATECMD && e_State == GOF_D_SUCCESS && b_HasData == true)
    {
      rc = esg_acq_dac_Switch(r_ReqResp.FileName, site->name());
      if (rc != OK)
      {
        LOG(INFO) << fname << ": esg_acq_dac_Switch : site=", site->name();
      }
    }
  }

  if (rc == OK)
  {
    if (e_State == GOF_D_SUCCESS)
    {
      if (r_IReadRequest.r_ExchangedRequest.h_ReqId == ESG_ESG_D_BASID_STATECMD)
      {
        // Stop initialisation command for not operationnal site
        // -----------------------------------------------------
        rc = esg_acq_dac_GetSiteState(r_ReqResp.FileName,
                                     site->name()
                                     &i_SiteStateValue);
        if (rc == OK)
        {
          if (i_SiteStateValue != ECH_D_STATEVAL_OP)
          {
            // Update synthetic state
            // ----------------------
            if (i_SiteStateValue == ECH_D_STATEVAL_PREOP) {
              i_StateValue = GOF_D_BDR_SYNTHSTATE_PREOP;
            }
            else {
              i_StateValue = GOF_D_BDR_SYNTHSTATE_UNR;
            }

            rc = SynthStateMan(site, i_StateValue);
            if (rc == OK)
            {
              e_State = GOF_D_FAILURE;
              r_FailCause.i_error_code = 0;
              s_error_text[0] = '\0';
              b_DeleteReq = true;
            }
          }
          else
          {
            // repris de esg_esg_aut_StateFilter()
            //  Synchronisation of DIR-DIPL initialisation :
            //  DIR will start init to DIPL only when DIPL has terminated init with DIR (b_DistantInitTerminated is set to TRUE)
            i_StateStatus = ech_smd_p_ReadRecAttr(r_AcqSite.p_DataMemory,
                                                  site->name()
                                                  GOF_D_BDR_ATT_SYNTHSTATE,
                                                  &r_RecAttr,
                                                  &b_RecAttrChanged);
            if (i_StateStatus != OK)
            {
              LOG(ERROR) << fname << ": Get synth state of site " << site->name();
            }

            if (i_StateStatus == OK)
            {
              // Set synth state from unreachable to preopertionnel if response of STATECMD is ok
              // --------------------------------------------------------------------------------
              i_SynthState = r_RecAttr.val.val_Uint32;
              if (i_SynthState == GOF_D_BDR_SYNTHSTATE_UNR)
              {
                LOG(INFO) << fname << ": Set synth state from UNR to PREOP if state req ok ";
                i_StateValue = GOF_D_BDR_SYNTHSTATE_PREOP;

                i_StateStatus = SynthStateMan(site, i_StateValue);
                if (i_StateStatus != OK)
                {
                  LOG(INFO) << fname << ": Update and set synth state of site " << site->name();
                }
              }
            }
            esg_esg_odm_ConsultGeneralData(&r_LocalGenData);
            LOG(INFO) << fname
                    << ": Loc ReqId=" << r_OResultEntry.r_ManagedRequestDesc.r_ExchangedRequest.h_ReqId
                    << " Local Site obj type=" << r_LocalGenData.o_ObjectType
                    << " DistantInitTerminated=" << DistantInitTerminated();

            if ((r_OResultEntry.r_ManagedRequestDesc.
                 r_ExchangedRequest.h_ReqId == ESG_ESG_D_LOCID_INITCOMD)
                && (r_LocalGenData.o_ObjectType == GOF_D_BDR_OBJCLASS_DIR)
                && (r_AcqSite.b_DistantInitTerminated == false))
            {
              e_State = GOF_D_FAILURE;
              r_FailCause.i_error_code = 0;
              strcpy(r_FailCause.s_error_text, ESG_ESG_D_EMPTYSTRING);
              b_DeleteReq = true;
            }
          }
        }
      }
    }

    else
    {
      // Change synthetic state for initialisation command without success
      // -----------------------------------------------------------------
      if (r_OResultEntry.r_ManagedRequestDesc.r_ExchangedRequest.h_ReqId == ESG_ESG_D_LOCID_INITCOMD)
      {
        i_StateValue = GOF_D_BDR_SYNTHSTATE_PREOP;
        rc = SynthStateMan(site, i_StateValue);
        if (rc != OK)
        {
          LOG(ERROR) << fname << ": esg_acq_dac_SynthStateMan : site " << site->name();
        }
        if (rc == OK)
        {
          b_DeleteReq = true;
        }
      }
    }                           /* end if e_State != GOF_D_SUCCESS */
  }                             /* end if rc == OK */

  if (rc == OK)
  {
    rc = esg_esg_odm_ConsultLocalReq
        (r_OResultEntry.r_ManagedRequestDesc.r_ExchangedRequest.h_ReqId,
         &r_ResultLocalReq);
    if (rc != OK)
    {
      LOG(ERROR) << fname << ": esg_esg_odm_ConsultLocalReq : Site ", site->name();
    }
    else
    {
      // always read the composed decomposition
      // IF there is no decomposition then the number of basic request is 0.
      // In this case, the local request is considered.
      // Read the composed request in the odm tables request is found.
      rc = esg_esg_odm_ConsultComposedReq(site->name(),
                                                r_OResultEntry.r_ManagedRequestDesc.r_ExchangedRequest.
                                                h_ReqId, &r_ResultCompReq);
    }
  }

  if (rc == OK)
  {
    // get the number of basic requests
    if (r_ResultCompReq.h_NbBasicReq == 0)
      h_NbBasicReq = 1;
    else
      h_NbBasicReq = r_ResultCompReq.h_NbBasicReq;

    if (e_State == GOF_D_SUCCESS) // the answer to the basic request is OK
    {
      // If this is the answer to the last basic request and success, set synthetic state to OPERATIONNAL
      // ----------------------------------------------------------------

      // Now test on the general control request and not only on the last basic request of INIT
      // to consider the end of it and the settting of synthetic state to operational
      // The case of last basic request must be also treated for a DIPL
      // --------------------------------------------------------------------------------------
      if ((r_OResultEntry.r_ManagedRequestDesc.r_ExchangedRequest.h_ReqId == ESG_ESG_D_LOCID_INITCOMD)
        &&
         ((r_OResultEntry.ar_TabReqBas[h_RankBas].i_IdExchange.r_ExchangedRequest.h_ReqId == ESG_ESG_D_BASID_GENCONTROL)
// case of the answer to the last basic request
// This case is necessary for DIPL
          ||
          ((h_RankBas + 1) == h_NbBasicReq)))
      {
        LOG(INFO) << name()
            << " OPStateAuthorised=" << OPStateAuthorised()
            << " DistantInitTerminated=" << DistantInitTerminated()
            << " FirstDistInitOPStateAuth=" << FirstDistInitOPStateAuth();

        if (r_AcqSite.b_DistantInitTerminated == true)
        {
          i_StateValue = GOF_D_BDR_SYNTHSTATE_OP;
          rc = SynthStateMan(site, i_StateValue);

          if (rc == OK)
          {
            OPStateAuthorised(false);
            DistantInitTerminated(false);
            FirstDistInitOPStateAuth(false);

            LOG(INFO) << name()
                << " OPStateAuthorised=" << OPStateAuthorised()
                << " DistantInitTerminated=" << DistantInitTerminated()
                << " FirstDistInitOPStateAuth=" << FirstDistInitOPStateAuth();
          }
        }
        else
        {
          OPStateAuthorised(true);
          FirstDistInitOPStateAuth(true);
          LOG(INFO) << name()
              << " OPStateAuthorised=" << OPStateAuthorised()
              << " DistantInitTerminated=" << DistantInitTerminated()
              << " FirstDistInitOPStateAuth=" << FirstDistInitOPStateAuth();

        }
      }

      // If this is the answer to the last basic request send a message to the rigth CSCI to inform it that the answer
      // of the composed request is entirely received
      // --------------------------------------------------------
      // case of the answer to the last basic request
      if ((h_RankBas + 1) == h_NbBasicReq)
      {
        if (rc == OK)
        {
          // send a message to the rigth CSCI to inform it that the answer of the composed request is entirely received.
          // addition of reply type to send to EGSA
          rc = esg_ine_man_SendResp(r_ReqResp,
                                          ECH_D_ENDEXEC_REPLY,
                                          e_State,
                                          r_FailCause, r_ProviderRequestDesc);
          // Removes all the data concerning the composed request from the the progress list.
          // ------------------------------------------------------------- */
          i_DelReqStatus = esg_esg_lis_DeleteEntry (site, r_OResultEntry.r_ManagedRequestDesc);
          if (i_DelReqStatus != OK)
          {
            LOG(ERROR) << fname << ": Provider=" << site->name()
                       << " Basic ReqId" << r_ReqResp.Request_Id.r_ExchangedRequest.h_ReqId
                       << " filename=" << r_ReqResp.FileName;
          }
        }
      }
      // case of the answer not the last basic request */
      // --------------------------------------------- */
      else
      {
        /* update the state of the request in the proglist */
        r_OResultEntry.ar_TabReqBas[h_RankBas].i_StateRequest = ESG_ESG_LIS_D_EXECUTEDREQUESTSTATE;
        rc = esg_esg_lis_UpdateEntry(r_ReqResp.Provider_Id,
                                           ESG_ESG_LIS_D_UPSTATEREQUEST,
                                           &r_OResultEntry);

        // Look if the basic request with the rank h_Rank is an HHISTINFSACQ basic request.
        // In this case don't send the next basic request HHISTINFOSACQ
        // The treatment of next basic request must be performed even for an HHistTi request
/*
        if ((r_OResultEntry.ar_TabReqBas[h_RankBas].i_IdExchange.r_ExchangedRequest.h_ReqId != ESG_ESG_D_BASID_HHISTINFSACQ)
            ||
            ((r_OResultEntry.ar_TabReqBas[h_RankBas].i_IdExchange.r_ExchangedRequest.h_ReqId == ESG_ESG_D_BASID_HHISTINFSACQ) && (!b_HasData)))
        {
*/

// if the current basic request is not an HHistTI let us take the next request
// if the current basic request is an HHistTI let us take the next request
// if no more period has to be treated and remain on the same request on the contrary
// Now several basic requests are reserved for Hist Hist TI

// The request index is so always incremented even for Hist TI Hist request
          h_NextRankBas = h_RankBas + 1;
          if (r_OResultEntry.ar_TabReqBas[h_RankBas].i_IdExchange.r_ExchangedRequest.h_ReqId == ESG_ESG_D_BASID_HHISTINFSACQ)
          {
            // a possible non terminated period is searched
            b_PeriodToTreat = false;
            j_j = 0;
            while ((j_j < pr_IHHistTiPeriods->i_NbPeriod) && (b_PeriodToTreat == false))
            {
              if (pr_IHHistTiPeriods->ar_AcqHHistTi[j_j].b_Terminated == false) {
                b_PeriodToTreat = true;
              }
              else {
                j_j += 1;
              }
            }

            LOG(INFO) << fname() << ": PeriodToTreat=" << b_PeriodToTreat
                      << " NbPeriod=" << pr_IHHistTiPeriods->i_NbPeriod
                      << " j_j=" << j_j;
/*
            if (b_PeriodToTreat == true)
            {
              h_NextRankBas = h_RankBas;
            }
            else
            {
              h_NextRankBas = h_RankBas + 1;
            }
*/
          }

          // regarder l'etat de h_Rank + 1 si etat non envoye: creer le fichier et envoyer le message : voir crq
          if (rc == OK)
          {
            // send the next basic request
            memcpy(&r_ReqInv, &r_ReqResp, sizeof(esg_esg_t_ReqResp));
            strcpy(r_ReqInv.Receiver_Id, r_ReqResp.Provider_Id);
            strcpy(r_ReqInv.Provider_Id, r_ReqResp.Receiver_Id);

            // for a local init command having a general control basic request which is not on the first rank the
            // data category must be set to all category before calling esg_acq_crq_SendBreq
            if ((r_OResultEntry.r_ManagedRequestDesc.r_ExchangedRequest.h_ReqId == ESG_ESG_D_LOCID_INITCOMD)
                && (r_OResultEntry.ar_TabReqBas[h_NextRankBas].i_IdExchange.r_ExchangedRequest.h_ReqId == ESG_ESG_D_BASID_GENCONTROL))
            {
              r_ReqInv.DataCat = ESG_ESG_D_CATEGALL;
            }

            // the number of basic request is no longer always the one of the local request
            rc = esg_acq_crq_SendBreq(&r_OResultEntry, h_NextRankBas,
                                            h_NbBasicReq, r_ReqInv);
            if (rc != OK)
            {
              LOG(ERROR) << fname << ": esg_acq_crq_SendBreq : site " << site->name();
            }
          }
/*
        }
*/
      }
    }
  }

  if (((rc == OK) && (e_State == GOF_D_FAILURE)) || ((rc != OK) && (b_ResFlag == true)))
  {
    // treatment was incorrect for the identified request or the answer to the basic request is an error status
    // send a message to the right CSCI to inform an error occurred

    // DO NOT log an error if the exec result is failure with no error found - just put a trace of it
    if (rc != OK)
    {
      LOG(ERROR) << fname << ": Treat. err : e_State " << e_State <<", b_ResFlag " << b_ResFlag << ", rc " << rc << ", Site " << r_ReqResp.Provider_Id;
    }
    else
    {
      LOG(INFO) << fname << ": Treat. err : e_State " << e_State << ", b_ResFlag " << b_ResFlag << ", rc " << rc << ", Site " << r_ReqResp.Provider_Id;
    }

    // addition of reply type to send to EGSA
    rc = esg_ine_man_SendResp(r_ReqResp, ECH_D_ENDEXEC_REPLY, e_State, r_FailCause, r_ProviderRequestDesc);
    if (rc != OK)
    {
      LOG(ERROR) << fname << ": esg_ine_man_SendResp : site " << site->name();
    }
    // Removes all the data concerning the composed request from the the progress list
    // -------------------------------------------------------------------------------
    i_DelReqStatus = esg_esg_lis_DeleteEntry (r_ReqResp.Provider_Id, r_OResultEntry.r_ManagedRequestDesc);
    if (i_DelReqStatus != OK)
    {
      LOG(ERROR) << fname << ": Provider " << site->name()
                 << " Basic ReqId=" << r_ReqResp.Request_Id.r_ExchangedRequest.h_ReqId;
    }
  }

  if (rc == OK)
  {
    if (b_DeleteReq)
    {
      // Delete all requests in progress and the associated files
      // --------------------------------------------------------
      i_DelReqStatus = esg_ine_man_DeleteReqInProgress (r_ReqResp.Provider_Id);
      if (i_DelReqStatus != OK)
      {
        LOG(ERROR) << fname << ": esg_ine_man_DeleteReqInProgress : site " << site->name();
      }
    }
  }

  if (rc != OK)
  {
    LOG(ERROR) << fname << ": Provider " << site->name()
               << " Basic ReqId=" << r_ReqResp.Request_Id.r_ExchangedRequest.h_ReqId
               << " Filename=" << r_ReqResp.FileName;
  }

  LOG(INFO) << fname << ": End : stat=" << rc;

#endif
  return rc;
}

/*-END  esg_acq_rsh_CheckBreq ---------------------------------------------- */

// ==========================================================================================================
// EGSA request
int EGSA::esg_acq_inm_ReqMan(msg::Letter*)
{
  static const char* fname = "esg_acq_inm_ReqMan";
  int rc = NOK;

  LOG(INFO) << fname << ": run";

  return rc;
}

// ==========================================================================================================
// Broadcast receipt coming from COM
int EGSA::esg_acq_inm_ReceiptIndic(msg::Letter*)
{
  static const char* fname = "esg_acq_inm_ReceiptIndic";
  int rc = NOK;

  LOG(INFO) << fname << ": run";

  return rc;
}

// ==========================================================================================================
// Broadcast report coming from COM
int EGSA::esg_acq_inm_TransmReport(msg::Letter*)
{
  static const char* fname = "esg_acq_inm_TransmReport";
  int rc = NOK;

  LOG(INFO) << fname << ": run";

  return rc;
}

// ==========================================================================================================
// new state value for a SATO
int EGSA::esg_acq_inm_StatesMan(msg::Letter*)
{
  static const char* fname = "esg_acq_inm_StatesMan";
  int rc = NOK;

  LOG(INFO) << fname << ": run";

  return rc;
}

// ==========================================================================================================
// send basic request HHISTINFOSAQ
int EGSA::esg_acq_inm_SendHhist(msg::Letter*)
{
  static const char* fname = "esg_acq_inm_SendHhist";
  int rc = NOK;

  LOG(INFO) << fname << ": run";

  return rc;
}

// ==========================================================================================================
// request for emergency cycle
int EGSA::esg_acq_inm_EmergenCycReq(msg::Letter*)
{
  static const char* fname = "esg_acq_inm_EmergenCycReq";
  int rc = NOK;

  LOG(INFO) << fname << ": run";

  return rc;
}

// ==========================================================================================================
// distant init. phase is finished - treat the synthetic state of this site
int EGSA::esg_acq_inm_PhaseChgMan(msg::Letter*)
{
  static const char* fname = "esg_acq_inm_PhaseChgMan";
  int rc = NOK;

  LOG(INFO) << fname << ": run";

  return rc;
}

// ==========================================================================================================

