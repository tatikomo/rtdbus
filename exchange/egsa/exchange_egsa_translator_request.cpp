#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
//#include <assert.h>
//#include <time.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
//#include "xdb_common.hpp"
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_translator.hpp"

// Обработка Запросов смежных систем
// --------------------------------------------------------------
// NB: Используются коды запросов ГОФО, к примеру вместо ESG_BASID_STATECMD используется ESG_ESG_D_BASID_STATECMD
int ExchangeTranslator::processing_REQUEST(FILE * pi_FileId, size_t i_LgAppl, size_t i_ApplLength, AcqSiteEntry* site)
{
  static const char *fname = "processing_REQUEST";
  static const char* link_states[AcqSiteEntry::INITPHASE_END + 2] = { "NONE", "BEGIN", "END NO HHIST", "END", "UNDEFINED" };
  const char* s_link_state;
  int i_Status = OK;
  char s_Buffer[ECH_D_APPLSEGLG + 1];
  size_t i_NbSegReq = 0;
  size_t i_CDLength = 0;
  size_t i_LgDone = 0;
  size_t i_LEDRead = 0;
  size_t i_IxTi;
  size_t h_HistNbTi;
  struct tm *my_tmDate;
  char s_DateShow[26];
  elemstruct_item_t* r_SubTypeElem;
  bool b_TiFound = false;
  int32_t DataCat = 0;
  int32_t i_InitPhase;
  uint16_t h_ReqId;
  esg_esg_t_HeaderInterChg      r_HeaderInterChg;
  esg_esg_t_HeaderMsg			r_HeaderMsg;
  esg_esg_t_HeaderAppl			r_HeaderAppl;
  esg_esg_edi_t_StrComposedData r_InternalCData;
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
  char s_ExchCompId[ECH_D_COMPIDLG + 1];
  esg_esg_t_TCContents      r_TCContents;   // Addition of TC contents for a distant TC
  esg_esg_t_TRContents      r_TRContents;   // Addition of TR contents for a distant TR
  struct timeval    d_LastAlOp;     // last operationnel alarm
  struct timeval    d_LastNopAl;    // last non oper. alarm
  struct timeval    d_ArchiveDate;  // date since to get historic
  gof_t_PeriodType  e_HistPeriod;   // Historic period
  esg_esg_odm_t_ExchInfoElem r_ExchInfoElem;

  i_NbSegReq += 1;
  fprintf(stdout, "   Segment Type=Request \n");
  i_LgDone = 0;
  i_Status = esg_esg_fil_StringRead(pi_FileId, i_ApplLength, s_Buffer);

  if (i_Status != OK) {
    fprintf(stderr, "Error in reading body of request segment \n");
  }

  // Read and Decode the applicatif segment
  //----------------------------------------------------
  if (i_NbSegReq == 1) {
    if (i_Status == OK) {
      i_Status = esg_ine_man_CDProcessing(s_Buffer,
                                          i_ApplLength - i_LgDone,
                                          &i_CDLength,
                                          &r_InternalCData,
                                          r_SubTypeElem,
                                          &r_QuaCData);
      i_LgDone += i_CDLength;
    }

    // The request message has two different requests segment for historic TI request
    if (i_Status == OK) {

      /* get request id. */
      /* --------------- */
      i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[0].u_val,
                                 r_InternalCData.ar_EDataTable[0].type,
                                 /* &r_Request.Request_Id.r_ExchangedRequest. */ &h_ReqId,
                                 FIELD_TYPE_UINT16);
    }
    else {
      fprintf(stderr, "Error in processing composed data \n");
    }

    if (i_Status == OK) {
      fprintf(stdout,
              "   Request Id=%d (State=%d, GenControl=%d, HistoricTI=%d, HistoricAlarms=%d, DistantTC=%d, DistantTR=%d)\n",
              h_ReqId,
              ESG_ESG_D_BASID_STATECMD,
              ESG_ESG_D_BASID_GENCONTROL,
              ESG_ESG_D_BASID_HHISTINFSACQ,
              ESG_ESG_D_BASID_HISTALARM,
              ESG_ESG_D_BASID_TELECMD,
              ESG_ESG_D_BASID_TELEREGU);

      // get init. phase
      // ---------------
      i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[1].u_val, r_InternalCData.ar_EDataTable[1].type, &i_InitPhase, FIELD_TYPE_INT32);
    }
    else {
      fprintf(stderr, "   Error in getting Request Id \n");
    }

    if (i_Status == OK) {
      fprintf(stdout, "   Init phase=%d \n", i_InitPhase);
    }
    else {
      fprintf(stderr, "   Error in getting Init phase \n");
    }
  }

  switch(h_ReqId) {
    case ESG_ESG_D_BASID_STATECMD:  // Запрос состояния инициализации связи
      if ((AcqSiteEntry::INITPHASE_NONE <= i_InitPhase) && (i_InitPhase <= AcqSiteEntry::INITPHASE_END))
        s_link_state = link_states[i_InitPhase];
      else s_link_state = link_states[AcqSiteEntry::INITPHASE_END + 1];

      LOG(INFO) << fname << ": Сайт \"" << site->name() << "\" сообщает о состоянии связи \"" << s_link_state << "\" с локальным Сайтом";

#if _FUNCTIONAL_TEST
      LOG(INFO) << fname << ": PROCESS ESG_ESG_D_BASID_STATECMD(" << site->name() << "," << i_InitPhase << ") " << s_link_state;
#else
      // TODO: как обработать состояние инициализации удаленного Сайта?
#warning "How to handle distant site initialization state?"
      //i_Status = egsa()->processing_STATE(site, i_SynthState);
#endif
      break;

    case ESG_ESG_D_BASID_GENCONTROL: // general control ---------------
      // Get DataCat = enum INT32
      i_Status = ech_typ_GetAtt(&r_InternalCData.ar_EDataTable[2].u_val,
                                r_InternalCData.ar_EDataTable[2].type,
                                /* &r_Request. */ &DataCat,
                                FIELD_TYPE_INT32);
      if (i_Status == OK) {
        fprintf(stdout, "   Data Category=%d \n", DataCat);
      }
      else {
        fprintf(stdout, "   Error in getting Data Category \n");
      }
      break;

    case ESG_ESG_D_BASID_HISTALARM: // historized alarms -----------------
      // Get last alarm dates
      i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[2].u_val, r_InternalCData.ar_EDataTable[2].type, &d_LastAlOp, FIELD_TYPE_DATE);

      if (i_Status == OK) {
        my_tmDate = localtime(&d_LastAlOp.tv_sec);
        strcpy(s_DateShow, asctime(my_tmDate));
        fprintf(stdout, "   Operational Alarm Date begin= %s", s_DateShow);

        i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[3].u_val, r_InternalCData.ar_EDataTable[3].type, &d_LastNopAl, FIELD_TYPE_DATE);
      }
      else {
        fprintf(stdout, "   Error in getting Operational Alarm Date begin Status=%d \n", i_Status);
      }

      if (i_Status == OK) {
        my_tmDate = localtime(&d_LastNopAl.tv_sec);
        strcpy(s_DateShow, asctime(my_tmDate));
        fprintf(stdout, "   Non Operational Alarm Date begin= %s", s_DateShow);
      }
      else {
        fprintf(stdout, "   Error in getting Non Operational Alarm Date begin Status=%d \n", i_Status);
      }
      break;

    case ESG_ESG_D_BASID_HHISTINFSACQ: // historisation of historized Ti ------------------------------
      // only for first segment
      if (i_NbSegReq == 1) {

        // initialize last TI index treated
        i_IxTi = 0;
        // Get period
        i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[2].u_val, r_InternalCData.ar_EDataTable[2].type, &e_HistPeriod, FIELD_TYPE_INT32);
        if (i_Status == OK) {
          fprintf(stdout, "   Period of historic=%d \n", e_HistPeriod);
          // Get dates
          i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[3].u_val, r_InternalCData.ar_EDataTable[3].type, &d_ArchiveDate, FIELD_TYPE_DATE);
        }
        else {
          fprintf(stderr, "   Error in getting TI historic period Status=%d \n", i_Status);
        }

        if (i_Status == OK) {
          my_tmDate = localtime(&d_ArchiveDate.tv_sec);
          strcpy(s_DateShow, asctime(my_tmDate));
          fprintf(stdout, "   Historic Date begin= %s", s_DateShow);
          i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[4].u_val, r_InternalCData.ar_EDataTable[4].type, &h_HistNbTi, FIELD_TYPE_UINT16);
        }
        else {
          fprintf(stderr, "   Error in getting TI historic begin date Status=%d \n", i_Status);
        }
        if (i_Status == OK) {
          fprintf(stdout, "   TI Count= %d \n", h_HistNbTi);
        }
        else {
          fprintf(stderr, "   Error in getting TI count Status=%d \n", i_Status);
        }
      }

      // The segment count can be greater than 2 when TI List exceeds one segment
      if (i_NbSegReq >= 2) {
        i_LgDone = 0;
        memset(s_ExchCompId, 0, sizeof(s_ExchCompId));
        while ((i_LgDone < i_ApplLength) && (i_Status == OK)) {

          // Read the identifier of the composed data
          strncpy(s_ExchCompId, &s_Buffer[i_LgDone], ECH_D_COMPIDLG);

          // Verify if this application segment contains a Ti identifier list
          if ((strcmp(s_ExchCompId, ECH_D_TIIDLISTDCD)) == 0) {
            memset(&r_InternalCData, 0, sizeof(esg_esg_edi_t_StrQualifyComposedData));
            b_TiFound = false;

            // Conversion of Ti identifier
            //----------------------------------

            // The r_InternalCData structure is not used as the composite data analyzed here only contains a LED
            i_Status = esg_esg_edi_ComposedDataDecoding(s_ExchCompId,
                          &s_Buffer[i_LgDone],
                          i_ApplLength - i_LgDone,
                          &i_CDLength,
                          &r_QuaCData,
                          &r_InternalCData);

            if ((i_Status == OK) && (r_QuaCData.b_QualifyExist == true)) {
              i_LgDone = i_LgDone + i_CDLength;
#if 1
              LOG(INFO) << fname << " : esg_esg_odm_CheckExchInfoLed SITE=" << site->name() << ", LED=" << r_QuaCData.i_QualifyValue;
              b_TiFound = false;
#else
              i_Status = esg_esg_odm_CheckExchInfoLed (s_IAcqSiteId, r_QuaCData.i_QualifyValue, &b_TiFound, &r_ExchInfoElem);
#endif
            }
            else {
              i_Status = ESG_ESG_D_ERR_TILEDFORHISTACQ;
            }
            i_LEDRead = r_QuaCData.i_QualifyValue;
            if ((i_Status == OK) && (b_TiFound == true)) {
              fprintf(stdout, "   TI Index=%d Univ Name=%s TI LED=%d \n", i_IxTi, r_ExchInfoElem.s_Name, i_LEDRead);
            }
            else if (i_Status == OK) {
              fprintf(stderr, "   Error in getting TI Univ Name related to LED=%d for TI of index=%d\n", i_LEDRead, i_IxTi);
            }
            i_IxTi += 1;
          }
          else {
            if (i_LgDone != 0) {
              i_Status = ESG_ESG_D_ERR_INEXPCOMPDATA;
            }
            else {
              i_LgDone = i_ApplLength;
            }
          }
          if (i_Status != OK) {
            fprintf(stderr, "   Error in getting LED for TI of index=%d Status=%d \n", i_IxTi, i_Status);
          }

          fflush(stdout);
          fflush(stderr);
        }                         // end of while loop

      }
      break;

    case ESG_ESG_D_BASID_TELECMD: // DIR TC -----------------------------------
      i_Status = esg_snd_inm_DecodeTCReq(r_InternalCData, &r_TCContents);
      break;

    case ESG_ESG_D_BASID_TELEREGU: // DIR TR -----------------------------------
      i_Status = esg_snd_inm_DecodeTRReq(r_InternalCData, &r_TRContents);
      break;

    default:
      LOG(ERROR) << fname << ": unsupported request id " << h_ReqId;
  }

  return i_Status;
}

