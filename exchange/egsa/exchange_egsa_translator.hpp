//  EDI coding/decoding services
//
//
// ==========================================================================
#ifndef EXCHANGE_EGSA_TRANSLATOR_HPP
#define EXCHANGE_EGSA_TRANSLATOR_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <time.h>
#include <map>
#include <istream>

// Служебные заголовочные файлы сторонних утилит

// Служебные файлы RTDBUS
#include "exchange_config_egsa.hpp"
#include "exchange_egsa_impl.hpp"
#include "exchange_config_egsa.hpp"
// Конфигурация

//
// Internal EDI service segment structures length
// ----------------------------------------------
#define ESG_ESG_D_HICNBEDATA 6
#define ESG_ESG_D_EICNBEDATA 1
#define ESG_ESG_D_HMSGNBEDATA 3
#define ESG_ESG_D_EMSGNBEDATA 1
#define ESG_ESG_D_HAPPLNBEDATA 3

//
// Max nb of IEDs in an ICD
// ------------------------
#define ECH_D_IEDPERICD		ECH_D_COMPELEMNB
//
// Constants for catalogues
// ------------------------
#define ECH_D_ELEMIDLG		5
#define ECH_D_COMPIDLG		5
#define ECH_D_STYPEIDLG		5
#define ECH_D_FORMATLG		20

//
// Basic types
// -----------
#define ECH_D_TYPINT_CHAR	'I'
#define ECH_D_TYPREAL_CHAR	'R'
#define ECH_D_TYPSTR_CHAR	'S'
#define ECH_D_TYPLOGI_CHAR	'L'
#define ECH_D_TYPTIME_CHAR	'T'

#define ECH_D_TYPINT		1
#define ECH_D_TYPREAL		2
#define ECH_D_TYPSTR		3
#define ECH_D_TYPLOGI		4
#define ECH_D_TYPTIME		5

//
// Constants of EDI ASCII formats for elementary data
// --------------------------------------------------
#define ECH_D_FORMATEDATA_STREMPTY "0"
#define ECH_D_FORMATEDATA_TIMEUTC 'U'
#define ECH_D_FORMATEDATA_REALPRES '.'
#define ECH_D_FORMATEDATA_REALEXP 'e'
#define ECH_D_FORMATEDATA_STRREALEXP "11e4"

//
// Constants for coding elementary data
// ------------------------------------
#define ECH_D_CODEDFORMATLG 20

#define ECH_D_TIMEENTITYLG 2
// year on 4 digits format
#define ECH_D_TIMEYEARLG 4
#define ECH_D_TIMEMILLLG 3
// format for micro-sec
#define ECH_D_TIMEMICRLG 3

// addition of 2 digits for year on 4 digits
#define ECH_D_LIGHTTIMELG (ECH_D_TIMEYEARLG + (5 * ECH_D_TIMEENTITYLG))
#define ECH_D_FULLTIMELG (ECH_D_LIGHTTIMELG + ECH_D_TIMEMILLLG)
#define ECH_D_VERYFULLTIMELG (ECH_D_FULLTIMELG + ECH_D_TIMEMICRLG)


#define ECH_D_POURCENTSTR "%"
#define ECH_D_ZEROSTR "0"
#define ECH_D_POINTSTR "."
#define ECH_D_DECIMALSTR "d"
#define ECH_D_FLOATSTR "f"
#define ECH_D_DOUBLESTR "lf"
#define ECH_D_STRINGSTR "s"
// addition of exponent format
#define ECH_D_EXPSTR "e"


#define ECH_D_FORMATTIME "%02d"
// addition of year on 4 digits format
#define ECH_D_FORMATYEAR "%04d"
#define ECH_D_FORMATMILL "%03d"
// addition of format for micro-sec
#define ECH_D_FORMATMICR "%03d"
//#define ESG_ESG_EDI_D_MICROMILLICONVUNITY 1000
// Position of the qualify elementary data in the composed data
#define ESG_ESG_EDI_D_QUAEDATAPOSITION 0

