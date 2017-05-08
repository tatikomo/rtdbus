#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <memory>
#include <vector>
#include <map>
#include <cstring>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_egsa_site.hpp"
#include "exchange_egsa_impl.hpp"

#include "exchange_smad_int.hpp"
#include "exchange_config_sac.hpp"

// Строки 0 и 1 используются, если меняется значение атрибута SYNTHSTATE
// Строки 2 и 3 используются, если меняется значение атрибута EXPMODE
// Строки 4 и 5 используются, если меняется значение атрибута INHIB
const AcqSiteEntry::ega_ega_aut_t_automate AcqSiteEntry::m_ega_ega_aut_a_auto [EGA_EGA_AUT_D_NB_TRANS][EGA_EGA_AUT_D_NB_STATE] =
{
  /* Transition = OPERATIONAL */
  /* --------------------------- */
  /* suppression of GenCtrl in EGA_EGA_AUT_D_STATE_NI_NM_O when synth.state changes from non operationnel to operationel first case */
  {
    {EGA_EGA_AUT_D_STATE_NI_NM_O,   ACTION_NONE    },  // Normal operation
    {EGA_EGA_AUT_D_STATE_NI_M_O,    ACTION_NONE    },  // Dispatcher request
    {EGA_EGA_AUT_D_STATE_NI_NM_O,   ACTION_NONE    },  // Normal operation
    {EGA_EGA_AUT_D_STATE_NI_M_O,    ACTION_NONE    },  // Dispatcher request
    {EGA_EGA_AUT_D_STATE_I_NM_O,    ACTION_NONE    },  // Inhibition
    {EGA_EGA_AUT_D_STATE_I_M_O,     ACTION_NONE    },  // Inhibition
    {EGA_EGA_AUT_D_STATE_I_NM_O,    ACTION_NONE    },  // Inhibition
    {EGA_EGA_AUT_D_STATE_I_M_O,     ACTION_NONE    }   // Inhibition
  },

  /* Transition = NO_OPERATIONAL */
  /* ------------------------------- */
  {
    {EGA_EGA_AUT_D_STATE_NI_NM_NO,  ACTION_NONE    },  // SA commands only
    {EGA_EGA_AUT_D_STATE_NI_M_NO,   ACTION_NONE    },  // SA commands only
    {EGA_EGA_AUT_D_STATE_NI_NM_NO,  ACTION_AUTOINIT},  // SA commands only
    {EGA_EGA_AUT_D_STATE_NI_M_NO,   ACTION_NONE    },  // SA commands only
    {EGA_EGA_AUT_D_STATE_I_NM_NO,   ACTION_NONE    },  // Inhibition
    {EGA_EGA_AUT_D_STATE_I_M_NO,    ACTION_NONE    },  // Inhibition
    {EGA_EGA_AUT_D_STATE_I_NM_NO,   ACTION_NONE    },  // Inhibition
    {EGA_EGA_AUT_D_STATE_I_M_NO,    ACTION_NONE    }   // Inhibition
  },

  /* Transition = EXPLOITATION */
  /* ------------------------- */
  {
    {EGA_EGA_AUT_D_STATE_NI_NM_NO,  ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_NI_NM_NO,  ACTION_AUTOINIT},
    {EGA_EGA_AUT_D_STATE_NI_NM_O,   ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_NI_NM_O,   ACTION_GENCONTROL},
    {EGA_EGA_AUT_D_STATE_I_NM_NO,   ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_NM_NO,   ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_NM_O,    ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_NM_O,    ACTION_NONE    }
  },

  /* Transition = MAINTENANCE */
  /* ------------------------ */
  {
    {EGA_EGA_AUT_D_STATE_NI_M_NO,   ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_NI_M_NO,   ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_NI_M_O,    ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_NI_M_O,    ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_M_NO,    ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_M_NO,    ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_M_O,     ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_M_O,     ACTION_NONE    }
  },

  /* Transition = INHIBITION */
  /* ----------------------- */
  {
    {EGA_EGA_AUT_D_STATE_I_NM_NO,   ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_M_NO,    ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_NM_O,    ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_M_O,     ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_NM_NO,   ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_M_NO,    ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_NM_O,    ACTION_NONE    },
    {EGA_EGA_AUT_D_STATE_I_M_O,     ACTION_NONE    }
  },

  /* Transition = DE-INHIBITION */
  /* --------------------------- */
  /* change GenCtrl to AutInit in EGA_EGA_AUT_D_STATE_NI_NM_NO (= synth.state -> non operationnel, exploit.mode -> exploit.) */
  /* and in EGA_EGA_AUT_D_STATE_NI_NM_O (= synth.state -> operationnel, exploit.mode -> exploit.) */
  {
    {EGA_EGA_AUT_D_STATE_NI_NM_NO,  ACTION_NONE     },
    {EGA_EGA_AUT_D_STATE_NI_M_NO,   ACTION_NONE     },
    {EGA_EGA_AUT_D_STATE_NI_NM_O,   ACTION_NONE     },
    {EGA_EGA_AUT_D_STATE_NI_M_O,    ACTION_NONE     },
    {EGA_EGA_AUT_D_STATE_NI_NM_NO,  ACTION_AUTOINIT },
    {EGA_EGA_AUT_D_STATE_NI_M_NO,   ACTION_NONE     },
    {EGA_EGA_AUT_D_STATE_NI_NM_O,   ACTION_AUTOINIT },
    {EGA_EGA_AUT_D_STATE_NI_M_O,    ACTION_NONE     }
  }
}; /* End of a_auto initialisation */


