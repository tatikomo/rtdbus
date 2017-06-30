//............................................................................
//  OVERVIEW
//   Manages the disposal of Remote Data for EGSA or SIDL :
//     . Convert the received data in the right format
//     . Send the converted data to SIDL or store them into the SMAD according to their type
//     . Manages the answer to a synthetic message
//............................................................................
// MODIFICATIONS
// Send a message to SIDL only if the connection is OP
//............................................................................

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <time.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_translator.hpp"

// Максимальный размер сообщения для RTAP - rtMAXMSG
#define GOF_MSG_D_MAXBODY 4000

//----------------------------------------------------------------------------
//  FUNCTION            esg_acq_dac_TeleinfoAcq
//  FULL MAME
//----------------------------------------------------------------------------
//  ROLE
//  This function convertes all transmitted data in the right form and decide if the converted
//  data are to be stored into the SMAD or to be sent to SIDL.
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  When acquired data must be converted and written into the SMAD or sent to SIDL
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_TeleinfoAcq(
                // Input parameters
                FILE * pi_FileId,
                const int i_LgAppl,
                const int i_IApplLength,
                const gof_t_UniversalName s_IAcqSiteId,
                const struct timeval d_IReceivedDate)
{
  //----------------------------------------------------------------------------
  static const char* fname = "esg_acq_dac_TeleinfoAcq";

  int i_Status;
  int i_FreeStatus;
  int i_RetStatus;
  int i_CDLength;
  int i_LgDone;
  char s_Trace[200 + 1];
  char s_Buffer[ECH_D_APPLSEGLG + 1];
  esg_esg_edi_t_StrComposedData r_InternalCData;
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;

  // --------------------------------------------------------------------------
  // composed data identifier
  char s_ExchCompId[ECH_D_COMPIDLG + 1];
  // subtype structure
  elemstruct_item_t* pr_ExchCompElem = NULL;
  // number of TI Thresholds
  int i_NbTiThres;
  // pointer Multi TI thresholds structure
  sig_t_msg_MultiThresholds *pr_MultiThresholds;
  // pointer on one TI thresholds structure
  gof_t_TiThreshold *pr_TiThreshold;
  // current TI thresholds structure (to copy in SIDL message)
  gof_t_TiThreshold r_TiThreshold;
  // Thresholds message to SIDL
  char s_BuffMsg[GOF_MSG_D_MAXBODY];
  // message size
  int i_MessSize;
  // exchange id
  static int i_ExchId = 0;

  // --------------------------------------------------------------------------
  i_RetStatus = OK;
  i_Status = OK;

  i_NbTiThres = 0;
  i_MessSize = 0;
  memset(s_BuffMsg, 0, sizeof(s_BuffMsg));
  pr_MultiThresholds = (sig_t_msg_MultiThresholds*) &s_BuffMsg;
  pr_TiThreshold = &(pr_MultiThresholds->ar_TiThreshold[0]);

  // Read the rest of the segment
  i_Status = esg_esg_fil_StringRead(pi_FileId, i_IApplLength, s_Buffer);

  // Process each composed data included in the segment
  // --------------------------------------------------------
  if (i_Status == OK) {

    i_CDLength = 0;
  
    smed()->accelerate(true);
    for (i_LgDone = 0; ((i_LgDone < i_IApplLength) && (i_Status == OK)); i_LgDone = i_LgDone + i_CDLength) {

      memset(&r_InternalCData, 0, sizeof(esg_esg_edi_t_StrComposedData));

      // Read and Decode the applicatif segment
      //----------------------------------------------------
      i_Status = esg_ine_man_CDProcessing(&s_Buffer[i_LgDone],
                                          i_IApplLength - i_LgDone,
                                          &i_CDLength,
                                          &r_InternalCData,
                                          pr_ExchCompElem,
                                          &r_QuaCData);

//1      LOG(ERROR) << fname << ": CDProcessing delme: " << ((NULL != pr_ExchCompElem)? pr_ExchCompElem->name : "<NULL>");

      // add into a TI applicative segment
      memset(s_ExchCompId, 0, sizeof(s_ExchCompId));
      strncpy(s_ExchCompId, &s_Buffer[i_LgDone], ECH_D_COMPIDLG);

      // Get the corresponding sub_type name related to the composed data
      //----------------------------------------------------------------------------
      if (NULL != (pr_ExchCompElem = esg_esg_odm_ConsultExchCompArr(s_ExchCompId))) {
#if VERBOSE > 7
        sprintf(s_Trace, "Composed Data=%s, Subtype=%s", s_ExchCompId, pr_ExchCompElem->associate);
        LOG(INFO) << fname << ": " << s_Trace;
#endif

        // test if the current composed data is a threshold one
        // IF The current composed data is a threshold one
        // THEN call threshold decoding function
        // IF no error then memorize the current decoded threshold
        if (strcmp(pr_ExchCompElem->associate, ECH_D_THRESHOLDS_SET) == 0) {
          i_Status = esg_acq_dac_TIThresholdAcq(s_IAcqSiteId, r_InternalCData, r_QuaCData, &r_TiThreshold);
          if (i_Status == OK) {
            i_NbTiThres += 1;
            memcpy(pr_TiThreshold, &r_TiThreshold, sizeof(gof_t_TiThreshold));
            pr_TiThreshold++;
          }
        }
        else {
          // Process the applicatif segment
          //-------------------------------------------
          i_Status = esg_acq_dac_SmdProcessing(s_IAcqSiteId,
                                               &r_InternalCData,
                                               pr_ExchCompElem,
                                               &r_QuaCData,
                                               d_IReceivedDate);
        } // End if: Process applicatif segment
      }

      if (i_Status == OK) {
        i_FreeStatus = esg_ine_man_FreeCompData(&r_InternalCData);
        if (OK != i_FreeStatus) {
          LOG(ERROR) << fname << ": release memory, rc=" << i_FreeStatus;
        }
      }
    }                           // for to process composed data of the segment
    smed()->accelerate(false);
  }                             // Composed data included in the segment

  // If it's a differential message then send the data to SIDL
  // Send directly to SIDL in order to respect the 10s cycle acq.
  //----------------------------------------------------------------------------
  if (i_Status == OK) {
    // Send the TI to SIDL
    // -------------------
    i_Status = esg_acq_dac_SendAcqTIToSidl(s_IAcqSiteId); // Send diff acq data to SIDL
  }

  // End Send differential acquisition to SIDL
  // IF at least one threshold has been found
  // THEN build and send the related message to SIDL
  // END IF
  if (i_NbTiThres > 0) {
    // Send message to SINF
    //----------------------------------------------------------------------------
    i_ExchId += 1;
    pr_MultiThresholds->i_exchange_id = i_ExchId;
    pr_MultiThresholds->h_Cnt = i_NbTiThres;
    // SIDL Message Size Init
    // ----------------------
    i_MessSize = sizeof(sig_t_msg_MultiThresholds) + (i_NbTiThres - 1) * sizeof(gof_t_TiThreshold);
    i_Status = sig_ext_msg_p_InpSendMessageToSinf(SIG_D_MSG_MULTITHRESHOLDS, i_MessSize, (const char*)pr_MultiThresholds);
    sprintf(s_Trace, "Multi-Thres message to SIDX TiCnt=%d ExchId=%d MessSize=%d SendStatus=%d",
            pr_MultiThresholds->h_Cnt,
            pr_MultiThresholds->i_exchange_id,
            i_MessSize,
            i_Status);
    LOG(INFO) << fname << ": " << s_Trace;
  }

  // Global message
  // --------------
  if (i_Status != OK) {
    LOG(ERROR) << fname << ": rc=" << i_Status;
  }

  std::cout << fname << ": END" << std::endl;

  i_RetStatus = i_Status;
  return (i_RetStatus);
}
//-END esg_acq_dac_TeleinfoAcq------------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION                            esg_acq_dac_TIThresholdAcq
//----------------------------------------------------------------------------
//  ROLE
//      Manages the threshold set acquisition :
//         . Reads the data from received file
//         . Convert the received data in the right format
//         . Transmit them to SINF
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_TIThresholdAcq(
                                // Input parameters
                                const gof_t_UniversalName s_IAcqSiteId,
                                const esg_esg_edi_t_StrComposedData r_IInternalCData,
                                const esg_esg_edi_t_StrQualifyComposedData r_IQuaCData,
                                // Output parameters
                                gof_t_TiThreshold * pr_TiThreshold)
{
  //----------------------------------------------------------------------------
  static const char* fname = "esg_acq_dac_TIThresholdAcq";
  char s_Trace[200 + 1];
  int i_Status = OK;
  int i_RetStatus = OK;
  size_t i_IxThres;
  int i_CntThresholds;
  int i_VectorTabNb;
  esg_esg_odm_t_ExchInfoElem r_ExchInfoElem;
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];
  //............................................................................

