#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <libgen.h> // dirname
#include <values.h>
#include <algorithm>
#include <map>
#include <istream>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_translator.hpp"




#define ESG_ESG_D_LOGGEDTEXTLENGTH 200



//============================================================================
void print_elemstruct(std::pair < const std::string, elemstruct_item_t > pair)
{
  std::cout << "ES:" << pair.first << " ";
}

void print_elemtype(std::pair < const std::string, elemtype_item_t > pair)
{
  std::cout << "ET:" << pair.first << " ";
}

//============================================================================
ExchangeTranslator::ExchangeTranslator(elemtype_item_t* _elemtypes,
                                       elemstruct_item_t* _elemstructs)
{
  LOG(INFO) << "CTOR ExchangeTranslator";
  elemtype_item_t *elemtype = _elemtypes;
  elemstruct_item_t *elemstruct = _elemstructs;

  while (elemtype && elemtype->name[0])
  {
//    LOG(INFO) << "elemtype: " << elemtype->name;
    m_elemtypes[elemtype->name] = *elemtype;
    elemtype++;
  }

  while (elemstruct && elemstruct->name[0])
  {
//    LOG(INFO) << "elemstruct: " << elemstruct->name;
    m_elemstructs[elemstruct->name] = *elemstruct;
    elemstruct++;
  }
};

//============================================================================
ExchangeTranslator::~ExchangeTranslator()
{
  LOG(INFO) << "DTOR ExchangeTranslator";
//  std::for_each(m_elemtypes.begin(), m_elemtypes.end(), print_elemtype);
//  std::cout << std::endl;
//  std::for_each(m_elemstructs.begin(), m_elemstructs.end(), print_elemstruct);
//  std::cout << std::endl;
};

//============================================================================
// Прочитать из потока столько данных, сколько ожидается форматом Элементарного типа.
// Результат помещается в value.
int ExchangeTranslator::read_lex_value(std::stringstream & buffer,
                                       elemtype_item_t & elemtype,
                                       char *value)
{
  int rc = OK;
  int bytes_to_read = 0;
  char *point;
  char *mantissa;

  value[0] = '\0';
  switch (elemtype.tm_type)
  {
    case TM_TYPE_INTEGER:      // I
      // <число>
      bytes_to_read = atoi(elemtype.format_size);
      break;
    case TM_TYPE_TIME:         // T
      // U<число>
      bytes_to_read = atoi(elemtype.format_size + 1);  // пропустить первый символ 'U'
      break;
    case TM_TYPE_STRING:       // S
      // <число>
      bytes_to_read = atoi(elemtype.format_size);
      break;
    case TM_TYPE_REAL:         // R
      // <цифра>.<цифра> или <число>e<цифра>
      if (NULL != (point = strchr(elemtype.format_size, '.')))
      {
        // Убедимся в том, что формат соотвествует <цифра>.<цифра>
        assert(ptrdiff_t(point - elemtype.format_size) == 1);
        // Формат с фиксированной точкой
        bytes_to_read += elemtype.format_size[0] - '0';        // <цифра>
        bytes_to_read += 1;     // '.'
        bytes_to_read += elemtype.format_size[2] - '0';        // <цифра>
      }
      else
      {
        // Формат с плавающей точкой - 0.000000000e0000
        if (NULL != (mantissa = strchr(elemtype.format_size, 'e')))
        {
          char t[10];

          strncpy(t, elemtype.format_size, ptrdiff_t(mantissa - elemtype.format_size) + 1);
          bytes_to_read += atoi(t);     // мантисса
          bytes_to_read += 1;   // 'e'
          bytes_to_read += atoi(mantissa + 1);  // порядок
        }
        else
        {
          LOG(ERROR) << elemtype.name << ": unsupported size \"" << elemtype.format_size << "\"";
        }
      }
      break;
    case TM_TYPE_LOGIC:        // L
      // <число> (всегда = 1)
      bytes_to_read = atoi(elemtype.format_size);
      assert(bytes_to_read == 1);
      break;
    default:
      LOG(ERROR) << elemtype.name << ": unsupported type " << elemtype.tm_type;
      rc = NOK;
  }

  if (bytes_to_read)
    buffer.get(value, bytes_to_read + 1);

  LOG(INFO) << "read '" << value << "' bytes:" << bytes_to_read + 1 << " [name:" << elemtype.name
            << " type: " << elemtype.tm_type << " size:" << elemtype.format_size << "]";

  return rc;
}

//============================================================================
int ExchangeTranslator::load(std::stringstream & buffer)
{
  //static const char *fname = "ExchangeTranslator::load";
  int rc = OK;

#if 0
  std::string line;
  char service[10];
  char elemstruct[10];
  char elemtype[10];
  char lexem[10];               // лексема, могущая оказаться как UN?, так и C????
  char value[100];
  std::map < const std::string, elemstruct_item_t >::iterator it_es;
  std::map < const std::string, elemtype_item_t >::iterator it_et;

  // Заголовок
  while ((OK == rc) && buffer.good())
  {

    buffer.get(service, 3 + 1);
    // Может быть:
    // UNB    Interchange Header Segment
    // UNH    Message Header segment
    // UNT    Message End Segment
    // UNZ    Interchange End segment
    // APB    Applicative segment header
    // APE
    if (0 == strcmp(service, "UNB"))
    {
      LOG(INFO) << service << ": Interchange Header Segment";
    }
    else if (0 == strcmp(service, "UNH"))
    {
      LOG(INFO) << service << ": Message Header Segment";
    }
    else if (0 == strcmp(service, "UNT"))
    {
      LOG(INFO) << service << ": Message End Segment";
    }
    else if (0 == strcmp(service, "UNZ"))
    {
      LOG(INFO) << service << ": Interchange End segment";
    }
    else if (0 == strcmp(service, "APB"))
    {
      LOG(INFO) << service << ": Applicative segment header";
    }
    else if (0 == strcmp(service, "APE"))
    {
      LOG(INFO) << service << ": ? segment";
    }
    else
    {
      LOG(ERROR) << service << ": unsupported segment";
      rc = NOK;
      break;
    }

    while ((OK == rc) && buffer.good())
    {
      buffer.get(lexem, 3 + 1);
      // Может быть:
      // UNB    Interchange Header Segment
      // UNH    Message Header segment
      // UNT    Message End Segment
      // UNZ    Interchange End segment
      // APB    Applicative segment header
      // APE
      // C????
      if (0 == strcmp(lexem, "UNB"))
      {
        LOG(INFO) << lexem << ": Interchange Header Segment";
        continue;
      }
      else if (0 == strcmp(lexem, "UNH"))
      {
        LOG(INFO) << lexem << ": Message Header Segment";
        continue;
      }
      else if (0 == strcmp(lexem, "UNT"))
      {
        LOG(INFO) << lexem << ": Message End Segment";
        continue;
      }
      else if (0 == strcmp(lexem, "UNZ"))
      {
        LOG(INFO) << lexem << ": Interchange End segment";
        continue;
      }
      else if (0 == strcmp(lexem, "APB"))
      {
        LOG(INFO) << lexem << ": Applicative segment header";
        continue;
      }
      else if (0 == strcmp(lexem, "APE"))
      {
        LOG(INFO) << lexem << ": ? segment";
        continue;
      }
      else
      {
        // Первые 3 символа от C???? уже содержатся в lexem, прочитать оставшиеся два
        strcpy(elemstruct, lexem);
        buffer.get(elemstruct + 3, 2 + 1);

        it_es = m_elemstructs.find(elemstruct);
        if (it_es != m_elemstructs.end())
        {

          std::cout << elemstruct << std::endl;

          // Читать все элементарные типы, содержащиеся в (*it_es)
          for (size_t idx = 0; idx < (*it_es).second.num_fileds; idx++)
          {

            buffer.get(elemtype, 5 + 1);
            it_et = m_elemtypes.find(elemtype);
            if (it_et != m_elemtypes.end())
            {
              if (0 == strcmp((*it_et).second.name, (*it_es).second.fields[idx]))
              {
                // Прочитать данные этого элементарного типа
                rc = read_lex_value(buffer, (*it_et).second, value);
              }
              else
              {
                // Прочитали лексему, не принадлежащую текущей elemstruct
                LOG(ERROR) << "expect:" << (*it_es).second.
                    fields[idx] << ", read:" << elemtype;
                rc = NOK;
                //              break;
              }
            }
            else
            {
              // Прочитали неизвестную лексему
              LOG(ERROR) << "expect:" << (*it_es).second.fields[idx] << ", read unknown:" << elemtype;
              rc = NOK;
              //            break;
            }                   // конец блока if "не нашли элементарный тип"

          }                     // конец цикла чтения всех полей текущего композитного типа elemstruct

        }                       // конец блока if "этот композитный тип известен"
        else
        {
          LOG(ERROR) << elemstruct << ": unsupported element";
        }

      }                         // конец проверки, что это не U??, а C????

    }                           // конец цикла "пока данные не закончились"
  } // конец блока if "корректный заголовок"

#endif
  return rc;
}

#if 1
//============================================================================
//  ROLE
//      This function convertes all transmitted data in the right form and decide
//      if the converted data are to be stored into the SMAD or to be sent to SIDL
//
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//      When acquired data must be converted and written into the SMAD or sent to SIDL
//
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_acq_dac_Switch(const char *s_ILongFileName, const char *s_IAcqSiteId)
{
  static const char* fname = "esg_acq_dac_Switch";

  int i_Status;
  int i_RetStatus = OK;
  uint32_t i_ApplLength;
  uint32_t ii;
  uint32_t i_MsgApplNb;

  char s_Trace[70 + 1];

  esg_esg_t_HeaderInterChg r_HeaderInterChg;
  esg_esg_t_HeaderMsg r_HeaderMsg;
  esg_esg_t_HeaderAppl r_HeaderAppl;

  FILE *pi_FileId = NULL;
  int32_t i_LgInterChg;
  int32_t i_LgMsg;
  int32_t i_LgAppl;
  long i_FileDepl;

  int32_t i_AlOpNb;         // number of operative alarms
  int32_t i_NopAlNb;        // number of non operative alarms
  esg_esg_t_HistAlElem ar_AlarmsOp[GOF_D_LST_ALAmax];
  esg_esg_t_HistAlElem ar_NopAlarms[GOF_D_LST_ALAmax];
  char s_HistFileName[100];
  char s_FileName[100];

  bool b_LastSegment;  // last segment indication

  //............................................................................
  i_RetStatus = OK;
  i_Status = OK;
  i_AlOpNb = 0;
  i_NopAlNb = 0;

  // Open the file
  // Read the interchange header
  // Read the the message header
  //----------------------------------------------------------------------------
  i_Status = esg_esg_fil_HeadFileRead(s_ILongFileName,
                                      &r_HeaderInterChg,
                                      &r_HeaderMsg,
                                      &i_LgInterChg,
                                      &i_LgMsg,
                                      &pi_FileId);

  i_FileDepl = i_LgInterChg + i_LgMsg;

  if (i_Status == OK)
  {
    // Get segments number from the message
    //----------------------------------------------------------------------------
    i_MsgApplNb = r_HeaderMsg.h_MsgApplNb;
    i_ApplLength = 0;

    for (ii = 0; ((ii < i_MsgApplNb) && (i_Status == OK)); ii++)
    {
      // Read and decode the segment header
      //------------------------------------------
      i_Status = esg_esg_fil_HeadApplRead(pi_FileId, &r_HeaderAppl, &i_LgAppl);

      // Process only segments with data
      //-------------------------------------
      if (i_Status == OK)
      {
        // Get the segment length
        // ---------------------------
        i_ApplLength = r_HeaderAppl.i_ApplLength;

        std::cout << "Segment Header :" << std::endl;
        std::cout << "\tSegment Number=" << r_HeaderAppl.h_ApplOrderNb
                  << " Segment Type=" << r_HeaderAppl.i_ApplType
                  << " Segment Length=" << r_HeaderAppl.i_ApplLength
                  << std::endl;
        std::cout << "Segment Body :" << std::endl;

        // Read the rest of the segment
        //----------------------------------
        switch (r_HeaderAppl.i_ApplType)
        {
          // ALARM
          // -------------------
          case ECH_D_EDI_EDDSEG_ALARM:
#if 0
            i_Status = esg_acq_dac_AlarmAcq(pi_FileId,
                                            i_LgAppl,
                                            i_ApplLength,
                                            s_IAcqSiteId);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif
            break;

          // CHANGE HOUR
          // --------------------------
          case ECH_D_EDI_EDDSEG_CHGHOUR:

#if 0
            i_Status = esg_acq_dac_ChgHourAcq(pi_FileId,
                                              i_LgAppl,
                                              i_ApplLength,
                                              s_IAcqSiteId);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif
            break;

          // TI
          // -------------------
          case ECH_D_EDI_EDDSEG_TI:
#if 0
            i_Status = esg_acq_dac_TeleinfoAcq(pi_FileId,
                                               i_LgAppl,
                                               i_ApplLength,
                                               s_IAcqSiteId,
                                               r_HeaderInterChg.
                                               d_InterChgDate);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif
            break;

          // TI HISTORICS
          // --------------------------
          case ECH_D_EDI_EDDSEG_HISTTI:

            // indication of last segment for database update
            if (ii == i_MsgApplNb - 1)
            {
              b_LastSegment = true;
            }
            else
            {
              b_LastSegment = false;
            }

            sprintf(s_Trace, " Segment Number= %d Last segment= %d\n", ii + 1, b_LastSegment);
            LOG(INFO) << fname << s_Trace;
#if 0
            i_Status = esg_acq_dac_HistTiAcq(b_LastSegment,
                                             pi_FileId,
                                             i_ApplLength,
                                             s_IAcqSiteId);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif
            break;

          // HISTORIC ALARM
          // ----------------------------
          case ECH_D_EDI_EDDSEG_HISTALARM:
#if 0
            i_Status = esg_acq_dac_HistAlAcq(pi_FileId,
                                             i_LgAppl,
                                             i_ApplLength,
                                             s_IAcqSiteId,
                                             &i_AlOpNb,
                                             (esg_esg_t_HistAlElem *) &ar_AlarmsOp,
                                             &i_NopAlNb,
                                             (esg_esg_t_HistAlElem *) &ar_NopAlarms);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif
            break;

          // ORDER
          // -------------------
          case ECH_D_EDI_EDDSEG_ORDER:
#if 0
            i_Status = esg_acq_dac_OrderAcq(pi_FileId,
                                            i_LgAppl,
                                            r_HeaderAppl,
                                            r_HeaderInterChg,
                                            s_IAcqSiteId);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif

            break;
          // ORDER responses
          // -----------------------------
          case ECH_D_EDI_EDDSEG_ORDERRESP:
#if 0
            i_Status = esg_acq_dac_OrderRespAcq(pi_FileId,
                                                i_LgAppl,
                                                r_HeaderAppl,
                                                r_HeaderInterChg,
                                                s_IAcqSiteId);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif

            break;
          // INCIDENT
          // -----------------------
          case ECH_D_EDI_EDDSEG_INCIDENT:
#if 0
            i_Status = esg_acq_dac_IncidentAcq(pi_FileId,
                                               i_LgAppl,
                                               r_HeaderAppl,
                                               r_HeaderInterChg,
                                               s_IAcqSiteId);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif

            break;

          // OUTLINE THRESHOLDS
          // --------------------------------
          case ECH_D_EDI_EDDSEG_MULTITHRES:
#if 0
            i_Status = esg_acq_dac_MultiThresAcq(pi_FileId,
                                                 i_ApplLength,
                                                 s_IAcqSiteId);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif
            break;

          // THRESHOLDS
          // ------------------------
          case ECH_D_EDI_EDDSEG_THRESHOLD:
#if 0
            i_Status = esg_acq_dac_ThresholdAcq(pi_FileId,
                                                i_ApplLength,
                                                s_IAcqSiteId);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif
            break;

          // HISTORIC OF HISTORIZED TI
          // ---------------------------------------
          case ECH_D_EDI_EDDSEG_HISTHISTTI:
            strcpy(s_FileName, s_ILongFileName);
            sprintf(s_HistFileName, "%s/%s", dirname(s_FileName), "SIG_TIHHIST.DAT" /*ESG_ACQ_DAC_D_TIHHISTFNAME*/);
#if 0
            i_Status = esg_acq_dac_HHistTiAcq(i_MsgApplNb,
                                              i_ApplLength,
                                              s_IAcqSiteId,
                                              s_HistFileName,
                                              pi_FileId,
                                              &ii);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif
            break;

          // segment with dispatcher name
          // ---------------------------------------
          case ECH_D_EDI_EDDSEG_DISPN:
#if 0
            i_Status = esg_acq_dac_DispNameAcq(pi_FileId,
                                               i_LgAppl,
                                               i_ApplLength,
                                               s_IAcqSiteId,
                                               r_HeaderInterChg.
                                               d_InterChgDate);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif
            break;

          // ACD List
          // ----------------------
          case ECH_D_EDI_EDDSEG_ACDLIST:
#if 0
            i_Status = esg_acq_dac_ACDList(pi_FileId,
                                           i_ApplLength,
                                           s_IAcqSiteId);
#else
            LOG(ERROR) << fname << ": esg_acq_dac_AlarmAcq";
#endif
            break;

          case ECH_D_EDI_EDDSEG_STATE:
            LOG(ERROR) << fname << ": Segment Type= Distant state reply";
            i_Status = processing_STATE(pi_FileId,
                                       i_LgAppl,
                                       i_ApplLength,
                                       s_IAcqSiteId);
            break;

          case ECH_D_EDI_EDDSEG_REQUEST:
            LOG(ERROR) << fname << ": Segment Type= Request";
           /* i_Status = processing_REQUEST(pi_FileId,
                                       i_LgAppl,
                                       i_ApplLength,
                                       s_IAcqSiteId);*/
            break;

          case ECH_D_EDI_EDDSEG_REPLY:
            LOG(ERROR) << fname << ": ECH_D_EDI_EDDSEG_REPLY";
            break;

          case ECH_D_EDI_EDDSEG_DIFFUSION:
            LOG(ERROR) << fname << ": ECH_D_EDI_EDDSEG_REPLY";
            break;

          case ECH_D_EDI_EDDSEG_ACDQUERY		:
            LOG(ERROR) << fname << ": Segment Type= ACD Query";
            break;

          default:
            LOG(ERROR) << fname << ": unsupported segment type #" << r_HeaderAppl.i_ApplType;
            break;
        }  // end Switch

        // File deplacement
        // ----------------
        if (r_HeaderAppl.i_ApplType != ECH_D_EDI_EDDSEG_HISTHISTTI)
        {
          i_FileDepl += (i_ApplLength + i_LgAppl + ECH_D_SEGLABELLG);
          i_Status = fseek ( pi_FileId, i_FileDepl, SEEK_SET ) ;
        }
      }                         /* End if: Segment with data */
    }                           /* for to process segments of the message */
  }                             /* End if */

  // Close the file (even if not OK)
  //----------------------------------------------------------------------------
  if (pi_FileId) {
    fclose(pi_FileId);
    pi_FileId = NULL;
  }

  // Put files to SINF for historized alarms
  //----------------------------------------------------------------------------
  if ((i_Status == OK) && (i_AlOpNb > 0))
  {
    strcpy(s_FileName, s_ILongFileName);
    sprintf(s_HistFileName, "%s/%s", dirname(s_FileName), "SIG_OPEALHIST.DAT" /*ESG_ACQ_DAC_D_OPEALHISTFNAME*/);
    i_Status = esg_acq_dac_HistAlPut(s_IAcqSiteId,
                                     s_HistFileName,
                                     GOF_D_LST_ALA_OPE,
                                     i_AlOpNb,
                                     (esg_esg_t_HistAlElem*) &ar_AlarmsOp);
  }

  if ((i_Status == OK) && (i_NopAlNb > 0))
  {
    strcpy(s_FileName, s_ILongFileName);
    sprintf(s_HistFileName, "%s/%s", dirname(s_FileName), "SIG_NOPALHIST.DAT" /*ESG_ACQ_DAC_D_NOPALHISTFNAME*/);
    i_Status = esg_acq_dac_HistAlPut(s_IAcqSiteId,
                                     s_HistFileName,
                                     GOF_D_LST_ALA_NON_OPE,
                                     i_NopAlNb,
                                     (esg_esg_t_HistAlElem*) &ar_NopAlarms);
  }

  i_RetStatus = i_Status;

  return (i_RetStatus);
}
#else
#warning "Функция esg_acq_dac_Switch должна использоваться уровенем выше"
#endif