//bool [EGA_GENCONTROL..NOT_EXISTENT][NI_NM_NO..I_M_O] = {
// Номер строки - тип текущего Запроса
// Номер столбца - состояние связанной с Запросом СС
#define X true
#define _ false
const bool AcqSiteEntry::enabler_matrix[REQUEST_ID_LAST][SA_STATE_LAST] = {
    //                       NI_NM_NO
    //                       |  NI_M_NO
    //                       |  |  NI_NM_O
    //                       |  |  |  NI_M_O
    //                       |  |  |  |  I_NM_NO
    //                       |  |  |  |  |  I_M_NO
    //                       |  |  |  |  |  |  I_NM_O
    //                       |  |  |  |  |  |  |  I_M_O
    //                       |  |  |  |  |  |  |  |
    /* EGA_GENCONTROL */   { _, _, X, _, _, _, _, _ },
    /* EGA_INFOSACQ   */   { _, _, X, _, _, _, _, _ },
    /* EGA_URGINFOS   */   { _, _, X, _, _, _, _, _ },
    /* EGA_GAZPROCOP  */   { _, _, X, _, _, _, _, _ },
    /* EGA_EQUIPACQ   */   { _, _, X, X, _, _, _, _ },
    /* EGA_ACQSYSACQ  */   { _, _, X, X, X, X, X, _ },
    /* EGA_ALATHRES   */   { _, _, X, _, _, _, _, _ },
    /* EGA_TELECMD    */   { _, _, X, _, _, _, _, _ },
    /* EGA_TELEREGU   */   { _, _, X, _, _, _, _, _ },
    /* EGA_SERVCMD    */   { _, _, X, X, _, _, X, X },
    /* EGA_GLOBDWLOAD */   { _, _, _, X, _, _, _, _ },
    /* EGA_PARTDWLOAD */   { _, _, _, X, _, _, _, _ },
    /* EGA_GLOBUPLOAD */   { _, _, _, X, _, _, _, _ },
    /* EGA_INITCMD    */   { X, X, X, X, _, _, _, _ },
    /* EGA_GCPRIMARY  */   { _, _, X, _, _, _, _, _ },
    /* EGA_GCSECOND   */   { _, _, X, _, _, _, _, _ },
    /* EGA_GCTERTIARY */   { _, _, X, _, _, _, _, _ },
    /* EGA_DIFFPRIMARY*/   { _, _, X, _, _, _, _, _ },
    /* EGA_DIFFSECOND */   { _, _, X, _, _, _, _, _ },
    /* EGA_DIFFTERTIARY */ { _, _, X, _, _, _, _, _ },
    /* EGA_INFODIFFUSION*/ { _, _, X, _, _, _, _, _ },
    /* EGA_DELEGATION */   { _, _, X, X, _, _, _, _ },
    /* ESG_STATECMD   */   { _, _, X, _, _, _, _, _ },
    /* ESG_STATEACQ   */   { _, _, X, _, _, _, _, _ },
    /* ESG_SELECTLIST */   { _, _, X, _, _, _, _, _ },
    /* ESG_GENCONTROL */   { _, _, X, _, _, _, _, _ },
    /* ESG_INFOSACQ   */   { _, _, X, _, _, _, _, _ },
    /* ESG_HISTINFOSACQ */ { _, _, X, _, _, _, _, _ },
    /* ESG_ALARM      */   { _, _, X, _, _, _, _, _ },
    /* ESG_THRESHOLD  */   { _, _, X, _, _, _, _, _ },
    /* ESG_ORDER      */   { _, _, X, _, _, _, _, _ },
    /* ESG_HHISTINFSACQ */ { _, _, X, _, _, _, _, _ },
    /* ESG_HISTALARM  */   { _, _, X, _, _, _, _, _ },
    /* ESG_CHGHOUR    */   { _, _, X, _, _, _, _, _ },
    /* ESG_INCIDENT   */   { _, _, X, _, _, _, _, _ },
    /* ESG_MULTITHRES */   { _, _, X, _, _, _, _, _ },
    /* ESG_TELECMD    */   { _, _, X, _, _, _, _, _ },
    /* ESG_TELEREGU   */   { _, _, X, _, _, _, _, _ },
    /* ESG_EMERGENCY  */   { _, _, X, _, _, _, _, _ },
    /* ESG_ACDLIST    */   { _, _, X, _, _, _, _, _ },
    /* ESG_ACDQUERY   */   { _, _, X, _, _, _, _, _ },
  };