#if VERBOSE >= 9
  std::cout << fname << ": Begin" << std::endl;
#endif

  memset(pr_TiThreshold, 0, sizeof(gof_t_TiThreshold));
  memset(s_LoggedText, 0, sizeof(s_LoggedText));

  // thresholds have vector structures, therefore they can't be saved no more in SMAD; they are transmitted directly to SINF
  // Get the TI uname
  //----------------------------------------------------------------------------
#if 1
  // TODO: рассмотреть возможность замены явного указания LED на автоматически вычисляемую контрольную сумму от TAG.
  // Это позволит автоматически поддерживать синхронность словарей на разных сторонах информационного обмена.
  // Но при этом нужно отказаться от словаря, в котором указаны разрешенные к передаче удаленной стороне свои данные,
  // отдавая все, что попросит удаленная сторона.
  i_Status = esg_esg_odm_ConsExchInfoLed(s_IAcqSiteId, r_IQuaCData.i_QualifyValue, &r_ExchInfoElem);
  if (OK != i_Status) {
    LOG(WARNING) << fname << ": rc=" << i_Status << ", skip LED #" << r_IQuaCData.i_QualifyValue << " " << s_IAcqSiteId;
    i_Status = OK;
  }
#else
  LOG(ERROR) << fname << ": get the TAG by LED=" << r_IQuaCData.i_QualifyValue;