//----------------------------------------------------------------------------
//  FUNCTION            esg_acq_dac_HistAlPut
//  FULL MAME           Put Historized Alarms for SINF
//----------------------------------------------------------------------------
//  ROLE
//    Processing of the the historized alarm notification
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//    Receive of a device of alrms file deposit message
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//
//    - open the file to be furnished to SINF
//    - write the origin DIPL
//    - write the list type
//    - write the count of alarms in the file
//    - FOR (each alarm)
//    -    write the record
//
//    - END_FOR
//    - close the file
//----------------------------------------------------------------------------

int ExchangeTranslator::esg_acq_dac_HistAlPut (
             const gof_t_UniversalName s_IDistantSite,  // name of distant site
             const char* s_ILongFileName,         // name of the file
             const int16_t h_IAlListType,           // alarms type
             const uint32_t i_IHistAlNb,            // number of alarms
             const esg_esg_t_HistAlElem* pr_IHistAlarms// description of alarms
  )
//----------------------------------------------------------------------------
{
  static const char *fname = "esg_acq_dac_HistAlPut";
  int i_RetStatus = OK;     // returned status
  FILE *pi_FileId;          // file pointer
  uint16_t i_i;         // index
  char s_CurStr[256];  // current string
  esg_esg_t_HistAlElem *pr_CurHistAl;    // work variable
  //1 int32_t i_MsgLength;  // message length
  int32_t i_AlListType;     // work variable

// (CD)    open the file to be furnished to SINF
// --------------------------------------------------------------------------
//1  i_RetStatus = esg_esg_fil_FileOpen (s_ILongFileName, ESG_ESG_FIL_D_WRITEMODE, &pi_FileId);
  if (NULL != (pi_FileId = fopen(s_ILongFileName, "w")))
  {
// (CD)    write the origin DIPL
// --------------------------------------------------------------------------
#if 0
    sprintf (s_CurStr, "%s\n", s_IDistantSite);
    i_RetStatus = esg_esg_fil_StringWrite (pi_FileId, s_CurStr);
#else
    std::cout << "Origin: " << s_IDistantSite;
#endif

// (CD)    write the list type
// --------------------------------------------------------------------------
    if (i_RetStatus == OK)
    {
      i_AlListType = GOF_D_NON_OPE_ALA;
      if (h_IAlListType == GOF_D_LST_ALA_OPE)
      {
        i_AlListType = GOF_D_OPE_ALA;
      }
#if 0
      sprintf (s_CurStr, "%d\n", i_AlListType);
      i_RetStatus = esg_esg_fil_StringWrite (pi_FileId, s_CurStr);
#else
      std::cout << " AlListType:" << i_AlListType;
#endif
    }

// (CD)    write the count of alarms in the file
// --------------------------------------------------------------------------
    if (i_RetStatus == OK)
    {
#if 0
      sprintf (s_CurStr, "%d\n", i_IHistAlNb);
      i_RetStatus = esg_esg_fil_StringWrite (pi_FileId, s_CurStr);
#else
      std::cout << " HistAlNb:" << i_IHistAlNb;
#endif
    }

// (CD)    FOR (each alarm)
// --------------------------------------------------------------------------
    if (i_RetStatus == OK)
    {
      pr_CurHistAl = (esg_esg_t_HistAlElem *) pr_IHistAlarms;

      for (i_i = 0; i_i < i_IHistAlNb; i_i++)
      {
// (CD)        write the record universal : name date type degree state value
// --------------------------------------------------------------------------
          sprintf (s_CurStr, "%s %ld %ld %d %d %d %e\n",
               pr_CurHistAl->s_AlRef,
               pr_CurHistAl->d_AlDate.tv_sec,
               pr_CurHistAl->d_AlDate.tv_usec,
               pr_CurHistAl->o_AlType,
               pr_CurHistAl->o_AlDeg,
               pr_CurHistAl->o_AlState,
               pr_CurHistAl->g_AlVal);

#if 0
          i_RetStatus = esg_esg_fil_StringWrite (pi_FileId, s_CurStr);
#else
      std::cout << " Attributes: " << s_CurStr << std::endl; 
#endif
          pr_CurHistAl = pr_CurHistAl + 1;

      }
    }

// (CD)    close the file
// --------------------------------------------------------------------------
    if (pi_FileId) {
      fclose(pi_FileId);
      pi_FileId = NULL;
    }
  }

#if 0
  sig_t_msg_AlFile r_MsgBody;   // message body
// (CD)    Inform SINF that file is ready
// --------------------------------------------------------------------------
  if (i_RetStatus == OK)
  {
      i_MsgLength = sizeof (r_MsgBody);
      memset (&r_MsgBody, 0, i_MsgLength);
      strcpy (r_MsgBody.s_file_name, s_ILongFileName);

      i_RetStatus = sig_ext_msg_p_InpSendMessageToSinf (SIG_D_MSG_ALFILE,
                            i_MsgLength,
                            &r_MsgBody);
  }
#else
      LOG(WARNING) << "sig_ext_msg_p_InpSendMessageToSinf";
#endif

  // Global message
  // --------------
  if (i_RetStatus != OK)
  {
      LOG(ERROR) << fname << ": rc=" << i_RetStatus;
  }

  return (i_RetStatus);
}
// End esg_acq_dac_HistAlPut -------------------------------------------------

//============================================================================
//  FUNCTION                    esg_esg_fil_HeadFileRead
//  FULL MAME                   Read A File Header
//----------------------------------------------------------------------------
//  ROLE
//      The purpose of this routine is to read a header from a file.
//      Read mode for file is checked before reading.
//      The kind of header is EDI and is composed of an Interchange header
//      and a Message header.
//      Given file is open before reading.
//
//----------------------------------------------------------------------------
//  RETURNED EXCEPTIONS
//      report  file has not read mode
//      report  cannot open file
//      report  cannot read file
//
//----------------------------------------------------------------------------
//  CONSULTED GLOBAL VARIABLES
//      esg_esg_fil_ar_OpenFileArr      Array of open files
//
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//      - Check if file has read mode
//      - If read is available
//      -       Open the file
//      -       Read the interchange header from the file
//      -       Decode the interchange header
//      -       Read the message header from the file
//      -       Decode the message header
//
//----------------------------------------------------------------------------
//  ERRORS AND EXCEPTIONS PROCESSING
//      If the file cannot be read, the function logs an error and returns.
//
//----------------------------------------------------------------------------

int ExchangeTranslator::esg_esg_fil_HeadFileRead(const char* s_ILongName,
                                      esg_esg_t_HeaderInterChg* pr_OHeadInterChg,
                                      esg_esg_t_HeaderMsg* pr_OHeadMsg,
                                      int32_t* i_OLgInterChg,
                                      int32_t* i_OLgMsg,
                                      FILE** pi_OFileId)
{
  static const char* fname = "esg_esg_fil_HeadFileRead";
  int i_RetStatus = OK;     // routine report
  int i_Status = OK;        // called routines report
  int32_t i_i;              // loop counter
  char s_CodedData[ECH_D_APPLSEGLG + 1];            // coded data as string
  uint32_t i_LgCodedData;                           // length of coded data
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;  // qualifiers
  esg_esg_edi_t_StrComposedData r_InternalCData;    // Internal composed data

  //............................................................................
  i_RetStatus = ESG_ESG_D_ERR_CANNOTREADFILE;
  *i_OLgInterChg = 0;
  *i_OLgMsg = 0;

  // Open the given file
  // --------------------------------------------------------------------------
  if (NULL != (*pi_OFileId = fopen(s_ILongName, "r")))
  {
    i_i = 0;
    r_InternalCData.ar_EDataTable[i_i].i_type = (uint16_t) FIELD_TYPE_INT32;
    i_i++;
    r_InternalCData.ar_EDataTable[i_i].i_type = (uint16_t) FIELD_TYPE_FLOAT;
    i_i++;
    r_InternalCData.ar_EDataTable[i_i].i_type = (uint16_t) FIELD_TYPE_STRING;
    r_InternalCData.ar_EDataTable[i_i].u_val.r_Str.i_LgString = sizeof(gof_t_UniversalName); // NB: Из-за совместимости с ГОФО - точно 32 байта
    i_i++;
    r_InternalCData.ar_EDataTable[i_i].i_type = (uint16_t) FIELD_TYPE_STRING;
    r_InternalCData.ar_EDataTable[i_i].u_val.r_Str.i_LgString = sizeof(gof_t_UniversalName);
    i_i++;
    r_InternalCData.ar_EDataTable[i_i].i_type = (uint16_t) FIELD_TYPE_DATE;
    i_i++;
    r_InternalCData.ar_EDataTable[i_i].i_type = (uint16_t) FIELD_TYPE_UINT16;
    i_i++;

    r_InternalCData.i_NbEData = i_i;

    r_QuaCData.b_QualifyUse = (bool) false;
    r_QuaCData.i_QualifyValue = (uint32_t) 0;
    r_QuaCData.b_QualifyExist = (bool) false;
  }
  else
  {
    i_RetStatus = ESG_ESG_D_ERR_NOTREADABLE;
    i_Status = NOK;
    LOG(ERROR) << fname << ": rc=" << i_RetStatus;
  }

  if (i_Status == OK)
  {
    i_Status = esg_esg_edi_GetLengthCData(ECH_D_INCHSEGHICD,
                                          &r_QuaCData,
                                          &r_InternalCData,
                                          &i_LgCodedData);
  }

  if (i_Status == OK)
  {
    i_Status = esg_esg_fil_StringRead(*pi_OFileId, i_LgCodedData, s_CodedData);

    if (i_Status != OK)
    {
      LOG(ERROR) << fname << ": rc=" << i_Status;
    }
  }

  // Decode interchange header
  // --------------------------------------------------------------------------
  if (i_Status == OK)
  {
    i_Status =
        esg_esg_edi_HeaderInterChgDecoding(s_CodedData,
                                           ECH_D_APPLSEGLG,
                                           &i_LgCodedData,
                                           pr_OHeadInterChg);
  }

  // ==========================================================================
  if (i_Status == OK)
  {
    *i_OLgInterChg = i_LgCodedData;
    i_i = 0;
    r_InternalCData.ar_EDataTable[i_i].i_type = (uint16_t) FIELD_TYPE_UINT16;
    i_i++;

    r_InternalCData.ar_EDataTable[i_i].i_type = (uint16_t) FIELD_TYPE_UINT32;
    i_i++;
    r_InternalCData.ar_EDataTable[i_i].i_type = (uint16_t) FIELD_TYPE_UINT16;
    i_i++;
    r_InternalCData.i_NbEData = i_i;

    r_QuaCData.b_QualifyUse   = false;
    r_QuaCData.i_QualifyValue = 0;
    r_QuaCData.b_QualifyExist = false;

    i_Status = esg_esg_edi_GetLengthCData(ECH_D_MSGSEGHICD,
                                          &r_QuaCData,
                                          &r_InternalCData,
                                          &i_LgCodedData);
  }

  if (i_Status == OK)
  {
    i_Status = esg_esg_fil_StringRead(*pi_OFileId, i_LgCodedData, s_CodedData);
  }

  // Decode message header
  // --------------------------------------------------------------------------
  if (i_Status == OK)
  {
    i_Status = esg_esg_edi_HeaderMsgDecoding(s_CodedData,
                                             ECH_D_APPLSEGLG,
                                             &i_LgCodedData,
                                             pr_OHeadMsg);
  }

  if (i_Status == OK)
  {
    *i_OLgMsg = i_LgCodedData;
  }

  i_RetStatus = i_Status;

  if ((i_RetStatus != OK) && *pi_OFileId)
  {
    fclose(*pi_OFileId);
    *pi_OFileId = NULL;
  }

  return (i_RetStatus);
}//-END esg_esg_fil_HeadFileRead ---------------------------------------------