#undef _
#undef X

// ==============================================================================
AcqSiteEntry::AcqSiteEntry(EGSA* egsa, const egsa_config_site_item_t* entry)
  : m_egsa(egsa),
    m_synthstate(SYNTHSTATE_UNREACH),
    m_expmode(true),
    m_inhibition(false),
    m_IdAcqSite(),
    m_DispatcherName(),
    m_NatureAcqSite(entry->nature),
    m_AutomaticalInit(entry->auto_init),
    m_AutomaticalGenCtrl(entry->auto_gencontrol),
    m_FunctionalState(EGA_EGA_AUT_D_STATE_NI_NM_NO),
    m_Level(entry->level),
    m_InterfaceComponentActive(false),
    m_smad(NULL)
{
  std::string sa_config_filename;
  sa_common_t sa_common;
  AcquisitionSystemConfig* sa_config = NULL;

  strncpy(m_IdAcqSite, entry->name.c_str(), TAG_NAME_MAXLEN);

  // Имя СС не может содержать символа "/"
  assert(name()[0] != '/');

  sa_config_filename.assign(name());
  sa_config_filename += ".json";

  // Определить для указанной СС название файла-снимка SMAD
  sa_config = new AcquisitionSystemConfig(sa_config_filename.c_str());
  if (NOK == sa_config->load_common(sa_common)) {
     LOG(ERROR) << "Unable to parse SA " << name() << " common config";
  }
  else {
    m_smad = new InternalSMAD(sa_common.name.c_str(), sa_common.nature, sa_common.smad.c_str());

    // TODO: подключаться к InternalSMAD только после успешной инициализации модуля данной СС
#if 0
    if (STATE_OK != (m_smad->state() = m_smad->attach(name(), nature()))) {
      LOG(ERROR) << "FAIL attach to '" << name() << "', file=" << sa_common.smad
                 << ", rc=" << m_smad->state();
    }
    else {
      LOG(INFO) << "OK attach to  '" << name() << "', file=" << sa_common.smad;
    }
#endif
  }

//  m_cycles = look_my_cycles();
  init_functional_state();

  delete sa_config;
}

// ==============================================================================
AcqSiteEntry::~AcqSiteEntry()
{
  delete m_smad;
}

// ==============================================================================
// Инициализация внутреннего состояния в зависимости от атрибутов SYNTHSTATE, INHIB, EXPMODE
//  INHIB = true - в запрете
//        = false - разрешена
//  EXPMODE = true - в режиме эксплуатации
//          = false - в режиме техобслуживания
void AcqSiteEntry::init_functional_state()
{
   m_FunctionalState = EGA_EGA_AUT_D_STATE_NI_NM_NO;

   if (!m_inhibition) {  // Inhibition State is NO inhibited
      // -------------------------------------------------
      if ((m_synthstate == SYNTHSTATE_OPER) && (m_expmode)) {
	    m_FunctionalState = EGA_EGA_AUT_D_STATE_NI_NM_O; // Не в запрете, не в техобслуживании,    оперативна
      }
      else if ((m_synthstate == SYNTHSTATE_OPER) && (!m_expmode)) {
        m_FunctionalState = EGA_EGA_AUT_D_STATE_NI_M_O;  // Не в запрете,    в техобслуживании,    оперативна
      }
      else if ((m_synthstate != SYNTHSTATE_OPER) && (!m_expmode)) {
        m_FunctionalState = EGA_EGA_AUT_D_STATE_NI_M_NO; // Не в запрете,    в техобслуживании, не оперативна
      }
      else if ((m_synthstate != SYNTHSTATE_OPER) && (m_expmode)) {
        m_FunctionalState = EGA_EGA_AUT_D_STATE_NI_NM_NO;// Не в запрете, не в техобслуживании, не оперативна
      }
      else {
        LOG(ERROR) << name() << ": synthetic " << m_synthstate << ", inhibition " << m_inhibition;
      }
   }
   else {  // Inhibition State is inhibited
      // -------------------------------------------------
      if ((m_synthstate == SYNTHSTATE_OPER) && (m_expmode)) {
	    m_FunctionalState = EGA_EGA_AUT_D_STATE_I_NM_O;  //    в запрете, не в техобслуживании,    оперативна
      }
      else if ((m_synthstate == SYNTHSTATE_OPER) && (!m_expmode)) {
        m_FunctionalState = EGA_EGA_AUT_D_STATE_I_M_O;   //    в запрете,    в техобслуживании,    оперативна
      }
      else if ((m_synthstate != SYNTHSTATE_OPER) && (!m_expmode)) {
        m_FunctionalState = EGA_EGA_AUT_D_STATE_I_M_NO;  //    в запрете,    в техобслуживании, не оперативна
      }
      else if ((m_synthstate != SYNTHSTATE_OPER) && (m_expmode)) {
	    m_FunctionalState = EGA_EGA_AUT_D_STATE_I_NM_NO; //    в запрете, не в техобслуживании, не оперативна
      }
      else {
        LOG(ERROR) << name() << ": synthetic " << m_synthstate << ", inhibition " << m_inhibition;
      }
   }
}