#endif

  //----------------------------------------------------------------------------
  // Prepare message to SINF
  //----------------------------------------------------------------------------
  strcpy(pr_TiThreshold->s_uni_name, r_ExchInfoElem.s_Name);
  i_IxThres = 0;
  i_CntThresholds = 0;
  i_VectorTabNb = 0;

  while ((i_IxThres < r_IInternalCData.i_NbEData) && (i_Status == OK)) {

    switch(i_VectorTabNb) {
      case 0:
        // put set of thresholds values
        //----------------------------------------------------------------------------
        if (i_CntThresholds < GOF_D_BDR_CNT_THRESHOLDS) {
          if (r_IInternalCData.ar_EDataTable[i_IxThres].type == FIELD_TYPE_DOUBLE) {
            pr_TiThreshold->r_MsgThresholds.ag_value[i_CntThresholds] = r_IInternalCData.ar_EDataTable[i_IxThres].u_val.g_Double;
          }
          else {
            i_Status = ESG_ESG_D_ERR_BADEDINTERTYPE;
            sprintf(s_LoggedText,
                    "%s, sinf msg : type %d, subtype %d",
                    pr_TiThreshold->s_uni_name, FIELD_TYPE_DOUBLE, r_IInternalCData.ar_EDataTable[i_IxThres].type);
          }
          i_CntThresholds++;
        }
        else {
          i_CntThresholds = 0;
          i_VectorTabNb++;
        }
        break;

      case 1:
        // put set of degrees values
        //----------------------------------------------------------------------------
        if (i_CntThresholds < GOF_D_BDR_CNT_THRESHOLDS) {
          if (r_IInternalCData.ar_EDataTable[i_IxThres].type == FIELD_TYPE_UINT8) {
            pr_TiThreshold->r_MsgThresholds.ao_degree[i_CntThresholds] = r_IInternalCData.ar_EDataTable[i_IxThres].u_val.o_Uint8;
          }
          else {
            i_Status = ESG_ESG_D_ERR_BADEDINTERTYPE;
            sprintf(s_LoggedText,
                    "%s, sinf msg : type %d, subtype %d",
                    pr_TiThreshold->s_uni_name, FIELD_TYPE_DOUBLE, r_IInternalCData.ar_EDataTable[i_IxThres].type);
          }
          i_CntThresholds++;
        }
        else {
          i_CntThresholds = 0;
          i_VectorTabNb++;
        }
        break;

      case 2:
        // put set of categories values
        //----------------------------------------------------------------------------
        if (i_CntThresholds < GOF_D_BDR_CNT_THRESHOLDS) {
          if (r_IInternalCData.ar_EDataTable[i_IxThres].type == FIELD_TYPE_UINT8) {
            pr_TiThreshold->r_MsgThresholds.ao_categ[i_CntThresholds] = r_IInternalCData.ar_EDataTable[i_IxThres].u_val.o_Uint8;
          }
          else {
            i_Status = ESG_ESG_D_ERR_BADEDINTERTYPE;
            sprintf(s_LoggedText,
                    "%s, sinf msg : type %d, subtype %d",
                    pr_TiThreshold->s_uni_name, FIELD_TYPE_DOUBLE, r_IInternalCData.ar_EDataTable[i_IxThres].type);
          }
          i_CntThresholds += 1;
        }
        else {
          i_CntThresholds = 0;
          i_VectorTabNb += 1;
        }
        break;
    }

    i_IxThres += 1;
  }

  // Trace threshold contents
  if (i_Status == OK) {
    sprintf(s_Trace, "Thresholds for TI=%s", pr_TiThreshold->s_uni_name);
    std::cout << s_Trace << std::endl;
    sprintf(s_Trace, "   Thr Val VH=%g H=%g L=%g VL=%g Grad=%g",
            pr_TiThreshold->r_MsgThresholds.ag_value[SIG_DBA_D_IND_VH],
            pr_TiThreshold->r_MsgThresholds.ag_value[SIG_DBA_D_IND_H],
            pr_TiThreshold->r_MsgThresholds.ag_value[SIG_DBA_D_IND_L],
            pr_TiThreshold->r_MsgThresholds.ag_value[SIG_DBA_D_IND_VL],
            pr_TiThreshold->r_MsgThresholds.ag_value[SIG_DBA_D_IND_G]);
    std::cout << s_Trace << std::endl;
    sprintf(s_Trace, "   Thr Deg VH=%d H=%d L=%d VL=%d Grad=%d",
            pr_TiThreshold->r_MsgThresholds.ao_degree[SIG_DBA_D_IND_VH],
            pr_TiThreshold->r_MsgThresholds.ao_degree[SIG_DBA_D_IND_H],
            pr_TiThreshold->r_MsgThresholds.ao_degree[SIG_DBA_D_IND_L],
            pr_TiThreshold->r_MsgThresholds.ao_degree[SIG_DBA_D_IND_VL],
            pr_TiThreshold->r_MsgThresholds.ao_degree[SIG_DBA_D_IND_G]);
    std::cout << s_Trace << std::endl;
    sprintf(s_Trace, "   Thr Categ VH=%d H=%d L=%d VL=%d Grad=%d",
            pr_TiThreshold->r_MsgThresholds.ao_categ[SIG_DBA_D_IND_VH],
            pr_TiThreshold->r_MsgThresholds.ao_categ[SIG_DBA_D_IND_H],
            pr_TiThreshold->r_MsgThresholds.ao_categ[SIG_DBA_D_IND_L],
            pr_TiThreshold->r_MsgThresholds.ao_categ[SIG_DBA_D_IND_VL],
            pr_TiThreshold->r_MsgThresholds.ao_categ[SIG_DBA_D_IND_G]);
    std::cout << s_Trace << std::endl << std::endl;
  }

  // Global message
  // --------------
  if (i_Status != OK) {
    LOG(ERROR) << fname << ": " << s_LoggedText;
  }

