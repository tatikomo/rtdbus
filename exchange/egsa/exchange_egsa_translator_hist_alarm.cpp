//*****************************************************************************
// OVERVIEW
// Manages the disposal of historized alarms for SIDL :
//			. Convert the received data in the right format
//			. Send the converted data to SIDL
//*****************************************************************************

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

//----------------------------------------------------------------------------
//  FUNCTION				esg_acq_dac_HistAlAcq
//  FULL MAME
//----------------------------------------------------------------------------
//  ROLE
//	This function converts the transmitted historized alarms, included
//      in one segment, in the right form and send them to SIDL via a file.
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	When historized alarms have been received.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_HistAlAcq (
				     // Input parameters
				     FILE * pi_IFileId,
				     size_t i_ILgHead,
				     size_t i_IApplLength,
                     const gof_t_UniversalName s_IAcqSiteId,
				     // In-Out parameters
				     size_t* i_IONbAlOp,    // ALO count
				     esg_esg_t_HistAlElem* ar_IOHistAlOp,   // ALO data
				     size_t* i_IONbNopAl,   // ALN count
                     esg_esg_t_HistAlElem* ar_IOHistNopAl)  // ALN data
{
  //----------------------------------------------------------------------------
  static const char* fname = "esg_acq_dac_HistAlAcq";
  int rc = OK;

  int i_Status = OK;
  int i_RetStatus = OK;
  size_t i_CDLength = 0;
  size_t i_LgDone = 0;

  char s_Trace[70 + 1];
  char s_Buffer[ECH_D_APPLSEGLG + 1];

  esg_esg_edi_t_StrComposedData r_InternalCData;
  elemstruct_item_t* r_SubTypeElem = NULL;
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;

  size_t i_FileDepl;	// work variable

//  ech_typ_t_AttElem r_AttElem;	// type of the attibute
  size_t i_LEDRead;	// LED read
  esg_esg_odm_t_ExchInfoElem r_AssocInfo;	// associated teleinfo
  int h_AlTypeRead;	// alarm type read
  esg_esg_t_HistAlElem *r_CurHistAl = 0;	// work variable

  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];

  //............................................................................
  LOG(ERROR) << fname << ": TODO " << s_IAcqSiteId;

  // Read the segment
  //-------------------------
  i_Status = esg_esg_fil_StringRead (pi_IFileId, i_IApplLength, s_Buffer);

  // Process the alarm list type, first data of the segment
  // ------------------------------------------------------------
  if (i_Status == OK) {
    i_CDLength = 0;

    i_Status = esg_ine_man_CDProcessing (s_Buffer,
					 i_IApplLength,
                     &i_CDLength,
                     &r_InternalCData,
                     r_SubTypeElem,
                     &r_QuaCData);
  }	// first data included in the segment

  if (i_Status == OK) {
    i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[0].u_val,
			       r_InternalCData.ar_EDataTable[0].type,
                   &h_AlTypeRead,
                   r_SubTypeElem->fields[0].type);
  }	// end if i_Status

  if (i_Status == OK) {

    // Process each composed data included in the segment
    // --------------------------------------------------------
    for (i_LgDone = i_CDLength;
	   ((i_LgDone < i_IApplLength) &&
	    (i_Status == OK) && (*i_IONbAlOp < GOF_D_LST_ALAmax));
         i_LgDone = i_LgDone + i_CDLength)
    {

      // set historized alarms array according to type
      // ---------------------------------------------
      if (h_AlTypeRead == GOF_D_LST_ALA_OPE) {
        r_CurHistAl = ar_IOHistAlOp + *i_IONbAlOp;
      }
      else {
        r_CurHistAl = ar_IOHistNopAl + *i_IONbNopAl;
      }

      // Read and Decode the applicative segment
	  //----------------------------------------------------
      i_Status = esg_ine_man_CDProcessing (&s_Buffer[i_LgDone],
					   i_IApplLength - i_LgDone,
					   &i_CDLength,
                       &r_InternalCData,
                       r_SubTypeElem,
                       &r_QuaCData);

      // Process the applicative segment
	  //--------------------------------------------
      if (i_Status == OK) {
        i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[0].u_val,
				   r_InternalCData.ar_EDataTable[0].type,
                   &i_LEDRead,
                   r_SubTypeElem->fields[0].type);
      }				// end if i_Status

      if (i_Status == OK) {
        // convert LED into universal name
        // -------------------------------
        i_Status = esg_esg_odm_ConsExchInfoLed (s_IAcqSiteId, i_LEDRead, &r_AssocInfo);
      }				// end if i_Status

      if (i_Status == OK) {
        strcpy ((char *) &r_CurHistAl->s_AlRef, r_AssocInfo.s_Name);

#if 1
        for (size_t idx=0; r_SubTypeElem->num_attributes; idx++)
        {
          // Get next field
          // --------------
          i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[idx].u_val,
                     r_InternalCData.ar_EDataTable[idx].type,
                     &r_CurHistAl->d_AlDate,
                     r_SubTypeElem->fields[idx].type);
          if (OK != i_Status) {
            LOG(ERROR) << fname << ": unable to read attr " << r_SubTypeElem->name << ", idx #" << idx << ", type=" << r_SubTypeElem->fields[idx].type << ", rc=" << i_Status;
            break;
          }
        }
#else
      if (i_Status == OK) {
        // Get next field
        // --------------
        i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[1].u_val,
				   r_InternalCData.ar_EDataTable[1].i_type,
				   &r_CurHistAl->d_AlDate,
                   r_SubTypeElem.ar_AttElem[1]);
      }				// end if i_Status

      if (i_Status == OK) {
        // Get next field
        // --------------
        i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[2].u_val,
				   r_InternalCData.ar_EDataTable[2].i_type,
				   &r_CurHistAl->o_AlType,
                   r_SubTypeElem.ar_AttElem[2]);
      }				// end if i_Status

      if (i_Status == OK) {
        // Get next field */
        // -------------- */
        i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[3].u_val,
				   r_InternalCData.ar_EDataTable[3].i_type,
				   &r_CurHistAl->o_AlDeg,
                   r_SubTypeElem.ar_AttElem[3]);

      }				// end if i_Status

      if (i_Status == OK) {
        // Get next field */
        // -------------- */
        i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[4].u_val,
				   r_InternalCData.ar_EDataTable[4].i_type,
				   &r_CurHistAl->o_AlState,
                   r_SubTypeElem.ar_AttElem[4]);
      }				// end if i_Status

      if (i_Status == OK) {
        // Get next field */
        // -------------- */
        i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[5].u_val,
				   r_InternalCData.ar_EDataTable[5].i_type,
				   &r_CurHistAl->g_AlVal,
                   r_SubTypeElem.ar_AttElem[5]);
      }				// end if i_Status