// ==============================================================================
// Получить ссылку на экземпляр Запроса указанного типа
const Request* AcqSiteEntry::get_dict_request(ech_t_ReqId type)
{
  Request *dict = NULL;

  assert(m_egsa);
  dict = m_egsa->dictionary_requests().query_by_id(type);

  return dict;
}

// ==============================================================================
// Функция вызывается при необходимости создания Запросов на инициализацию связи
int AcqSiteEntry::cbAutoInit()
{
  int rc = NOK;
  const char *fname = "cbAutoInit";
  const Request *rq_init = get_dict_request(EGA_INITCMD);

  if (true == add_request(rq_init)) {
    LOG(INFO) << fname << ": " << name() << ": push " << rq_init->name();
  }
  else {
    LOG(WARNING) << fname << ": " << name() << ": unable to push request #" << EGA_INITCMD << " (" << Request::name(EGA_INITCMD) << ")";
  }

  return rc;
}

// ==============================================================================
// Функция вызывается при необходимости создания Запросов на общий сбор информации (есть, Генерал Контрол!)
int AcqSiteEntry::cbGeneralControl()
{
  int rc = NOK;
  LOG(INFO) << "cbGeneralControl";
  return rc;
}

// ==============================================================================
int AcqSiteEntry::send(int msg_id)
{
  int rc = OK;

  switch(msg_id) {
    case ADG_D_MSG_ENDALLINIT:  /*process_end_all_init();*/ break;
    case ADG_D_MSG_ENDINITACK:  /*process_end_init_acq();*/ break;
    case ADG_D_MSG_INIT:        /*process_init();        */ break;
    case ADG_D_MSG_DIFINIT:     /*process_dif_init();    */ break;
    default:
      LOG(ERROR) << "Unsupported message (" << msg_id << ") to send to " << name();
      assert(0 == 1);
      rc = NOK;
  }

  return rc;
}

// -----------------------------------------------------------------------------------
// TODO: послать сообщение об успешном завершении инициализации связи
// сразу после того, как последняя из CC успешно ответила на сообщение об инициализации
void AcqSiteEntry::process_end_all_init()
{
//1  m_timer_ENDALLINIT = new Timer("ENDALLINIT");
//  m_egsa->send_to(name(), ADG_D_MSG_ENDALLINIT);
  LOG(INFO) << "Process ENDALLINIT for " << name();
}

// -----------------------------------------------------------------------------------
// Запрос состояния завершения инициализации
void AcqSiteEntry::process_end_init_acq()
{
  LOG(INFO) << "Process ENDINITACQ for " << name();
}

// -----------------------------------------------------------------------------------
// Конец инициализации
void AcqSiteEntry::process_init()
{
  LOG(INFO) << "Process INIT for " << name();
}

// -----------------------------------------------------------------------------------
// Запрос завершения инициализации после аварийного завершения
void AcqSiteEntry::process_dif_init()
{
  LOG(INFO) << "Process DIFINIT for " << name();
}

// -----------------------------------------------------------------------------------
// TODO: послать сообщение об завершении связи
void AcqSiteEntry::shutdown()
{
//1  m_timer_INIT = new Timer("SHUTDOWN");
  LOG(INFO) << "Process SHUTDOWN for " << name();
}

// -----------------------------------------------------------------------------------
int AcqSiteEntry::control(int code)
{
  int rc = NOK;

  // TODO: Передать интерфейсному модулю указанную команду
  LOG(INFO) << "Control SA '" << name() << "' with " << code << " code";
  return rc;
}

// -----------------------------------------------------------------------------------
// Прочитать изменившиеся данные
int AcqSiteEntry::pop(sa_parameters_t&)
{
  int rc = NOK;
  LOG(INFO) << "Pop changed data from SA '" << name() << "'";
  return rc;
}

// -----------------------------------------------------------------------------------
// Передать в СС указанные значения
int AcqSiteEntry::push(sa_parameters_t&)
{
  int rc = NOK;
  LOG(INFO) << "Push data to SA '" << name() << "'";
  return rc;
}

// -----------------------------------------------------------------------------------
int AcqSiteEntry::ask_ENDALLINIT()
{
  LOG(INFO) << "CALL AcqSiteEntry::ask_ENDALLINIT() for " << name();
  return NOK;
}
// -----------------------------------------------------------------------------------
void AcqSiteEntry::check_ENDALLINIT()
{
  LOG(INFO) << "CALL AcqSiteEntry::check_ENDALLINIT() for " << name();
}