//
// Constants application segments
// ------------------------------
#define ECH_D_SEGLABELLG	3
// Modification of max segment size in order to store one week of hour historic that is to say 168 points (FFT 2393).
// One point of history costs 51 bytes (C0073) and the header 51 bytes too (C0072).
// So one week hour costs (168+1) * 51 = 8619 bytes (ok with the defined length).
// The maximum size MUST fit on 4 digits. On the contrary the DED should be modify for E1808 elementary data format
#define ECH_D_APPLSEGLG		9216
// Max number of elements in an historic of historised TI It represents one week of hour historic
// CAUTIOUS : if this value is increased then do not forget to increase the max segment length
#define ECH_D_HIST_RECORDS_MAX    168

//
// Service segments identifier
// --------------------------
#define ECH_D_INCHSEGHICD 	"C0001"
#define ECH_D_INCHSEGEICD 	"C0002"
#define ECH_D_MSGSEGHICD 	"C0003"
#define ECH_D_MSGSEGEICD 	"C0004"
#define ECH_D_APPLSEGHICD 	"C0005"
#define ECH_D_DIFFSEGHICD 	"C0009"
#define ECH_D_REPLYSEGHICD 	"C0010"
#define ECH_D_REPLYSEGFICD 	"C0011"
#define ECH_D_REPLYSEGXICD 	"C0012"
#define ECH_D_TIIDLISTDCD 	"C0015"
#define ECH_D_HHISTINF		"C0071"

//
// Constants qualifier elementary data
// -----------------------------------
#define ECH_D_QUAVARLGED_IDED "L1800"
#define ECH_D_QUALED_IDED "L3800"

//
// Segment labels
// --------------
#define ECH_D_STR_ICHSEGLABEL "UNB"
#define ECH_D_STR_ICESEGLABEL "UNZ"
#define ECH_D_STR_MSGHSEGLABEL "UNH"
#define ECH_D_STR_MSGESEGLABEL "UNT"
#define ECH_D_STR_APPHSEGLABEL "APB"
#define ECH_D_STR_APPESEGLABEL "APE"

//
// Constants segments type
// -----------------------
#define ECH_D_EDI_EDDSEG_REQUEST    0       // basic requests
#define ECH_D_EDI_EDDSEG_REPLY      1       // reply
#define ECH_D_EDI_EDDSEG_DIFFUSION  2       // reply
#define ECH_D_EDI_EDDSEG_TI         3       // current process TI
#define ECH_D_EDI_EDDSEG_HISTTI     4       // historized TI
#define ECH_D_EDI_EDDSEG_ALARM      5       // list of alarms
#define ECH_D_EDI_EDDSEG_ORDER      6       // orders and texts
#define ECH_D_EDI_EDDSEG_STATE      7       // communicating system states
#define ECH_D_EDI_EDDSEG_THRESHOLD  8       // list of updated tresholds
#define ECH_D_EDI_EDDSEG_HISTHISTTI 9       // historic of historized TI
#define ECH_D_EDI_EDDSEG_HISTALARM  10      // historic of alarms
#define ECH_D_EDI_EDDSEG_CHGHOUR    11      // Hour changing
#define ECH_D_EDI_EDDSEG_INCIDENT   12      // Incidents
#define ECH_D_EDI_EDDSEG_ORDERRESP  13      // orders responses
#define ECH_D_EDI_EDDSEG_MULTITHRES 14      // OutLine Multi Thresholds
#define ECH_D_EDI_EDDSEG_TELECMD    15      // Telecommand order
#define ECH_D_EDI_EDDSEG_DISPN      16      // dispatcher name
#define ECH_D_EDI_EDDSEG_TELEREGU   17      // Teleregulation order
#define ECH_D_EDI_EDDSEG_ACDLIST    18      // ACD list
#define ECH_D_EDI_EDDSEG_ACDQUERY   19      // ACD query

#define ECH_D_COMPELEMNB		    20
#define ECH_D_CARZERO               '0'
#define ECH_D_ENDSTRING             '\0'

typedef char    gof_t_UniversalName[40];
typedef int32_t gof_t_ExchangeId;