#endif
      }

      if (i_Status == OK) {
        // increased the number of treated values
    	// --------------------------------------
    	if (h_AlTypeRead == GOF_D_LST_ALA_OPE) {
	      *i_IONbAlOp = *i_IONbAlOp + 1;
    	}
	    else {
    	  *i_IONbNopAl = *i_IONbNopAl + 1;
	    }

      }				// end if i_Status

    }				// end for to process composed data of the segment

  }				// Composed data included in the segment

  // Global message
  // --------------
  if (i_Status != OK) {
    LOG(ERROR) << fname << ": rc=" << i_Status;
  }

  i_RetStatus = i_Status;
  return (i_RetStatus);
} //-END esg_acq_dac_HistAlAcq------------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION			esg_acq_dac_HistAlPut
//  FULL MAME			Put Historized Alarms for SINF
//----------------------------------------------------------------------------
//  ROLE
//	Processing of the the historized alarm notification
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	Receive of a device of alrms file deposit message
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	- open the file to be furnished to SINF
//	- write the origin DIPL
//	- write the list type
//	- write the count of alarms in the file
//	- FOR (each alarm)
//	-     write the record
//	- END_FOR
//	- close the file
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_HistAlPut (
				     // Input parameters
				     const gof_t_UniversalName s_IDistantSite,  // name of distant site
				     const char* s_ILongFileName,               // name of the file
				     const int h_IAlListType,                   // alarms type
				     const size_t i_IHistAlNb,                  // number of alarms
				     const esg_esg_t_HistAlElem* pr_IHistAlarms)// description of alarms