// ==============================================================================
// TODO: СС и EGSA могут работать на разных хостах, в этом случае подключение EGSA к smad СС
// не получит доступа к реальным данным от СС. Их придется EGSA туда заносить самостоятельно.
int AcqSiteEntry::attach_smad()
{
#if 0
    if (STATE_OK != (m_smad->state() = m_smad->attach(name(), nature()))) {
      LOG(ERROR) << "FAIL attach to '" << name() << "', file=" << sa_common.smad
                 << ", rc=" << m_smad->state();
    }
    else {
      LOG(INFO) << "OK attach to  '" << name() << "', file=" << sa_common.smad;
    }
#endif

  return NOK;
}

// ==============================================================================
int AcqSiteEntry::detach_smad()
{
  int rc = NOK;

  LOG(WARNING) << "Not implemented";

  return rc;
}

// ==============================================================================
// Управление состояниями СС в зависимости от асинхронных изменений состояния атрибутов
// SYNTHSTATE, INHIBITION, EXPMODE в БДРВ, приходящих по подписке
int AcqSiteEntry::change_state(int attribute_idx, int val)
{
  char attr_name[100] = "";
  //
  int rc = OK;
  int i_indtrans = 0;
  const sa_state_t prev_state = m_FunctionalState;
  const synthstate_t prev_synthstate = m_synthstate;
  const bool prev_inhibition = m_inhibition;
  const bool prev_expmode = m_expmode;

  switch (attribute_idx) {
    case RTDB_ATT_IDX_SYNTHSTATE:
      if ((SYNTHSTATE_UNREACH <= val) && (val <= SYNTHSTATE_PRE_OPER)) {
        m_synthstate = static_cast<synthstate_t>(val);
        sprintf(attr_name, "%s:=%d", RTDB_ATT_SYNTHSTATE, val);

        if (SYNTHSTATE_OPER == m_synthstate)
          i_indtrans = EGA_EGA_AUT_D_TRANS_O;
        else
          i_indtrans = EGA_EGA_AUT_D_TRANS_NO;
      }
      else {
        LOG(ERROR) << name() << ": skip unsupported " << RTDB_ATT_SYNTHSTATE << " value:" << val;
        rc = NOK;
      }
      break;

    case RTDB_ATT_IDX_INHIB:
      if ((0 == val) || (1 == val)) {
        m_inhibition = (1 == val);
        sprintf(attr_name, "%s:=%d", RTDB_ATT_INHIB, val);

        if (1 == m_inhibition)
          i_indtrans = EGA_EGA_AUT_D_TRANS_I;
        else
          i_indtrans = EGA_EGA_AUT_D_TRANS_NI;
      }
      else {
        LOG(ERROR) << name() << ": skip unsupported " << RTDB_ATT_INHIB << " value:" << val;
        rc = NOK;
      }
      break;

    case RTDB_ATT_IDX_EXPMODE:
      if ((0 == val) || (1 == val)) {
        m_expmode = (1 == val);
        sprintf(attr_name, "%s:=%d", RTDB_ATT_EXPMODE, val);

        if (1 == m_expmode)
          i_indtrans = EGA_EGA_AUT_D_TRANS_E;
        else
          i_indtrans = EGA_EGA_AUT_D_TRANS_NE;
      }
      else {
        LOG(ERROR) << name() << ": skip unsupported " << RTDB_ATT_EXPMODE << " value:" << val;
        rc = NOK;
      }
      break;

    default:
      LOG(ERROR) << name() << ": unsupported attribute idx#" << attribute_idx;
      rc = NOK;
  }

  const sa_state_t new_state = m_ega_ega_aut_a_auto[i_indtrans][m_FunctionalState].next_state;

  LOG(INFO) << name() << ": change \"" << attr_name << "\" "
            << prev_state << " => "
            << new_state
            << " (inhib,expmode,syntstate) ("
            << prev_inhibition << ":"
            << prev_expmode << ":"
            << prev_synthstate
            << ") => ("
            << m_inhibition << ":"
            << m_expmode << ":"
            << m_synthstate
            << ") "
            << "[" << i_indtrans << "," << m_FunctionalState << "] "
            << "act:" << m_ega_ega_aut_a_auto[i_indtrans][m_FunctionalState].action_type;

  switch (m_ega_ega_aut_a_auto[i_indtrans][m_FunctionalState].action_type) {
    case ACTION_AUTOINIT:
      rc = cbAutoInit();
      break;

    case ACTION_GENCONTROL:
      rc = cbGeneralControl();
      break;

    case ACTION_NONE:
    default:
      break;
  }

  m_FunctionalState = new_state;
  return rc;
}