#if VERBOSE >= 9
  std::cout << fname << ": End Status=" << i_Status << std::endl;
#endif

  i_RetStatus = i_Status;
  return (i_RetStatus);
}
//-END esg_acq_dac_TIThresholdAcq---------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION            esg_acq_dac_DispNameAcq
//----------------------------------------------------------------------------
//  ROLE
//  This function converts the received data in the right form and decide if the converted data are to be transmitted to SIDX
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_DispNameAcq(
                             // Input parameters
                             FILE * pi_FileId,
                             const int i_LgAppl,
                             const int i_IApplLength,
                             const gof_t_UniversalName s_IAcqSiteId,    // sender site
                             const struct timeval d_IReceivedDate)
{
  //----------------------------------------------------------------------------
  static const char* fname = "esg_acq_dac_DispNameAcq";
  int i_Status = OK;
  int i_FreeStatus;
  int i_RetStatus = OK;
  int i_CDLength = 0;
  int i_LgDone;
  char s_Trace[200 + 1];
  char s_Buffer[ECH_D_APPLSEGLG + 1];
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];
  esg_esg_edi_t_StrComposedData r_InternalCData;
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
  // dispatcher name
  char s_DispatchName[60 + 1]; // NB: rtap_db.mco defines s_dispatch as char[60]
  size_t i_LgrStr;
  //1 esg_esg_odm_t_AcqSiteEntry r_AcqSite;
  AcqSiteEntry* r_AcqSite = NULL;
  // composed data identifier
  char s_ExchCompId[ECH_D_COMPIDLG + 1];
  // subtype structure
  elemstruct_item_t* pr_ExchCompElem = NULL;
  // dispatcher name message to SIDL
  char s_BuffMsg[GOF_MSG_D_MAXBODY];
  // message size