// decoding TC request from DIR
int ExchangeTranslator::esg_snd_inm_DecodeTCReq(esg_esg_edi_t_StrComposedData r_InternalCData,  // Input parameters
                                                esg_esg_t_TCContents* pr_OTCContents)           // Output parameters
{
  //----------------------------------------------------------------------------
  static const char *fname = "esg_snd_inm_DecodeTCReq";
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];
  char s_ExchCompId[ECH_D_COMPIDLG + 1];
  int i_RetStatus;              // returned report
  int i_Status;                 // called routines report
  char s_Trace[250 + 1];
  // elementary data index
  size_t i_FieldIx;
  size_t i_LgrStr;

  //............................................................................
  i_RetStatus = OK;
  memset(pr_OTCContents, 0, sizeof(esg_esg_t_TCContents));

  // attribute index is set to 2 as the two first fields (basic req id and init phase) have already been treated in the caller
  i_FieldIx = 2;

  // -- DeCode Equip universal name (with string length)
  // ------------------
  if (r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.ps_String != NULL) {
    i_LgrStr = r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.i_LgString;
    if (sizeof(pr_OTCContents->s_TechnObjId) >= i_LgrStr) {
      memcpy(&pr_OTCContents->s_TechnObjId, r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.ps_String, i_LgrStr);
    }
    else {
      i_RetStatus = ESG_ESG_D_ERR_BADCONFFILE;
      sprintf(s_LoggedText,
                     ", The TC equip name string size %d is not coherent with the struct size %d",
                     i_LgrStr, sizeof(pr_OTCContents->s_TechnObjId));
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_LoggedText;
    }

    // free the string pointer
    free(r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.ps_String);
  }
  else {
    i_RetStatus = ESG_ESG_D_ERR_BADALLOC;
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " on Request Response";
  }

  // -- DeCode Equip Type
  // --------------------
  if (i_RetStatus == OK) {
    i_FieldIx = i_FieldIx + 1;
    i_RetStatus = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[i_FieldIx].u_val,
              r_InternalCData.ar_EDataTable[i_FieldIx].type,
              &pr_OTCContents->e_tc_type, FIELD_TYPE_INT32);
  }

  // -- DeCode TC operation
  // ------------------
  if (i_RetStatus == OK) {
    i_FieldIx = i_FieldIx + 1;
    i_RetStatus = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[i_FieldIx].u_val,
         r_InternalCData.ar_EDataTable[i_FieldIx].type,
         &pr_OTCContents->e_operation, FIELD_TYPE_INT32);
  }                             // End if i_RetStatus

  // -- DeCode Dispatcher name (with string length)
  // ------------------
  if (i_RetStatus == OK) {
    i_FieldIx = i_FieldIx + 1;

    if (r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.ps_String != NULL) {
      i_LgrStr = r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.i_LgString;
      if (sizeof(pr_OTCContents->s_dispatch_name) >= i_LgrStr) {
        memcpy(&pr_OTCContents->s_dispatch_name,
               r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.ps_String,
               i_LgrStr);
      }
      else {
        i_RetStatus = ESG_ESG_D_ERR_BADCONFFILE;
        sprintf(s_LoggedText,
                       ", The TC dispatcher name string size %d is not coherent with the struct size %d",
                       i_LgrStr, sizeof(pr_OTCContents->s_dispatch_name));
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_LoggedText;
      }

      // free the string pointer
      free(r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.ps_String);
    }
    else {
      i_RetStatus = ESG_ESG_D_ERR_BADALLOC;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " on Request Response";
    }

  }                             // End if i_RetStatus

  // -- DeCode TC responsability change flag
  // ------------------
  if (i_RetStatus == OK) {
    i_FieldIx = i_FieldIx + 1;
    i_RetStatus = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[i_FieldIx].u_val,
         r_InternalCData.ar_EDataTable[i_FieldIx].type,
         &pr_OTCContents->b_responsability_change,
         FIELD_TYPE_LOGIC);
  }

  // trace TC contents
  if (i_RetStatus == OK) {
    sprintf(s_Trace, " TC Contents Equip=%s Type=%d Oper=%d Disp=%s Resp=%d",
            pr_OTCContents->s_TechnObjId,
            pr_OTCContents->e_tc_type,
            pr_OTCContents->e_operation, pr_OTCContents->s_dispatch_name, pr_OTCContents->b_responsability_change);
    LOG(INFO) << fname << s_Trace;
  }
  return (i_RetStatus);

}//-END esg_snd_inm_DecodeTCReq ---------------------------------------------