// ==============================================================================
// Попытаться добавить в очередь указанный Запрос
bool AcqSiteEntry::add_request(const Request* new_req)
{
  // const char* fname = "AcqSiteEntry::add_request";
  // Признак - разрешить или запретить указанный Запрос для СС
  bool permit = false;

  // GEV TEST
  // Проверить, разрешен ли Запрос для текущего состояния СС
  LOG(INFO) << "Enabler: " << enabler_matrix[new_req->id()][state()];

  if (new_req->rclass() == Request::AUTOMATIC) {
    // Automatic request class treatment
    // ---------------------------------
    if (state() == EGA_EGA_AUT_D_STATE_NI_NM_NO) {
      // Автоматические Запросы разрешено подавать только из состояния СС "Не запрещена, не в техобслуживании, не оперативна"
      permit = true;
    }
    else {
      permit = false;
    }
  }
  else {
    switch (state()) {
      // ---------------------------------
      case EGA_EGA_AUT_D_STATE_NI_NM_O:     // Normal operation state
        permit = true;
        break;

      // ---------------------------------
      case EGA_EGA_AUT_D_STATE_I_NM_NO:     // Inhibition states
      case EGA_EGA_AUT_D_STATE_I_M_NO:
      case EGA_EGA_AUT_D_STATE_I_M_O:
      case EGA_EGA_AUT_D_STATE_I_NM_O:
        permit = false;
        break;

      // ---------------------------------
      case EGA_EGA_AUT_D_STATE_NI_M_O:      // Dispatcher requests only
        if (new_req->rclass() != Request::ASYNC) {
          permit = false;
        }
        else {
          permit = true;
        }
        break;

      case EGA_EGA_AUT_D_STATE_NI_NM_NO:
        // Acquisition systems requests only
        // ---------------------------------
        if (new_req->rprocess() == true) {
          permit = false;
        }
        else {
          if (new_req->id() != EGA_SERVCMD) {
            permit = true;
          }
        }
        break;

      case EGA_EGA_AUT_D_STATE_NI_M_NO:
        if (new_req->rclass() == Request::ASYNC) {
          permit = true;
        }
        else {
          permit = false;
        }
        break;
    }





















#if 0
    if (state() == EGA_EGA_AUT_D_STATE_NI_NM_O) {
      // Normal operation state
      // ---------------------------------
      permit = true;
    }
    else if ((state() == EGA_EGA_AUT_D_STATE_I_NM_NO) ||
       (state() == EGA_EGA_AUT_D_STATE_I_M_NO) ||
       (state() == EGA_EGA_AUT_D_STATE_I_M_O) ||
       (state() == EGA_EGA_AUT_D_STATE_I_NM_O) )
    {
      // Inhibition state
      // ---------------------------------
      permit = false;
    }
    else {
      // Others states
      // ---------------------------------
/* -------------------------------------------------------------------------- */
/*  Remarques :
    ---------
 new_req->b_Requestprocess = TRUE for processed request
 new_req->b_Requestprocess = FALSE for SA request
 e_RequestClass = FALSE for dispatcher request
 e_RequestClass = TRUE for another request class
                                                                              */
/* -------------------------------------------------------------------------- */
      // Dispatcher requests only
      // ------------------------
      if (state() == EGA_EGA_AUT_D_STATE_NI_M_O)   {
        if (new_req->rclass() != Request::ASYNC) {
          permit = false;
        }
        else {
          permit = true;
        }
      }
      else if (state() == EGA_EGA_AUT_D_STATE_NI_NM_NO) {
        // Acquisition systems requests only
        // ---------------------------------
        if (new_req->rprocess() == true) {
          permit = false;
        }
        else {
          if (new_req->id() != EGA_SERVCMD) {
            permit = true;
          }
        }
      }
      else if (state() == EGA_EGA_AUT_D_STATE_NI_M_NO) {
        if (new_req->rclass() == Request::ASYNC) {
          permit = true;
        }
        else {
          permit = false;
        }
      }
    } // end if other request
#endif
  } // end if automat request

  return permit;
}

