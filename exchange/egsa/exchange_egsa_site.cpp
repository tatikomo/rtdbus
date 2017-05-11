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
    {EGA_EGA_AUT_D_STATE_NI_NM_O,   ACTION_NONE    },  // synthstate 1 -> 0 | (0) -> (2) : NI NM NO -> NI NM  O
    {EGA_EGA_AUT_D_STATE_NI_M_O,    ACTION_NONE    },  // synthstate 1 -> 0 | (1) -> (3) : NI  M NO -> NI  M  O
    {EGA_EGA_AUT_D_STATE_NI_NM_O,   ACTION_NONE    },  // synthstate 1 -> 0 | (2) -> (2) : NI NM  O -> NI NM  O
    {EGA_EGA_AUT_D_STATE_NI_M_O,    ACTION_NONE    },  // synthstate 1 -> 0 | (3) -> (3) : NI  M  O -> NI  M  O
    {EGA_EGA_AUT_D_STATE_I_NM_O,    ACTION_NONE    },  // synthstate 1 -> 0 | (4) -> (6) :  I NM NO ->  I NM  O
    {EGA_EGA_AUT_D_STATE_I_M_O,     ACTION_NONE    },  // synthstate 1 -> 0 | (5) -> (7) :  I  M NO ->  I  M  O
    {EGA_EGA_AUT_D_STATE_I_NM_O,    ACTION_NONE    },  // synthstate 1 -> 0 | (6) -> (6) :  I NM  O ->  I NM  O
    {EGA_EGA_AUT_D_STATE_I_M_O,     ACTION_NONE    }   // synthstate 1 -> 0 | (7) -> (7) :  I  M  O ->  I  M  O
  },

  /* Transition = NO_OPERATIONAL */
  /* ------------------------------- */
  {
    {EGA_EGA_AUT_D_STATE_NI_NM_NO,  ACTION_NONE    },  // synthstate 1 -> 0 | (0) -> (0) : NI NM NO -> NI NM NO
    {EGA_EGA_AUT_D_STATE_NI_M_NO,   ACTION_NONE    },  // synthstate 1 -> 0 | (1) -> (1) : NI  M NO -> NI  M NO
    {EGA_EGA_AUT_D_STATE_NI_NM_NO,  ACTION_AUTOINIT},  // synthstate 1 -> 0 | (2) -> (0) : NI NM  O -> NI NM NO
    {EGA_EGA_AUT_D_STATE_NI_M_NO,   ACTION_NONE    },  // synthstate 1 -> 0 | (3) -> (1) : NI  M  O -> NI  M NO
    {EGA_EGA_AUT_D_STATE_I_NM_NO,   ACTION_NONE    },  // synthstate 1 -> 0 | (4) -> (4) :  I NM NO ->  I NM NO
    {EGA_EGA_AUT_D_STATE_I_M_NO,    ACTION_NONE    },  // synthstate 1 -> 0 | (5) -> (5) :  I  M NO ->  I  M NO
    {EGA_EGA_AUT_D_STATE_I_NM_NO,   ACTION_NONE    },  // synthstate 1 -> 0 | (6) -> (4) :  I NM  O ->  I NM NO
    {EGA_EGA_AUT_D_STATE_I_M_NO,    ACTION_NONE    }   // synthstate 1 -> 0 | (7) -> (5) :  I  M  O ->  I  M NO
  },

  /* Transition = EXPLOITATION */
  /* ------------------------- */
  {
    {EGA_EGA_AUT_D_STATE_NI_NM_NO,  ACTION_NONE    },   // expmode 0 -> 1 | (0) -> (0) : NI NM NO -> NI NM NO
    {EGA_EGA_AUT_D_STATE_NI_NM_NO,  ACTION_AUTOINIT},   // expmode 0 -> 1 | (1) -> (0) : NI  M NO -> NI NM NO
    {EGA_EGA_AUT_D_STATE_NI_NM_O,   ACTION_NONE    },   // expmode 0 -> 1 | (2) -> (2) : NI NM  O -> NI NM  O
    {EGA_EGA_AUT_D_STATE_NI_NM_O,   ACTION_GENCONTROL}, // expmode 0 -> 1 | (3) -> (2) : NI  M  O -> NI NM  O
    {EGA_EGA_AUT_D_STATE_I_NM_NO,   ACTION_NONE    },   // expmode 0 -> 1 | (4) -> (4) :  I NM NO ->  I NM NO
    {EGA_EGA_AUT_D_STATE_I_NM_NO,   ACTION_NONE    },   // expmode 0 -> 1 | (5) -> (4) :  I  M NO ->  I NM NM
    {EGA_EGA_AUT_D_STATE_I_NM_O,    ACTION_NONE    },   // expmode 0 -> 1 | (6) -> (6) :  I NM  O ->  I NM  O
    {EGA_EGA_AUT_D_STATE_I_NM_O,    ACTION_NONE    }    // expmode 0 -> 1 | (7) -> (6) :  I  M  O ->  I NM  O
  },

  /* Transition = MAINTENANCE */
  /* ------------------------ */
  {
    {EGA_EGA_AUT_D_STATE_NI_M_NO,   ACTION_NONE    },   // expmode 1 -> 0 | (0) -> (1) : NI NM NO -> NI  M NO
    {EGA_EGA_AUT_D_STATE_NI_M_NO,   ACTION_NONE    },   // expmode 1 -> 0 | (1) -> (1) : NI  M NO -> NI  M NO
    {EGA_EGA_AUT_D_STATE_NI_M_O,    ACTION_NONE    },   // expmode 1 -> 0 | (2) -> (3) : NI NM  O -> NI  M  O
    {EGA_EGA_AUT_D_STATE_NI_M_O,    ACTION_NONE    },   // expmode 1 -> 0 | (3) -> (3) : NI  M  O -> NI  M  O
    {EGA_EGA_AUT_D_STATE_I_M_NO,    ACTION_NONE    },   // expmode 1 -> 0 | (4) -> (5) :  I NM NO ->  I  M NO
    {EGA_EGA_AUT_D_STATE_I_M_NO,    ACTION_NONE    },   // expmode 1 -> 0 | (5) -> (5) :  I  M NO ->  I  M NM
    {EGA_EGA_AUT_D_STATE_I_M_O,     ACTION_NONE    },   // expmode 1 -> 0 | (6) -> (7) :  I NM  O ->  I  M  O
    {EGA_EGA_AUT_D_STATE_I_M_O,     ACTION_NONE    }    // expmode 1 -> 0 | (7) -> (7) :  I  M  O ->  I  M  O
  },

  /* Transition = INHIBITION */
  /* ----------------------- */
  {
    {EGA_EGA_AUT_D_STATE_I_NM_NO,   ACTION_NONE    },   // inhib 0 -> 1 | (0) -> (4) : NI NM NO ->  I NM NO
    {EGA_EGA_AUT_D_STATE_I_M_NO,    ACTION_NONE    },   // inhib 0 -> 1 | (1) -> (5) : NI  M NO ->  I  M NO
    {EGA_EGA_AUT_D_STATE_I_NM_O,    ACTION_NONE    },   // inhib 0 -> 1 | (2) -> (6) : NI NM  O ->  I NM  O
    {EGA_EGA_AUT_D_STATE_I_M_O,     ACTION_NONE    },   // inhib 0 -> 1 | (3) -> (7) : NI  M  O ->  I  M  O
    {EGA_EGA_AUT_D_STATE_I_NM_NO,   ACTION_NONE    },   // inhib 0 -> 1 | (4) -> (4) :  I NM NO ->  I NM NO
    {EGA_EGA_AUT_D_STATE_I_M_NO,    ACTION_NONE    },   // inhib 0 -> 1 | (5) -> (5) :  I  M NO ->  I  M NO
    {EGA_EGA_AUT_D_STATE_I_NM_O,    ACTION_NONE    },   // inhib 0 -> 1 | (6) -> (6) :  I NM  O ->  I NM  O
    {EGA_EGA_AUT_D_STATE_I_M_O,     ACTION_NONE    }    // inhib 0 -> 1 | (7) -> (7) :  I  M  O ->  I  M  O
  },

  /* Transition = DE-INHIBITION */
  /* -------------------------- */
  {
    {EGA_EGA_AUT_D_STATE_NI_NM_NO,  ACTION_NONE    },   // inhib 1 -> 0 | (0) -> (0) : NI NM NO -> NI NM NO
    {EGA_EGA_AUT_D_STATE_NI_M_NO,   ACTION_NONE    },   // inhib 1 -> 0 | (1) -> (1) : NI  M NO -> NI  M NO
    {EGA_EGA_AUT_D_STATE_NI_NM_O,   ACTION_NONE    },   // inhib 1 -> 0 | (2) -> (2) : NI NM  O -> NI NM  O
    {EGA_EGA_AUT_D_STATE_NI_M_O,    ACTION_NONE    },   // inhib 1 -> 0 | (3) -> (3) : NI  M  O -> NI  M  O
    {EGA_EGA_AUT_D_STATE_NI_NM_NO,  ACTION_AUTOINIT},   // inhib 1 -> 0 | (4) -> (0) :  I NM NO -> NI NM NO
    {EGA_EGA_AUT_D_STATE_NI_M_NO,   ACTION_NONE    },   // inhib 1 -> 0 | (5) -> (1) :  I  M NO -> NI  M NO
    {EGA_EGA_AUT_D_STATE_NI_NM_O,   ACTION_AUTOINIT},   // inhib 1 -> 0 | (6) -> (2) :  I NM  O -> NI NM  O
    {EGA_EGA_AUT_D_STATE_NI_M_O,    ACTION_NONE    }    // inhib 1 -> 0 | (7) -> (3) :  I  M  O -> NI  M  O
  }
}; /* End of a_auto initialisation */