//----------------------------------------------------------------------------
{
  static const char* fname = "esg_acq_dac_HistAlPut";   // function name
  int i_RetStatus = OK;  // returned status
  FILE *pi_FileId;  // file pointer
  size_t i_i;       // index
  char s_CurStr[256]; // current string
  esg_esg_t_HistAlElem *pr_CurHistAl;	// work variable
  size_t i_MsgLength = 0; // message length
//1  sig_t_msg_AlFile r_MsgBody;	// message body
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];
  int i_AlListType;	// work variable

  // open the file to be furnished to SINF
  // --------------------------------------------------------------------------
  if (NULL == (pi_FileId = fopen(s_ILongFileName, "w"))) {
    LOG(ERROR) << fname << ": unable to open " << s_ILongFileName << " for writing";
    i_RetStatus = NOK;
  }

  if (i_RetStatus == OK) {

    // write the origin DIPL
    // --------------------------------------------------------------------------
    sprintf (s_CurStr, "%s\n", s_IDistantSite);
    i_RetStatus = esg_esg_fil_StringWrite (pi_FileId, s_CurStr);

    // write the list type
    // --------------------------------------------------------------------------
    if (i_RetStatus == OK) {

      i_AlListType = GOF_D_NON_OPE_ALA;
      if (h_IAlListType == GOF_D_LST_ALA_OPE) {
        i_AlListType = GOF_D_OPE_ALA;
      }

      sprintf (s_CurStr, "%d\n", i_AlListType);
      i_RetStatus = esg_esg_fil_StringWrite (pi_FileId, s_CurStr);
    }				// end if i_RetStatus

    // write the count of alarms in the file
    // --------------------------------------------------------------------------
    if (i_RetStatus == OK) {
      sprintf (s_CurStr, "%d\n", i_IHistAlNb);
      i_RetStatus = esg_esg_fil_StringWrite (pi_FileId, s_CurStr);
    }				// end if i_RetStatus

    // FOR (each alarm)
    // --------------------------------------------------------------------------
    if (i_RetStatus == OK) {
      pr_CurHistAl = (esg_esg_t_HistAlElem *) pr_IHistAlarms;

      for (i_i = 0; i_i < i_IHistAlNb; i_i++) {
        // write the record universal : name date type degree state value
        // --------------------------------------------------------------------------
        sprintf(s_CurStr, "%s %ld %ld %d %d %d %e\n",
                pr_CurHistAl->s_AlRef,
                pr_CurHistAl->d_AlDate.tv_sec,
                pr_CurHistAl->d_AlDate.tv_usec,
                pr_CurHistAl->o_AlType,
                pr_CurHistAl->o_AlDeg,
                pr_CurHistAl->o_AlState,
                pr_CurHistAl->g_AlVal);

        i_RetStatus = esg_esg_fil_StringWrite (pi_FileId, s_CurStr);
        pr_CurHistAl = pr_CurHistAl + 1;
      }				// END_FOR

    }				// end if i_RetStatus

    // close the file
    // --------------------------------------------------------------------------
    if (-1 == fclose(pi_FileId)) {
      LOG(ERROR) << fname << ": unable to close file " << s_ILongFileName;
      i_RetStatus = NOK;
    }
    else pi_FileId = NULL;

  }	// END_IF open ok

  // Inform SINF that file is ready
  // --------------------------------------------------------------------------
  if (i_RetStatus == OK) {

#if 0
    i_MsgLength = sizeof (r_MsgBody);
    memset (&r_MsgBody, 0, i_MsgLength);
    strcpy (r_MsgBody.s_file_name, s_ILongFileName);
#endif

    i_RetStatus = sig_ext_msg_p_InpSendMessageToSinf (SIG_D_MSG_ALFILE, i_MsgLength, NULL /* &r_MsgBody */);

  }	// END_IF i_RetStatus

  // Global message
  // --------------
  if (i_RetStatus != OK) {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus;
  }

  return (i_RetStatus);
}
// End esg_acq_dac_HistAlPut -------------------------------------------------