#if 0
// ==============================================================================
#warning "Продолжить тут - Реализовать машину состояний CC"
// Изменение состояния СС вызывают изменения в очереди Запросов
// TODO: Реализовать машину состояний
int AcqSiteEntry::change_state_to(sa_state_t new_state)
{
  bool state_changed = false;

  switch (m_FunctionalState) {  // Старое состояние
    // --------------------------------------------------------------------------
    case SA_STATE_UNKNOWN:  // Неопределено
    // --------------------------------------------------------------------------

      switch(new_state) {
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_UNREACH:
          state_changed = true;
          // TODO: удалить возможно оставшиеся в очереди Запросы
        case SA_STATE_UNKNOWN:
          m_FunctionalState = new_state;
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_OPER:
          state_changed = true;
          // TODO: активировать запросы данных для этой СС
          // activate_cycle()
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_PRE_OPER:
          state_changed = true;
          // TODO: активировать запросы на инициализацию связи с этой СС
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_INHIBITED:
        case SA_STATE_FAULT:
        case SA_STATE_MAINTENANCE:
        case SA_STATE_DISCONNECTED:
          state_changed = true;
          // TODO: удалить возможно оставшиеся в очереди Запросы
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
      }

      break;

    // --------------------------------------------------------------------------
    case SA_STATE_UNREACH:  // Недоступна
    // --------------------------------------------------------------------------
      switch(new_state) {
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_UNREACH:
          state_changed = true;
          // TODO: удалить возможно оставшиеся в очереди Запросы
        case SA_STATE_UNKNOWN:
          m_FunctionalState = new_state;
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_OPER:
          state_changed = true;
          // TODO: активировать запросы данных для этой СС
          // activate_cycle()
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_PRE_OPER:
          state_changed = true;
          // TODO: активировать запросы на инициализацию связи с этой СС
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_INHIBITED:
        case SA_STATE_FAULT:
        case SA_STATE_MAINTENANCE:
        case SA_STATE_DISCONNECTED:
          state_changed = true;
          // TODO: удалить возможно оставшиеся в очереди Запросы
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

      }
      break;

    // --------------------------------------------------------------------------
    case SA_STATE_OPER:     // Оперативная работа
    // --------------------------------------------------------------------------
      switch(new_state) {
        case SA_STATE_UNKNOWN: break;
        case SA_STATE_UNREACH: break;
        case SA_STATE_OPER: break;
        case SA_STATE_PRE_OPER: break;
        case SA_STATE_INHIBITED: break;
        case SA_STATE_FAULT: break;
        case SA_STATE_MAINTENANCE: break;
        case SA_STATE_DISCONNECTED: break;
      }
      break;

    // --------------------------------------------------------------------------
    case SA_STATE_PRE_OPER: // В процессе инициализации
    // --------------------------------------------------------------------------
      switch(new_state) {
        case SA_STATE_UNKNOWN: break;
        case SA_STATE_UNREACH: break;
        case SA_STATE_OPER: break;
        case SA_STATE_PRE_OPER: break;
        case SA_STATE_INHIBITED: break;
        case SA_STATE_FAULT: break;
        case SA_STATE_MAINTENANCE: break;
        case SA_STATE_DISCONNECTED: break;
      }
      break;

    // --------------------------------------------------------------------------
    case SA_STATE_INHIBITED:// в запрете работы
    // --------------------------------------------------------------------------
      switch(new_state) {
        case SA_STATE_UNKNOWN: break;
        case SA_STATE_UNREACH: break;
        case SA_STATE_OPER: break;
        case SA_STATE_PRE_OPER: break;
        case SA_STATE_INHIBITED: break;
        case SA_STATE_FAULT: break;
        case SA_STATE_MAINTENANCE: break;
        case SA_STATE_DISCONNECTED: break;
      }

      break;

    // --------------------------------------------------------------------------
    case SA_STATE_FAULT:    // сбой
    // --------------------------------------------------------------------------
      switch(new_state) {
        case SA_STATE_UNKNOWN: break;
        case SA_STATE_UNREACH: break;
        case SA_STATE_OPER: break;
        case SA_STATE_PRE_OPER: break;
        case SA_STATE_INHIBITED: break;
        case SA_STATE_FAULT: break;
        case SA_STATE_MAINTENANCE: break;
        case SA_STATE_DISCONNECTED: break;
      }
      break;

    // --------------------------------------------------------------------------
    case SA_STATE_MAINTENANCE: // техобслуживание
    // --------------------------------------------------------------------------
      break;

    // --------------------------------------------------------------------------
    case SA_STATE_DISCONNECTED: // не подключена
    // --------------------------------------------------------------------------
      switch(new_state) {
        case SA_STATE_UNKNOWN: break;
        case SA_STATE_UNREACH: break;
        case SA_STATE_OPER: break;
        case SA_STATE_PRE_OPER: break;
        case SA_STATE_INHIBITED: break;
        case SA_STATE_FAULT: break;
        case SA_STATE_MAINTENANCE: break;
        case SA_STATE_DISCONNECTED: break;
      }
      break;
  }

  if (state_changed) {
    LOG(INFO) << name() << ": state was changed from " << m_FunctionalState << " to " << new_state;
  }

  m_FunctionalState = new_state;
  return m_FunctionalState;
}
#endif