//bool [EGA_GENCONTROL..NOT_EXISTENT][NI_NM_NO..I_M_O] = {
// Номер строки - тип текущего Запроса
// Номер столбца - состояние связанной с Запросом СС
#define X true
#define _ false
const bool AcqSiteEntry::enabler_matrix[REQUEST_ID_LAST+1][EGA_EGA_AUT_D_NB_STATE+1] = {
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
    /* LOCID_GENCONTROL */ { _, _, X, _, _, _, _, _ },
    /* LOCID_GCPRIMARY  */ { _, _, X, _, _, _, _, _ },
    /* LOCID_GCSECONDARY*/ { _, _, X, _, _, _, _, _ },
    /* LOCID_GCTERTIARY */ { _, _, X, _, _, _, _, _ },
    /* LOCID_INFOSACQ   */ { _, _, X, _, _, _, _, _ },
    /* LOCID_INITCOMD   */ { _, _, X, _, _, _, _, _ },
    /* LOCID_CHGHOURCMD */ { _, _, X, _, _, _, _, _ },
    /* LOCID_TELECMD    */ { _, _, X, _, _, _, _, _ },
    /* LOCID_TELEREGU   */ { _, _, X, _, _, _, _, _ },
    /* LOCID_EMERGENCY  */ { _, _, X, _, _, _, _, _ },
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
    m_smad(NULL),
    m_history_info(),
    m_OPStateAuthorised(false),
    m_init_phase(NONE),
    m_Alarms(UNKNOWN),
    m_Infos(UNKNOWN)
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
  const Request *rq_init = NULL;

  switch(m_Level) {
    case LEVEL_LOCAL:
    case LEVEL_LOWER:
      rq_init = get_dict_request(EGA_INITCMD); // Композитный общий сбор
      break;

    case LEVEL_UNKNOWN:
      LOG(ERROR) << name() << ": skip INIT because SA level is UNKNOWN";
      break;

    case LEVEL_ADJACENT:
    case LEVEL_UPPER:
      rq_init = get_dict_request(ESG_LOCID_INITCOMD); // Композитный запрос инициализации связи
      break;
  }

  assert(rq_init);

  if (OK == add_request(rq_init)) {
    LOG(INFO) << fname << ": " << name() << ": push " << rq_init->name();
  }
  else {
    LOG(WARNING) << fname << ": " << name() << ": unable to push request #" << rq_init->id()
                 << " (" << Request::name(rq_init->id()) << ")";
  }

  return rc;
}

