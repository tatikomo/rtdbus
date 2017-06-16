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
#include "exchange_smed.hpp"

//............................................................................
//  CONTENTS
//  esg_acq_dac_HHistTiAcq
//      esg_acq_dac_HHTISndMsg
//      esg_acq_dac_HHTITIProcess
//      esg_acq_dac_HHTIValProcess
//      esg_acq_dac_InitInternalBis
//............................................................................

#define MAXHOSTNAMELEN  50

#define ESG_ESG_ODM_D_HIST_QH       1  // Quarter of hour historization
#define ESG_ESG_ODM_D_HIST_HOUR     2  // Hourly historization
#define ESG_ESG_ODM_D_HIST_DAY      4  // Dayly historization
#define ESG_ESG_ODM_D_HIST_MONTH    8  // Monthly historization
//
// Header of the historical acquisition message
// --------------------------------------------
typedef struct {
  struct timeval    d_Time;
  uint16_t          h_NbInfosHist;
  gof_t_UniversalName   s_Info[1];
} gof_t_HistUpdReq;

// --------------------------------------------
static int esg_acq_dac_HHTISndMsg(
    const gof_t_UniversalName,
    const gof_t_PeriodType,
    const char*);

// --------------------------------------------
static int esg_acq_dac_HHTITIProcess(
    const gof_t_UniversalName,
    const gof_t_UniversalName,
    const esg_esg_edi_t_StrComposedData*,
    const elemstruct_item_t*,
    FILE*,
    int*,
    // the number of treated group of data is output parameter
    int*,
    // the allocated pointers are input/output parameters
    esg_acq_dac_t_HhistTIDate**,
    esg_acq_dac_t_HhistTIValue**,
    esg_acq_dac_t_HhistTIValid**);

// --------------------------------------------
static int esg_acq_dac_HHTIValProcess(
    const esg_esg_edi_t_StrComposedData*,
    const elemstruct_item_t*,
    esg_acq_dac_t_HhistTIDate*,
    esg_acq_dac_t_HhistTIValue*,
    esg_acq_dac_t_HhistTIValid*);

// --------------------------------------------
static int esg_acq_dac_InitInternalBis(esg_esg_edi_t_StrComposedData * pr_OInternalCDataBis);

//----------------------------------------------------------------------------
//  BODIES OF SUB-PROGRAMS
//----------------------------------------------------------------------------

//============================================================================

//----------------------------------------------------------------------------
//  FUNCTION               esg_acq_dac_HHTISndMsg
//  FULL MAME
//----------------------------------------------------------------------------
//  ROLE
//  This function indicates the name of the historical file to SIDR
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_HHTISndMsg(
                           // Input parameters
                           const gof_t_UniversalName s_IDIPL,
                           const gof_t_PeriodType e_IHistPeriod,
                           const char* s_IHistFileName)
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_acq_dac_HHTISndMsg";
  int i_Status = OK;
//............................................................................
#if 1
  LOG(ERROR) << fname << ": inform SIDR about Hist Hist TI (" << s_IDIPL << ", " << e_IHistPeriod << ", " << s_IHistFileName << ")";
  sig_ext_msg_p_InpSendMessageToSinf(SIG_D_MSG_ACQHISTFILE, 0, NULL);
#else
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];
  char s_Trace[120 + 1];
  char s_HostName[MAXHOSTNAMELEN];
//1  sig_t_msg_AcqHistFile r_AcqHistFile;

  strcpy(r_AcqHistFile.s_DIPL, s_IDIPL);

  r_AcqHistFile.e_Hist_period = e_IHistPeriod;

  // Get the station_name
  //---------------------------
  i_Status = gethostname(s_HostName, MAXHOSTNAMELEN);

  if (i_Status != OK) {
    LOG(ERROR) << fname << ": rc=" << i_Status << ", Sending the historical file name";
  }
  else {
    strcpy(r_AcqHistFile.r_HistFileName.s_station_name, s_HostName);
    strcpy(r_AcqHistFile.r_HistFileName.s_pathname, s_IHistFileName);

    // Send Message to SIDR
    //-----------------------------------------------------------------------
    i_Status = sig_ext_msg_p_InpSendMessageToSinf(SIG_D_MSG_ACQHISTFILE,
                                                  sizeof(r_AcqHistFile),
                                                  (void*)&r_AcqHistFile);

    if (i_Status != OK) {
      LOG(ERROR) << fname << ": rc=" << i_Status << ", Sending the historical file name";
    }
  }