//  int i_MessSize = 0;
  // exchange id
//  static int i_ExchId = 0;
  // --------------------------------------------------------------------------

  memset(s_BuffMsg, 0, sizeof(s_BuffMsg));

  // Read the rest of the segment
  //-------------------------------------
  i_Status = esg_esg_fil_StringRead(pi_FileId, i_IApplLength, s_Buffer);

  // Process each composed data included in the segment
  // --------------------------------------------------------
  if (i_Status == OK) {

    for (i_LgDone = 0; ((i_LgDone < i_IApplLength) && (i_Status == OK)); i_LgDone = i_LgDone + i_CDLength) {
      memset(&r_InternalCData, 0, sizeof(esg_esg_edi_t_StrComposedData));

      // Read and Decode the applicatif segment
      //----------------------------------------------------
      i_Status = esg_ine_man_CDProcessing(&s_Buffer[i_LgDone],
                                          i_IApplLength - i_LgDone,
                                          &i_CDLength,
                                          &r_InternalCData,
                                          pr_ExchCompElem,
                                          &r_QuaCData);

      LOG(ERROR) << fname << ": CDProcessing delme: " << ((NULL != pr_ExchCompElem)? pr_ExchCompElem->name : "<NULL>");

      memset(s_ExchCompId, 0, sizeof(s_ExchCompId));
      strncpy(s_ExchCompId, &s_Buffer[i_LgDone], ECH_D_COMPIDLG);

      // Get the corresponding sub_type name related to the composed data
      //----------------------------------------------------------------------------
      if (NULL != (pr_ExchCompElem = esg_esg_odm_ConsultExchCompArr(s_ExchCompId))) {
        sprintf(s_Trace, "Composed Data=%s, Subtype=%s", s_ExchCompId, pr_ExchCompElem->associate);
        LOG(INFO) << fname << ": " << s_Trace;

        // test if the current composed data is a dispatcher name one
        // IF The current composed data is a dispatcher name one
        // THEN decode it

        // IF no error then memorize the current decoded dispatcher name
        if (strcmp(pr_ExchCompElem->associate, ECH_D_DISP_NAME) == 0) {
          if (r_InternalCData.ar_EDataTable[0].u_val.r_Str.ps_String != NULL) {
            i_LgrStr = r_InternalCData.ar_EDataTable[0].u_val.r_Str.i_LgString;
            if (sizeof(s_DispatchName) >= i_LgrStr) {
              memcpy(&s_DispatchName, r_InternalCData.ar_EDataTable[0].u_val.r_Str.ps_String, i_LgrStr);
              sprintf(s_Trace, "Dispatcher Name=%s, Sender SA=%s", s_DispatchName, s_IAcqSiteId);
              LOG(INFO) << fname << ": " << s_Trace;

              // Get the Acquisition site table (and the dispatcher name of sending site)
              r_AcqSite = esg_esg_odm_ConsultAcqSiteEntry(s_IAcqSiteId);

              if (NULL == (r_AcqSite = esg_esg_odm_ConsultAcqSiteEntry(s_IAcqSiteId))) {
                i_RetStatus = ESG_ESG_D_ERR_NOACQSITE;
                LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", unknown site " << s_IAcqSiteId;
              }

              // compare the last sent dispatcher name and the read one
              // IF the two names differ
              // THEN send the message to SIDX with the new name
              //      memorize the new received name
              // ELSE do not send the message
              // END IF
              if (i_RetStatus == OK) {
                sprintf(s_Trace, " Old Disp Name=%s, New Disp Name=%s", r_AcqSite->DispatchName(), s_DispatchName);
                LOG(INFO) << fname << ": " << s_Trace;
                if (strcmp(r_AcqSite->DispatchName(), s_DispatchName) != 0) {
                  i_RetStatus = esg_acq_inm_SendDispNameToSidl(s_IAcqSiteId, s_DispatchName);
                  r_AcqSite->DispatchName(s_DispatchName);
                }
              }
            }
            else {
              i_RetStatus = ESG_ESG_D_ERR_BADCONFFILE;
              sprintf(s_LoggedText, "The dispatcher name string size %d is not coherent with the struct size %d",
                      i_LgrStr, sizeof(s_DispatchName));
              LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", " << s_LoggedText;
            }
          }
          else {
            i_RetStatus = ESG_ESG_D_ERR_BADALLOC;
            LOG(ERROR) << fname << ": rc=" << i_RetStatus << " on Request Response";
          }

        }                       // End if: Process applicatif segment
      }

      // free strings in internal composed data structure allocated by esg_ine_man_CDProcessing
      if (i_Status == OK) {
        i_FreeStatus = esg_ine_man_FreeCompData(&r_InternalCData);
        if (OK != i_FreeStatus) {
          LOG(ERROR) << fname << ": release memory, rc=" << i_FreeStatus;
        }
      }

    }                           // for to process composed data of the segment

  }                             // Composed data included in the segment

  // Global message
  // --------------
  if (i_Status != OK) {
    LOG(ERROR) << fname << ": rc=" << i_Status;
  }

  std::cout << fname << ": End" << std::endl;

  i_RetStatus = i_Status;
  return (i_RetStatus);
}
//-END esg_acq_dac_DispNameAcq -----------------------------------------------