// decoding TR request from DIR
int ExchangeTranslator::esg_snd_inm_DecodeTRReq(// Input parameters
                            esg_esg_edi_t_StrComposedData r_InternalCData,
                            // Output parameters
                            esg_esg_t_TRContents* pr_OTRContents)
{
  //----------------------------------------------------------------------------
  static const char *fname = "esg_snd_inm_DecodeTRReq";
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];
  char s_ExchCompId[ECH_D_COMPIDLG + 1];
  // Composed data DCD identifier
  int i_RetStatus;              // returned report
  int i_Status;                 // called routines report
  char s_Trace[250 + 1];
  // elementary data index
  size_t i_FieldIx;
  size_t i_LgrStr;

  //............................................................................
  i_RetStatus = OK;
  memset(pr_OTRContents, 0, sizeof(esg_esg_t_TRContents));

  // attribute index is set to 2 as the two first fields (basic req id and init phase) have already been treated in the caller
  i_FieldIx = 2;
  // -- DeCode Equip universal name (with string length)
  // ------------------
  if (r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.ps_String != NULL) {
    i_LgrStr = r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.i_LgString;
    if (sizeof(pr_OTRContents->s_TechnObjId) >= i_LgrStr) {
      memcpy(&pr_OTRContents->s_TechnObjId, r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.ps_String, i_LgrStr);
    }
    else {
      i_RetStatus = ESG_ESG_D_ERR_BADCONFFILE;
      sprintf(s_LoggedText,
                     ", The TR equip name string size %d is not coherent with the struct size %d",
                     i_LgrStr, sizeof(pr_OTRContents->s_TechnObjId));
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_LoggedText;
    }

    // free the string pointer
    free(r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.ps_String);
  }
  else {
    i_RetStatus = ESG_ESG_D_ERR_BADALLOC;
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " on Request Response";
  }

  // -- DeCode Teleregulation Value
  // ------------------------------
  if (i_RetStatus == OK) {
    i_FieldIx = i_FieldIx + 1;
    i_RetStatus = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[i_FieldIx].u_val,
         r_InternalCData.ar_EDataTable[i_FieldIx].type,
         &pr_OTRContents->g_value,
         FIELD_TYPE_DOUBLE);
  }

  // -- DeCode Dispatcher name (with string length)
  // ------------------
  if (i_RetStatus == OK) {
    i_FieldIx = i_FieldIx + 1;

    if (r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.ps_String != NULL) {
      i_LgrStr = r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.i_LgString;
      if (sizeof(pr_OTRContents->s_dispatch_name) >= i_LgrStr) {
        memcpy(&pr_OTRContents->s_dispatch_name, r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.ps_String, i_LgrStr);
      }
      else {
        i_RetStatus = ESG_ESG_D_ERR_BADCONFFILE;
        sprintf(s_LoggedText,
                       ", The TR dispatcher name string size %d is not coherent with the struct size %d",
                       i_LgrStr, sizeof(pr_OTRContents->s_dispatch_name));
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_LoggedText;
      }

      // free the string pointer
      free(r_InternalCData.ar_EDataTable[i_FieldIx].u_val.r_Str.ps_String);
    }
    else {
      i_RetStatus = ESG_ESG_D_ERR_BADALLOC;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " on Request Response";
    }

  }   // End if i_RetStatus

  // -- DeCode TR responsability change flag
  // ------------------
  if (i_RetStatus == OK) {
    i_FieldIx = i_FieldIx + 1;
    i_RetStatus = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[i_FieldIx].u_val,
         r_InternalCData.ar_EDataTable[i_FieldIx].type,
         &pr_OTRContents->b_responsability_change,
         FIELD_TYPE_LOGIC);
  }

  if (i_RetStatus == OK) {
    sprintf(s_Trace, " TR Contents Equip=%s Value=%f Disp=%s Resp=%d",
            pr_OTRContents->s_TechnObjId,
            pr_OTRContents->g_value, pr_OTRContents->s_dispatch_name, pr_OTRContents->b_responsability_change);
    LOG(INFO) << fname << s_Trace;
  }

  return (i_RetStatus);

} //-END esg_snd_inm_DecodeTRReq ---------------------------------------------