// ==============================================================================
// Функция вызывается при необходимости создания Запросов на общий сбор информации (есть, Генерал Контрол!)
int AcqSiteEntry::cbGeneralControl()
{
  const char *fname = "cbGeneralControl";
  const Request *rq_gencontrol = NULL;
  int rc = NOK;


  switch(m_Level) {
    case LEVEL_LOCAL:
    case LEVEL_LOWER:
      rq_gencontrol = get_dict_request(EGA_GENCONTROL); // Композитный общий сбор
      break;

    case LEVEL_UNKNOWN:
      LOG(ERROR) << name() << ": skip INIT because SA level is UNKNOWN";
      break;

    case LEVEL_ADJACENT:
    case LEVEL_UPPER:
      rq_gencontrol = get_dict_request(ESG_BASID_GENCONTROL); // Композитный общий сбор
      break;
  }

  assert(rq_gencontrol);

  // Поместить этот Запрос, если он не комбинированный, в очередь.
  // Если комбинированный, в очередь помещаются его подчинённые (egsa.json - INCLUDED_REQUESTS)
  if (OK == add_request(rq_gencontrol)) {
    LOG(INFO) << fname << ": " << name() << ": push " << rq_gencontrol->name();
  }
  else {
    LOG(WARNING) << fname << ": " << name() << ": unable to push request #" << rq_gencontrol->id()
                 << " (" << Request::name(rq_gencontrol->id()) << ")";
  }

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
  const action_type_t new_action = m_ega_ega_aut_a_auto[i_indtrans][m_FunctionalState].action_type;
  m_FunctionalState = new_state;

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
            << "[" << i_indtrans << "," << prev_state << "] "
            << "act:" << new_action;

  switch (new_action) {
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

  return rc;
}

// ==============================================================================
bool AcqSiteEntry::esg_state_filter(const Request* req, int hist_period)
{
  static const char* fname = "esg_state_filter";
  bool permission = false;
  sa_state_t current_state;
  bool b_DuplicateFlag;

//............................................................................
//	Execution sequence
//  Testing the functional state to authorise or to forbit the execution
//  of the request:
//    . state OPER :
// 	     Permission -> authorised
//
//    . state NO_OPER :
//      - Testing the request :
//       . basic requests :
// 		     Permission -> authorised
//       . local requests :
//          - requests for "exploitation" :
// 		        Permission -> forbidden
//          - other requests :
// 		        Permission -> authorised

  LOG(INFO) << fname << ": Site " << name() << " ReqId:" << req->id();

  current_state = m_FunctionalState;

  switch(current_state) {
    // All requests are authorized for operationnal state
    // -----------------------------------------------------------
    case EGA_EGA_AUT_D_STATE_NI_NM_O:
	// -----------------------------------------------------------
      permission = true;

      // if basic general control in progress, basic diffusion is refused
      if ((req->id() == ESG_BASID_INFOSACQ)) {
#if 0
        i_DuplicStatus = esg_esg_lis_ConsultEntry
                                  (name(),
                                   ESG_ESG_LIS_D_GETDUPLICATEDREQUEST,
                                   &r_ConsultEntry,
                                   r_ReadRequest,
                                   &b_DuplicateFlag,
                                   &r_ResultEntry);
#else
#warning "TODO: GETDUPLICATEDREQUEST"
        b_DuplicateFlag = false;
#endif
        LOG(INFO) << fname << ": ESG_ESG_LIS_D_GETDUPLICATEDREQUEST for state " << EGA_EGA_AUT_D_STATE_NI_NM_O;
        if (b_DuplicateFlag == true) {
          permission = false;
        }
      } // end if request ID is ESG_BASID_INFOSACQ

      // local init command is refused if the last one was terminated with success and a distant init command is in progress
      if (req->id() == ESG_LOCID_INITCOMD) {
        if ((OPStateAuthorised() == true) && (InitPhase() != END)) {
          permission = false;
        }
      }
      break;

    // request type for no operationnal state
	// for basic requests : check site flags according to req. id.
	// -----------------------------------------------------------
    case EGA_EGA_AUT_D_STATE_NI_NM_NO:
	// -----------------------------------------------------------
      if (req->id() <= LAST_BASIC_REQUEST) {
        permission = true;

        switch (req->id()) {
          case ESG_BASID_ALARM:
            if (UNKNOWN == m_Alarms) {
              permission = false;
            }
            break;

          case ESG_BASID_HISTINFOSACQ:
            if (UNKNOWN == m_history_info[hist_period]) {
              permission = false;
            }
            break;

          case ESG_BASID_INFOSACQ:
            if (UNKNOWN == m_Infos) {
              permission = false;
            }
            break;

          case ESG_BASID_GENCONTROL:
            // if basic general control in progress, basic diffusion is refused
#if 0
            esg_esg_lis_ConsultEntry (name(),
                                 ESG_ESG_LIS_D_GETDUPLICATEDREQUEST,
                                 &r_ConsultEntry,
                                 r_ReadRequest,
                                 &b_DuplicateFlag,
                                 &r_ResultEntry);

             sprintf ( s_Trace, "STATE_B - Result duplicate GenCtrl : DuplicStatus %d, DuplicateFlag %d", i_DuplicStatus, b_DuplicateFlag);

             if (b_DuplicateFlag == true) {
               permission = false;
             }
#else
            LOG(INFO) << fname << ": CHECK DUPLICATED GENCONTROL REQUEST for state " << EGA_EGA_AUT_D_STATE_NI_NM_NO;
#endif
            break;

          default:
            permission = true;
		} // end switch request ID

      } // end if request type is BASIC
	  else {   // request type is LOCAL
        if (req->id() == ESG_LOCID_INITCOMD) {
          // local init command is refused if the last one was terminated with success and a distant init command is in progress
          if ((OPStateAuthorised() == true) && (InitPhase() != END)) {
            permission = false;
          }
		}
		else {
          permission = true;
 		}
      } // end if request is LOCAL

      break;

    // ----------------------------------------------------------------------------------
    default:
      LOG(WARNING) << fname << ": unsupported state " << current_state << " for " << name();
  }

  LOG(INFO) << fname << ": permission:" << permission;

  return permission;
}

// ==============================================================================
// Проверка допустимости нового Запроса для указанного состояния СС
bool AcqSiteEntry::state_filter(const Request* checked_req)
{
  bool permission = false;

  if ((EGA_GENCONTROL <= checked_req->id()) && (checked_req->id() <= EGA_DELEGATION)) {
    permission = ega_state_filter(checked_req);
  }
  else if ((ESG_BASID_STATECMD <= checked_req->id()) && (checked_req->id() <= ESG_LOCID_EMERGENCY)) {
    permission = esg_state_filter(checked_req, 0); // TODO: задай идентификатор предыстории
  }
  else {
    LOG(ERROR) << name() << ": unsupported request id " << checked_req->id();
  }

  return permission;
}

// ==============================================================================
// Проверка допустимости нового Запроса для указанного состояния СС
bool AcqSiteEntry::ega_state_filter(const Request* checked_req)
{
  static const char* fname = "esg_state_filter";
  // Признак - разрешить или запретить указанный Запрос для СС
  bool permit = false;

  assert(checked_req);
  // GEV TEST
  // Проверить, разрешен ли Запрос для текущего состояния СС
  LOG(INFO) << fname << " enabler[" << checked_req->id() << "," << state() << "]=" << enabler_matrix[checked_req->id()][state()];

  if (checked_req->rclass() == Request::AUTOMATIC) {
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
      // checked_req->b_Requestprocess = TRUE for processed request
      // checked_req->b_Requestprocess = FALSE for SA request
      // e_RequestClass = FALSE for dispatcher request
      // e_RequestClass = TRUE for another request class
      case EGA_EGA_AUT_D_STATE_NI_M_O:      // Dispatcher requests only
        if (checked_req->rclass() != Request::ASYNC) {
          permit = false;
        }
        else {
          permit = true;
        }
        break;

      case EGA_EGA_AUT_D_STATE_NI_NM_NO:
        // Acquisition systems requests only
        // ---------------------------------
        if (checked_req->rprocess() == true) {
          permit = false;
        }
        else {
          if (checked_req->id() != EGA_SERVCMD) {
            permit = true;
          }
        }
        break;

      case EGA_EGA_AUT_D_STATE_NI_M_NO:
        if (checked_req->rclass() == Request::ASYNC) {
          permit = true;
        }
        else {
          permit = false;
        }
        break;
    } // end switch(state)
  } // end if requestclass != AUTOMATIC

  return permit;
}