typedef struct  {
  gof_t_ExchangeId      i_exchange_id;   // exchange identifier
  gof_t_UniversalName   s_dipl_un;       // object name
  char s_dispatch_name[60]; // dispatcher name
} sig_t_msg_DispName ;

// Sends the dispatcher name message to SIDX each time it has received it in a TI message form distant site
int ExchangeTranslator::esg_acq_inm_SendDispNameToSidl(
                // Input parameters
                const gof_t_UniversalName s_ISAName,    // name of the distant SA site that has send its dispatcher name
                const char* s_IDispatchName)    // dispatcher name
{
  //----------------------------------------------------------------------------
  static const char* fname = "esg_acq_inm_SendDispNameToSidl";
  char s_Trace[200 + 1];        // printed trace
  // returned status
  int i_Status = OK;
  int i_MsgSize;                // message size
  // message structure sent to SIDX with dispatcher and site names
  sig_t_msg_DispName r_DispName;
  //............................................................................

#if VERBOSE >= 9
  std::cout << fname << ": Begin" << std::endl;
#endif

  // The /SAK must be transformed into /K
  r_DispName.s_dipl_un[0] = '/';
  strcpy(&r_DispName.s_dipl_un[1], &s_ISAName[3]);
  strcpy(r_DispName.s_dispatch_name, s_IDispatchName);
  i_MsgSize = sizeof(sig_t_msg_DispName);
  sprintf(s_Trace, "Disp name message to SIDX - Dispatcher Name=%s Site=%s MsgSize=%d",
          r_DispName.s_dispatch_name, r_DispName.s_dipl_un, i_MsgSize);
  LOG(INFO) << fname << ": " << s_Trace;

  i_Status = sig_ext_msg_p_InpSendMessageToSinf(SIG_D_MSG_DISPNAME, i_MsgSize, (const char*)&r_DispName);
  return (i_Status);

}//-END esg_acq_inm_SendDispNameToSidl ---------------------------------------