#endif

  return (i_Status);
}
//-END esg_acq_dac_HHTISndMsg ------------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION               esg_acq_dac_HHTITIProcess
//  FULL MAME
//----------------------------------------------------------------------------
//  ROLE
//  This function allows to write the values of the TI into a file for SIDR.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_HHTITIProcess(
                              // Input parameters
                              const gof_t_UniversalName s_IAcqSiteId,
                              const gof_t_UniversalName s_ITIName,
                              const esg_esg_edi_t_StrComposedData* pr_IInternalCData,
                              const elemstruct_item_t* pr_ISubTypeElem,
                              FILE* pi_IFileIdRes, // identification of SIDL result file
                              // Input/Output parameters
                              // INPUT = number of groups of data of the previous TI
                              // OUTPUT = number of groups of data of the current TI
                              int* pi_IONbEch,
                              // Output       parameters
                              // the number of treated groups of data is output parameter
                              // This parameter is always set to 0 as the TI is a new one to treat
                              int* pi_OEchDone,
                              // Input/Output parameters
                              // the allocated pointers are input/output parameters
                              esg_acq_dac_t_HhistTIDate** pr_IOHhistTIDate,
                              esg_acq_dac_t_HhistTIValue** pr_IOHhistTIValue,
                              esg_acq_dac_t_HhistTIValid** pr_IOHhistTIValid)
{
  //----------------------------------------------------------------------------
  static const char* fname = "esg_acq_dac_HHTITIProcess";
  //char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];
  char s_Trace[120 + 1];
  int i_Status = OK;
  esg_acq_dac_t_HhistTIBody r_HhistTIBody;
  // internal allocated pointers
  esg_acq_dac_t_HhistTIDate *pr_HhistTIDate;
  esg_acq_dac_t_HhistTIValue *pr_HhistTIValue;
  esg_acq_dac_t_HhistTIValid *pr_HhistTIValid;
  // index of TI value group for trace
  int i_IxEch;
  // internal validity on a simple field used to avoid the holes in the esg_acq_dac_t_HhistTIValid structure
  uint16_t h_HhistTIValid;
  //............................................................................

  // Write the values of the previous TI in the file
  //------------------------------------------------------
  if (*pi_IONbEch != 0) {
    // Write values
    //-------------------
    // write historised dates
    i_Status = esg_esg_fil_DataWrite(pi_IFileIdRes, *pr_IOHhistTIDate, sizeof(esg_acq_dac_t_HhistTIDate), *pi_IONbEch);

    // addition of write of historised values and validities
    if (i_Status == OK) {
      i_Status = esg_esg_fil_DataWrite(pi_IFileIdRes,
                                       *pr_IOHhistTIValue,
                                       sizeof(esg_acq_dac_t_HhistTIValue),
                                       *pi_IONbEch);
    }

    // treatment of conversion from structure validity to simple integer fields to avoid holes
    for (i_IxEch = 0; i_IxEch < *pi_IONbEch; i_IxEch++) {
      h_HhistTIValid = (*pr_IOHhistTIValid + i_IxEch)->ah_Valid;
      if (i_Status == OK) {
        i_Status = esg_esg_fil_DataWrite(pi_IFileIdRes, &h_HhistTIValid, sizeof(uint16_t), 1);
        if (i_Status == OK) {

          sprintf(s_Trace, "SIDR Written File TI Value Group : IxEch=%d Date=%ld Value=%g Validity=%d ",
                  i_IxEch,
                  (*pr_IOHhistTIDate + i_IxEch)->ad_Date.tv_sec,
                  (*pr_IOHhistTIValue + i_IxEch)->g_Value,
                  h_HhistTIValid);

          std::cout << fname << ": " << s_Trace << std::endl;
        }
      }
    }

    // Free the allocated memory
    //--------------------------------
    //   reset of allocated output pointers
    free(*pr_IOHhistTIDate);
    free(*pr_IOHhistTIValue);
    free(*pr_IOHhistTIValid);
  }

  // Process the body part of the new TI
  //------------------------------------------
  if (i_Status == OK) {
    // TI universal name
    // initialize the TI body with spaces
    memset(&r_HhistTIBody, 0, sizeof(esg_acq_dac_t_HhistTIBody));

    strcpy(r_HhistTIBody.s_Info, s_ITIName);

    // status
    i_Status = ech_typ_GetAtt(&pr_IInternalCData->ar_EDataTable[1].u_val,
                              pr_IInternalCData->ar_EDataTable[1].type,
                              &r_HhistTIBody.ai_Status,
                              pr_ISubTypeElem->fields[1].type);
  }

  if (i_Status == OK) {
    // NbEch : number of values for the TI
    i_Status = ech_typ_GetAtt(&pr_IInternalCData->ar_EDataTable[2].u_val,
                              pr_IInternalCData->ar_EDataTable[2].type,
                              &r_HhistTIBody.h_NbEch,
                              pr_ISubTypeElem->fields[2].type);
  }

  if (i_Status == OK) {
    // type of historised values
    i_Status = ech_typ_GetAtt(&pr_IInternalCData->ar_EDataTable[3].u_val,
                              pr_IInternalCData->ar_EDataTable[3].type,
                              &r_HhistTIBody.ai_Type,
                              pr_ISubTypeElem->fields[3].type);
  }

  // Write the body part of the TI
  //------------------------------------
  if (i_Status == OK) {

    // do not use a structure because of holes. write each field separately
    i_Status = esg_esg_fil_DataWrite(pi_IFileIdRes, &r_HhistTIBody.s_Info,   sizeof(gof_t_UniversalName), 1); // char[32]
    i_Status = esg_esg_fil_DataWrite(pi_IFileIdRes, &r_HhistTIBody.ai_Status,sizeof(long), 1);      // gof_t_Status
    i_Status = esg_esg_fil_DataWrite(pi_IFileIdRes, &r_HhistTIBody.h_NbEch,  sizeof(uint32_t), 1);  // gof_t_Uint32
    i_Status = esg_esg_fil_DataWrite(pi_IFileIdRes, &r_HhistTIBody.ai_Type,  sizeof(uint8_t), 1);   // gof_t_Uint8
  }

  // Prepare the value part of the TI
  //--------------------------------------------
  if (i_Status == OK) {

    sprintf(s_Trace, "SIDR Written File TI Body : TIName=%s SIDR Status=%ld NbEch=%d Value Type=%d ",
            r_HhistTIBody.s_Info, r_HhistTIBody.ai_Status, r_HhistTIBody.h_NbEch, r_HhistTIBody.ai_Type);
    std::cout << fname << ": " << s_Trace << std::endl;

    // Save the number of historical samples for the TI
    //-------------------------------------------------------
    i_Status = ech_typ_GetAtt(&pr_IInternalCData->ar_EDataTable[2].u_val,
                              pr_IInternalCData->ar_EDataTable[2].type,
                              pi_IONbEch,
                              pr_ISubTypeElem->fields[2].type);

    *pi_OEchDone = 0;

    if (*pi_IONbEch != 0) {

      // reserve i_NbEch records for each value type
      //-------------------------------------------------
      // use of intermediate variables
      pr_HhistTIDate     = (esg_acq_dac_t_HhistTIDate *)  calloc(*pi_IONbEch, sizeof(esg_acq_dac_t_HhistTIDate));
      pr_HhistTIValue    = (esg_acq_dac_t_HhistTIValue *) calloc(*pi_IONbEch, sizeof(esg_acq_dac_t_HhistTIValue));
      pr_HhistTIValid    = (esg_acq_dac_t_HhistTIValid *) calloc(*pi_IONbEch, sizeof(esg_acq_dac_t_HhistTIValid));
      *pr_IOHhistTIDate  = pr_HhistTIDate;
      *pr_IOHhistTIValue = pr_HhistTIValue;
      *pr_IOHhistTIValid = pr_HhistTIValid;

      // Verify the allocated memory
      //----------------------------------
      if (*pr_IOHhistTIDate == NULL || *pr_IOHhistTIValue == NULL || *pr_IOHhistTIValid == NULL) {
        i_Status = ESG_ESG_D_ERR_BADALLOC;
        LOG(ERROR) << fname << ": rc=" << i_Status << ", on Request Response";
      }
    }
  }

  return (i_Status);

}
//-END esg_acq_dac_HHTITIProcess -------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION               esg_acq_dac_HHTIValProcess
//  FULL MAME
//----------------------------------------------------------------------------
//  ROLE
//  This function allows to write historical values of the TI into a file for  SIDR.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_HHTIValProcess(
                               // Input parameters
                               const esg_esg_edi_t_StrComposedData * pr_IInternalCData,
                               const elemstruct_item_t* pr_ISubTypeElem, // P0073
                               // Input/Output parameters
                               esg_acq_dac_t_HhistTIDate * pr_IOHhistTIDate,
                               esg_acq_dac_t_HhistTIValue * pr_IOHhistTIValue,
                               esg_acq_dac_t_HhistTIValid * pr_IOHhistTIValid)
{
  //----------------------------------------------------------------------------
  static const char* fname = "esg_acq_dac_HHTIValProcess";
  //char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];
  char s_Trace[120 + 1];
  int i_Status = OK;

  // init of HhistDate
  //------------------------
  i_Status = ech_typ_GetAtt(&pr_IInternalCData->ar_EDataTable[0].u_val,
                            pr_IInternalCData->ar_EDataTable[0].type,
                            &pr_IOHhistTIDate->ad_Date,
                            pr_ISubTypeElem->fields[0].type);

  // init of HhistValue
  //-------------------------
  if (i_Status == OK) {
    i_Status = ech_typ_GetAtt(&pr_IInternalCData->ar_EDataTable[1].u_val,
                            pr_IInternalCData->ar_EDataTable[1].type,
                            &pr_IOHhistTIValue->g_Value,
                            pr_ISubTypeElem->fields[1].type);
  }

  // init of HhistValid
  //-------------------------
  if (i_Status == OK) {
    i_Status = ech_typ_GetAtt(&pr_IInternalCData->ar_EDataTable[2].u_val,
                            pr_IInternalCData->ar_EDataTable[2].type,
                            &pr_IOHhistTIValid->ah_Valid,
                            pr_ISubTypeElem->fields[2].type);
  }

  sprintf(s_Trace, "End : stat=%d Date=%ld Valid=%d Value=%g ",
          i_Status, pr_IOHhistTIDate->ad_Date.tv_sec, pr_IOHhistTIValid->ah_Valid, pr_IOHhistTIValue->g_Value);
  std::cout << fname << ": " << s_Trace << std::endl;

  return (i_Status);
}
//-END esg_acq_dac_HHTIValProcess --------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION               esg_acq_dac_InitInternalBis
//  FULL MAME
//----------------------------------------------------------------------------
//  ROLE
//  This function allows to initialize InternalBis structure for TI only known by DIR.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_InitInternalBis( /* output parameters */ esg_esg_edi_t_StrComposedData* pr_OInternalCDataBis)
{
  //----------------------------------------------------------------------------
  //static const char* fname = "esg_acq_dac_InitInternalBis";
  int i_Status = NOK;
  int i_Val;
  uint8_t o_Val;
  elemstruct_item_t* r_SubTypeElem;
  char s_SubTypeId[10];
  //............................................................................

  memset(pr_OInternalCDataBis, 0, sizeof(esg_esg_edi_t_StrComposedData));

  // Get the structure of the correspondig subtype
  //-----------------------------------------------
  strcpy(s_SubTypeId, ECH_D_HHISTTI_TI);
  // По ассоцированному названию найти соответствующую часть данных из ESG_LOCSTRUCT
  if (NULL != (r_SubTypeElem = ech_typ_ConsultSubType(s_SubTypeId))) {
    i_Status = OK;

    // TODO: Структуру полей и атрибутов БДРВ можно редактировать в конфигурационном файле. Однако изменения их по сравнению с ГОФО таковы,
    // что теперь поля fields не всегда являются атрибутами БДРВ, однако код ГОФО рассчитывает на это (ar_AttElem ранее содержали только
    // атрибуты БДРВ, но сейчас могут содержать и служебные поля, у которых тип БДРВ равен UNKNOWN. Из-за этого в нумерации атрибутов могут
    // возникать промежутки, и нельзя полагаться на "старую", абсолютную нумерацию.
    // Таких мест не много. Нужно или обрабатывать этот случай, или сигнализировать о возникшей проблеме.
    // Будем сигнализировать.
    // ECH_D_HHISTTI_TI = C0072, associate = P0072
    assert(r_SubTypeElem->fields[0].type == FIELD_TYPE_INT32);  // C0072.<TI ident>
    assert(r_SubTypeElem->fields[1].type == FIELD_TYPE_INT32);  // C0072.STATUS
    assert(r_SubTypeElem->fields[2].type == FIELD_TYPE_UINT32); // C0072.<number exchange>
    assert(r_SubTypeElem->fields[3].type == FIELD_TYPE_UINT8);  // C0072.<infotype>
    
    // status
    //--------
    i_Val = 0; // SIG_HIS_D_ERR_NOVAL;
    pr_OInternalCDataBis->ar_EDataTable[1].type = r_SubTypeElem->fields[1].type;
    i_Status = ech_typ_StoreAtt(&i_Val,
                                r_SubTypeElem->fields[1].type,
                                &pr_OInternalCDataBis->ar_EDataTable[1].u_val,
                                r_SubTypeElem->fields[1].type);
  }

  // NbEch
  //-------
  if (i_Status == OK) {
    i_Val = 0;
    pr_OInternalCDataBis->ar_EDataTable[2].type = r_SubTypeElem->fields[2].type;
    i_Status = ech_typ_StoreAtt(&i_Val,
                                r_SubTypeElem->fields[2].type,
                                &pr_OInternalCDataBis->ar_EDataTable[2].u_val,
                                r_SubTypeElem->fields[2].type);
  }

  // type
  //------
  if (i_Status == OK) {
    o_Val = 99;
    pr_OInternalCDataBis->ar_EDataTable[3].type = r_SubTypeElem->fields[3].type;
    i_Status = ech_typ_StoreAtt(&o_Val,
                                r_SubTypeElem->fields[3].type,
                                &pr_OInternalCDataBis->ar_EDataTable[3].u_val,
                                r_SubTypeElem->fields[3].type);
  }

  return (i_Status);
}
//-END esg_acq_dac_InitInternalBis -------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION    esg_acq_dac_HHistTiAcq
//  FULL MAME
//----------------------------------------------------------------------------
//  ROLE
//  This function treats all the  ECH_D_EDI_EDDSEG_HISTHISTTI segment
//  included into an historic of historised TI file
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  When historized TI for DIR have been received.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_HHistTiAcq(
                           // Input parameters
                           const int i_IMsgApplNb,   // number of applicative segments of the file
                           const int i_IApplLength,   // length of current applicative segment of the file
                           const char* s_IAcqSiteId,    // universal name of the distant site of acquisition
                           const char* s_IHistFileName, // name of the result historic file for SIDR
                           FILE* pi_IFileId,                 // id of the exchanged file (opened in the caller)
                           // Input-output parameters
                           int* i_IOApplDone)       // number of processed applicative segments
{
  //----------------------------------------------------------------------------
  static const char* fname = "esg_acq_dac_HHistTiAcq";
  int i_Status = OK;
  int i_RetStatus = OK;
  int i_CDLength = 0;
  int i_LgDone = 0;
  // number of group of data for one TI of the historic
  int i_NbEch = 0;
  // number of group of treated data for the current TI
  int i_EchDone = 0;
  int i_MsgSize = 0;
  // current number of explored TI in the list given by SIDR
  int i_InfosHistDone = 0;
  // number of historised TI given by SIDR at the DIR
  int i_NbInfosHist = 0;
  int i_ApplLength = 0;
  int i_ApplDone = 0;
  int i_LgAppl = 0;
  gof_t_HistUpdReq *pr_HistUpdReq = NULL;
  // the size of trace buffer must be at least this size
  char s_Trace[120 + 1];
  char s_Buffer[ECH_D_APPLSEGLG + 1];
  //char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];
  bool b_TIFound = false;
  // This data is used when TI has been processed at the DIPL
  esg_esg_edi_t_StrComposedData r_InternalCData;
  // This data is used when TI has not been processed at the DIPL
  esg_esg_edi_t_StrComposedData r_InternalCDataBis;
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
  esg_esg_odm_t_ExchInfoElem r_ExchInfoElem;
//1  esg_esg_odm_t_GeneralData r_LocalGenData;
  esg_acq_dac_t_HhistTIDate *pr_HhistTIDate = NULL;
  esg_acq_dac_t_HhistTIValue *pr_HhistTIValue = NULL;
  esg_acq_dac_t_HhistTIValid *pr_HhistTIValid = NULL;
  esg_esg_t_HeaderAppl r_HeaderAppl;
  esg_acq_dac_t_HhistTIHeader r_HhistTIHeader;
  elemstruct_item_t* r_SubTypeElem = NULL;
// identification of SIDR result file
  FILE *pi_FileIdRes = NULL;
// SIDL period value
  gof_t_PeriodType e_SinfPeriod;
// ESDX exchanged period value
  int i_ExchPeriod;
// distant site for SIDL (ex: "/K42667" and not "/SAK42667")
// An SIDL function must be called with this site
  gof_t_UniversalName s_Site;
// LED read
  int i_LEDRead;
// Ti index in tracing loop
  int i_Ti;
// indication of end of TI processed, the file for SIDR is so completed
  bool b_EndTi;
// indication of end of received file processed
  bool b_EndReceivedFile;
// saving of last TI struct used when received file is over
  elemstruct_item_t* r_SubTypeElemLastTi = NULL;
// name of the result historic file for SIDR with a suffix for each historic period
  char s_SidlHistFileName[256];
//............................................................................

  // Init of r_InternalCDataBis to process TI only known by DIR
  // This structure alllows to fill the SIDR file for TI not processed by the DIPL but asked at the DIR (each TI must be present)
  //-----------------------------------------------------------------
  i_Status = esg_acq_dac_InitInternalBis(&r_InternalCDataBis);

  // The name of the SIDL file depends on the hist period
  // The open of the file is performed after
  // Process each segment
  //---------------------------
  i_ApplLength = i_IApplLength;
  i_ApplDone = 0;

  // Process each applicative segment of the received file from DIPL.
  // The number of segment to consider is the total number - 1 because of the reply number.
  // The condition of end of treatment is modified.
  // When the received file is ended, the last TI NOT processed by DIPL must be included into the result file for SIDR.
  // An indicator tells if the last TI has been processed and if the treatment of received file is over.
  b_EndTi = false;
  b_EndReceivedFile = false;
  while ((i_Status == OK) && (b_EndTi == false)) {

    // indication of end of received file
    if (i_ApplDone == (i_IMsgApplNb - 1)) {
      b_EndReceivedFile = true;
    }

    // Read the header of all segments, except the first one (It has already been read by the caller)
    //------------------------------------------------------------
    if (b_EndReceivedFile == false) {

      if (i_ApplDone != 0) {

        // Read and decode the segment header
        //-------------------------------------------
        i_Status = esg_esg_fil_HeadApplRead(pi_IFileId, &r_HeaderAppl, &i_LgAppl);

        // Get the segment length
        // ----------------------------
        if (i_Status == OK) {
          i_ApplLength = r_HeaderAppl.i_ApplLength;
        }
      }

      // Read the rest of the segment
      //-------------------------------------
      if (i_Status == OK) {
        i_Status = esg_esg_fil_StringRead(pi_IFileId, i_ApplLength, s_Buffer);
      }
    }

    // Process each composed data in the current segment
    // -------------------------------------------------------
    i_LgDone = 0;

    while ((i_LgDone < i_ApplLength) && (i_Status == OK) && (b_EndTi == false)) {

      // Get a composed data
      //--------------------------
      if (b_EndReceivedFile == false) {
        i_Status = esg_ine_man_CDProcessing(&s_Buffer[i_LgDone],
                                            i_ApplLength - i_LgDone,
                                            &i_CDLength,
                                            &r_InternalCData,
                                            r_SubTypeElem,
                                            &r_QuaCData);
        LOG(ERROR) << fname << "esg_ine_man_CDProcessing delme: " << ((NULL != r_SubTypeElem)? r_SubTypeElem->name : "<NULL>");
        i_LgDone += i_CDLength;
      }
      else {
        // when the received file is ended just consider new NOT processed TI so force type to ECH_D_HHISTTI_TI
        //1 strcpy(r_SubTypeElem.s_SubTypeId, ECH_D_HHISTTI_TI);
        // Принудительно выставляем подтип как P0072 (ECH_D_HHISTTI_TI)
        r_SubTypeElem = ech_typ_ConsultSubType(ECH_D_HHISTTI_TI);
      }

      if (i_Status == OK) {

        // The composed data is an ECH_D_HHISTTI_TYPE (P0071)
        // This data gives the historic period type
        // ------------------------------------------------
        if (strcmp(r_SubTypeElem->associate, ECH_D_HHISTTI_TYPE) == 0) {

          // Could be present in the first segment only
          //-------------------------------------------------
          if (i_ApplDone != 0) {
            i_Status = ESG_ESG_D_ERR_INEXPCOMPDATA;
          }
          else {
            // Get the period
            //---------------------
            // GEV: диагностика некорректной работы из-за изменения типа нулевого элемента
            assert(r_SubTypeElem->fields[0].type == FIELD_TYPE_UINT32);

            i_Status = ech_typ_GetAtt(&r_InternalCData.ar_EDataTable[0].u_val,
                                      r_InternalCData.ar_EDataTable[0].type,
                                      &i_ExchPeriod,
                                      r_SubTypeElem->fields[0].type);

#if 1
              LOG(ERROR) << fname << ": esg_esg_odm_ConsultGeneralData";
#else
            // Get the list of TI prepared by the DIR
            // This list has already been asked in the preparation of the basic request
            //---------------------------------------------
            if (i_Status == OK) {
              esg_esg_odm_ConsultGeneralData(&r_LocalGenData);
            }
#endif

            // the SIDR function is called to get the list of TI
            // required by SIDR at the DIR. The received TI from DIPL may be fewer
            // than those asked by SIDR
            // This call is only performed once after the period of historic has
            // been detected in the ECH_D_HHISTTI_TYPE segment
            if (i_Status == OK) {

              // conversion of DIPL universal name from string "/SAKxxxx" into "/Kxxxx"
              strcpy(s_Site, "/");
              strcat(s_Site, (s_IAcqSiteId + 3));

              // conversion of internal historic period into SIDL period
              if (i_ExchPeriod == ESG_ESG_ODM_D_HIST_QH) {
                e_SinfPeriod = GOF_D_PER_QH;
              }
              else if (i_ExchPeriod == ESG_ESG_ODM_D_HIST_HOUR) {
                e_SinfPeriod = GOF_D_PER_HOUR;
              }
              else if (i_ExchPeriod == ESG_ESG_ODM_D_HIST_DAY) {
                e_SinfPeriod = GOF_D_PER_DAY;
              }
              else if (i_ExchPeriod == ESG_ESG_ODM_D_HIST_MONTH) {
                e_SinfPeriod = GOF_D_PER_MONTH;
              }
              else {
                e_SinfPeriod = GOF_D_PER_NOHISTO;
              }
#if 1
              LOG(ERROR) << fname << ": sir_his_rhd_p_GetHistUpdReq " << s_Site << " period=" << e_SinfPeriod;
#else
              i_Status = sir_his_rhd_p_GetHistUpdReq(r_LocalGenData.r_SrvClientId,
                                                     s_Site,
                                                     e_SinfPeriod,
                                                     &pr_HistUpdReq,
                                                     &i_MsgSize);
#endif

              if (i_Status == OK) {
                for (i_Ti = 0; i_Ti < pr_HistUpdReq->h_NbInfosHist; i_Ti++) {
                  sprintf(s_Trace, "TI given by SIDR i_Ti=%d UnivName=%s ", i_Ti, pr_HistUpdReq->s_Info[i_Ti]);
                  std::cout << fname << ": " << s_Trace << std::endl;
                }
              }
            }

            // Open the file containing the results for SIDR
            //----------------------------------------------------
            if (i_Status == OK) {
              strcpy(s_SidlHistFileName, s_IHistFileName);
              if (i_ExchPeriod == ESG_ESG_ODM_D_HIST_QH) {
                strcat(s_SidlHistFileName, "_QH");
              }
              else if (i_ExchPeriod == ESG_ESG_ODM_D_HIST_HOUR) {
                strcat(s_SidlHistFileName, "_HOUR");
              }
              else if (i_ExchPeriod == ESG_ESG_ODM_D_HIST_DAY) {
                strcat(s_SidlHistFileName, "_DAY");
              }
              else if (i_ExchPeriod == ESG_ESG_ODM_D_HIST_MONTH) {
                strcat(s_SidlHistFileName, "_MONTH");
              }
              else {
                strcat(s_SidlHistFileName, "_NOHISTO");
              }
#if 1
              LOG(INFO) << fname << ": creating history file " << s_SidlHistFileName;
              if (NULL != (pi_FileIdRes = fopen(s_SidlHistFileName, "w"))) {
                i_Status = ESG_ESG_D_ERR_CANNOTWRITFILE;
                LOG(ERROR) << fname << ": rc=" << i_Status;
              }
#else
              i_Status = esg_esg_fil_FileOpen(s_SidlHistFileName, ESG_ESG_FIL_D_WRITEMODE, &pi_FileIdRes);
#endif
            }

            if (i_Status == OK) {
              if ((i_MsgSize == 0) ||
                  (pr_HistUpdReq == NULL) ||
                  ((pr_HistUpdReq != NULL) && (pr_HistUpdReq->h_NbInfosHist == 0))) {
                i_Status = ESG_ESG_D_ERR_NOTIFORHHISTTIREQ;
              }
            }

            // Process the SIDR header file part
            //----------------------------------------
            if (i_Status == OK) {

// initialize the file header with spaces
              memset(&r_HhistTIHeader, 0, sizeof(esg_acq_dac_t_HhistTIHeader));

// the site given to SIDL has the "/K....." form
              strcpy(r_HhistTIHeader.s_DIPL, s_Site);

// the SIDL structure must be filled with the SIDL period and not the exchanged period
              r_HhistTIHeader.e_HistPeriod = e_SinfPeriod;
              r_HhistTIHeader.h_NbInfosHist = pr_HistUpdReq->h_NbInfosHist;

              // Write general information in the file
              // -------------------------------------------
// addition of traces
              sprintf(s_Trace, "SIDR Written File Header : DIPL=%s HistPeriod=%d NbTi=%d ",
                      r_HhistTIHeader.s_DIPL,
                      r_HhistTIHeader.e_HistPeriod,
                      r_HhistTIHeader.h_NbInfosHist);
              std::cout << fname << ": " << s_Trace << std::endl;;

// do not use a structure because of holes. write each field separately
              i_Status = esg_esg_fil_DataWrite(pi_FileIdRes, &r_HhistTIHeader.s_DIPL,       sizeof(gof_t_UniversalName),1);
              i_Status = esg_esg_fil_DataWrite(pi_FileIdRes, &r_HhistTIHeader.e_HistPeriod, sizeof(gof_t_PeriodType),   1);
              i_Status = esg_esg_fil_DataWrite(pi_FileIdRes, &r_HhistTIHeader.h_NbInfosHist,sizeof(uint16_t),           1);

            }

            // Save historized TI number
            //----------------------------------
            if (i_Status == OK) {
              i_NbInfosHist = pr_HistUpdReq->h_NbInfosHist;
              i_InfosHistDone = 0;
            }
          }
        }
        else if (strcmp(r_SubTypeElem->associate, ECH_D_HHISTTI_TI) == 0) { // The composed data is an ECH_D_HHISTTI_TI
          // This data gives the identification of a TI
          // Verify that all of historic values of the previous TI have been processed
          //------------------------------------------------------------
          if (i_NbEch != i_EchDone) {
            i_Status = ESG_ESG_D_ERR_INEXPCOMPDATA;
          }
          else {

// The TI LED is considered as an ordinary field. It is described in the LOCSTRUCTS (same as for the historic of alarms)
// The r_QuaCData.b_QualifyExist indicator is no longer used as a test on number of fields in DCD and LOCSTRUCTS is performed which caused an error
// if file is ended just remain on the same fictive LED
// if file is not ended memorize the last TI subtype elem structure
            if (b_EndReceivedFile == false) {
              if (i_Status == OK) {

                // GEV: диагностика некорректной работы из-за изменения типа нулевого элемента
                assert(r_SubTypeElem->fields[0].type == FIELD_TYPE_INT32);

                r_SubTypeElemLastTi = r_SubTypeElem;
                i_Status = ech_typ_GetAtt(&r_InternalCData.ar_EDataTable[0].u_val,
                                          r_InternalCData.ar_EDataTable[0].type,
                                          &i_LEDRead,
                                          r_SubTypeElem->fields[0].type);

              }

              // convert LED into universal name
              // -------------------------------
              if (i_Status == OK) {
#if 0
                LOG(ERROR) << fname << ": get the TAG of " << s_IAcqSiteId << " by LED=" << i_LEDRead;
#else
                i_Status = esg_esg_odm_ConsExchInfoLed(s_IAcqSiteId, i_LEDRead, &r_ExchInfoElem);
                if (OK != i_Status) {
                  LOG(WARNING) << fname << ": rc=" << i_Status << ", skip " << s_IAcqSiteId << ", LED #" << i_LEDRead;
                  i_Status = OK;    // Пропустить ошибочный элемент, попытаться обработать следующие
                }
#endif
              }
            }

            // A TI to treat has been found in the received file from adjacent DIPL
            // Try to find this TI in the list of TI required at the DIR by SIDR
            b_TIFound = false;

            while ((i_InfosHistDone < i_NbInfosHist) && (b_TIFound == false) && (i_Status == OK)) {
              // The TI has been processed by the DIPL,
              //       and is required by the DIR
              //---------------------------------------------
              if (strcmp(r_ExchInfoElem.s_Name, pr_HistUpdReq->s_Info[i_InfosHistDone]) == 0) {
                b_TIFound = true;

                i_Status = esg_acq_dac_HHTITIProcess(s_IAcqSiteId,
                                                     r_ExchInfoElem.s_Name,
                                                     &r_InternalCData,
                                                     r_SubTypeElem,
                                                     pi_FileIdRes,
                                                     &i_NbEch,
                                                     &i_EchDone,
                                                     &pr_HhistTIDate,
                                                     &pr_HhistTIValue,
                                                     &pr_HhistTIValid);
                sprintf(s_Trace, "TI processed by DIPL Stat=%d UnivName=%s Index in SIDR List=%d NbData=%d ",
                        i_Status,
                        r_ExchInfoElem.s_Name,
                        i_InfosHistDone,
                        i_NbEch);
                std::cout << fname << ": " << s_Trace << std::endl;
              }
              else {    // The TI has not been processed by the DIPL, but is required by the DIR
                //-------------------------------------------------
                i_Status = esg_acq_dac_HHTITIProcess(s_IAcqSiteId,
                  // the universal name is the one given at the DIR and not in the received file from DIPL
                                                     pr_HistUpdReq->s_Info[i_InfosHistDone],
                                                     &r_InternalCDataBis,
                                                     r_SubTypeElemLastTi,
                                                     pi_FileIdRes,
                                                     &i_NbEch,
                                                     &i_EchDone,
                                                     &pr_HhistTIDate,
                                                     &pr_HhistTIValue,
                                                     &pr_HhistTIValid);

                sprintf(s_Trace, "TI NOT processed by DIPL Stat=%d UnivName=%s Index in SIDR List=%d NbData=%d ",
                        i_Status,
                        pr_HistUpdReq->s_Info[i_InfosHistDone],
                        i_InfosHistDone,
                        i_NbEch);
                std::cout << fname << ": " << s_Trace << std::endl;
              }

              // incrementation of TI treated index
              i_InfosHistDone += 1;
            }                   // end while research of TI

            // Error if a TI has been processed by the DIPL, but is not required by the DIR
            //--------------------------------------------------------
            // the file must not be ended
            if ((b_TIFound == false) && (i_Status == OK) && (b_EndReceivedFile == false)) {
              i_Status = ESG_ESG_D_ERR_INCOMPLIST;
            }
          }
        }                       // end if ECH_D_HHISTTI_TI composed data found
        else if (strcmp(r_SubTypeElem->associate, ECH_D_HHISTTI_VAL) == 0) { // the composed data is an ECH_D_HHISTTI_VAL
          // This data gives date, value and validity of one group of data for a TI
          // Verify the waited composed data
          //--------------------------------------
          if (i_EchDone == i_NbEch) {
            i_Status = ESG_ESG_D_ERR_INEXPCOMPDATA;
          }
          else {
            // Process the value part of the TI
            //---------------------------------------
            i_Status = esg_acq_dac_HHTIValProcess(&r_InternalCData,
                                                  r_SubTypeElem,
                                                  pr_HhistTIDate + i_EchDone,
                                                  pr_HhistTIValue + i_EchDone,
                                                  pr_HhistTIValid + i_EchDone);

          }

          if (i_Status == OK) {

            // Increase the number of processed historic values
            //-------------------------------------------------------
            i_EchDone += 1;
          }
        }
      }

      // Free the allocated memory
      //--------------------------------
      esg_ine_man_FreeCompData(&r_InternalCData);

      // indication of end of TI processing
      if (i_InfosHistDone == i_NbInfosHist) {
        b_EndTi = true;
      }
    }                           // end while for each composed data (end of segment reached)

    // Increase the number of segments already processed
    //--------------------------------------------------------
    if (i_Status == OK) {
      i_ApplDone += 1;

      // the end segment label must be read in order to point on the beginning segment label of the following segment
      if (b_EndReceivedFile == false) {
        // only if end of received file not reached
        i_Status = esg_esg_fil_EndApplRead(pi_IFileId);
      }
    }
  }                             // end while for each segment

  // Let the caller knows the number of processed segments
  //------------------------------------------------------------
  *i_IOApplDone = i_ApplDone;

  // Close the file
  //---------------------
  // a special status is used here as an error may have been detected which would be lost with the same status
  if (-1 == fclose(pi_FileIdRes)) {
    LOG(ERROR) << fname << ": closing file: " << strerror(errno);
  }

  // Free the allocated memory
  //--------------------------------
  if (pr_HhistTIDate != NULL) {
    free(pr_HhistTIDate);
  }

  if (pr_HhistTIValue != NULL) {
    free(pr_HhistTIValue);
  }

  if (pr_HhistTIValid != NULL) {
    free(pr_HhistTIValid);
  }

  // Send the message SIG_M_ACQHISTFILE in order to warn SIDL of the creation of the historic file
  //----------------------------------------------------------------------
  if (i_Status == OK) {

    // the site given to SIDL has the "/K....." form
    // the name of the Hist SIDL file has changed and depends on the period type
    i_Status = esg_acq_dac_HHTISndMsg(s_Site, r_HhistTIHeader.e_HistPeriod, s_SidlHistFileName);
  }

  // Global message
  // --------------
  if (i_Status != OK) {
    LOG(ERROR) << fname << ": rc=" << i_Status;
  }

  i_RetStatus = i_Status;

  return (i_RetStatus);
}
//-END esg_acq_dac_HHistTiAcq------------------------------------------------