// ==============================================================================
// Попытаться добавить в очередь указанный Запрос.
// Входной параметр - ссылка на экземпляр Запроса из НСИ.
// Проверить его на комбинированность, и поместить в очередь запросов на исполнение
// только базовые Запросы. 
// NB: Соответствующее значение массива по индексу подзапроса представляет собой
// порядковый номер его включения в очередь, поскольку очередность важна!
int AcqSiteEntry::add_request(const Request* dict_req)
{
  const char* fname = "AcqSiteEntry::add_request";
  // Временный массив для сортировки вложенных подзапросов
  ech_t_ReqId sorted_sequence[NBREQUESTS] = {};
  int rc = OK;
  int num_composed = 0;
  int rid;
  Request *basic_request = NULL;

  if (true == state_filter(dict_req)) {
    // Данный Запрос допустим к исполнению
    LOG(INFO) << fname << ": request " << dict_req->name() << " is enabled to " << name();

    // Если запрос комбинированный, вместо него поместить его вложенные подзапросы
    if (0 == (num_composed = dict_req->composed())) { // Запрос простой

      // Создать на его базе новый экземпляр, и поместить его в список исполняемых Запросов
      basic_request = new Request(dict_req);
      basic_request->generate_exchange_id();
      LOG(INFO) << fname << ": 1/1 " << basic_request->name();

      m_requests_in_progress.push_back(basic_request);
    }
    else { // Запрос составной

      for (int rid = REQUEST_ID_FIRST; rid <= REQUEST_ID_LAST; rid++) { // Поместить в список его подзапросы
        // Запрос с таким ID используется?
        if (dict_req->included()[rid]) { // Да - запомним его ID
          sorted_sequence[dict_req->included()[rid] - 1] = static_cast<ech_t_ReqId>(rid);
        }
      }

      int idx = 0;
      for (rid = REQUEST_ID_FIRST; rid <= REQUEST_ID_LAST; rid++) {

        if (sorted_sequence[rid]) { // Встретилось ненулевое значение - порядковый номер запроса в группе

          // Создадим экземпляр базового для этого составного Запроса
          basic_request = new Request(get_dict_request(static_cast<ech_t_ReqId>(sorted_sequence[rid])));
          LOG(INFO) << fname << ": " << ++idx << "/" << num_composed << " " << basic_request->name();

          // Этот запрос не последний в группе
          basic_request->last_in_bundle(false);
          basic_request->generate_exchange_id();

          m_requests_in_progress.push_back(basic_request);
        }
        else { // прекратить, как только пойдут нулевые значения - неиспользуемые запросы
          // Последний созданный Запрос basic_request будет последним в группе
          if (basic_request) {
            basic_request->last_in_bundle(true);
          }
          break; 
        }
      }

    } // end комбинированный запрос

  } // end state_filter == true
  else {
    LOG(WARNING) << fname << ": request " << dict_req->name() << " is forbidden to " << name();
    rc = NOK;
  }

  return rc;
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
int AcqSiteEntry::push_request_for_cycle(Cycle* cycle, const int* included_requests)
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
  // Разные экземпляры AcqSiteList в Cycle и в EGSA содержат ссылки на одни
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