//----------------------------------------------------------------------------
// Sends to SIDX the data from the acquisition of a distant site
//----------------------------------------------------------------------------
int ExchangeTranslator::sig_ext_msg_p_InpSendMessageToSinf(const rtdbMsgType msgId, int i_MsgSize, const char*)
{
  static const char* fname = "sig_ext_msg_p_InpSendMessageToSinf";
  int rc = OK;

  LOG(INFO) << fname << ": Sends to SIDX the data (" << i_MsgSize << " bytes) as msg #" << msgId << " from the acquisition of a distant site";

  return rc;
}

//----------------------------------------------------------------------------
// Sends to SIDX the data from the acquisition of a distant site
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_SendAcqTIToSidl(
                // Input parameters
                const gof_t_UniversalName s_IAcqSiteId) // name of the distant SA site that has send its dispatcher name
{
  //----------------------------------------------------------------------------
  static const char* fname = "esg_acq_dac_SendAcqTIToSidl";
  int i_Status = OK;
#if 0
  char s_Trace[200 + 1];
  // returned status
  // complementary error text
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1] = "";
//1  esg_esg_odm_t_AcqSiteEntry r_AcqSite;
  AcqSiteEntry* r_AcqSite = NULL;
  int i_MsgSize;                // message size
//1  char s_DataToSidl[GOF_MSG_D_MAXBODY];
//1  gof_t_AcqMsgHeader *p_HeaderToSidl;
  int i_RecordLen;              // Length of the record read
  bool b_End;
  // indication of smad record found or not to send to SIDL
  bool b_RecordFound;
  // buffer received with ech_smd_p_ReadRecForSidl call
  char s_RecBody[GOF_MSG_D_MAXBODY];
 //1 gof_t_StructAtt r_RecAttr;    // Attribut of a smad record
  bool b_RecAttrChanged;       // Changed indicator of smad record
  uint32_t i_OSynthStateValue;  // new value
#endif
  //............................................................................

#if VERBOSE >= 9
  std::cout << fname << ": Begin" << std::endl;
#endif

#if 1
  LOG(ERROR) << fname << ": Sends to SIDX the data from the acquisition of a distant site " << s_IAcqSiteId;