// service segments
// ----------------
// interchange segment
//   . identification
//   . version number
//   . sender identification
//   . receiver identification
//   . timestamp of transmission
//   . number of messages
typedef struct {
  gof_t_ExchangeId i_IdExchange;
  float f_InterChgVersion;
  gof_t_UniversalName s_IdSender;
  gof_t_UniversalName s_IdReceiver;
  timeval d_InterChgDate;
  uint16_t h_InterChgMsgNb;
} esg_esg_t_HeaderInterChg;

typedef struct {
  gof_t_ExchangeId i_IdExchange;
} esg_esg_t_EndInterChg;

// message segment
//   . order number
//   . type
//   . number of application segments
typedef struct {
  uint16_t h_MsgOrderNb;
  uint32_t i_MsgType;
  uint16_t h_MsgApplNb;
} esg_esg_t_HeaderMsg;

typedef struct {
  uint16_t h_MsgOrderNb;
} esg_esg_t_EndMsg;

// application segments
// --------------------
//   . order number
//   . length
typedef struct {
  uint16_t h_ApplOrderNb;
  uint32_t i_ApplType;
  uint32_t i_ApplLength;
} esg_esg_t_HeaderAppl;

// Internal elementary data
// ------------------------
// internal strings
//   . string pointer
//   . string length
typedef struct {
  char* ps_String;
  uint32_t i_LgString;
} ech_t_InternalString;

// internal data types
typedef union { bool b_Logical;
                uint8_t  o_Uint8;
                uint16_t h_Uint16;
                uint32_t i_Uint32;
                int8_t  o_Int8;
                int16_t h_Int16;
                int32_t i_Int32;
                float f_Float;
                double g_Double;
                timeval d_Timeval;
                ech_t_InternalString r_Str;
} ech_t_InternalVal;

// elementary data
// ---------------
//
// all internal data types to code to or to decode from EDI format
//   . for coding :
//      IN : type and valeur of data are set
//           (for string fixed/variable the length of the string is set)
//   . for decoding :
//      IN : type of data is set, for string length is set
//           (maximum length of the reserved buffer string)
//      OUT : valeur of data is set, for string length is set
//           (the length of the received string)
//
typedef struct {
	uint32_t i_type;
	ech_t_InternalVal u_val;
} esg_esg_edi_t_StrElementaryData;

//
// qualify of composed data
//   . - b_QualifyUse --> used for coding Composed data
//       Input parameter
//          set to TRUE : if qualify "L3800" has to be taken to account
//          otherwise set to FALSE
//   . - i_QualifyValue is set :
//          for coding by the caller of EDI services
//          for decoding by EDI services
//   . - b_QualifyExist --> Output parameter updated by decoding procedure
//          (i.e EDI services) to TRUE : qualify "L3800" exists in the DCD entity
typedef struct {
	bool b_QualifyUse;
	uint32_t  i_QualifyValue;
	bool b_QualifyExist;
} esg_esg_edi_t_StrQualifyComposedData;

//
// internal composed data table
//   . number of internal elementary data
//   . elementary data table
typedef struct {
	uint32_t i_NbEData;
	esg_esg_edi_t_StrElementaryData ar_EDataTable[ECH_D_COMPELEMNB];
} esg_esg_edi_t_StrComposedData;

//
// Elementary data element :
// . identifier of the elementary data
// . basic type
// . format
typedef struct {
  char      s_ExchDataId[ECH_D_ELEMIDLG + 1];
  int32_t   i_BasicType;
  char      s_Format[ECH_D_FORMATLG + 1];
} esg_esg_odm_t_ExchDataElem;

// Fields used for historized alarm
// --------------------------------
typedef struct {
  gof_t_UniversalName s_AlRef ;
  struct timeval      d_AlDate ;
  uint8_t             o_AlType ;
  uint8_t             o_AlDeg ;
  uint8_t             o_AlState ;
  double              g_AlVal ;
} esg_esg_t_HistAlElem;

// Composed data element :
// . identifier of the composed data
// . array of elementary data
typedef struct {
    char  s_ExchCompId[ECH_D_COMPIDLG + 1];
    int32_t i_NbData;
    char  as_ElemDataArr[ECH_D_IEDPERICD][ECH_D_ELEMIDLG + 1];
    char  s_AssocSubType[ECH_D_STYPEIDLG + 1];
} esg_esg_odm_t_ExchCompElem;

