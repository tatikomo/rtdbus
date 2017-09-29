#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
//#include <assert.h>
//#include <time.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_translator.hpp"

// Обработка Состояний смежных систем
// --------------------------------------------------------------
int ExchangeTranslator::processing_STATE(FILE * pi_FileId, size_t i_LgAppl, size_t i_ApplLength, AcqSiteEntry* site)
{
  static const char *fname = "processing_STATE";
  int i_Status = OK;
  uint32_t distant_site_state;
  synthstate_t i_SynthState = SYNTHSTATE_UNREACH;

  i_Status = esg_acq_dac_GetSiteState(pi_FileId, i_LgAppl, i_ApplLength, site, &distant_site_state);
  if (i_Status == OK) {

    // Проверить допустимость значения distant_site_state, преобразовав его в synthstate_t
    if (distant_site_state <= SYNTHSTATE_PRE_OPER) {
      i_SynthState = static_cast<synthstate_t>(distant_site_state);
    }
    else {
      LOG(ERROR) << fname << ": unsupported value: " << distant_site_state;
    }

#if _FUNCTIONAL_TEST
    LOG(INFO) << fname << ": CALL EGSA::processing_STATE(" << site->name() << "," << distant_site_state << ")";
#else
    i_Status = egsa()->processing_STATE(site, i_SynthState);
#endif
  }
  else {
    LOG(ERROR) << fname << ": unable to process distant site " << site->name() << " state";
  }

  return i_Status;
}


//----------------------------------------------------------------------------
// FUNCTION				esg_acq_dac_GetSiteState
// FULL MAME
//----------------------------------------------------------------------------
// ROLE
// This function furnishs distant site state in internal format.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_GetSiteState (
                // Input parameters
                FILE * pi_FileId, size_t i_LgAppl, size_t i_ApplLength,
                // acquisition site
                AcqSiteEntry* site,
                // Output parameters
                uint32_t * pi_OSiteStateValue)
{
  //----------------------------------------------------------------------------
  static const char *fname = "esg_acq_dac_GetSiteState";
  int i_Status = OK;
  int i_RetStatus = OK;
  bool b_SegStateExist = false;
  size_t i_CDLength = 0;
  size_t i_i = 0;
  char s_Buffer[ECH_D_APPLSEGLG + 1];
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];
  size_t i_LgInterChg;
  size_t i_LgMsg;
  elemstruct_item_t* r_SubTypeElem = NULL;
  esg_esg_edi_t_StrComposedData r_InternalCData; // decoded composed data buffer
  elemstruct_item_t* r_TypeElem = NULL;
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
  //............................................................................

  *pi_OSiteStateValue = SYNTHSTATE_UNREACH;

  i_Status = esg_esg_fil_StringRead (pi_FileId, i_ApplLength, s_Buffer);
  if ( i_Status != OK )
  {
    LOG(ERROR) << fname << ": reading body of state segment, i_LgAppl=" << i_LgAppl << ", i_ApplLength="
               << i_ApplLength << ", site=" << site->name() << ", rc=" << i_Status << " buf=" << s_Buffer;
  }

  if ( i_Status == OK ) {
    // Read and Decode the applicatif segment
    //----------------------------------------------------
    i_Status = esg_ine_man_CDProcessing(s_Buffer,
                                        i_ApplLength,
                                        &i_CDLength,
                                        &r_InternalCData,
                                        r_TypeElem,
                                        &r_QuaCData);
  }

  if ( i_Status == OK ) {
    // Get value of distant site state
    // ------------------------------------
    i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[0].u_val,
                               r_InternalCData.ar_EDataTable[0].type,
                               pi_OSiteStateValue,
                               FIELD_TYPE_UINT32);
  }

  if ( i_Status == OK ) {
    LOG(INFO) << fname << ": Value of \"" << site->name() << "\" distant state=" << *pi_OSiteStateValue;
  }
  else {
    LOG(ERROR) << fname << ": get Value of distant state, rc=" << i_Status;
  }

  return i_Status;

#if 0
  i_RetStatus = esg_esg_fil_HeadFileRead (s_ILongFileName,
					  &r_HeaderInterChg,
                      &r_HeaderMsg,
                      &i_LgInterChg,
                      &i_LgMsg,
                      &pi_FileId);

  i_FileDepl = i_LgInterChg + i_LgMsg;

  if (i_RetStatus == OK) {

    // Get segments number from the message
    // ----------------------------------------------------------------------------
    i_ApplLength = 0;
    i_i = 0;
    b_SegStateExist = GOF_D_FALSE;

    while ((!b_SegStateExist) && (i_RetStatus == OK)) {

      // Read and decode the segment header
      //-------------------------------------------
      i_RetStatus = esg_esg_fil_HeadApplRead (pi_FileId, &r_HeaderAppl, &i_LgAppl);

      // Process only segments with data
      //---------------------------------------
      if (i_RetStatus == OK) {
        // Get the segment length
        // ------------------------------------
        i_ApplLength = r_HeaderAppl.i_ApplLength;

        if (r_HeaderAppl.i_ApplType == ECH_D_EDI_EDDSEG_STATE) {
          // Read the rest of the segment
          //-------------------------------------
          i_RetStatus = esg_esg_fil_StringRead (pi_FileId, i_ApplLength, s_Buffer);
          b_SegStateExist = GOF_D_TRUE;
        }
        else {
          i_i++;
          if (i_i == r_HeaderMsg.h_MsgApplNb) {
            i_RetStatus = ESG_ESG_D_ERR_NOSTATESEG;
          }
          else {
            i_FileDepl += (i_ApplLength + i_LgAppl + ECH_D_SEGLABELLG);
            i_RetStatus = esg_esg_fil_FileSeek (pi_FileId, i_FileDepl);
          }
        }			// end Switch
      }				// End if: Segment with data
    }
  }

  if (i_RetStatus == OK) {

    // Read and Decode the applicatif segment
    //----------------------------------------------------
    i_RetStatus = esg_ine_man_CDProcessing (s_Buffer,
					    i_ApplLength,
                        &i_CDLength,
                        &r_InternalCData,
                        r_SubTypeElem,
                        &r_QuaCData);
  }

  if (i_RetStatus == OK) {

    // Get value of distant site state */
    // ------------------------------------ */
    i_RetStatus = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[0].u_val,
				  r_InternalCData.ar_EDataTable[0].i_type,
                  pi_OSiteStateValue,
                  ECH_D_LOC_UINT32);
  }
  else {
    sprintf (s_LoggedText, ", site='%s' appli_segmend_size=%d CD_size=%d", s_IAcqSiteId, i_ApplLength, i_CDLength);
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_LoggedText;
  }

  // Close the file (even if not OK)
  //----------------------------------------------------------------------------
  i_Status = esg_esg_fil_FileClose (pi_FileId);

  if (i_RetStatus != OK) {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus;
  }
#else

#endif
  return (i_RetStatus);
}

//-END esg_acq_dac_GetSiteState-----------------------------------------------