#else

  // Get the Acquisition site table
  // ------------------------------
  r_AcqSite = m_egsa_instance->sites()[s_IAcqSiteId];

  if (!r_AcqSite) {
    i_Status = NOK;
    LOG(ERROR) << fname << ": consult " << s_IAcqSiteId;
  }
  else {
    // Need not lock the SMED, because only es_acq is writing in it
    // ------------------------------------------------------------
    //   Read synthetic state in the smad
    // ------------------------------- **
    i_Status = ech_smd_p_ReadRecAttr(r_AcqSite.p_DataMemory,
                                     s_IAcqSiteId,
                                     GOF_D_BDR_ATT_SYNTHSTATE,
                                     &r_RecAttr,
                                     &b_RecAttrChanged);

    if (i_Status != OK) {
      sprintf(s_LoggedText, "Site : %s, fail in reading SYNTHSTATE", s_IAcqSiteId);
      LOG(ERROR) << fname << ": " << s_LoggedText << ", rc=" << i_Status;
    }
    else {
      i_OSynthStateValue = r_RecAttr.val.val_Uint32;

      sprintf(s_Trace, "Site : %s, new SYNTHSTATE=%d was %s changed",
              s_IAcqSiteId, i_OSynthStateValue, (b_RecAttrChanged == 0) ? "not" : "");
      LOG(INFO) << fname << ": " << s_Trace;
    }

    p_HeaderToSidl = (gof_t_AcqMsgHeader *) s_DataToSidl;
    memset(p_HeaderToSidl, 0, sizeof(gof_t_AcqMsgHeader));
    p_HeaderToSidl->b_database_update = true;

    i_MsgSize = sizeof(gof_t_AcqMsgHeader);

    // Add test on synthetic state value to send an ACQUIRED message to SIDL
    // ------------------------------------------------
    if ((i_Status == OK) && (i_OSynthStateValue == GOF_D_SYNTHSTATE_OP)) {
      b_End = false;
    }
    else {
      b_End = true;
    }

    while (!b_End) {

      // Read the next record in the smad
      // --------------------------------
      i_Status = ech_smd_p_ReadRecForSidl(r_AcqSite.p_DataMemory, &b_RecordFound, &i_RecordLen, s_RecBody);

      if (i_Status == OK) {

        if (b_RecordFound == true) {

          // Need not test the length of RecordLen, because everywhere the buffer size is GOF_MSG_D_MAXBODY.
          // --------------------------------------------------------------
          if (i_MsgSize > (GOF_MSG_D_MAXBODY - i_RecordLen)) {

            // The buffer is full => Send it to SIDL
            // -------------------------------------
            i_Status = sig_ext_msg_p_InpSendMessageToSinf(SIG_D_MSG_ACQUIREDDATA,       // Type of the message
                                                          i_MsgSize,        // Size of the body
                                                          s_DataToSidl);    // Ptr to the msg body

            if (i_Status != OK) {
              sprintf(s_LoggedText, "Site: %s, Error to send TI to SIDL (MsgSize=%d)", s_IAcqSiteId, i_MsgSize);
              LOG(ERROR) << fname << ": " << s_LoggedText;
            }

            // Reset the parameters for the next message to send
            // -------------------------------------------------
            p_HeaderToSidl->h_nb_objects = 0;
            i_MsgSize = sizeof(gof_t_AcqMsgHeader);
          }

          // Copy the record in the buffer to send to SIDL
          // ---------------------------------------------
          p_HeaderToSidl->h_nb_objects++;
          memcpy(s_DataToSidl + i_MsgSize, s_RecBody, i_RecordLen);
          i_MsgSize += i_RecordLen;
        }
      }

      //    Computed the end of loop flag

      // -----------------------------
      if ((b_RecordFound == false) || (i_Status != OK)) {
        b_End = true;
      }
    } // Fin while

    if (i_MsgSize > sizeof(gof_t_AcqMsgHeader)) {
      // Send the end of the buffer
      // --------------------------
      i_Status = sig_ext_msg_p_InpSendMessageToSinf(SIG_D_MSG_ACQUIREDDATA,     // Type of the message
                                                    i_MsgSize,  // Size of the body
                                                    s_DataToSidl);        // Ptr to the msg body

      if (i_Status != OK) {
        sprintf(s_LoggedText, "Site: %s, Error to send the end of TI to SIDL (MsgSize=%d)", s_IAcqSiteId, i_MsgSize);
        LOG(ERROR) << fname << ": " << s_LoggedText;
      }
    }
  }
#endif

  return (i_Status);
} //- END esg_acq_dac_SendAcqTIToSidl ----------------------------------------------