//============================================================================
//----------------------------------------------------------------------------
//  FUNCTION            esg_esg_edi_HeaderMsgDecoding
//  FULL NAME           Decoding from EDI format in internal interchange header
//----------------------------------------------------------------------------
//  ROLE
//  Transforms message header from basis type in EDI format to internal format
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to obtain from the ASCII character string in EDI format
//  the internal format of message header
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//  To provide message header in internal format
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_HeaderMsgDecoding(
      // Input parameters
      const char* ps_ICodedCData,       // coded composed data
      const uint32_t i_ILgMaxCodedCData,// max length of coded composed data
      // Output parameters
      uint32_t* pi_OLgCodedCData,       // length of coded composed data
      esg_esg_t_HeaderMsg* pr_OInternalHMsg// Internal message header
      )
{
//----------------------------------------------------------------------------*/
  static const char* fname = "esg_esg_edi_HeaderMsgDecoding";
 // internal composed data table
  esg_esg_edi_t_StrComposedData r_HMInternalCData;
 // indice internal composed data table
  uint32_t i_IndIEData;
 // qualifier interface structure
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
 // return status of the function
  int i_RetStatus = OK;

//............................................................................*/
// (CD) providing coded composed data                                         */
  i_IndIEData = 0;
  r_HMInternalCData.i_NbEData = ESG_ESG_D_HMSGNBEDATA;
  r_HMInternalCData.ar_EDataTable[i_IndIEData++].i_type = FIELD_TYPE_UINT16;
  r_HMInternalCData.ar_EDataTable[i_IndIEData++].i_type = FIELD_TYPE_UINT32;
  r_HMInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT16;

  memset(&r_QuaCData, 0, sizeof(esg_esg_edi_t_StrQualifyComposedData));
  i_RetStatus = esg_esg_edi_ComposedDataDecoding(ECH_D_MSGSEGHICD,
                                                 ps_ICodedCData,
                                                 i_ILgMaxCodedCData,
                                                 pi_OLgCodedCData,
                                                 &r_QuaCData,
                                                 &r_HMInternalCData);
  if (i_RetStatus != OK)
  {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_MSGSEGHICD;
  }
  else
  {
    i_IndIEData = 0;
    pr_OInternalHMsg->h_MsgOrderNb  = r_HMInternalCData.ar_EDataTable[i_IndIEData++].u_val.h_Uint16;
    pr_OInternalHMsg->i_MsgType     = r_HMInternalCData.ar_EDataTable[i_IndIEData++].u_val.i_Uint32;
    pr_OInternalHMsg->h_MsgApplNb   = r_HMInternalCData.ar_EDataTable[i_IndIEData].u_val.h_Uint16;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_HeaderMsgDecoding------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION            esg_esg_edi_ComposedDataDecoding
//  FULL NAME           Decoding of composed data in internal format
//----------------------------------------------------------------------------
//  ROLE
//  Transforms composed data from basis type in EDI format to internal format
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to obtain from the ASCII character string in EDI
//  format the internal format of an composed data.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//      To provide DCD entity in internal format.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_ComposedDataDecoding(
               // Input parameters
               const char* s_IIdCData,              // Composed data DCD identifier
               const char* ps_ICodedCData,          // pointer to coded composed data
               const uint32_t i_ILgMaxCodedCData,   // max length of coded composed data
               //       Output parameters
               uint32_t* pi_OLgCodedCData,          // length of coded composed data
// The function gives an indicator whether a LED exists and the possible  value of this LED if found
               esg_esg_edi_t_StrQualifyComposedData* pr_OQuaCData,// Qualify of internal composed data
               // Input-Output parameters
// in entry, the number of elementary data (i_NbEData) must be filled
               esg_esg_edi_t_StrComposedData* pr_IOInternalCData // Internal composed data
               )
{
  static const char* fname = "esg_esg_edi_ComposedDataDecoding";
 // list of elementary data for the composed data
  elemstruct_item_t* r_ListEData = NULL;
 // identifier and label of composed data
  char s_IdCData[ECH_D_COMPIDLG + 1];
  char s_IdEData[ECH_D_ELEMIDLG + 1];
  char s_LabelCData[ECH_D_SEGLABELLG + 1];
 // table indices and number of entities
  uint32_t i_IndLEData = 0;
  uint32_t i_IndICData = 0;
  uint32_t i_NbInternalEData = 0;
  uint32_t i_NbEData = 0;
  uint32_t i_LgMaxCodedCData = 0;
  uint32_t i_LgCodedEData = 0;
 // coded composed data string
  char *ps_CodedCData = NULL;
 // internal elementary data
  esg_esg_edi_t_StrElementaryData *pr_InternalEData;
  esg_esg_edi_t_StrElementaryData r_InternalQuaCData;
 // elementary data length quality indicator
  bool b_LgQuaEData = false;
 // return status of the function
  int i_RetStatus = OK;
 // logged text
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];

  *pi_OLgCodedCData = 0;

  // obtaining the composition of the composed data
  if (NULL != (r_ListEData = esg_esg_odm_ConsultExchCompArr(s_IIdCData)))
  {
    i_RetStatus = OK;
    i_LgMaxCodedCData = i_ILgMaxCodedCData;
    ps_CodedCData = (char *) ps_ICodedCData;

    if (i_LgMaxCodedCData < ECH_D_SEGLABELLG)
    {
      i_RetStatus = ESG_ESG_D_ERR_DCDLGTOOSHORT;
      sprintf(s_LoggedText, "lg %d of %s to treat : provided lg %d ", ECH_D_SEGLABELLG, s_IIdCData, i_ILgMaxCodedCData);
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_LoggedText;
    }
  }
  else {
    LOG(ERROR) << fname << ": unable to find " << s_IIdCData;
    i_RetStatus = NOK;
  }

  if (i_RetStatus == OK)
  {
// (CD) Controling of composed data label
    memset(s_LabelCData, 0, sizeof(s_LabelCData));
    memcpy(s_LabelCData, ps_CodedCData, ECH_D_SEGLABELLG);

    if ((strcmp(s_IIdCData, ECH_D_INCHSEGHICD) == 0))
    {
      if ((strcmp(s_LabelCData, ECH_D_STR_ICHSEGLABEL) != 0))
      {
        i_RetStatus = ESG_ESG_D_ERR_BADSEGLABEL;
      }
      else
      {
        ps_CodedCData = ps_CodedCData + ECH_D_SEGLABELLG;
        i_LgMaxCodedCData = i_LgMaxCodedCData - ECH_D_SEGLABELLG;
      }
    }

    if ((strcmp(s_IIdCData, ECH_D_INCHSEGEICD) == 0))
    {
      if ((strcmp(s_LabelCData, ECH_D_STR_ICESEGLABEL) != 0))
      {
        i_RetStatus = ESG_ESG_D_ERR_BADSEGLABEL;
      }
      else
      {
        ps_CodedCData = ps_CodedCData + ECH_D_SEGLABELLG;
        i_LgMaxCodedCData = i_LgMaxCodedCData - ECH_D_SEGLABELLG;
      }
    }

    if ((strcmp(s_IIdCData, ECH_D_MSGSEGHICD) == 0))
    {
      if ((strcmp(s_LabelCData, ECH_D_STR_MSGHSEGLABEL) != 0))
      {
        i_RetStatus = ESG_ESG_D_ERR_BADSEGLABEL;
      }
      else
      {
        ps_CodedCData = ps_CodedCData + ECH_D_SEGLABELLG;
        i_LgMaxCodedCData = i_LgMaxCodedCData - ECH_D_SEGLABELLG;
      }
    }

    if ((strcmp(s_IIdCData, ECH_D_MSGSEGEICD) == 0))
    {
      if ((strcmp(s_LabelCData, ECH_D_STR_MSGESEGLABEL) != 0))
      {
        i_RetStatus = ESG_ESG_D_ERR_BADSEGLABEL;
      }
      else
      {
        ps_CodedCData = ps_CodedCData + ECH_D_SEGLABELLG;
        i_LgMaxCodedCData = i_LgMaxCodedCData - ECH_D_SEGLABELLG;
      }
    }

    if ((strcmp(s_IIdCData, ECH_D_APPLSEGHICD) == 0))
    {
      if ((strcmp(s_LabelCData, ECH_D_STR_APPHSEGLABEL) != 0))
      {
        i_RetStatus = ESG_ESG_D_ERR_BADSEGLABEL;
      }
      else
      {
        ps_CodedCData = ps_CodedCData + ECH_D_SEGLABELLG;
        i_LgMaxCodedCData = i_LgMaxCodedCData - ECH_D_SEGLABELLG;
      }
    }

    if (i_RetStatus != OK)
    {
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdCData;
    }

  }

  if (i_RetStatus == OK)
  {
    if (i_LgMaxCodedCData < ECH_D_COMPIDLG)
    {
      i_RetStatus = ESG_ESG_D_ERR_DCDLGTOOSHORT;
      sprintf(s_LoggedText, "lg %d of %s to treat : provided lg %d ", ECH_D_COMPIDLG, s_IIdCData, i_ILgMaxCodedCData);
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_LoggedText;
    }
  }

  if (i_RetStatus == OK)
  {
// (CD) controling composed data identifier
    memset(s_IdCData, 0, sizeof(s_IdCData));
    memcpy(s_IdCData, ps_CodedCData, ECH_D_COMPIDLG);
    if ((strcmp(s_IIdCData, s_IdCData) != 0))
    {
      i_RetStatus = ESG_ESG_D_ERR_BADDCDIDENT;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdCData;
    }
    else
    {
      i_LgMaxCodedCData = i_LgMaxCodedCData - ECH_D_COMPIDLG;
      ps_CodedCData = ps_CodedCData + ECH_D_COMPIDLG;
    }
  }

  if (i_RetStatus == OK)
  {
// (CD) controling coherence of elementary data list and internal composed data table
    if (i_LgMaxCodedCData < ECH_D_ELEMIDLG)
    {
      i_RetStatus = ESG_ESG_D_ERR_DCDLGTOOSHORT;
      sprintf(s_LoggedText, "lg %d of %s to treat : provided lg %d ", (ECH_D_ELEMIDLG + ECH_D_COMPIDLG), s_IIdCData, i_ILgMaxCodedCData);
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_LoggedText;
    }
    else
    {
      i_NbInternalEData = pr_IOInternalCData->i_NbEData;
      memset(s_IdEData, 0, sizeof(s_IdEData));
      memcpy(s_IdEData, ps_CodedCData, ECH_D_ELEMIDLG);
      i_NbEData = r_ListEData->num_fileds;

// Treatment for variable string length elementary data
// A special L1800 data is reserved for string length. So, the number of effective elementary data must be reduced of 1
      for (i_IndLEData = 0; i_IndLEData < r_ListEData->num_fileds; i_IndLEData++)
      {
        if ((strcmp(r_ListEData->fields[i_IndLEData].field, ECH_D_QUAVARLGED_IDED)) == 0)
        {
          i_NbEData -= 1;
        }
      }

// Treatment for LED elem data
// A special elem data is reserved for TI identification, called LED
// An LED is considered if the special ECH_D_QUALED_IDED identifier is found and if the number
// of elem data is equal to the number of internal fields
// incremented of 1 (the LED is not described in internal fields)
      if ((strcmp(s_IdEData, (char *) ECH_D_QUALED_IDED) == 0))
      {
        if (i_NbEData == (i_NbInternalEData + 1))
        {
          pr_OQuaCData->b_QualifyExist = true;
          i_NbInternalEData += 1;
        }
        else
        {
          pr_OQuaCData->b_QualifyExist = false;
        }
      }

      if (i_NbEData != i_NbInternalEData)
      {
        i_RetStatus = ESG_ESG_D_ERR_NBEDINTERDCD;
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdCData;
      }
    }
  }

  if (i_RetStatus == OK)
  {
// (CD) decoding of each elementary data
    i_IndLEData = 0;
    i_IndICData = 0;
    pr_InternalEData = (esg_esg_edi_t_StrElementaryData *) & pr_IOInternalCData->ar_EDataTable[i_IndICData];

    while ((i_IndLEData < r_ListEData->num_fileds) && (i_RetStatus == OK))
    {

      if ((pr_OQuaCData->b_QualifyExist) && (i_IndLEData == ESG_ESG_EDI_D_QUAEDATAPOSITION))
      {
        r_InternalQuaCData.i_type = FIELD_TYPE_UINT32;
        pr_InternalEData = &r_InternalQuaCData;
      }

      if ((strcmp(r_ListEData->fields[i_IndLEData].field, ECH_D_QUAVARLGED_IDED)) == 0)
      {
        b_LgQuaEData = true;
        i_IndLEData++;
        if (i_IndLEData == r_ListEData->num_fileds)
        {
          i_RetStatus = ESG_ESG_D_ERR_BADDCDDEDLIST;
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdCData;
        }
      }
      else
      {
        b_LgQuaEData = false;
      }

      if (i_RetStatus == OK)
      {
        // Providing the length of ASCII complete coded elementary data
        i_RetStatus = esg_esg_edi_ElementaryDataDecoding(r_ListEData->fields[i_IndLEData].field,
                                                         b_LgQuaEData,
                                                         ps_CodedCData,
                                                         i_LgMaxCodedCData,
                                                         &i_LgCodedEData,
                                                         pr_InternalEData);

        if (i_RetStatus != OK)
        {
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << r_ListEData->fields[i_IndLEData].field;
        }
      }

      if (i_RetStatus == OK)
      {
        // get next saved address for coded elementary data and next elementary data of the list
        i_LgMaxCodedCData = i_LgMaxCodedCData - i_LgCodedEData;
        ps_CodedCData = ps_CodedCData + i_LgCodedEData;
        if ((pr_OQuaCData->b_QualifyExist) && (i_IndLEData == ESG_ESG_EDI_D_QUAEDATAPOSITION))
        {
          pr_OQuaCData->i_QualifyValue = pr_InternalEData->u_val.i_Uint32;
        }
        else
        {
          i_IndICData++;
        }
        if (i_IndLEData < r_ListEData->num_fileds - 1)
        {
          pr_InternalEData = (esg_esg_edi_t_StrElementaryData *) &pr_IOInternalCData->ar_EDataTable[i_IndICData];
        }
        else
        {
          *pi_OLgCodedCData = i_ILgMaxCodedCData - i_LgMaxCodedCData;
        }
        i_IndLEData++;
      }
    }
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_ComposedDataDecoding---------------------------------------*/


// GEV 11/01/2011 - Add the following two functions to prevent NAN processing */
float ExchangeTranslator::getGoodFloatValue(float d, float substitution)
{
  static float result;

  if (!finite(d))
  {
    result = substitution;
    fprintf(stdout, "WARN: force convert %f to %f\n", d, substitution);
  }
  else
    result = d;

  return result;
}

double ExchangeTranslator::getGoodDoubleValue(double d, double substitution)
{
  static double result;

  if (!finite(d))
  {
    fprintf(stdout, "WARN: force convert %g to %g\n", d, substitution);
    result = substitution;
  }
  else
    result = d;

  return result;
}

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_ElementaryDataCoding
//  FULL NAME		Coding of elementary data in EDI format
//----------------------------------------------------------------------------
//  ROLE
//	Uses basis type to code internal elementary data in ASCII EDI format character string
//----------------------------------------------------------------------------
//  CALLING CONTEXT	
//	 The service permits to obtain ASCII character string in EDI format
//       for a DED entity
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_ElementaryDataCoding(
              // Input parameters
              const char* s_IIdEData,       // Elementary data DED identifier
              const bool b_ILgQuaEData,     // Elementary data length qualify indicator
              const esg_esg_edi_t_StrElementaryData *pr_IInternalEData,// Internal elementary data
              const uint32_t i_ILgMaxCodedEData,// max length of coded elementary data
              // Output parameters
              uint32_t* pi_OLgCodedEData,   // length of coded elementary data
              char* ps_OCodedEData          // pointer to coded elementary data
              )
{
//----------------------------------------------------------------------------*/
  static const char* fname = "esg_esg_edi_ElementaryDataCoding";
 // EDI format of elementary data
  elemtype_item_t* r_FormatEData = NULL;
  elemtype_item_t* r_FormatQuaEData = NULL;
 // internal format of variable length elementary data
  esg_esg_edi_t_StrElementaryData r_VariableLength;
 // lengths of elementary data
  uint32_t i_LgFullEData;
  uint32_t i_LgEData;
  uint32_t i_LgQuaEData;
 // coded elementary data pointers
  char *ps_CodedEData;
  char *ps_ValueCodedEData;
 // return status of the function
  int i_RetStatus = OK;
 // logged text
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];

//............................................................................
  ps_CodedEData = NULL;
  *pi_OLgCodedEData = 0;

  // Providing the basis type of the elementary data from DED
  if (NULL != (r_FormatEData = esg_esg_odm_ConsultExchDataArr(s_IIdEData)))
  {
    i_RetStatus = OK;
    // Providing the length of ASCII complete coded elementary data and contoling the associated format
    i_RetStatus = esg_esg_edi_GetLengthFullCtrlEData(r_FormatEData,
                                                     b_ILgQuaEData,
                                                     pr_IInternalEData->u_val.r_Str.i_LgString,
                                                     &i_LgFullEData,
                                                     &i_LgEData,
                                                     &i_LgQuaEData);
    if (i_RetStatus != OK)
    {
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdEData;
    }
  }
  else {
    LOG(ERROR) << fname << ": unable to find " << s_IIdEData;
    i_RetStatus = NOK;
  }

  if (i_RetStatus == OK)
  {
    // Controling the length of ASCII coded EDI format elementary data
    *pi_OLgCodedEData = i_LgFullEData;
    if (i_ILgMaxCodedEData < i_LgFullEData)
    {
      i_RetStatus = ESG_ESG_D_ERR_DEDLGTOOSHORT;
      sprintf(s_LoggedText, "lg %d of %s to treat : provided lg %d ", i_LgFullEData, s_IIdEData, i_ILgMaxCodedEData);
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_LoggedText;
    }
  }

  if (i_RetStatus == OK)
  {
    ps_CodedEData = (char*) malloc(i_LgFullEData + 1);
    if (ps_CodedEData == NULL)
    {
      i_RetStatus = ESG_ESG_D_ERR_BADALLOC;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus;
    }
  }

  if (i_RetStatus == OK)
  {
    // treatment of string basis type with variable length
    if ((r_FormatEData->tm_type == TM_TYPE_STRING) &&
        ((strcmp(r_FormatEData->format_size, ECH_D_FORMATEDATA_STREMPTY)) == 0))
    {

      // Providing format for the basis type of the elementary data from DED
      if (NULL != (r_FormatQuaEData = (esg_esg_odm_ConsultExchDataArr(ECH_D_QUAVARLGED_IDED))))
      {
        // Coding of integer qualifier
        strcpy(ps_CodedEData, ECH_D_QUAVARLGED_IDED);
        ps_ValueCodedEData = ps_CodedEData + ECH_D_ELEMIDLG;
        r_VariableLength.i_type = FIELD_TYPE_UINT32;
        r_VariableLength.u_val.i_Uint32 = i_LgEData;
        i_RetStatus = esg_esg_edi_IntegerCoding (r_FormatQuaEData->format_size, &r_VariableLength, ps_ValueCodedEData);
        if (i_RetStatus != OK)
        {
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_QUAVARLGED_IDED;
        }
        else
        {
          ps_ValueCodedEData = ps_ValueCodedEData + i_LgQuaEData;
          strcpy(ps_ValueCodedEData, s_IIdEData);
          ps_ValueCodedEData = ps_ValueCodedEData + ECH_D_ELEMIDLG;
        }
      }
      else {
        LOG(ERROR) << fname << i_RetStatus << " " << ECH_D_QUAVARLGED_IDED;
      }
    }
    else
    {
      // Updating elemantary data identifier
      strcpy(ps_CodedEData, s_IIdEData);
      ps_ValueCodedEData = ps_CodedEData + ECH_D_ELEMIDLG;
    }
  }

  if (i_RetStatus == OK)
  {

#if 0
// (CD) treatment of integer and logical basis type
    if ((r_FormatEData.i_BasicType == ECH_D_TYPINT)
     || (r_FormatEData.i_BasicType == ECH_D_TYPLOGI))
    {
      i_RetStatus = esg_esg_edi_IntegerCoding ((char*) r_FormatEData.s_Format, pr_IInternalEData, ps_ValueCodedEData);
      if (i_RetStatus != OK)
      {
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdEData;
      }
    }

// (CD) treatment of real basis type
    if (r_FormatEData.i_BasicType == ECH_D_TYPREAL)
    {
      i_RetStatus = esg_esg_edi_RealCoding ((char*) r_FormatEData.s_Format, pr_IInternalEData, ps_ValueCodedEData);
      if (i_RetStatus != OK)
      {
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdEData;
      }
    }

// (CD) treatment of time basis type
    if (r_FormatEData.i_BasicType == ECH_D_TYPTIME)
    {
      i_RetStatus = esg_esg_edi_TimeCoding ((char*) r_FormatEData.s_Format, pr_IInternalEData, ps_ValueCodedEData);
      if (i_RetStatus != OK)
      {
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdEData;
      }
    }

// (CD) treatment of string basis type
    if (r_FormatEData.i_BasicType == ECH_D_TYPSTR)
    {
      strncpy(ps_ValueCodedEData, pr_IInternalEData->u_val.r_Str.ps_String, i_LgEData);
    }
#else
    switch (r_FormatEData->tm_type) {
      case TM_TYPE_LOGIC:
      case TM_TYPE_INTEGER:
        i_RetStatus = esg_esg_edi_IntegerCoding (r_FormatEData->format_size, pr_IInternalEData, ps_ValueCodedEData);
        break;

      case TM_TYPE_TIME:
        i_RetStatus = esg_esg_edi_TimeCoding (r_FormatEData->format_size, pr_IInternalEData, ps_ValueCodedEData);
        break;

      case TM_TYPE_STRING:
        strncpy(ps_ValueCodedEData, pr_IInternalEData->u_val.r_Str.ps_String, i_LgEData);
        break;

      case TM_TYPE_REAL:
        i_RetStatus = esg_esg_edi_RealCoding (r_FormatEData->format_size, pr_IInternalEData, ps_ValueCodedEData);
        break;

      default:
        i_RetStatus = NOK;
    }

    if (i_RetStatus != OK)
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", DED " << s_IIdEData;
#endif
  }

  if (i_RetStatus == OK)
  {
    memcpy(ps_OCodedEData, ps_CodedEData, *pi_OLgCodedEData);
  }

  if (ps_CodedEData != NULL)
  {
    free(ps_CodedEData);
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_ElementaryDataCoding---------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_ElementaryDataDecoding
//  FULL NAME		Decoding from EDI format in internal elementary data
//----------------------------------------------------------------------------
//  ROLE
//	Transforms elementary data from basis type in EDI format
//      to internal format.
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	 The service permits to obtain from the ASCII character string in EDI
//       format the internal format of an elementary data.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide DED entity in internal format.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_ElementaryDataDecoding(
             // Input parameters
             const char* s_IIdEData,    // Elementary data DED identifier
             const bool b_ILgQuaEData,  // Elementary data length qualify indicator
             const char* ps_ICodedEData,        // coded elementary data
             const uint32_t i_ILgMaxCodedEData, // max length of coded elementary data
             // Input-Output/Output parameters
             uint32_t* pi_OLgCodedEData,        // length of coded elementary data
             esg_esg_edi_t_StrElementaryData* pr_IOInternalEData   // Internal elementary data
             )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_ElementaryDataDecoding";
 // EDI format of elementary data
  elemtype_item_t* r_FormatEData = NULL;
  elemtype_item_t* r_FormatQuaEData = NULL;
 // internal format of variable length elementary data
  esg_esg_edi_t_StrElementaryData r_VariableLength;
 // lengths of elementary data
  uint32_t i_LgEData = 0;
  uint32_t i_LgQuaEData = 0;
  uint32_t i_LgCodedEData = 0;
 // identifier of elementary data
  char s_IdEData[ECH_D_ELEMIDLG + 1];
  char s_IdQuaEData[ECH_D_ELEMIDLG + 1];
 // coded elementary data pointers
  char *ps_CodedEData = NULL;
  char *ps_ValueCodedEData = NULL;
  char *ps_StrValueCodedEData = NULL;
 // IC 21-04-99 analyse float received string
  char *ps_Float = NULL;
  int32_t i_MantLg = 0;
  int32_t i_ExpLg = 0;
  uint32_t i_FloatLg = 0;
  bool b_EndFloat = false;
  bool b_ESep = false;
  char *s_SearchResult = NULL;
  char s_CodedExponentFormat[ECH_D_CODEDFORMATLG + 1];
 // return status of the function
  int i_RetStatus = OK;
 // logged text
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];
  char s_LoggedAux[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];

//............................................................................*/
  *pi_OLgCodedEData = 0;
  ps_CodedEData = (char *) ps_ICodedEData;
  ps_ValueCodedEData = NULL;
  ps_StrValueCodedEData = NULL;

  // Providing the basis type of the elementary data from DED
  if (NULL != (r_FormatEData = (esg_esg_odm_ConsultExchDataArr(s_IIdEData))))
  {
    i_RetStatus = OK;

    if ((r_FormatEData->tm_type == TM_TYPE_STRING) && ((strcmp(r_FormatEData->format_size, ECH_D_FORMATEDATA_STREMPTY)) == 0))
    {
      if (b_ILgQuaEData)
      {
        // Providing the basis type of the elementary data qualifier without id
        i_RetStatus = esg_esg_edi_GetForLgQuaEData(r_FormatQuaEData, &i_LgQuaEData);
        if (i_RetStatus != OK)
        {
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_QUAVARLGED_IDED;
        }
      }
      else
      {
        i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
        LOG(ERROR) << fname << " " << s_IIdEData << " is not compatible with " << ECH_D_QUAVARLGED_IDED;
      }

      if (i_RetStatus == OK)
      {
        // (CD) Controling length of elementary data qualifier with id
        if (i_ILgMaxCodedEData < (i_LgQuaEData + ECH_D_ELEMIDLG))
        {
          i_RetStatus = ESG_ESG_D_ERR_DEDLGTOOSHORT;
          sprintf(s_LoggedText, " length %d of %s to treat : provided length %d",
                 (i_LgQuaEData + ECH_D_ELEMIDLG), s_IIdEData, i_ILgMaxCodedEData);
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_LoggedText;
        }
      }

      if (i_RetStatus == OK)
      {
// (CD) Controling identifier of elementary data qualifier
        memset(s_IdQuaEData, 0, sizeof(s_IdQuaEData));
        memcpy(s_IdQuaEData, ps_CodedEData, ECH_D_ELEMIDLG);
        if ((strcmp(ECH_D_QUAVARLGED_IDED, s_IdQuaEData)) != 0)
        {
          i_RetStatus = ESG_ESG_D_ERR_BADDEDIDENT;
          sprintf(s_LoggedText, "%s to treat : %s received", ECH_D_QUAVARLGED_IDED, s_IdQuaEData);
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_LoggedText;
        }
      }

      if (i_RetStatus == OK)
      {
        ps_StrValueCodedEData = (char*) malloc(i_LgQuaEData + 1);
        if (ps_StrValueCodedEData == NULL)
        {
          i_RetStatus = ESG_ESG_D_ERR_BADALLOC;
          LOG(ERROR) << fname << ": rc=" << i_RetStatus;
        }
      }

      if (i_RetStatus == OK)
      {
// (CD) Providing format for the basis type of the elementary data from DED
        if (NULL == (r_FormatQuaEData = esg_esg_odm_ConsultExchDataArr (ECH_D_QUAVARLGED_IDED)))
        {
          i_RetStatus = NOK;
          LOG(ERROR) << fname << i_RetStatus << " " << ECH_D_QUAVARLGED_IDED;
        }

        if (i_RetStatus == OK)
        {
// (CD) decoding of integer qualifier
          memset(ps_StrValueCodedEData, 0, i_LgQuaEData + 1);
          ps_ValueCodedEData = ps_CodedEData + ECH_D_ELEMIDLG;

          memcpy(ps_StrValueCodedEData, ps_ValueCodedEData, i_LgQuaEData);
          r_VariableLength.i_type = FIELD_TYPE_UINT32;
          i_RetStatus = esg_esg_edi_IntegerDecoding (r_FormatQuaEData->format_size, ps_StrValueCodedEData, &r_VariableLength);

          free(ps_StrValueCodedEData);
          ps_StrValueCodedEData = NULL;

          if (i_RetStatus != OK)
          {
            LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_QUAVARLGED_IDED;
          }
          else
          {
            i_LgEData = r_VariableLength.u_val.i_Uint32;
            ps_CodedEData = ps_ValueCodedEData + i_LgQuaEData;
            i_LgCodedEData = i_LgEData + i_LgQuaEData + (2 * ECH_D_ELEMIDLG);
          }
        }
      }
    }
    else
    {
      if (!b_ILgQuaEData)
      {
// (CD) Providing the length of ASCII coded elementary data without id
        i_RetStatus = esg_esg_edi_GetLengthEData(r_FormatEData, 0, &i_LgEData);
        if (i_RetStatus != OK)
        {
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_IIdEData;
        }
        else
        {
          i_LgCodedEData = i_LgEData + ECH_D_ELEMIDLG;
        }
      }
      else
      {
        i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdEData << " is not compatible with " << ECH_D_QUAVARLGED_IDED;
      }
    }
  }
  else {
    LOG(ERROR) << fname << i_RetStatus << " " << s_IIdEData;
    i_RetStatus = NOK;
  }

  if (i_RetStatus == OK)
  {

// Controling the length of ASCII coded EDI format elementary data
// Receiver has to accept distant 8e4 float format also when float DED format is 11e4 :
//          - analyse length of received data float
//          - if local(11e4/8e4) and distant(8e4/11e4) format are different :
//            set local to distant format
//---------------------------------------------------------------------------
    if ((r_FormatEData->tm_type == TM_TYPE_REAL) &&
        ((s_SearchResult = strchr(r_FormatEData->format_size, ECH_D_FORMATEDATA_REALEXP)) != NULL))
    {
      i_MantLg = 0;
      i_ExpLg = 0;
      i_FloatLg = 0;
      b_EndFloat = false;
      b_ESep = false;
      ps_Float = ps_CodedEData + ECH_D_ELEMIDLG;

      while ((b_EndFloat == false) && (i_FloatLg < i_ILgMaxCodedEData))
      {
        if (((*ps_Float >= '0') && (*ps_Float <= '9')) ||
            (*ps_Float == '-') ||
            (*ps_Float == ECH_D_FORMATEDATA_REALPRES) ||
            (*ps_Float == ECH_D_FORMATEDATA_REALEXP))
        {
          i_FloatLg++;
          if (*ps_Float == ECH_D_FORMATEDATA_REALEXP)
          {
            b_ESep = true;
            i_MantLg = i_FloatLg - 1;
          }
        }
        else
        {
          b_EndFloat = true;
        }
        ps_Float++;
      }

      if ((b_EndFloat == false) || (b_ESep == false))
      {
        i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
        memset(s_LoggedAux, 0, sizeof(s_LoggedAux));
        memcpy(s_LoggedAux, ps_CodedEData, 25);
        sprintf(s_LoggedText, "%s : float flag=%d, float lg=%d, e flag=%d, max CodedEData lg=%d, mantisse lg=%d",
                             s_LoggedAux,
                             b_EndFloat,
                             i_FloatLg,
                             b_ESep,
                             i_ILgMaxCodedEData,
                             i_MantLg);
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_LoggedText;
      }

      if (i_RetStatus == OK)
      {
        i_ExpLg = i_FloatLg - i_MantLg - 1;

        // verify if local and distant are equal
        if (i_LgEData != i_FloatLg)
        {
          // set distant format
          memset(r_FormatEData->format_size, 0, sizeof(r_FormatEData->format_size));
          sprintf(r_FormatEData->format_size, "%d", i_MantLg);
          r_FormatEData->format_size[strlen(r_FormatEData->format_size)] = ECH_D_FORMATEDATA_REALEXP;
          sprintf(s_CodedExponentFormat, "%d", i_ExpLg);
          strcat(r_FormatEData->format_size, s_CodedExponentFormat);

          // update length of coded elementary data with and without DED ident
          i_LgCodedEData = i_FloatLg + ECH_D_ELEMIDLG;
          i_LgEData = i_FloatLg;
        }
      }
    }
  }

  if (i_RetStatus == OK)
  {
    *pi_OLgCodedEData = i_LgCodedEData;
    if (i_ILgMaxCodedEData < *pi_OLgCodedEData)
    {
      i_RetStatus = ESG_ESG_D_ERR_DEDLGTOOSHORT;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", " << s_IIdEData << " : " << *pi_OLgCodedEData;
    }
  }

  if (i_RetStatus == OK)
  {
    // Controling identifier of elementary data
    memset(s_IdEData, 0, sizeof(s_IdEData));
    memcpy(s_IdEData, ps_CodedEData, ECH_D_ELEMIDLG);

    if ((strcmp(s_IIdEData, s_IdEData)) != 0)
    {
      i_RetStatus = ESG_ESG_D_ERR_BADDEDIDENT;
      sprintf(s_LoggedText, " %s to treat, %s received", s_IIdEData, s_IdEData);
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_LoggedText;
    }
  }

  if (i_RetStatus == OK)
  {
    // TODO: 2017/06/08 GEV - память ps_StrValueCodedEData выделялась ранее, проверить возможность утечки памяти
    assert(ps_StrValueCodedEData == NULL);
    if (!ps_StrValueCodedEData)
      free(ps_StrValueCodedEData);

    // Preparing ASCII coded EDI format elementary data string
    ps_StrValueCodedEData = (char*) malloc(i_LgEData + 1);
    if (ps_StrValueCodedEData == NULL)
    {
      i_RetStatus = ESG_ESG_D_ERR_BADALLOC;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus;
    }
    else
    {
      memset(ps_StrValueCodedEData, 0, i_LgEData + 1);
      ps_ValueCodedEData = ps_CodedEData + ECH_D_ELEMIDLG;
      memcpy(ps_StrValueCodedEData, ps_ValueCodedEData, i_LgEData);
    }
  }

  if (i_RetStatus == OK)
  {
#if 0
// (CD) treatment of integer and logical basis type
    if ((r_FormatEData.i_BasicType == ECH_D_TYPINT)
     || (r_FormatEData.i_BasicType == ECH_D_TYPLOGI))
    {
      i_RetStatus = esg_esg_edi_IntegerDecoding (r_FormatEData.s_Format, ps_StrValueCodedEData, pr_IOInternalEData);
      if (i_RetStatus != OK)
      {
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_IIdEData;
      }
    }

// (CD) treatment of real basis type
    if (r_FormatEData.i_BasicType == ECH_D_TYPREAL)
    {
      i_RetStatus = esg_esg_edi_RealDecoding (r_FormatEData.s_Format, ps_StrValueCodedEData, pr_IOInternalEData);
      if (i_RetStatus != OK)
      {
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdEData;
      }
    }

// (CD) treatment of time basis type
    if (r_FormatEData.i_BasicType == ECH_D_TYPTIME)
    {
      i_RetStatus = esg_esg_edi_TimeDecoding (r_FormatEData.s_Format, ps_StrValueCodedEData, pr_IOInternalEData);
      if (i_RetStatus != OK)
      {
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdEData;
      }
    }

// (CD) treatment of string basis type if poiner non zero to reserved memory
    if ((r_FormatEData.i_BasicType == ECH_D_TYPSTR) && (pr_IOInternalEData->u_val.r_Str.ps_String != 0))
    {
      strncpy(pr_IOInternalEData->u_val.r_Str.ps_String, ps_StrValueCodedEData, i_LgEData);
      pr_IOInternalEData->u_val.r_Str.i_LgString = i_LgEData;
    }
#else

    switch(r_FormatEData->tm_type) {
      case TM_TYPE_LOGIC:
      case TM_TYPE_INTEGER:
        i_RetStatus = esg_esg_edi_IntegerDecoding (r_FormatEData->format_size, ps_StrValueCodedEData, pr_IOInternalEData);
        break;

      case TM_TYPE_TIME:
        i_RetStatus = esg_esg_edi_TimeDecoding (r_FormatEData->format_size, ps_StrValueCodedEData, pr_IOInternalEData);
        break;

      case TM_TYPE_STRING:
        if (pr_IOInternalEData->u_val.r_Str.ps_String != 0)
        {
          strncpy(pr_IOInternalEData->u_val.r_Str.ps_String, ps_StrValueCodedEData, i_LgEData);
          pr_IOInternalEData->u_val.r_Str.i_LgString = i_LgEData;
        }
        break;

      case TM_TYPE_REAL:
        i_RetStatus = esg_esg_edi_RealDecoding (r_FormatEData->format_size, ps_StrValueCodedEData, pr_IOInternalEData);
        break;

      default:
        i_RetStatus = NOK;
    }

    if (i_RetStatus != OK)
    {
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdEData;
    }

#endif
  }

  if (ps_StrValueCodedEData != NULL)
  {
    free(ps_StrValueCodedEData);
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_ElementaryDataDecoding-------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_ComposedDataCoding
//  FULL NAME		Coding of composed data in EDI format
//----------------------------------------------------------------------------
//  ROLE
//	Uses basis type to code internal composed data in ASCII EDI format character string
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to obtain ASCII character string in EDI format for a DCD entity
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide DCD entity in EDI format.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_ComposedDataCoding(
             // Input parameters
             const char* s_IIdCData,    // Composed data DCD identifier
             const esg_esg_edi_t_StrQualifyComposedData* pr_IQuaCData,  // Qualify of internal composed data
             const esg_esg_edi_t_StrComposedData* pr_IInternalCData,    // Internal composed data
             const uint32_t i_ILgMaxCodedCData, // max length of coded composed data
             // Output parameters
             uint32_t* pi_OLgCodedCData,// length of coded composed data
             char* ps_OCodedCData       // pointer to coded composed data
             )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_ComposedDataCoding";
 // list of elementary data for the composed data
  elemstruct_item_t* r_ListEData = NULL;
 // table indices
  uint32_t i_IndLEData;
  uint32_t i_IndICData;
 // lengths of data
  uint32_t i_LgCodedCData;
  uint32_t i_LgMaxCodedCData;
  uint32_t i_LgCodedEData;
 // coded composed data string
  char *ps_CodedCData;
 // internal elementary data
  esg_esg_edi_t_StrElementaryData *pr_InternalEData;
  esg_esg_edi_t_StrElementaryData r_InternalQuaCData;
 // elementary data length quality indicator
  bool b_LgQuaEData;
 // return status of the function
  int i_RetStatus = OK;
 // logged text
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];

//............................................................................
  i_RetStatus = OK;
  *pi_OLgCodedCData = 0;

  // obtaining the composition of the composed data
  if (NULL != (r_ListEData = esg_esg_odm_ConsultExchCompArr(s_IIdCData)))
  {
    i_RetStatus = OK;
    // providing complete length of ASCII coded EDI format composed data
    i_RetStatus = esg_esg_edi_GetLengthCData(s_IIdCData,
                                             pr_IQuaCData,
                                             pr_IInternalCData,
                                             &i_LgCodedCData);
    if (i_RetStatus != OK)
    {
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_IIdCData;
    }
  }
  else {
    LOG(ERROR) << fname << ": unable to find " << s_IIdCData;
    i_RetStatus = NOK;
  }

  if (i_RetStatus == OK)
  {
    // controling the length of ASCII coded EDI format composed data
    *pi_OLgCodedCData = i_LgCodedCData;
    if (i_ILgMaxCodedCData < i_LgCodedCData)
    {
      i_RetStatus = ESG_ESG_D_ERR_DCDLGTOOSHORT;
      sprintf(s_LoggedText, "lg %d of %s to treat : provided lg %d ", i_LgCodedCData, s_IIdCData, i_ILgMaxCodedCData);
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_LoggedText;
    }
  }

  if (i_RetStatus == OK)
  {
    i_LgMaxCodedCData = i_ILgMaxCodedCData;
    ps_CodedCData = ps_OCodedCData;

    // Coding of composed data label
    if ((strcmp(s_IIdCData, ECH_D_INCHSEGHICD) == 0))
    {
      strcpy(ps_CodedCData, ECH_D_STR_ICHSEGLABEL);
      ps_CodedCData = ps_CodedCData + ECH_D_SEGLABELLG;
    }
    if ((strcmp(s_IIdCData, ECH_D_INCHSEGEICD) == 0))
    {
      strcpy(ps_CodedCData, ECH_D_STR_ICESEGLABEL);
      ps_CodedCData = ps_CodedCData + ECH_D_SEGLABELLG;
    }
    if ((strcmp(s_IIdCData, ECH_D_MSGSEGHICD) == 0))
    {
      strcpy(ps_CodedCData, ECH_D_STR_MSGHSEGLABEL);
      ps_CodedCData = ps_CodedCData + ECH_D_SEGLABELLG;
    }
    if ((strcmp(s_IIdCData, ECH_D_MSGSEGEICD) == 0))
    {
      strcpy(ps_CodedCData, ECH_D_STR_MSGESEGLABEL);
      ps_CodedCData = ps_CodedCData + ECH_D_SEGLABELLG;
    }
    if ((strcmp(s_IIdCData, ECH_D_APPLSEGHICD) == 0))
    {
      strcpy(ps_CodedCData, ECH_D_STR_APPHSEGLABEL);
      ps_CodedCData = ps_CodedCData + ECH_D_SEGLABELLG;
    }

// (CD) Coding of composed data identifier
    strcpy(ps_CodedCData, s_IIdCData);
    ps_CodedCData = ps_CodedCData + ECH_D_COMPIDLG;

// (CD) coding of each elementary data
    i_IndLEData = 0;
    i_IndICData = 0;
    pr_InternalEData = (esg_esg_edi_t_StrElementaryData *) & pr_IInternalCData->ar_EDataTable[i_IndICData];

    while ((i_IndLEData < r_ListEData->num_fileds) && (i_RetStatus == OK))
    {

// (CD) providing the coded elementary data from internal format
      if ((pr_IQuaCData->b_QualifyUse) && (i_IndLEData == ESG_ESG_EDI_D_QUAEDATAPOSITION))
      {
        r_InternalQuaCData.i_type = FIELD_TYPE_UINT32;
        r_InternalQuaCData.u_val.i_Uint32 = pr_IQuaCData->i_QualifyValue;
        pr_InternalEData = &r_InternalQuaCData;
      }

      if ((strcmp(r_ListEData->fields[i_IndLEData].field, ECH_D_QUAVARLGED_IDED)) == 0)
      {
        b_LgQuaEData = true;
        i_IndLEData++;
        if (i_IndLEData == r_ListEData->num_fileds)
        {
          i_RetStatus = ESG_ESG_D_ERR_BADDCDDEDLIST;
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IIdCData;
        }
      }
      else
      {
        b_LgQuaEData = false;
      }

      if (i_RetStatus == OK)
      {
        // providing the length of ASCII complete coded elementary data
        i_RetStatus = esg_esg_edi_ElementaryDataCoding(r_ListEData->fields[i_IndLEData].field,
                                                       b_LgQuaEData,
                                                       pr_InternalEData,
                                                       i_LgMaxCodedCData,
                                                       &i_LgCodedEData,
                                                       ps_CodedCData);
        if (i_RetStatus != OK)
        {
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", " << r_ListEData->fields[i_IndLEData].field;
        }
      }

      if (i_RetStatus == OK)
      {
        // Get next saved address for coded elementary data and next elementary data of the list
        i_LgMaxCodedCData = i_LgMaxCodedCData - i_LgCodedEData;
        ps_CodedCData = ps_CodedCData + i_LgCodedEData;
        if (i_IndLEData < r_ListEData->num_fileds - 1)
        {
          if ((!pr_IQuaCData->b_QualifyUse)
           || ((pr_IQuaCData->b_QualifyUse) && (i_IndLEData != ESG_ESG_EDI_D_QUAEDATAPOSITION)))
          {
            i_IndICData++;
          }
          pr_InternalEData = (esg_esg_edi_t_StrElementaryData*) &pr_IInternalCData->ar_EDataTable[i_IndICData];
        }
        i_IndLEData++;
      }
    }
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_ComposedDataCoding-----------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_GetLengthCData
//  FULL NAME		Getting length of Coded composed data in ASCII EDI format
//----------------------------------------------------------------------------
//  ROLE
//  Calculate length of composed data character string.
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to obtain the length of ASCII character string in EDI format for a DCD entity.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//  To provide length of DCD entity in EDI format with EDI identifier.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_GetLengthCData(
            const char* s_IIdCData, // composed data DCD identifier
            const esg_esg_edi_t_StrQualifyComposedData* pr_IQuaCData,   // Qualify of internal composed data
            const esg_esg_edi_t_StrComposedData* pr_IInternalCData,     // Internal composed data
            // Output parameters
            uint32_t* pi_OLgCodedCData // length of coded composed data
            )
{
//----------------------------------------------------------------------------*/
//	Local declarations
  static const char* fname = "esg_esg_edi_GetLengthCData";
 // list of elementary data for the composed data
  elemstruct_item_t* r_ListEData = NULL;
 // table indices and number of entities
  uint32_t i_IndLEData;
  uint32_t i_IndICData;
  uint32_t i_NbInternalEData;
  uint32_t i_NbEData;
 // lengths of data
  uint32_t i_LgFullEData;
  uint32_t i_LgEData;
  uint32_t i_LgQuaEData;
  uint32_t i_LgCodedCData;
 // EDI format of elementary data
  elemtype_item_t* r_FormatEData = NULL;
 // elementary data length quality indicator
  bool b_LgQuaEData;
 // internal elementary data
  uint32_t i_LgString;
  esg_esg_edi_t_StrElementaryData *pr_InternalEData;
 // return status of the function
  int i_RetStatus = OK;

//............................................................................
  i_RetStatus = OK;
  *pi_OLgCodedCData = 0;

  // obtaining the composition of the composed data
  if (NULL != (r_ListEData = esg_esg_odm_ConsultExchCompArr(s_IIdCData)))
  {
    i_RetStatus = OK;
    // Controling coherence of elementary data list and internal composed data table
    i_NbInternalEData = pr_IInternalCData->i_NbEData;
    if (pr_IQuaCData->b_QualifyUse)
    {
      i_NbInternalEData++;
    }

    i_NbEData = r_ListEData->num_fileds;
    for (i_IndLEData = 0; i_IndLEData < r_ListEData->num_fileds; i_IndLEData++)
    {
      if ((strcmp(r_ListEData->fields[i_IndLEData].field, ECH_D_QUAVARLGED_IDED)) == 0)
      {
        i_NbEData--;
      }
    }

    if (i_NbEData != i_NbInternalEData)
    {
      i_RetStatus = ESG_ESG_D_ERR_NBEDINTERDCD;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", " << s_IIdCData;
    }
  }
  else {
    LOG(ERROR) << fname << ": unable to find " << s_IIdCData;
    i_RetStatus = NOK;
  }

  if (i_RetStatus == OK)
  {
    // Calculate length of composed data
    i_LgCodedCData = 0;
    i_IndLEData = 0;
    i_IndICData = 0;
    pr_InternalEData = (esg_esg_edi_t_StrElementaryData *) &pr_IInternalCData->ar_EDataTable[i_IndICData];

    while ((i_IndLEData < r_ListEData->num_fileds) && (i_RetStatus == OK))
    {
      // providing the basis type of the elementary data from DED
      if ((strcmp(r_ListEData->fields[i_IndLEData].field, ECH_D_QUAVARLGED_IDED)) == 0)
      {
        b_LgQuaEData = true;
        i_IndLEData++;
        if (i_IndLEData == r_ListEData->num_fileds)
        {
          i_RetStatus = ESG_ESG_D_ERR_BADDCDDEDLIST;
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", " << fname << ", " << s_IIdCData;
        }
      }
      else
      {
        b_LgQuaEData = false;
      }

      if (i_RetStatus == OK)
      {
        if (NULL == (r_FormatEData = esg_esg_odm_ConsultExchDataArr(r_ListEData->fields[i_IndLEData].field)))
        {
          i_RetStatus = NOK;
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", search " << r_ListEData->fields[i_IndLEData].field;
        }
      }

      if (i_RetStatus == OK)
      {

        if ((pr_IQuaCData->b_QualifyUse) && (i_IndLEData == ESG_ESG_EDI_D_QUAEDATAPOSITION))
        {
          i_LgString = 0;
        }
        else
        {
          i_LgString = pr_InternalEData->u_val.r_Str.i_LgString;
        }

// (CD) Providing the length of ASCII complete coded elementary data
        i_RetStatus = esg_esg_edi_GetLengthFullCtrlEData(r_FormatEData,
                                                        b_LgQuaEData,
                                                        i_LgString,
                                                        &i_LgFullEData,
                                                        &i_LgEData,
                                                        &i_LgQuaEData);
        if (i_RetStatus != OK)
        {
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", " << r_ListEData->fields[i_IndLEData].field;
        }
      }

      if (i_RetStatus == OK)
      {
// (CD) Save length and get next elementary data of the list
        i_LgCodedCData = i_LgCodedCData + i_LgFullEData;
        if (i_IndLEData == r_ListEData->num_fileds - 1)
        {
          i_LgCodedCData = i_LgCodedCData + ECH_D_COMPIDLG;
        }
        else
        {
          if ((!pr_IQuaCData->b_QualifyUse)
            ||((pr_IQuaCData->b_QualifyUse) && (i_IndLEData != ESG_ESG_EDI_D_QUAEDATAPOSITION)))
          {
            i_IndICData++;
          }
          pr_InternalEData = (esg_esg_edi_t_StrElementaryData *) &pr_IInternalCData->ar_EDataTable[i_IndICData];
        }
        i_IndLEData++;
      }
    }
  }

  if (i_RetStatus == OK)
  {
    if ((strcmp(s_IIdCData, ECH_D_INCHSEGHICD) == 0))
    {
      i_LgCodedCData = i_LgCodedCData + ECH_D_SEGLABELLG;
    }
    else if ((strcmp(s_IIdCData, ECH_D_INCHSEGEICD) == 0))
    {
      i_LgCodedCData = i_LgCodedCData + ECH_D_SEGLABELLG;
    }
    else if ((strcmp(s_IIdCData, ECH_D_MSGSEGHICD) == 0))
    {
      i_LgCodedCData = i_LgCodedCData + ECH_D_SEGLABELLG;
    }
    else if ((strcmp(s_IIdCData, ECH_D_MSGSEGEICD) == 0))
    {
      i_LgCodedCData = i_LgCodedCData + ECH_D_SEGLABELLG;
    }
    else if ((strcmp(s_IIdCData, ECH_D_APPLSEGHICD) == 0))
    {
      i_LgCodedCData = i_LgCodedCData + ECH_D_SEGLABELLG;
    }
    *pi_OLgCodedCData = i_LgCodedCData;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_GetLengthCData---------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_HeaderInterChgCoding
//  FULL NAME		Coding of interchange header in EDI format
//----------------------------------------------------------------------------
//  ROLE
//	Transforms internal interchange header in ASCII EDI format character string.
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	The service permits to obtain ASCII character string in EDI format for the interchange header.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide interchange header in EDI format.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_HeaderInterChgCoding(
           // Input parameters
           const esg_esg_t_HeaderInterChg* pr_IInternalHInterChg,   // Internal interchange header
           const uint32_t i_ILgMaxCodedCData,   // max length of coded composed data
           // Output parameters
           uint32_t* pi_OLgCodedCData,  // length of coded composed data
           char* ps_OCodedCData         // pointer to coded composed data
           )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_HeaderInterChgCoding";
 // internal composed data table
  esg_esg_edi_t_StrComposedData r_HICInternalCData;
 // fixed strings for sender and receiver identifier
  gof_t_UniversalName s_IdSender;
  gof_t_UniversalName s_IdReceiver;
 // indice internal composed data table
  uint32_t i_IndIEData;
 // qualifier interface structure
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
 // return status of the function
  int i_RetStatus = OK;

//............................................................................
// (CD) providing coded composed data
  memset(s_IdSender, ' ', sizeof(s_IdSender));
  memset(s_IdReceiver, ' ', sizeof(s_IdReceiver));
  memcpy(s_IdSender, pr_IInternalHInterChg->s_IdSender, strlen(pr_IInternalHInterChg->s_IdSender));
  s_IdSender[strlen(pr_IInternalHInterChg->s_IdSender)] = ECH_D_CARZERO;
  memcpy(s_IdReceiver, pr_IInternalHInterChg->s_IdReceiver, strlen(pr_IInternalHInterChg->s_IdReceiver));
  s_IdReceiver[strlen(pr_IInternalHInterChg->s_IdReceiver)] = ECH_D_CARZERO;

  i_IndIEData = 0;
  r_HICInternalCData.i_NbEData = ESG_ESG_D_HICNBEDATA;
  r_HICInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT32;
  r_HICInternalCData.ar_EDataTable[i_IndIEData++].u_val.i_Uint32 = pr_IInternalHInterChg->i_IdExchange;
  r_HICInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_FLOAT;
  r_HICInternalCData.ar_EDataTable[i_IndIEData++].u_val.f_Float = pr_IInternalHInterChg->f_InterChgVersion;
  r_HICInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_STRING;
  r_HICInternalCData.ar_EDataTable[i_IndIEData].u_val.r_Str.ps_String = s_IdSender;
  r_HICInternalCData.ar_EDataTable[i_IndIEData++].u_val.r_Str.i_LgString = sizeof(gof_t_UniversalName);
  r_HICInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_STRING;
  r_HICInternalCData.ar_EDataTable[i_IndIEData].u_val.r_Str.ps_String = s_IdReceiver;
  r_HICInternalCData.ar_EDataTable[i_IndIEData++].u_val.r_Str.i_LgString = sizeof(gof_t_UniversalName);
  r_HICInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_DATE;

  memcpy(&r_HICInternalCData.ar_EDataTable[i_IndIEData++].u_val.d_Timeval,
         &pr_IInternalHInterChg->d_InterChgDate,
         sizeof(timeval));

  r_HICInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT16;
  r_HICInternalCData.ar_EDataTable[i_IndIEData].u_val.h_Uint16 = pr_IInternalHInterChg->h_InterChgMsgNb;

  memset(&r_QuaCData, '\0', sizeof(esg_esg_edi_t_StrQualifyComposedData));
  i_RetStatus = esg_esg_edi_ComposedDataCoding(ECH_D_INCHSEGHICD,
                                               &r_QuaCData,
                                               &r_HICInternalCData,
                                               i_ILgMaxCodedCData,
                                               pi_OLgCodedCData,
                                               ps_OCodedCData);
  if (i_RetStatus != OK)
  {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_INCHSEGHICD;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_HeaderInterChgCoding---------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_HeaderInterChgDecoding
//  FULL NAME		Decoding from EDI format in internal interchange header
//----------------------------------------------------------------------------
//  ROLE
//	Transforms interchange header from basis type in EDI format to internal format
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	The service permits to obtain from the ASCII character string in EDI format
//	the internal format of interchange header
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide interchange header in internal format
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_HeaderInterChgDecoding(
             // Input parameters
             const char* ps_ICodedCData,                        // coded composed data
             const uint32_t i_ILgMaxCodedCData,                 // max length of coded composed data
             // Output parameters
             uint32_t* pi_OLgCodedCData,                        // length of coded composed data
             esg_esg_t_HeaderInterChg* pr_OInternalHInterChg    // Internal interchange header
             )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_HeaderInterChgDecoding";
 // internal composed data table
  esg_esg_edi_t_StrComposedData r_HICInternalCData;
 // fixed strings for sender and receiver identifier
  gof_t_UniversalName s_IdSender;
  gof_t_UniversalName s_IdReceiver;
  int32_t i_Indice;
  bool b_CarZeroExist;
 // indice internal composed data table
  uint32_t i_IndIEData;
 // qualifier interface structure
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
 // return status of the function
  int i_RetStatus = OK;

//............................................................................
// (CD) providing coded composed data
  i_IndIEData = 0;
  r_HICInternalCData.i_NbEData = ESG_ESG_D_HICNBEDATA;
  r_HICInternalCData.ar_EDataTable[i_IndIEData++].i_type = FIELD_TYPE_UINT32;
  r_HICInternalCData.ar_EDataTable[i_IndIEData++].i_type = FIELD_TYPE_FLOAT;
  r_HICInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_STRING;
  r_HICInternalCData.ar_EDataTable[i_IndIEData].u_val.r_Str.ps_String = s_IdSender;
  r_HICInternalCData.ar_EDataTable[i_IndIEData++].u_val.r_Str.i_LgString = sizeof(gof_t_UniversalName);
  r_HICInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_STRING;
  r_HICInternalCData.ar_EDataTable[i_IndIEData].u_val.r_Str.ps_String = s_IdReceiver;
  r_HICInternalCData.ar_EDataTable[i_IndIEData++].u_val.r_Str.i_LgString = sizeof(gof_t_UniversalName);
  r_HICInternalCData.ar_EDataTable[i_IndIEData++].i_type = FIELD_TYPE_DATE;
  r_HICInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT16;

  memset(&r_QuaCData, 0, sizeof(esg_esg_edi_t_StrQualifyComposedData));
  i_RetStatus = esg_esg_edi_ComposedDataDecoding(ECH_D_INCHSEGHICD,
                                                 ps_ICodedCData,
                                                 i_ILgMaxCodedCData,
                                                 pi_OLgCodedCData,
                                                 &r_QuaCData,
                                                 &r_HICInternalCData);
  if (i_RetStatus != OK)
  {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_INCHSEGHICD;
  }

  if (i_RetStatus == OK)
  {
    i_IndIEData = 0;
    pr_OInternalHInterChg->i_IdExchange = r_HICInternalCData.ar_EDataTable[i_IndIEData++].u_val.i_Uint32;
    pr_OInternalHInterChg->f_InterChgVersion = r_HICInternalCData.ar_EDataTable[i_IndIEData++].u_val.f_Float;

    b_CarZeroExist = false;
    i_Indice = sizeof(gof_t_UniversalName) - 1;
    while ((i_Indice >= 0) && (!b_CarZeroExist))
    {
      if (s_IdSender[i_Indice] == ECH_D_CARZERO)
      {
        s_IdSender[i_Indice] = ECH_D_ENDSTRING;
        b_CarZeroExist = true;
      }
      i_Indice--;
    }

    if (!b_CarZeroExist)
    {
      i_RetStatus = ESG_ESG_D_ERR_BADSNDRCVID;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_INCHSEGHICD;
    }
    else
    {
      strcpy(pr_OInternalHInterChg->s_IdSender, s_IdSender);
    }
  }

  if (i_RetStatus == OK)
  {
    b_CarZeroExist = false;
    i_Indice = sizeof(gof_t_UniversalName) - 1;

    while ((i_Indice >= 0) && (!b_CarZeroExist))
    {
      if (s_IdReceiver[i_Indice] == ECH_D_CARZERO)
      {
        s_IdReceiver[i_Indice] = ECH_D_ENDSTRING;
        b_CarZeroExist = true;
      }
      i_Indice--;
    }

    if (!b_CarZeroExist)
    {
      i_RetStatus = ESG_ESG_D_ERR_BADSNDRCVID;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_INCHSEGHICD;
    }
    else
    {
      strcpy(pr_OInternalHInterChg->s_IdReceiver, s_IdReceiver);
    }
  }

  if (i_RetStatus == OK)
  {
    i_IndIEData++;
    i_IndIEData++;
    memcpy(&pr_OInternalHInterChg->d_InterChgDate,
           &r_HICInternalCData.ar_EDataTable[i_IndIEData++].u_val.d_Timeval,
           sizeof(timeval));
    pr_OInternalHInterChg->h_InterChgMsgNb = r_HICInternalCData.ar_EDataTable[i_IndIEData].u_val.h_Uint16;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_HeaderInterChgDecoding-------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_EndInterChgCoding
//  FULL NAME		Coding of interchange end in EDI format
//----------------------------------------------------------------------------
//  ROLE
//	Transforms internal interchange end in ASCII EDI format character string
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	 The service permits to obtain ASCII character string in EDI format for the interchange end
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide interchange end in EDI format.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_EndInterChgCoding(
                // Input parameters
                const esg_esg_t_EndInterChg* pr_IInternalEInterChg, // Internal interchange end
                const uint32_t i_ILgMaxCodedCData,                  // max length of coded composed data
                // Output parameters
                uint32_t* pi_OLgCodedCData,                         // length of coded composed data
                char* ps_OCodedCData                                // pointer to coded composed data
                )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_EndInterChgCoding";
 // internal composed data table
  esg_esg_edi_t_StrComposedData r_EICInternalCData;
 // indice internal composed data table
  uint32_t i_IndIEData;
 // qualifier interface structure
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
 // return status of the function
  int i_RetStatus = OK;
//............................................................................

// (CD) providing coded composed data
  i_IndIEData = 0;
  r_EICInternalCData.i_NbEData = ESG_ESG_D_EICNBEDATA;
  r_EICInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT32;
  r_EICInternalCData.ar_EDataTable[i_IndIEData].u_val.i_Uint32 = pr_IInternalEInterChg->i_IdExchange;

  memset(&r_QuaCData, 0, sizeof(esg_esg_edi_t_StrQualifyComposedData));
  i_RetStatus = esg_esg_edi_ComposedDataCoding(ECH_D_INCHSEGEICD,
                                               &r_QuaCData,
                                               &r_EICInternalCData,
                                               i_ILgMaxCodedCData,
                                               pi_OLgCodedCData,
                                               ps_OCodedCData);
  if (i_RetStatus != OK)
  {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_INCHSEGEICD;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_EndInterChgCoding------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_EndInterChgDecoding
//  FULL NAME		Decoding from EDI format in internal interchange end
//----------------------------------------------------------------------------
//  ROLE
//	Transforms interchange end from basis type in EDI format to internal format
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to obtain from the ASCII character string in EDI format
//  the internal format of interchange end
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide interchange end in internal format
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_EndInterChgDecoding(
                  // Input parameters
                  const char* ps_ICodedCData,       // coded composed data
                  const uint32_t i_ILgMaxCodedCData,// max length of coded composed data
                  // Output parameters
                  uint32_t* pi_OLgCodedCData,       // length of coded composed data
                  esg_esg_t_EndInterChg* pr_OInternalEInterChg  // Internal interchange header
                  )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_EndInterChgDecoding";
  //internal composed data table
  esg_esg_edi_t_StrComposedData r_EICInternalCData;
 // indice internal composed data table
  uint32_t i_IndIEData;
 // qualifier interface structure
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
 // return status of the function
  int i_RetStatus = OK;

//............................................................................
// providing coded composed data
  i_IndIEData = 0;
  r_EICInternalCData.i_NbEData = ESG_ESG_D_EICNBEDATA;
  r_EICInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT32;

  memset(&r_QuaCData, 0, sizeof(esg_esg_edi_t_StrQualifyComposedData));
  i_RetStatus = esg_esg_edi_ComposedDataDecoding(ECH_D_INCHSEGEICD,
                                                 ps_ICodedCData,
                                                 i_ILgMaxCodedCData,
                                                 pi_OLgCodedCData,
                                                 &r_QuaCData,
                                                 &r_EICInternalCData);
  if (i_RetStatus != OK)
  {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_INCHSEGEICD;
  }
  else
  {
    i_IndIEData = 0;
    pr_OInternalEInterChg->i_IdExchange = r_EICInternalCData.ar_EDataTable[i_IndIEData].u_val.i_Uint32;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_EndInterChgDecoding----------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_HeaderMsgCoding
//  FULL NAME		Coding of interchange message in EDI format
//----------------------------------------------------------------------------
//  ROLE
//	Transforms internal interchange message in ASCII EDI format
//      character string
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	 The service permits to obtain ASCII character string in EDI format
//       for the message header.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide message header in EDI format
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_HeaderMsgCoding(
                  // Input parameters
                  const esg_esg_t_HeaderMsg* pr_IInternalHMsg,  // Internal interchange message
                  const uint32_t i_ILgMaxCodedCData,    // max length of coded composed data
                  // Output parameters
                  uint32_t* pi_OLgCodedCData,   // length of coded composed data
                  char* ps_OCodedCData          // pointer to coded composed data
                  )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_HeaderMsgCoding";
 // internal composed data table
  esg_esg_edi_t_StrComposedData r_HMInternalCData;
 // indice internal composed data table
  uint32_t i_IndIEData;
 // qualifier interface structure
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
 // return status of the function
  int i_RetStatus = OK;

//............................................................................
// (CD) providing coded composed data
  i_IndIEData = 0;
  r_HMInternalCData.i_NbEData = ESG_ESG_D_HMSGNBEDATA;
  r_HMInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT16;
  r_HMInternalCData.ar_EDataTable[i_IndIEData++].u_val.h_Uint16 = pr_IInternalHMsg->h_MsgOrderNb;
  r_HMInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT32;
  r_HMInternalCData.ar_EDataTable[i_IndIEData++].u_val.i_Uint32 = pr_IInternalHMsg->i_MsgType;
  r_HMInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT16;
  r_HMInternalCData.ar_EDataTable[i_IndIEData].u_val.h_Uint16 = pr_IInternalHMsg->h_MsgApplNb;

  memset(&r_QuaCData, 0, sizeof(esg_esg_edi_t_StrQualifyComposedData));
  i_RetStatus = esg_esg_edi_ComposedDataCoding(ECH_D_MSGSEGHICD,
                                               &r_QuaCData,
                                               &r_HMInternalCData,
                                               i_ILgMaxCodedCData,
                                               pi_OLgCodedCData,
                                               ps_OCodedCData);
  if (i_RetStatus != OK)
  {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_MSGSEGHICD;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_HeaderMsgCoding--------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_EndMsgCoding
//  FULL NAME		Coding of message end in EDI format
//----------------------------------------------------------------------------
//  ROLE
//	Transforms internal message end in ASCII EDI format character string
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	 The service permits to obtain ASCII character string in EDI format for the message end
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide message end in EDI format
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_EndMsgCoding(
               // Input parameters
               const esg_esg_t_EndMsg* pr_IInternalEMsg,    // Internal message end
               const uint32_t i_ILgMaxCodedCData,   // max length of coded composed data
               // Output parameters
               uint32_t* pi_OLgCodedCData,     // length of coded composed data
               char* ps_OCodedCData            // pointer to coded composed data
               )
{
//----------------------------------------------------------------------------*/
  static const char* fname = "esg_esg_edi_EndMsgCoding";
 // internal composed data table
  esg_esg_edi_t_StrComposedData r_EMInternalCData;
 // indice internal composed data table
  uint32_t i_IndIEData;
 // qualifier interface structure
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
 // return status of the function
  int i_RetStatus = OK;

//............................................................................
// providing coded composed data
  i_IndIEData = 0;
  r_EMInternalCData.i_NbEData = ESG_ESG_D_EMSGNBEDATA;
  r_EMInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT16;
  r_EMInternalCData.ar_EDataTable[i_IndIEData++].u_val.h_Uint16 = pr_IInternalEMsg->h_MsgOrderNb;

  memset(&r_QuaCData, 0, sizeof(esg_esg_edi_t_StrQualifyComposedData));
  i_RetStatus = esg_esg_edi_ComposedDataCoding(ECH_D_MSGSEGEICD,
                                               &r_QuaCData,
                                               &r_EMInternalCData,
                                               i_ILgMaxCodedCData,
                                               pi_OLgCodedCData,
                                               ps_OCodedCData);
  if (i_RetStatus != OK)
  {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_MSGSEGEICD;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_EndMsgCoding-----------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_EndMsgDecoding
//  FULL NAME		Decoding from EDI format in internal message end
//----------------------------------------------------------------------------
//  ROLE
//	Transforms message end from basis type in EDI format to internal format.
//----------------------------------------------------------------------------
//  CALLING CONTEXT	
//  The service permits to obtain from the ASCII character string in EDI format
//  the internal format of message end.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide message end in internal format
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_EndMsgDecoding(
                 // Input parameters
                 const char* ps_ICodedCData,            // coded composed data
                 const uint32_t i_ILgMaxCodedCData,     // max length of coded composed data
                 // Output parameters
                 uint32_t* pi_OLgCodedCData,            // length of coded composed data
                 esg_esg_t_EndMsg* pr_OInternalEMsg     // Internal message header
                 )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_EndMsgDecoding";
 // internal composed data table
  esg_esg_edi_t_StrComposedData r_EMInternalCData;
 // indice internal composed data table
  uint32_t i_IndIEData;
 // qualifier interface structure
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
 // return status of the function
  int i_RetStatus = OK;

//............................................................................
// (CD) providing coded composed data
  i_IndIEData = 0;
  r_EMInternalCData.i_NbEData = ESG_ESG_D_EMSGNBEDATA;
  r_EMInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT16;

  memset(&r_QuaCData, 0, sizeof(esg_esg_edi_t_StrQualifyComposedData));
  i_RetStatus = esg_esg_edi_ComposedDataDecoding(ECH_D_MSGSEGEICD,
                                                 ps_ICodedCData,
                                                 i_ILgMaxCodedCData,
                                                 pi_OLgCodedCData,
                                                 &r_QuaCData,
                                                 &r_EMInternalCData);
  if (i_RetStatus != OK)
  {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_MSGSEGEICD;
  }
  else
  {
    i_IndIEData = 0;
    pr_OInternalEMsg->h_MsgOrderNb = r_EMInternalCData.ar_EDataTable[i_IndIEData++].u_val.h_Uint16;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_EndMsgDecoding---------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_HeaderApplCoding
//  FULL NAME		Coding of application header in EDI format
//----------------------------------------------------------------------------
//  ROLE
//	Transforms internal application header in ASCII EDI format character string
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	The service permits to obtain ASCII character string in EDI format for the
//	application header
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide application header in EDI format
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_HeaderApplCoding(
               // Input parameters
               const esg_esg_t_HeaderAppl* pr_IInternalHAppl,   // Internal interchange application
               const uint32_t i_ILgMaxCodedCData,   // max length of coded composed data
               // Output parameters
               uint32_t* pi_OLgCodedCData,  // length of coded composed data
               char* ps_OCodedCData         // pointer to coded composed data
               )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_HeaderApplCoding";
 // internal composed data table
  esg_esg_edi_t_StrComposedData r_HAInternalCData;
 // indice internal composed data table
  uint32_t i_IndIEData;
 // qualifier interface structure
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
 // return status of the function
  int i_RetStatus = OK;

//............................................................................
// (CD) providing coded composed data
  i_IndIEData = 0;
  r_HAInternalCData.i_NbEData = ESG_ESG_D_HAPPLNBEDATA;
  r_HAInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT16;
  r_HAInternalCData.ar_EDataTable[i_IndIEData++].u_val.h_Uint16 = pr_IInternalHAppl->h_ApplOrderNb;
  r_HAInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT32;
  r_HAInternalCData.ar_EDataTable[i_IndIEData++].u_val.i_Uint32 = pr_IInternalHAppl->i_ApplType;
  r_HAInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT32;
  r_HAInternalCData.ar_EDataTable[i_IndIEData].u_val.i_Uint32 = pr_IInternalHAppl->i_ApplLength;

  memset(&r_QuaCData, 0, sizeof(esg_esg_edi_t_StrQualifyComposedData));
  i_RetStatus = esg_esg_edi_ComposedDataCoding(ECH_D_APPLSEGHICD,
                                               &r_QuaCData,
                                               &r_HAInternalCData,
                                               i_ILgMaxCodedCData,
                                               pi_OLgCodedCData,
                                               ps_OCodedCData);
  if (i_RetStatus != OK)
  {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_APPLSEGHICD;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_HeaderApplCoding-------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_HeaderApplDecoding
//  FULL NAME		Decoding from EDI format in internal interchange header
//----------------------------------------------------------------------------
//  ROLE
//	Transforms application header from basis type in EDI format to internal format
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to obtain from the ASCII character string in EDI format
//  the internal format of application header.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide application header in internal format
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_HeaderApplDecoding(
                 // Input parameters
                 const char* ps_ICodedCData,        // coded composed data
                 const uint32_t i_ILgMaxCodedCData, // max length of coded composed data
                 // Output parameters
                 uint32_t* pi_OLgCodedCData,        // length of coded composed data
                 esg_esg_t_HeaderAppl* pr_OInternalHAppl  // Internal application header
                 )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_HeaderApplDecoding";
 // internal composed data table
  esg_esg_edi_t_StrComposedData r_HAInternalCData;
 // indice internal composed data table
  uint32_t i_IndIEData;
 // qualifier interface structure
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;
 // return status of the function
  int i_RetStatus = OK;

//............................................................................
// providing coded composed data
  i_IndIEData = 0;
  r_HAInternalCData.i_NbEData = ESG_ESG_D_HAPPLNBEDATA;
  r_HAInternalCData.ar_EDataTable[i_IndIEData++].i_type = FIELD_TYPE_UINT16;
  r_HAInternalCData.ar_EDataTable[i_IndIEData++].i_type = FIELD_TYPE_UINT32;
  r_HAInternalCData.ar_EDataTable[i_IndIEData].i_type = FIELD_TYPE_UINT32;

  memset(&r_QuaCData, 0, sizeof(esg_esg_edi_t_StrQualifyComposedData));
  i_RetStatus = esg_esg_edi_ComposedDataDecoding(ECH_D_APPLSEGHICD,
                                                 ps_ICodedCData,
                                                 i_ILgMaxCodedCData,
                                                 pi_OLgCodedCData,
                                                 &r_QuaCData,
                                                 &r_HAInternalCData);
  if (i_RetStatus != OK)
  {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_APPLSEGHICD;
  }
  else
  {
    i_IndIEData = 0;
    pr_OInternalHAppl->h_ApplOrderNb= r_HAInternalCData.ar_EDataTable[i_IndIEData++].u_val.h_Uint16;
    pr_OInternalHAppl->i_ApplType   = r_HAInternalCData.ar_EDataTable[i_IndIEData++].u_val.i_Uint32;
    pr_OInternalHAppl->i_ApplLength = r_HAInternalCData.ar_EDataTable[i_IndIEData].u_val.i_Uint32;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_HeaderApplDecoding-----------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_EndApplCoding
//  FULL NAME		Coding of application end in EDI format
//----------------------------------------------------------------------------
//  ROLE
//	Saves application end in ASCII EDI format character string
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to obtain ASCII character string in EDI format
//  for the application end.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide application end in EDI format
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_EndApplCoding(
                // Input parameters
                const uint32_t i_ILgMaxCodedCData,  // max length of coded label
                // Output parameters
                uint32_t* pi_OLgCodedCData,         // length of coded label
                char* ps_OCodedCData                // pointer to coded label
                )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_EndApplCoding";
 // return status of the function
  int i_RetStatus = OK;
 // logged text
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];

//............................................................................
// (CD) controling the length of ASCII coded EDI format application label
  *pi_OLgCodedCData = strlen(ECH_D_STR_APPESEGLABEL);
  if (i_ILgMaxCodedCData < *pi_OLgCodedCData)
  {
    i_RetStatus = ESG_ESG_D_ERR_DCDLGTOOSHORT;
    sprintf(s_LoggedText,"lg %d of %s to treat : provided lg %d ",
                         *pi_OLgCodedCData,
                         ECH_D_STR_APPESEGLABEL,
                         i_ILgMaxCodedCData);
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_LoggedText;
  }

// (CD) Saving ASCII coded EDI format application label
  if (i_RetStatus == OK)
  {
    memcpy(ps_OCodedCData, ECH_D_STR_APPESEGLABEL, strlen(ECH_D_STR_APPESEGLABEL));
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_EndApplCoding----------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_EndApplDecoding
//  FULL NAME		Coding of application end in EDI format
//----------------------------------------------------------------------------
//  ROLE
//	Controls application end in ASCII EDI format character string
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to control ASCII character string in EDI format
//  for the application end.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide application end in EDI format
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_EndApplDecoding(
                  // Input parameters
                  const char* ps_ICodedCData,           // pointer to coded label
                  const uint32_t i_ILgMaxCodedCData,    // max length of coded label
                  // Output parameters
                  uint32_t* pi_OLgCodedCData            // length of coded label
                  )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_EndApplDecoding";
 // return status of the function
  int i_RetStatus = OK;
 // logged text
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];

//............................................................................
// (CD) controling the length of ASCII coded EDI format application label
  *pi_OLgCodedCData = strlen(ECH_D_STR_APPESEGLABEL);
  if (i_ILgMaxCodedCData < *pi_OLgCodedCData)
  {
    i_RetStatus = ESG_ESG_D_ERR_DCDLGTOOSHORT;
    sprintf(s_LoggedText, "lg %d of %s to treat : provided lg %d ",
            *pi_OLgCodedCData,
            ECH_D_STR_APPESEGLABEL,
            i_ILgMaxCodedCData);
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_LoggedText;
  }

// (CD) controling ASCII coded EDI format application label
  if (i_RetStatus == OK)
  {
    if ((memcmp(ps_ICodedCData, ECH_D_STR_APPESEGLABEL, strlen(ECH_D_STR_APPESEGLABEL))) != 0)
    {
      i_RetStatus = ESG_ESG_D_ERR_BADSEGLABEL;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_STR_APPESEGLABEL;
    }
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_EndApplDecoding--------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_GetLengthEData
//  FULL NAME		Getting length of Coded elementary data in ASCII EDI format
//----------------------------------------------------------------------------
//  ROLE
//	Calculate length of elementary data character string.
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to obtain the length of ASCII character string
//  in EDI format for a DED entity.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide length of DED entity in EDI format without EDI identifier.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_GetLengthEData(
                 // Input parameters
                 const elemtype_item_t* pr_IFormatEData, // format elementary data
                 const uint32_t i_ILgString,    // length of various string
                 // Output parameters
                 uint32_t* pi_OLgEData  // length of elementary data
                 )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_GetLengthEData";
 // lengths of elementary data
  uint32_t i_LgEData;
  uint32_t i_LgPartEData;
 // searching result pointer
  char* s_SearchResult;
 // part of the format
  char s_PartFormat[ECH_D_FORMATLG];
 // return status of the function
  int i_RetStatus = OK;

//............................................................................
  i_RetStatus = OK;
  i_LgEData = 0;

#if 0
// (CD) obtaining the length of ASCII coded EDI format elemenmtary data
  if ((pr_IFormatEData->tm_type == ECH_D_TYPINT)
   || (pr_IFormatEData->tm_type == ECH_D_TYPLOGI)
   || ((pr_IFormatEData->tm_type == ECH_D_TYPSTR)
       &&
      ((strcmp(pr_IFormatEData->format_size, ECH_D_FORMATEDATA_STREMPTY) != 0))))
  {
    sscanf(pr_IFormatEData->format_size, "%d", &i_LgEData);
  }

  if (pr_IFormatEData->tm_type == ECH_D_TYPTIME)
  {
    if (pr_IFormatEData->format_size[0] == ECH_D_FORMATEDATA_TIMEUTC)
    {
      sscanf(&pr_IFormatEData->format_size[1], "%d", &i_LgEData);
      if ((i_LgEData != ECH_D_FULLTIMELG) &&
          (i_LgEData != ECH_D_VERYFULLTIMELG) &&
          (i_LgEData != ECH_D_LIGHTTIMELG))
      {
        i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " Bad UTC time format: " << pr_IFormatEData->format_size;
      }
    }
    else
    {
      i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << "time indicator not in UTC format (" << pr_IFormatEData->format_size << ") doesn't supported";
    }
  }

  if (pr_IFormatEData->tm_type == ECH_D_TYPREAL)
  {
    strcpy(s_PartFormat, pr_IFormatEData->s_Format);
    if ((s_SearchResult = strchr(s_PartFormat, ECH_D_FORMATEDATA_REALPRES)) != NULL)
    {
      *s_SearchResult = '\0';
      sscanf(s_PartFormat, "%d", &i_LgEData);
      s_SearchResult++;
      sscanf(s_SearchResult, "%d", &i_LgPartEData);
      i_LgEData = i_LgEData + i_LgPartEData + 1;
    }
    else if (NULL != (s_SearchResult = strchr(s_PartFormat, ECH_D_FORMATEDATA_REALEXP)))
    {
      // real format E
      *s_SearchResult = '\0';
      sscanf(s_PartFormat, "%d", &i_LgEData);
      s_SearchResult++;
      sscanf(s_SearchResult, "%d", &i_LgPartEData);
      i_LgEData = i_LgEData + i_LgPartEData + 1;
    }
    else
    {
      i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", No separate character for real";
    }
  }

  if ((pr_IFormatEData->tm_type == ECH_D_TYPSTR)
   && ((strcmp(pr_IFormatEData->s_Format, ECH_D_FORMATEDATA_STREMPTY) == 0)))
  {
    i_LgEData = i_ILgString;
  }
#else

  switch (pr_IFormatEData->tm_type) {
    case TM_TYPE_LOGIC:
    case TM_TYPE_INTEGER:
      sscanf(pr_IFormatEData->format_size, "%d", &i_LgEData);
      break;

    case TM_TYPE_TIME:
      if (pr_IFormatEData->format_size[0] == ECH_D_FORMATEDATA_TIMEUTC)
      {
        sscanf(&pr_IFormatEData->format_size[1], "%d", &i_LgEData);
        if ((i_LgEData != ECH_D_FULLTIMELG) &&
            (i_LgEData != ECH_D_VERYFULLTIMELG) &&
            (i_LgEData != ECH_D_LIGHTTIMELG))
        {
          i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << " Bad UTC time format: " << pr_IFormatEData->format_size;
        }
      }
      else
      {
        i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << "time indicator not in UTC format (" << pr_IFormatEData->format_size << ") isn't supported";
      }
      break;

    case TM_TYPE_STRING:
      if (0 == (strcmp(pr_IFormatEData->format_size, ECH_D_FORMATEDATA_STREMPTY))) {
        i_LgEData = i_ILgString;
      }
      else {
        sscanf(pr_IFormatEData->format_size, "%d", &i_LgEData);
      }
      break;

    case TM_TYPE_REAL:
      strcpy(s_PartFormat, pr_IFormatEData->format_size);
      if ((s_SearchResult = strchr(s_PartFormat, ECH_D_FORMATEDATA_REALPRES)) != NULL)
      {
        *s_SearchResult = '\0';
        sscanf(s_PartFormat, "%d", &i_LgEData);
        s_SearchResult++;
        sscanf(s_SearchResult, "%d", &i_LgPartEData);
        i_LgEData = i_LgEData + i_LgPartEData + 1;
      }
      else if (NULL != (s_SearchResult = strchr(s_PartFormat, ECH_D_FORMATEDATA_REALEXP)))
      {
        // real format E
        *s_SearchResult = '\0';
        sscanf(s_PartFormat, "%d", &i_LgEData);
        s_SearchResult++;
        sscanf(s_SearchResult, "%d", &i_LgPartEData);
        i_LgEData = i_LgEData + i_LgPartEData + 1;
      }
      else
      {
        i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", No separate character for real";
      }
      break;

      default:
        i_RetStatus = NOK;
  }

#endif

  *pi_OLgEData = i_LgEData;

  return (i_RetStatus);
}
//-END esg_esg_edi_GetLengthEData---------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_GetForLgQuaEData
//  FULL NAME		Getting format and length of coded elementary data qualifier
//  in ASCII EDI format
//----------------------------------------------------------------------------
//  ROLE
//	Get format and calculate length of variable length elementary data.
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	The service permits to obtain the format and the length of ASCII character
//	string in EDI format for the DED variable length qualifier entity.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide format and length of DED variable length qualifier entity in EDI
//	format without EDI identifier.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_GetForLgQuaEData(
                   // Output parameters
                   elemtype_item_t* pr_OFormatEData, // format elementary data
                   uint32_t * pi_OLgEData       // length of elementary data
                   )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_GetForLgQuaEData";
 // length data
  uint32_t i_LgEData;
 // return status of the function
  int i_RetStatus = OK;

//............................................................................
  *pi_OLgEData = 0;

  if (NULL == (pr_OFormatEData = esg_esg_odm_ConsultExchDataArr(ECH_D_QUAVARLGED_IDED)))
  {
    i_RetStatus = NOK;
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_QUAVARLGED_IDED;
  }

  if (i_RetStatus == OK)
  {
    if (pr_OFormatEData->tm_type != TM_TYPE_INTEGER)
    {
      i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_QUAVARLGED_IDED;
    }
  }

  if (i_RetStatus == OK)
  {
    // Providing the length of ASCII coded elementary data qualifier without id
    i_RetStatus = esg_esg_edi_GetLengthEData(pr_OFormatEData, 0, &i_LgEData);
    if (i_RetStatus != OK)
    {
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << ECH_D_QUAVARLGED_IDED;
    }
    else
    {
      *pi_OLgEData = i_LgEData;
    }
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_GetForLgQuaEData-------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_GetLengthFullCtrlEData
//  FULL NAME		Getting complete length of coded variable / fixed
// elementary data in ASCII EDI format and controling of the associated format
//----------------------------------------------------------------------------
//  ROLE
//	Calculate length of variable / fixed elementary data with identifier.
//	Control the associated format
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to obtain the complete length of ASCII
//  character string in EDI format for the DED variable / fixed entity
//	and to control the associated format.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide complete length of DED variable / fixed entity in EDI format with EDI identifier
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_GetLengthFullCtrlEData(
                 // Input parameters
                 const elemtype_item_t* pr_IFormatEData, // format elementary data
                 const bool b_ILgQuaEData,  // Elementary data length qualify indicator
                 const uint32_t i_ILgString,// length of various string
                 // Output parameters
                 // lengths of elementary data
                 uint32_t* pi_OLgFullEData,
                 uint32_t* pi_OLgEData,
                 uint32_t* pi_OLgQuaEData
                 )
{
//----------------------------------------------------------------------------*/
  static const char* fname = "esg_esg_edi_GetLengthFullCtrlEData";
 // length of elementary data
  uint32_t i_LgEData;
  uint32_t i_LgQuaEData;
 // format of elementary data qualifier
  elemtype_item_t* r_FormatQuaEData = NULL;
 // return status of the function
  int i_RetStatus = OK;
 // logged text
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];

//............................................................................*/
  *pi_OLgFullEData = 0;
  *pi_OLgEData = 0;
  *pi_OLgQuaEData = 0;

  // Providing the length of ASCII coded elementary data without id
  i_RetStatus = esg_esg_edi_GetLengthEData(pr_IFormatEData, i_ILgString, &i_LgEData);
  if (i_RetStatus != OK)
  {
    LOG(ERROR) << fname << ": rc=" << i_RetStatus;
  }

  if (i_RetStatus == OK)
  {

    if ((pr_IFormatEData->tm_type == TM_TYPE_STRING) && ((strcmp(pr_IFormatEData->format_size, ECH_D_FORMATEDATA_STREMPTY)) == 0))
    {

      if (b_ILgQuaEData)
      {
        // Providing the basis type of the elementary data qualifier without id
        i_RetStatus = esg_esg_edi_GetForLgQuaEData(r_FormatQuaEData, &i_LgQuaEData);
        if (i_RetStatus != OK)
        {
          LOG(ERROR) << fname << ": rc=" << i_RetStatus << ECH_D_QUAVARLGED_IDED;
        }
        else
        {
          *pi_OLgEData = i_LgEData;
          *pi_OLgQuaEData = i_LgQuaEData;
          *pi_OLgFullEData = i_LgEData + i_LgQuaEData + (2 * ECH_D_ELEMIDLG);
        }
      }
      else
      {
        i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
        sprintf (s_LoggedText, " %d is not compatible with %s", pr_IFormatEData->tm_type, ECH_D_QUAVARLGED_IDED);
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_LoggedText;
      }
    }
    else
    {
      if (!b_ILgQuaEData)
      {
        if ((pr_IFormatEData->tm_type == TM_TYPE_STRING) && ((strcmp(pr_IFormatEData->format_size, ECH_D_FORMATEDATA_STREMPTY)) != 0))
        {
          if (i_LgEData != i_ILgString)
          {
            i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
            sprintf (s_LoggedText, " internal string length %d", i_ILgString);
            LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_LoggedText;
          }
        }

        if (i_RetStatus == OK)
        {
          *pi_OLgEData = i_LgEData;
          *pi_OLgFullEData = i_LgEData + ECH_D_ELEMIDLG;
        }

      }
      else
      {
        i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
        sprintf (s_LoggedText, " %d is not compatible with %s", pr_IFormatEData->tm_type, ECH_D_QUAVARLGED_IDED);
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_LoggedText;
      }
    }
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_GetLengthFullCtrlEData-------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_IntegerCoding
//  FULL NAME		Coding integer elementary data in EDI ASCII format
//----------------------------------------------------------------------------
//  ROLE
//	Codes internal integer elementary data character string
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	 The service permits to obtain ASCII character string in EDI format
//       for a basis type TYPINT or TYPLOGIC.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide an integer in EDI format
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_IntegerCoding(
                // Input parameters
                const char* s_IFormat,      // string format
                const esg_esg_edi_t_StrElementaryData* pr_IInternalEData,// internal elementary data
                // Output parameters
                char* ps_OCodedEData
                )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_IntegerCoding";
 // coded format string
  char s_CodedFormat[ECH_D_CODEDFORMATLG];
  uint32_t i_NbDigit;
  char s_CodedEData[ECH_D_APPLSEGLG];
 // return status of the function
  int i_RetStatus = OK;
 // logged text
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];

//............................................................................*/
  i_RetStatus = OK;

  sscanf(s_IFormat, "%d", &i_NbDigit);
  memset(s_CodedEData, 0, sizeof(s_CodedEData));

  strcpy(s_CodedFormat, ECH_D_POURCENTSTR);
  strcat(s_CodedFormat, ECH_D_ZEROSTR);
  strcat(s_CodedFormat, s_IFormat);
  strcat(s_CodedFormat, ECH_D_DECIMALSTR);
  switch (pr_IInternalEData->i_type)
  {
    case FIELD_TYPE_LOGIC:
      sprintf(s_CodedEData, s_CodedFormat, pr_IInternalEData->u_val.b_Logical);
      break;
    case FIELD_TYPE_UINT8:
      sprintf(s_CodedEData, s_CodedFormat, pr_IInternalEData->u_val.o_Uint8);
      break;
    case FIELD_TYPE_UINT16:
      sprintf(s_CodedEData, s_CodedFormat, pr_IInternalEData->u_val.h_Uint16);
      break;
    case FIELD_TYPE_UINT32:
      sprintf(s_CodedEData, s_CodedFormat, pr_IInternalEData->u_val.i_Uint32);
      break;
    case FIELD_TYPE_INT8:
      sprintf(s_CodedEData, s_CodedFormat, pr_IInternalEData->u_val.o_Int8);
      break;
    case FIELD_TYPE_INT16:
      sprintf(s_CodedEData, s_CodedFormat, pr_IInternalEData->u_val.h_Int16);
      break;
    case FIELD_TYPE_INT32:
      sprintf(s_CodedEData, s_CodedFormat, pr_IInternalEData->u_val.i_Int32);
      break;
    default:
      i_RetStatus = ESG_ESG_D_ERR_BADEDINTERTYPE;
      sprintf (s_LoggedText, "Internal data type : %d", pr_IInternalEData->i_type);
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_LoggedText;
  }

  // (CD) Verify coded string length and memorise coded elementary data
  if (i_RetStatus == OK)
  {
    if (i_NbDigit >= strlen(s_CodedEData))
    {
      strcpy(ps_OCodedEData, s_CodedEData);
    }
    else
    {
      i_RetStatus = ESG_ESG_D_ERR_DEDLGTOOSHORT;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << " Internal data length " << strlen(s_CodedEData);
    }
  }
  return (i_RetStatus);
}
//-END esg_esg_edi_IntegerCoding----------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_RealCoding
//  FULL NAME		Coding real elementary data in EDI ASCII format
//----------------------------------------------------------------------------
//  ROLE
//	Codes internal real elementary data character string.
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	 The service permits to obtain ASCII character string in EDI format
//       for a basis type TYPREAL
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide a real in EDI format.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_RealCoding(
             // Input parameters
             const char* s_IFormat, // string format
             const esg_esg_edi_t_StrElementaryData* pr_IInternalEData,  // internal elementary data
             // Output parameters
             char* ps_OCodedEData   // string
             )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_RealCoding";
 // format
  char s_PartFormat[ECH_D_FORMATLG];
 // coded format strings
  char s_CodedFormat[ECH_D_CODEDFORMATLG + 1];
  char s_PartCodedFormat[ECH_D_CODEDFORMATLG + 1];
 // lengths of elementary data
  uint32_t i_LgPart1EData;
  uint32_t i_LgPart2EData;
 // searching result pointer
  char* s_SearchResult;
  uint32_t i_NbDigit;
  char s_CodedEData[ECH_D_APPLSEGLG];
 // return status of the function
  int i_RetStatus = OK;
 // logged text
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];
// addition of data for exponent format
// mantissa decimal value of the exponent normalized float representation
  double g_Part1EData;

// exponent value of the exponent normalized float representation
  uint32_t i_Part2EData;
  char s_CodedMantissa[ECH_D_APPLSEGLG];
  char s_CodedExponent[ECH_D_APPLSEGLG];
  char s_CodedExponentFormat[ECH_D_CODEDFORMATLG + 1];

// FFT 2412 precision for some measure with "e" format the precision must be
// specified if we want more than 6 digits after the "."
// The precision is the number of digits of mantissa of the asked format
// minus 2 (the first digit before "." and the ".")
  uint32_t i_Precision;

//............................................................................
  i_RetStatus = OK;
  memset(s_CodedEData, 0, sizeof(s_CodedEData));

  strcpy(s_PartFormat, s_IFormat);
  if ((s_SearchResult = strchr(s_PartFormat, ECH_D_FORMATEDATA_REALPRES)) != NULL)
  {
    *s_SearchResult = '\0';
    sscanf(s_PartFormat, "%d", &i_LgPart1EData);
    s_SearchResult++;
    sscanf(s_SearchResult, "%d", &i_LgPart2EData);
    i_LgPart1EData = i_LgPart1EData + i_LgPart2EData;
    i_LgPart1EData++;
    strcpy(s_CodedFormat, ECH_D_POURCENTSTR);
    strcat(s_CodedFormat, ECH_D_ZEROSTR);
    sprintf(s_PartCodedFormat, "%d", i_LgPart1EData);
    strcat(s_CodedFormat, s_PartCodedFormat);
    strcat(s_CodedFormat, ECH_D_POINTSTR);
    sprintf(s_PartCodedFormat, "%d", i_LgPart2EData);
    strcat(s_CodedFormat, s_PartCodedFormat);
    strcat(s_CodedFormat, ECH_D_FLOATSTR);

    i_NbDigit = i_LgPart1EData;

    switch (pr_IInternalEData->i_type)
    {
      case FIELD_TYPE_DOUBLE:
        sprintf (s_CodedEData, s_CodedFormat, getGoodDoubleValue(pr_IInternalEData->u_val.g_Double, 0.0));
        break;
      case FIELD_TYPE_FLOAT:
        sprintf(s_CodedEData, s_CodedFormat, getGoodFloatValue(pr_IInternalEData->u_val.f_Float, 0.0));
        break;
      default:
        i_RetStatus = ESG_ESG_D_ERR_BADEDINTERTYPE;
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " Internal data type: " << pr_IInternalEData->i_type;
    }
  }

// exponent float format
// The format has the n"e"m form with n digits before the "e" symbol
// (mantissa) and m digits after the "e" symbol (exponent)
  else if ((s_SearchResult = strchr(s_PartFormat, ECH_D_FORMATEDATA_REALEXP)) != NULL)
  {
    *s_SearchResult = '\0';
    sscanf(s_PartFormat, "%d", &i_LgPart1EData);

// compute the precision = i_LgPart1EData - 2
    if (i_LgPart1EData > 8)
    {
      i_Precision = i_LgPart1EData - 2;
    }
    else
    {
      i_Precision = 6;
    }

    s_SearchResult++;
    sscanf(s_SearchResult, "%d", &i_LgPart2EData);
    strcpy(s_CodedFormat, ECH_D_POURCENTSTR);
    strcat(s_CodedFormat, ECH_D_ZEROSTR);
    sprintf(s_PartCodedFormat, "%d", i_LgPart1EData);
    strcat(s_CodedFormat, s_PartCodedFormat);

// -----------------------------------------------------------
// 15-04-99 by TH : addition of precision format
// The format has the "%0"n"."p"e" form: n digits before exponent
// and p digits for precision
    strcat(s_CodedFormat, ECH_D_POINTSTR);
    sprintf(s_PartCodedFormat, "%d", i_Precision);
    strcat(s_CodedFormat, s_PartCodedFormat);

// ----------------------------------------------------------- */

    strcat(s_CodedFormat, ECH_D_EXPSTR);
    i_NbDigit = i_LgPart1EData + i_LgPart2EData + 1;

    switch (pr_IInternalEData->i_type)
    {
      case FIELD_TYPE_DOUBLE:
        sprintf(s_CodedEData, s_CodedFormat, getGoodDoubleValue(pr_IInternalEData->u_val.g_Double, DBL_MAX));
        break;
      case FIELD_TYPE_FLOAT:
        sprintf(s_CodedEData, s_CodedFormat, getGoodFloatValue(pr_IInternalEData->u_val.f_Float, FLT_MAX));
        break;
      default:
        i_RetStatus = ESG_ESG_D_ERR_BADEDINTERTYPE;
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " Internal data type: " << pr_IInternalEData->i_type;
    }

// normalization of the real value in a normalized exponent format
    if (i_RetStatus == OK)
    {
      strcpy(s_CodedExponent, "");
      strcpy(s_CodedMantissa, "");
      if ((s_SearchResult = strchr(s_CodedEData, ECH_D_FORMATEDATA_REALEXP)) != NULL)
      {
        *s_SearchResult = '\0';
        // GEV: sscanf(s_CodedEData, ECH_D_DOUBLESTR, &g_Part1EData);
        sscanf(s_CodedEData, "%lf", &g_Part1EData);

// The mantissa is coded on number of digits before "e" of the given format
        strncpy(s_CodedMantissa, s_CodedEData, i_LgPart1EData);

// Add end-of-string for the mantissa part, to avoid the unexpected interpretation of following characters which are not always zero
        s_CodedMantissa[i_LgPart1EData] = '\0';

        s_SearchResult += 1;
        sscanf(s_SearchResult, "%d", &i_Part2EData);
        strcpy(s_CodedExponentFormat, ECH_D_POURCENTSTR);
        strcat(s_CodedExponentFormat, ECH_D_ZEROSTR);
        sprintf(s_PartCodedFormat, "%d", i_LgPart2EData);
        strcat(s_CodedExponentFormat, s_PartCodedFormat);
        strcat(s_CodedExponentFormat, ECH_D_DECIMALSTR);
        sprintf(s_CodedExponent, s_CodedExponentFormat, i_Part2EData);
        strcpy(s_CodedEData, s_CodedMantissa);
        strcat(s_CodedEData, ECH_D_EXPSTR);
        strcat(s_CodedEData, s_CodedExponent);
      }
      else
      {
        i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_CodedEData;
      }
    }
  }
  else
  {
    i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IFormat;
  }

// (CD) Verify coded string length and memorise coded elementary data
// The asked format must be identical to the coded length
// An error is also detected if the format length is greater than the coded
// length which means that the asked precision is not respected
// This will be disturbing for the distant site
  if (i_RetStatus == OK)
  {
    if (i_NbDigit == strlen(s_CodedEData))
    {
      strcpy(ps_OCodedEData, s_CodedEData);
    }
    else if (i_NbDigit > strlen(s_CodedEData))
    {
      i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
      sprintf (s_LoggedText, "Format length= %d Coded length= %d", i_NbDigit, strlen(s_CodedEData));
      LOG(ERROR) << fname << i_RetStatus << " " << s_LoggedText;
    }
    else
    {
      i_RetStatus = ESG_ESG_D_ERR_DEDLGTOOSHORT;
      sprintf (s_LoggedText, "Format length= %d Coded length= %d", i_NbDigit, strlen(s_CodedEData));
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << s_LoggedText;
    }
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_RealCoding-------------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_TimeCoding
//  FULL NAME		Coding time elementary data in EDI ASCII format
//----------------------------------------------------------------------------
//  ROLE
//	Codes internal time elementary data character string
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to obtain ASCII character string in EDI format
//  for a basis type TYPTIME.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide a time in EDI format.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_TimeCoding(
                 // Input parameters
                 const char* s_IFormat, // string format
                 const esg_esg_edi_t_StrElementaryData* pr_IInternalEData,  // internal elementary data
                 // Output parameters
                 char* ps_OCodedEData   // string
                 )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_TimeCoding";
 // length of date in EDI format
  uint32_t i_LgEData;
 // time string
  struct tm *pr_AscUTCTime;
  char s_PartTime[ECH_D_FULLTIMELG + 1];
 // milli seconds
  uint32_t i_MilliSec;
// 15-12-98 by TH : micro seconds */
  uint32_t i_MicroSec;
 // return status of the function
  int i_RetStatus = OK;
 // logged text
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];

//............................................................................
  i_RetStatus = OK;

  if (pr_IInternalEData->i_type == FIELD_TYPE_DATE)
  {
    sscanf(&s_IFormat[1], "%d", &i_LgEData);
    pr_AscUTCTime = gmtime ((time_t *) & pr_IInternalEData->u_val.d_Timeval.tv_sec);

// The year format has now 4 digits
// In the tm structure we must add 1900 to the year field
    pr_AscUTCTime->tm_year += 1900;
    sprintf(ps_OCodedEData, ECH_D_FORMATYEAR, pr_AscUTCTime->tm_year);

// 15-12-98 by TH : the tm month field has a value from 0 to 11 and the coded value is from 1 to 12
    pr_AscUTCTime->tm_mon += 1;
    sprintf(s_PartTime, ECH_D_FORMATTIME, (int32_t) pr_AscUTCTime->tm_mon);
    strcat(ps_OCodedEData, s_PartTime);
    sprintf(s_PartTime, ECH_D_FORMATTIME, (int32_t) pr_AscUTCTime->tm_mday);
    strcat(ps_OCodedEData, s_PartTime);
    sprintf(s_PartTime, ECH_D_FORMATTIME, (int32_t) pr_AscUTCTime->tm_hour);
    strcat(ps_OCodedEData, s_PartTime);
    sprintf(s_PartTime, ECH_D_FORMATTIME, (int32_t) pr_AscUTCTime->tm_min);
    strcat(ps_OCodedEData, s_PartTime);
    sprintf(s_PartTime, ECH_D_FORMATTIME, (int32_t) pr_AscUTCTime->tm_sec);
    strcat(ps_OCodedEData, s_PartTime);

// 15-12-98 by TH : addition of date with micro-sec
    if ((i_LgEData == ECH_D_FULLTIMELG)
     || (i_LgEData == ECH_D_VERYFULLTIMELG))
    {
      i_MilliSec = pr_IInternalEData->u_val.d_Timeval.tv_usec / 1000; /* ESG_ESG_EDI_D_MICROMILLICONVUNITY */

// A problem happened sometime on this conversion action
// a %03d format gives a minimum number of digits but not a maximum number
// So, a test on the milli-sec value is performed and a trace added
      if (i_MilliSec > 999)
      {
        sprintf(s_LoggedText, "Incorrect milli-sec value: %d - Forced to 0", i_MilliSec);
        LOG(WARNING) << fname << "ESG_ESG_D_ERR_DCDLGTOOSHORT: " << s_LoggedText;
        i_MilliSec = 0;
      }
      sprintf(s_PartTime, ECH_D_FORMATMILL, i_MilliSec);
      strcat(ps_OCodedEData, s_PartTime);
    }

    if (i_LgEData == ECH_D_VERYFULLTIMELG)
    {
      i_MicroSec = pr_IInternalEData->u_val.d_Timeval.tv_usec - (i_MilliSec * 1000 /*ESG_ESG_EDI_D_MICROMILLICONVUNITY*/);
      sprintf(s_PartTime, ECH_D_FORMATMICR, i_MicroSec);
      strcat(ps_OCodedEData, s_PartTime);
    }
  }
  else
  {
    i_RetStatus = ESG_ESG_D_ERR_BADEDINTERTYPE;
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", Internal data type: " << pr_IInternalEData->i_type;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_TimeCoding-------------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_IntegerDecoding
//  FULL NAME		Decoding integer elementary data in internal format
//----------------------------------------------------------------------------
//  ROLE
//	Decodes ASCII EDI format integer elementary data in internal format
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//	 The service permits to obtain internal format for ASCII character
//       string in EDI format for a basis type TYPINT or TYPLOGIC
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide an integer from EDI format.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_IntegerDecoding(
                  // Input parameters
                  const char* s_IFormat,    // string format
                  const char* ps_ICodedEData,   // internal elementary data
                  // Output parameters
                  esg_esg_edi_t_StrElementaryData* pr_IOInternalEData   // string
                  )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_IntegerDecoding";
 // coded format string
  char s_CodedFormat[ECH_D_CODEDFORMATLG];
 // return status of the function
  int i_RetStatus = OK;
 // Scanned data
  int i_ScanData;

//............................................................................*/
  i_RetStatus = OK;

  strcpy(s_CodedFormat, ECH_D_POURCENTSTR);
  strcat(s_CodedFormat, ECH_D_ZEROSTR);
  strcat(s_CodedFormat, s_IFormat);
  strcat(s_CodedFormat, ECH_D_DECIMALSTR);
  switch (pr_IOInternalEData->i_type)
  {
    case FIELD_TYPE_LOGIC:
      sscanf(ps_ICodedEData, s_CodedFormat, &i_ScanData);
      pr_IOInternalEData->u_val.b_Logical = (bool) i_ScanData;
      break;
    case FIELD_TYPE_UINT8:
      sscanf(ps_ICodedEData, s_CodedFormat, &i_ScanData);
      pr_IOInternalEData->u_val.o_Uint8 = (uint8_t) i_ScanData;
      break;
    case FIELD_TYPE_UINT16:
      sscanf(ps_ICodedEData, s_CodedFormat, &i_ScanData);
      pr_IOInternalEData->u_val.h_Uint16 = (uint16_t) i_ScanData;
      break;
    case FIELD_TYPE_UINT32:
      sscanf(ps_ICodedEData, s_CodedFormat, &i_ScanData);
      pr_IOInternalEData->u_val.i_Uint32 = (uint32_t) i_ScanData;
      break;
    case FIELD_TYPE_INT8:
      sscanf(ps_ICodedEData, s_CodedFormat, &i_ScanData);
      pr_IOInternalEData->u_val.o_Int8 = (int8_t) i_ScanData;
      break;
    case FIELD_TYPE_INT16:
      sscanf(ps_ICodedEData, s_CodedFormat, &i_ScanData);
      pr_IOInternalEData->u_val.h_Int16 = (int16_t) i_ScanData;
      break;
    case FIELD_TYPE_INT32:
      sscanf(ps_ICodedEData, s_CodedFormat, &i_ScanData);
      pr_IOInternalEData->u_val.i_Int32 = (int32_t) i_ScanData;
      break;
    default:
      i_RetStatus = ESG_ESG_D_ERR_BADEDINTERTYPE;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus << ", Internal data type: " << pr_IOInternalEData->i_type;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_IntegerDecoding--------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_RealDecoding
//  FULL NAME		Decoding real elementary data in internal format
//----------------------------------------------------------------------------
//  ROLE
//	Codes internal real elementary data character string.
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to obtain internal format from ASCII character
//  string in EDI format for a basis type TYPREAL.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide a real from EDI format to internal format.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_RealDecoding(
                   // Input parameters
                   const char* s_IFormat,       // string format
                   const char * ps_ICodedEData, // string
                   // Output parameters
                   esg_esg_edi_t_StrElementaryData* pr_IOInternalEData  // internal elementary data
                   )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_RealDecoding";
 // format
  char s_PartFormat[ECH_D_FORMATLG];
 // coded format strings
  char s_CodedFormat[ECH_D_CODEDFORMATLG + 1];
 // lengths of elementary data
  uint32_t i_LgPart1EData;
  uint32_t i_LgPart2EData;
 // searching result pointer
  char* s_SearchResult;
 // return status of the function
  int i_RetStatus = OK;
 // Scanned data
  double g_ScanData;
//............................................................................*/

  i_RetStatus = OK;

  strcpy(s_PartFormat, s_IFormat);
  if (NULL != (s_SearchResult = strchr(s_PartFormat, ECH_D_FORMATEDATA_REALPRES)))
  {
    *s_SearchResult = '\0';
    sscanf(s_PartFormat, "%d", &i_LgPart1EData);
    s_SearchResult++;
    sscanf(s_SearchResult, "%d", &i_LgPart2EData);
    i_LgPart1EData = i_LgPart1EData + i_LgPart2EData;
    i_LgPart1EData++;
    strcpy(s_CodedFormat, ECH_D_POURCENTSTR);
    strcat(s_CodedFormat, ECH_D_DOUBLESTR);
    switch (pr_IOInternalEData->i_type)
    {
      case FIELD_TYPE_DOUBLE:
        sscanf(ps_ICodedEData, s_CodedFormat, &g_ScanData);
        pr_IOInternalEData->u_val.g_Double = (double) g_ScanData;
        break;
      case FIELD_TYPE_FLOAT:
        sscanf(ps_ICodedEData, s_CodedFormat, &g_ScanData);
        pr_IOInternalEData->u_val.f_Float = (float) g_ScanData;
        break;
      default:
        i_RetStatus = ESG_ESG_D_ERR_BADEDINTERTYPE;
        LOG(ERROR) << fname << i_RetStatus << " Internal data type:" << pr_IOInternalEData->i_type;
    }
  }
  else if (NULL != (s_SearchResult = strchr(s_PartFormat, ECH_D_FORMATEDATA_REALEXP))) // exponent real format E
  {
    switch (pr_IOInternalEData->i_type)
    {
      case FIELD_TYPE_DOUBLE:
        sscanf(ps_ICodedEData, "%lf", &g_ScanData);
        pr_IOInternalEData->u_val.g_Double = (double) g_ScanData;
        break;
      case FIELD_TYPE_FLOAT:
        sscanf(ps_ICodedEData, "%lf", &g_ScanData);
        pr_IOInternalEData->u_val.f_Float = (float) g_ScanData;
        break;
      default:
        i_RetStatus = ESG_ESG_D_ERR_BADEDINTERTYPE;
        LOG(ERROR) << fname << ": rc=" << i_RetStatus << " Internal data type:" << pr_IOInternalEData->i_type;
    }
  }
  else
  {
    i_RetStatus = ESG_ESG_D_ERR_BADDEDFORMAT;
    LOG(ERROR) << fname << ": rc=" << i_RetStatus << " " << s_IFormat;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_RealDecoding-----------------------------------------------

//----------------------------------------------------------------------------
//  FUNCTION		esg_esg_edi_TimeDecoding
//  FULL NAME		Decoding time elementary data in internal format
//----------------------------------------------------------------------------
//  ROLE
//	Decodes time elementary data character string in internal format.
//----------------------------------------------------------------------------
//  CALLING CONTEXT
//  The service permits to obtain the internal format from ASCII
//  character string in EDI format for a basis type TYPTIME.
//----------------------------------------------------------------------------
//  NOMINAL PROCESSING
//	To provide a time in internal format.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_esg_edi_TimeDecoding(
               // Input parameters
               const char* s_IFormat,       // string format
               const char* ps_ICodedEData,  // internal elementary data
               // Output parameters
               esg_esg_edi_t_StrElementaryData* pr_IOInternalEData  // string
               )
{
//----------------------------------------------------------------------------
  static const char* fname = "esg_esg_edi_TimeDecoding";
 // length of date in EDI format
  uint32_t i_LgEData;
 // time parameters
  struct tm r_AscUTCTime;
  time_t d_UTCTime;
  const char *ps_Time;
  char s_PartTime[ECH_D_FULLTIMELG + 1];
// milli seconds
  uint32_t i_MilliSec;
// micro seconds
  uint32_t i_MicroSec;
// return status of the function
  int i_RetStatus = OK;
// Scanned data
  int i_ScanData;
  // GEV: Значение altzone есть смещение в секундах между UTC и локальным временем. В Линукс отсутствует.
  // TODO: проверить нужность этого вычисления, поскольку время внутри приложения должно быть в едином формате (UTC),
  // и приводиться к локальному только в момент отображения.
  time_t altzone = 0;
//............................................................................
  i_RetStatus = OK;

  if (pr_IOInternalEData->i_type == FIELD_TYPE_DATE)
  {
    sscanf(&s_IFormat[1], "%d", &i_LgEData);
    memset(&r_AscUTCTime, 0, sizeof(struct tm));
    ps_Time = ps_ICodedEData;

    memset(s_PartTime, 0, sizeof(s_PartTime));

    // format for year on 4 digits
    memcpy(s_PartTime, ps_Time, ECH_D_TIMEYEARLG);
    sscanf(s_PartTime, ECH_D_FORMATYEAR, &r_AscUTCTime.tm_year);

    // the tm year field is the year from 1900
    r_AscUTCTime.tm_year -= 1900;
    ps_Time = ps_Time + ECH_D_TIMEYEARLG;
    memset(s_PartTime, 0, sizeof(s_PartTime));
    memcpy(s_PartTime, ps_Time, ECH_D_TIMEENTITYLG);
    sscanf(s_PartTime, ECH_D_FORMATTIME, &r_AscUTCTime.tm_mon);

    // The tm month field has a value from 0 to 11 and the coded value is from 1 to 12
    r_AscUTCTime.tm_mon -= 1;
    ps_Time = ps_Time + ECH_D_TIMEENTITYLG;
    memset(s_PartTime, 0, sizeof(s_PartTime));
    memcpy(s_PartTime, ps_Time, ECH_D_TIMEENTITYLG);
    sscanf(s_PartTime, ECH_D_FORMATTIME, &r_AscUTCTime.tm_mday);
    ps_Time = ps_Time + ECH_D_TIMEENTITYLG;
    memset(s_PartTime, 0, sizeof(s_PartTime));
    memcpy(s_PartTime, ps_Time, ECH_D_TIMEENTITYLG);
    sscanf(s_PartTime, ECH_D_FORMATTIME, &r_AscUTCTime.tm_hour);
    ps_Time = ps_Time + ECH_D_TIMEENTITYLG;
    memset(s_PartTime, 0, sizeof(s_PartTime));
    memcpy(s_PartTime, ps_Time, ECH_D_TIMEENTITYLG);
    sscanf(s_PartTime, ECH_D_FORMATTIME, &r_AscUTCTime.tm_min);
    ps_Time = ps_Time + ECH_D_TIMEENTITYLG;
    memset(s_PartTime, 0, sizeof(s_PartTime));
    memcpy(s_PartTime, ps_Time, ECH_D_TIMEENTITYLG);
    sscanf(s_PartTime, ECH_D_FORMATTIME, &r_AscUTCTime.tm_sec);
    // we have to change UTC time into local time for calling mktime.
    // 'altzone' and 'daylight' are global variables defined in time.h.
    tzset();
#warning "altzone exists for Solaris and doesn't present in Linux"
    r_AscUTCTime.tm_sec = r_AscUTCTime.tm_sec - altzone;

    if (daylight)
      r_AscUTCTime.tm_isdst = 1;

    d_UTCTime = mktime(&r_AscUTCTime);
    if (-1 == d_UTCTime)
    {
      i_RetStatus = ESG_ESG_D_ERR_TRANSASCUTCTIME;
      LOG(ERROR) << fname << ": rc=" << i_RetStatus;
    }
    else
    {
      pr_IOInternalEData->u_val.d_Timeval.tv_sec = d_UTCTime;

      // addition of format with micro-sec
      if ((i_LgEData == ECH_D_FULLTIMELG)
       || (i_LgEData == ECH_D_VERYFULLTIMELG))
      {
        ps_Time = ps_Time + ECH_D_TIMEENTITYLG;
        memset(s_PartTime, 0, sizeof(s_PartTime));
        memcpy(s_PartTime, ps_Time, ECH_D_TIMEMILLLG);
        sscanf(s_PartTime, ECH_D_FORMATMILL, &i_MilliSec);
        i_ScanData = i_MilliSec * 1000; // ESG_ESG_EDI_D_MICROMILLICONVUNITY
        pr_IOInternalEData->u_val.d_Timeval.tv_usec = i_ScanData;
      }

      if (i_LgEData == ECH_D_VERYFULLTIMELG)
      {
        ps_Time = ps_Time + ECH_D_TIMEMILLLG;
        memset(s_PartTime, 0, sizeof(s_PartTime));
        memcpy(s_PartTime, ps_Time, ECH_D_TIMEMICRLG);
        sscanf(s_PartTime, ECH_D_FORMATMICR, &i_MicroSec);
        pr_IOInternalEData->u_val.d_Timeval.tv_usec += i_MicroSec;
      }
    }
  }
  else
  {
    i_RetStatus = ESG_ESG_D_ERR_BADEDINTERTYPE;
    LOG(ERROR) << fname << i_RetStatus << " Internal data type: " << pr_IOInternalEData->i_type;
  }

  return (i_RetStatus);
}
//-END esg_esg_edi_TimeDecoding-----------------------------------------------


// Consulting of an entry in the exchanged composed data Table (DCD_ELEMSTRUCT)
// замена esg_esg_odm_t_ExchCompElem на elemstruct_item_t
//          esg_esg_odm_t_ExchCompElem.s_ExchCompId   => elemstruct_item_t.name
//          esg_esg_odm_t_ExchCompElem.i_NbData       => elemstruct_item_t.num_fileds
//          esg_esg_odm_t_ExchCompElem.as_ElemDataArr => elemstruct_item_t.fields
// --------------------------------------------------------------
elemstruct_item_t* ExchangeTranslator::esg_esg_odm_ConsultExchCompArr(const char* dcd_name)
{
  elemstruct_item_t* dcd_element = NULL;
  std::map < const std::string, elemstruct_item_t >::iterator it_es;

  LOG(INFO) << "search DCD: " << dcd_name;

  it_es = m_elemstructs.find(dcd_name);
  if (it_es != m_elemstructs.end())
  {
    dcd_element = &(*it_es).second;
  }

  return dcd_element;
}

// DED_ELEMTYPES
// Замена esg_esg_odm_t_ExchDataElem на elemtype_item_t
//      s_ExchDataId    => name
//      i_BasicType     => tm_type
//      s_Format        => format_size
// --------------------------------------------------------------
elemtype_item_t* ExchangeTranslator::esg_esg_odm_ConsultExchDataArr(const char* ded_name)
{
  elemtype_item_t* ded_element = NULL;
  std::map < const std::string, elemtype_item_t >::iterator it_et;

  LOG(INFO) << "search DED: " << ded_name;

  it_et = m_elemtypes.find(ded_name);
  if (it_et != m_elemtypes.end())
  {
    ded_element = &(*it_et).second;
  }

  return ded_element;
}

// Read An Applicative Header
// --------------------------------------------------------------
int ExchangeTranslator::esg_esg_fil_HeadApplRead(FILE* pi_IFileId, esg_esg_t_HeaderAppl* pr_OHeadAppl, int32_t* i_OLgAppl)
{
  static const char* fname = "esg_esg_fil_HeadApplRead";
  int       i_RetStatus;    // routine report
  int       i_Status;       // called routines report
  int32_t   i_i;            // loop counter
  char      s_CodedData[ECH_D_APPLSEGLG + 1];       // coded data as string
  uint32_t  i_LgCodedData;  // length of coded data
  esg_esg_edi_t_StrQualifyComposedData  r_QuaCData; // qualifiers
  esg_esg_edi_t_StrComposedData         r_InternalCData; // Internal composed data

  i_i = 0 ;
  r_InternalCData.ar_EDataTable[i_i].i_type = FIELD_TYPE_UINT16;
  i_i++;
  r_InternalCData.ar_EDataTable[i_i].i_type = FIELD_TYPE_UINT32;
  i_i++;
  r_InternalCData.ar_EDataTable[i_i].i_type = FIELD_TYPE_UINT32;
  i_i++;
  r_InternalCData.i_NbEData = i_i;

  r_QuaCData.b_QualifyUse = false;
  r_QuaCData.i_QualifyValue = 0;
  r_QuaCData.b_QualifyExist = false;

  i_Status = esg_esg_edi_GetLengthCData(
              ECH_D_APPLSEGHICD,
              &r_QuaCData,
              &r_InternalCData,
              &i_LgCodedData);

  if ( i_Status == OK )
  {
      i_Status =  esg_esg_fil_StringRead ( pi_IFileId, i_LgCodedData, s_CodedData ) ;

      if (i_Status != OK)
      {
        LOG(ERROR) << fname << ": rc=" << i_Status;
      }
  }

  // Decode applicative header
  // --------------------------------------------------------------------------
  if ( i_Status == OK )
  {
    i_Status =  esg_esg_edi_HeaderApplDecoding (
                  s_CodedData,
                  ECH_D_APPLSEGLG,
                  &i_LgCodedData,
                  pr_OHeadAppl);
  }

  if ( i_Status == OK )
  {
    *i_OLgAppl = i_LgCodedData ;
  }

  i_RetStatus = i_Status ;

  return i_RetStatus;
}

// Обработка Состояний смежных систем
// --------------------------------------------------------------
int ExchangeTranslator::processing_STATE(FILE* pi_FileId, int i_LgAppl, int i_ApplLength, const char* s_IAcqSiteId)
{
  static const char* fname = "processing_STATE";
  char s_Buffer[ECH_D_APPLSEGLG + 1];
  int i_Status = OK;
  int i_LgDone = 0; // GEV: fake
  uint32_t i_CDLength;        // composed data length
  esg_esg_edi_t_StrComposedData r_InternalCData; // decoded composed data buffer
  esg_esg_edi_t_StrQualifyComposedData r_QuaCData;

  i_Status = esg_esg_fil_StringRead (pi_FileId, i_ApplLength, s_Buffer);
 
  if ( i_Status != OK )
  {
    LOG(ERROR) << fname << ": reading body of state segment, i_LgAppl=" << i_LgAppl << ", i_ApplLength=" << i_ApplLength << ", site=" << s_IAcqSiteId;
  }

  if ( i_Status == OK )
  {
    // Read and Decode the applicatif segment
    //----------------------------------------------------
    i_Status = esg_ine_man_CDProcessing(s_Buffer,
                                        i_ApplLength - i_LgDone,
                                        &i_CDLength,
                                        &r_InternalCData,
                                       //1 &r_SubTypeElem,
                                        &r_QuaCData);
  }

  if ( i_Status == OK )
  {
#if 0
    // Get value of distant site state
    // ------------------------------------
    r_AttElem.i_AttType = ECH_D_LOC_UINT32;
    i_Status = ech_typ_GetAtt (&r_InternalCData.ar_EDataTable[0].u_val,
                               r_InternalCData.ar_EDataTable[0].i_type,
                               &i_SiteStateValue,
                               r_AttElem);
#else
    LOG(INFO) << "new " << s_IAcqSiteId << " state, type=" << r_InternalCData.ar_EDataTable[0].i_type << ", val=" << r_InternalCData.ar_EDataTable[0].u_val.i_Uint32;
#endif
  }

  if ( i_Status == OK )
  {
    LOG(INFO) << fname << ": Value of distant " << s_IAcqSiteId << " state=i_SiteStateValue";
  }   
  else
  {
    LOG(ERROR) << fname << ": get Value of distant state, rc=" << i_Status;
  }

  return i_Status;
}

// --------------------------------------------------------------
int ExchangeTranslator::esg_esg_fil_StringRead(
        FILE  *pi_IFileId,
        const int32_t i_IBufLen,
	    char* s_OBuffer)
{
  static const char* fname = "esg_esg_fil_StringRead";
  int bytes_read;
  int i_Status = OK;

  if ((i_IBufLen != (bytes_read = fread(s_OBuffer, 1, i_IBufLen, pi_IFileId))) && (!feof(pi_IFileId)))
  {
    LOG(ERROR) << fname << ": rc=" << i_Status;
    if (feof(pi_IFileId))
    {
      i_Status = ESG_ESG_D_ERR_EOF;
    }
  }
  else
  {
    i_Status = OK ;
    s_OBuffer[bytes_read] = '\0';
  }

  return i_Status;
}

//----------------------------------------------------------------------------
//  FUNCTION                            esg_ine_man_CDProcessing
//  FULL MAME
//----------------------------------------------------------------------------
//  ROLE
//  This function process a composed data bloc.
//----------------------------------------------------------------------------
//  NOTES
//  For each decoded ECH_D_LOC_STRING type attribute, memory is allocated by
//  this function. The user of this funtion should free this memory after utilisation.
//----------------------------------------------------------------------------
int ExchangeTranslator::esg_ine_man_CDProcessing
(
        // Input parameters
        const char* ps_IBuffer,     // begining of the composed data in the buffer
        const uint32_t i_IBufLen,   //length of the segment body from the composed data to process
        // Output parameters
        uint32_t* pi_OCDLen,        // composed data length
        esg_esg_edi_t_StrComposedData   *pr_OInternalCData, // decoded composed data buffer
        //1 ech_typ_t_SubTypeElem           *pr_OSubTypeElem,   // subtype buffer
        esg_esg_edi_t_StrQualifyComposedData *pr_OQuaCData  // Quality data buffer
)
{
  //----------------------------------------------------------------------------
  static const char* fname="esg_ine_man_CDProcessing";
  int   i_Status = OK;
  int   i_RetStatus = OK;
  //1 int   i_AttNumber;
  char s_ExchCompId[ECH_D_COMPIDLG + 1];
  elemstruct_item_t* r_ExchCompElem = NULL;
  //............................................................................
 
  // Read the identifier of the composed data to process
  // --------------------------------------------------------------------------
  memset(s_ExchCompId, 0, sizeof(s_ExchCompId));
  strncpy(s_ExchCompId, ps_IBuffer, ECH_D_COMPIDLG);

#if 0
  // Get the corresponding sub_type name
  //----------------------------------------------------------------------------
  if (NULL != (r_ExchCompElem == esg_esg_odm_ConsultExchCompArr(s_ExchCompId)))
  {
    // Get the corresponding sub-type structure
    //----------------------------------------------------------------------------
    i_Status = ech_typ_ConsultSubType (r_ExchCompElem->s_AssocSubType, pr_OSubTypeElem);
  }

  if ( i_Status == GOF_D_NOERROR )
  {
    // Put the type of each att of the ComposedData
    //----------------------------------------------------------------------------
    pr_OInternalCData->i_NbEData = pr_OSubTypeElem->i_AttNumber;

    for (i_AttNumber=0; i_AttNumber < pr_OSubTypeElem->i_AttNumber; i_AttNumber++)
    {
      pr_OInternalCData->ar_EDataTable[i_AttNumber].i_type = pr_OSubTypeElem->ar_AttElem[i_AttNumber].i_AttType;

      if ( pr_OInternalCData->ar_EDataTable[i_AttNumber].i_type == ECH_D_LOC_STRING ) 
      {
        pr_OInternalCData->ar_EDataTable[i_AttNumber].u_val.r_Str.ps_String = malloc (pr_OSubTypeElem->ar_AttElem[i_AttNumber].i_AttLG + 1);
        if (pr_OInternalCData->ar_EDataTable[i_AttNumber].u_val.r_Str.ps_String == NULL) 
        {
          i_Status = ESG_ESG_D_ERR_BADALLOC;
	      LOG(ERROR) << fname << ": rc=" << i_Status << ": bad memory allocation";
	    }
        else
        {
          memset (pr_OInternalCData->ar_EDataTable[i_AttNumber].u_val.r_Str.ps_String,
                  0,
                  pr_OSubTypeElem->ar_AttElem[i_AttNumber].i_AttLG + 1);
        }
      }

      pr_OInternalCData->ar_EDataTable[i_AttNumber].u_val.r_Str.i_LgString = 0;
    } // End for
  }

  // Free all string allocated memory if any allocation failure
  //----------------------------------------------------------------------------
  if ( i_Status == ESG_ESG_D_ERR_BADALLOC )
  {
    for (i_AttNumber=0;i_AttNumber < pr_OSubTypeElem->i_AttNumber; i_AttNumber++)
	{
      if (pr_OInternalCData->ar_EDataTable[i_AttNumber].u_val.r_Str.ps_String != NULL) 
      {
        free (pr_OInternalCData->ar_EDataTable[i_AttNumber].u_val.r_Str.ps_String);
        pr_OInternalCData->ar_EDataTable[i_AttNumber].u_val.r_Str.ps_String = NULL;
      }
    }
  }

  // (CD) Decode the composed data
  //----------------------------------------------------------------------------
  if ( i_Status == GOF_D_NOERROR )
  {
    pr_OQuaCData->b_QualifyUse   = false;
    pr_OQuaCData->i_QualifyValue = 0;
    pr_OQuaCData->b_QualifyExist = false;

    i_Status = esg_esg_edi_ComposedDataDecoding(
                            s_ExchCompId,
                            ps_IBuffer,
                            i_IBufLen,
                            pi_OCDLen,
                            pr_OQuaCData,
                            pr_OInternalCData);
  }

#endif

  i_RetStatus = i_Status;
  return (i_RetStatus);
}
//-END esg_ine_man_CDProcessing-----------------------------------------------