// ==============================================================================
// Запомнить связку между Циклом и последовательностью Запросов
// NB: некоторые вложенные запросы могут быть отменены. Критерий отмены - отсутствие
// параметров, собираемых этим запросом.
// К примеру, виды собираемой от СС информации по Запросам:
//      Тревоги     : A_URGINFOS
//      Телеметрия  : A_INFOSACQ
//      Телеметрия? : A_GCPRIMARY | A_GCSECOND | A_GCTERTIARY
// Виды передаваемой в адрес СС информации по Запросам
//      Телеметрия  : A_IAPRIMARY | A_IASECOND | A_IATERTIARY
//      Уставки     : A_ALATHRES
int AcqSiteEntry::push_request_for_cycle(Cycle* cycle, int* included_requests)
{
  int rc = OK;
  int ir = 0;
  char tmp[200] = " ";
  char s_req[20];

  for (int idx = 0; idx < NBREQUESTS; idx++) {
    if (included_requests[idx]) {
      ir++;
      strcpy(s_req, Request::name(static_cast<ech_t_ReqId>(idx)));
      strcat(tmp, s_req);
      strcat(tmp, " ");
    }
  }

  // Игнорировать Cycle->req_id(), если included_requests не пуст
  if (!ir) { // Нет вложенных Подзапросов - используем сам Запрос
    strcat(tmp, Request::name(cycle->req_id()));
    strcat(tmp, " ");
  }

  LOG(INFO) << name() << ": store request(s) [" << tmp << "] for cycle " << cycle->name();
  return rc;
}

// ==============================================================================
AcqSiteList::AcqSiteList()
 : m_egsa(NULL)
{
}

// ==============================================================================
AcqSiteList::~AcqSiteList()
{
  // TODO Разные экземпляры AcqSiteList в Cycle и в EGSA содержат ссылки на одни
  // и те же экземпляры объектов AcqSiteEntry
  for (std::vector<std::shared_ptr<AcqSiteEntry*>>::iterator it = m_items.begin();
       it != m_items.end();
       ++it)
  {
    // Удалить ссылку на AcqSiteEntry
    (*it).reset();
  }
}

// ==============================================================================
void AcqSiteList::set_egsa(EGSA* egsa)
{
  m_egsa = egsa;
}

// ==============================================================================
int AcqSiteList::attach_to_smad(const char* sa_name)
{
  return NOK;
}

// ==============================================================================
int AcqSiteList::detach()
{
  int rc = NOK;

  // TODO: Для всех подчиненных систем сбора:
  // 1. Изменить их состояние SYNTHSTATE на "ОТКЛЮЧЕНО"
  // 2. Отключиться от их внутренней SMAD
  for(std::vector<std::shared_ptr<AcqSiteEntry*>>::const_iterator it = m_items.begin();
      it != m_items.end();
      ++it)
  {
    AcqSiteEntry* entry = *(*it);
    LOG(INFO) << "TODO: set " << entry->name() << ".SYNTHSTATE = 0";
    LOG(INFO) << "TODO: detach " << entry->name() << " SMAD";
  }

  return rc;
}

// ==============================================================================
int AcqSiteList::detach_from_smad(const char* name)
{
  int rc = NOK;

  LOG(INFO) << "TODO: detach " << name << " SMAD";
  return rc;
}

// ==============================================================================
void AcqSiteList::insert(AcqSiteEntry* the_new_one)
{
  m_items.push_back(std::make_shared<AcqSiteEntry*>(the_new_one));
}

// ==============================================================================
// Вернуть элемент по имени Сайта
AcqSiteEntry* AcqSiteList::operator[](const char* name)
{
  AcqSiteEntry *entry = NULL;

  for(std::vector< std::shared_ptr<AcqSiteEntry*> >::const_iterator it = m_items.begin();
      it != m_items.end();
      ++it)
  {
    if (0 == std::strcmp((*(*it))->name(), name)) {
      entry = *(*it);
      break;
    }
  }

  return entry;
}

// ==============================================================================
// Вернуть элемент по имени Сайта
AcqSiteEntry* AcqSiteList::operator[](const std::string& name)
{
  AcqSiteEntry *entry = NULL;

  for(std::vector<std::shared_ptr<AcqSiteEntry*>>::const_iterator it = m_items.begin();
      it != m_items.end();
      ++it)
  {
    if (0 == name.compare((*(*it))->name())) {
      entry = *(*it);
      break;
    }
  }

  return entry;
}

// ==============================================================================
// Вернуть элемент по индексу
AcqSiteEntry* AcqSiteList::operator[](const std::size_t idx)
{
  AcqSiteEntry *entry = NULL;

  if (idx < m_items.size()) {
    entry = *m_items.at(idx);
  }

  return entry;
}

// ==============================================================================
// Освободить все ресурсы
int AcqSiteList::release()
{
  for (std::vector<std::shared_ptr<AcqSiteEntry*>>::iterator it = m_items.begin();
       it != m_items.end();
       ++it)
  {
    LOG(INFO) << "release site " << (*(*it))->name();
    AcqSiteEntry *entry = *(*it);
    (*it).reset();
    delete entry;
  }
  m_site_map.clear();

  return OK;
}
// ==============================================================================