//============================================================================
class ExchangeTranslator
{
  public:
    ExchangeTranslator(locstruct_item_t*, elemtype_item_t*, elemstruct_item_t*);
   ~ExchangeTranslator();

    int load(std::stringstream&);

  private:
    DISALLOW_COPY_AND_ASSIGN(ExchangeTranslator);
    std::map <const std::string, locstruct_item_t>  m_locstructs;
    std::map <const std::string, elemtype_item_t>   m_elemtypes;
    std::map <const std::string, elemstruct_item_t> m_elemstructs;

    // Прочитать из потока столько данных, сколько ожидается форматом Элементарного типа
    int read_lex_value(std::stringstream&, elemtype_item_t&, char*);

    // Consulting of an entry in the exchanged composed data Table
    // --------------------------------------------------------------
    int esg_esg_odm_ConsultExchCompArr(const char*, esg_esg_odm_t_ExchCompElem*);
    // Consulting of an entry in the exchanged elementary data Table 
    // --------------------------------------------------------------
    int esg_esg_odm_ConsultExchDataArr(const char*, esg_esg_odm_t_ExchDataElem*);

    //  Getting length of Coded composed data in ASCII EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_GetLengthCData(
            const char*,                                // composed data DCD identifier
            const esg_esg_edi_t_StrQualifyComposedData*,// Qualify of internal composed data
            const esg_esg_edi_t_StrComposedData*,       // Internal composed data
            uint32_t* pi_OLgCodedCData);                // length of coded composed data

    // Coding of elementary data in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_ElementaryDataCoding(
                const char*,
                const bool,
                const esg_esg_edi_t_StrElementaryData*,
                const uint32_t i_ILgMaxCodedEData,
                uint32_t*,
                char*);

    // Decoding from EDI format in internal elementary data
    // --------------------------------------------------------------
    int esg_esg_edi_ElementaryDataDecoding(
                const char*,
                const bool,
                const char*,
                const uint32_t,
                uint32_t*,
                esg_esg_edi_t_StrElementaryData*);

    // Coding of composed data in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_ComposedDataCoding(
                const char*,
                const esg_esg_edi_t_StrQualifyComposedData*,
                const esg_esg_edi_t_StrComposedData*,
                const uint32_t,
                uint32_t*,
                char*);

    // Decoding of composed data in internal format
    // --------------------------------------------------------------
    int esg_esg_edi_ComposedDataDecoding(
                const char*,
                const char*,
                const uint32_t,
                uint32_t*,
                esg_esg_edi_t_StrQualifyComposedData*,
                esg_esg_edi_t_StrComposedData*);

    // Coding of interchange header in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_HeaderInterChgCoding(
                const esg_esg_t_HeaderInterChg*,
                const uint32_t,
                uint32_t*,
                char*);

    // Decoding from EDI format in internal interchange header
    // --------------------------------------------------------------
    int esg_esg_edi_HeaderInterChgDecoding(
                const char*,
                const uint32_t,
                uint32_t*,
                esg_esg_t_HeaderInterChg*);

    // Coding of interchange end in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_EndInterChgCoding(
                const esg_esg_t_EndInterChg*,
                const uint32_t,
                uint32_t*,
                char*);

    // Decoding from EDI format in internal interchange end
    // --------------------------------------------------------------
    int esg_esg_edi_EndInterChgDecoding(
                const char*,
                const uint32_t,
                uint32_t*,
                esg_esg_t_EndInterChg*);

    // Coding of interchange message in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_HeaderMsgCoding(
                const esg_esg_t_HeaderMsg*,
                const uint32_t,
                uint32_t*,
                char*);
    // --------------------------------------------------------------
    // Decoding from EDI format in internal interchange header
    int esg_esg_edi_HeaderMsgDecoding(
                const char*,    // coded composed data
                const uint32_t, // max length of coded composed data
                uint32_t*,      // length of coded composed data
                esg_esg_t_HeaderMsg*);  // Internal message header

    // Coding of message end in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_EndMsgCoding(
                const esg_esg_t_EndMsg*,// Internal message end
                const uint32_t, // max length of coded composed data
                uint32_t*,      // length of coded composed data
                char*);         // pointer to coded composed data

    // Decoding from EDI format in internal message end
    // --------------------------------------------------------------
    int esg_esg_edi_EndMsgDecoding(
                const char*,    // coded composed data
                const uint32_t, // max length of coded composed data
                uint32_t*,      // length of coded composed data
                esg_esg_t_EndMsg*);  // Internal message header

    // Coding of application header in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_HeaderApplCoding(
                const esg_esg_t_HeaderAppl*,   // Internal interchange application
                const uint32_t, // max length of coded composed data
                uint32_t* ,     // length of coded composed data
                char*);         // pointer to coded composed data

    // Decoding from EDI format in internal interchange header
    // --------------------------------------------------------------
    int esg_esg_edi_HeaderApplDecoding(
                const char*,    // coded composed data
                const uint32_t, // max length of coded composed data
                uint32_t*,      // length of coded composed data
                esg_esg_t_HeaderAppl*); // Internal application header

    // Coding of application end in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_EndApplCoding(
                const uint32_t, // max length of coded label
                uint32_t*,      // length of coded label
                char*);         // pointer to coded label

    // Decoding of application end in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_EndApplDecoding(
                const char*,    // pointer to coded label
                const uint32_t, // max length of coded label
                uint32_t*);     // length of coded label
    // --------------------------------------------------------------
    int esg_esg_edi_GetLengthEData(
                const esg_esg_odm_t_ExchDataElem*,
                const uint32_t,
                uint32_t*);

    // get format and length of elementary data qualifier
    // --------------------------------------------------------------
    int esg_esg_edi_GetForLgQuaEData(esg_esg_odm_t_ExchDataElem*,
                                  uint32_t* pi_OLgEData);

    // get completed length of elementary data ASCII string type
    // variable / fixed control of the associated format
    // --------------------------------------------------------------
    int esg_esg_edi_GetLengthFullCtrlEData(const esg_esg_odm_t_ExchDataElem*,
                                  const bool,
                                  const uint32_t,
                                  uint32_t*,
                                  uint32_t*,
                                  uint32_t*);

    // coding of elementary data in ASCII string
    // --------------------------------------------------------------
    int esg_esg_edi_IntegerCoding(const char*,
                                  const esg_esg_edi_t_StrElementaryData*,
                                  char*);

    // --------------------------------------------------------------
    int esg_esg_edi_RealCoding(const char*,
                               const esg_esg_edi_t_StrElementaryData*,
                               char*);

    // --------------------------------------------------------------
    int esg_esg_edi_TimeCoding(const char*,
                               const esg_esg_edi_t_StrElementaryData*,
                               char*);

    // decoding of elementary data in internal format
    // --------------------------------------------------------------
    int esg_esg_edi_IntegerDecoding(const char*,
                                  const char*,
                                  esg_esg_edi_t_StrElementaryData*);

    // --------------------------------------------------------------
    int esg_esg_edi_RealDecoding(const char*,
                                 const char*,
                                 esg_esg_edi_t_StrElementaryData*);

    // --------------------------------------------------------------
    int esg_esg_edi_TimeDecoding(const char*,
                                 const char*,
                                 esg_esg_edi_t_StrElementaryData*);

    // Put Historized Alarms for SINF
    // --------------------------------------------------------------
    int esg_acq_dac_HistAlPut(
             const gof_t_UniversalName,  // name of distant site
             const char*,         // name of the file
             const int16_t,           // alarms type
             const uint32_t,            // number of alarms
             const esg_esg_t_HistAlElem*);
    int esg_esg_fil_HeadFileRead(const char*,
                                 esg_esg_t_HeaderInterChg*,
                                 esg_esg_t_HeaderMsg*,
                                 int32_t*,
                                 int32_t*,
                                 FILE **);
    // --------------------------------------------------------------
    float getGoodFloatValue(float, float);
    // --------------------------------------------------------------
    double getGoodDoubleValue(double, double);
    // --------------------------------------------------------------
};

//============================================================================
#endif
