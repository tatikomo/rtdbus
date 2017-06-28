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
#include "exchange_smed.hpp"

// Конфигурация

#define GOF_D_LST_ALA_OPE           6 // List of Operational alarms
#define GOF_D_LST_ALA_NON_OPE       7 // List of non Operational alarms
#define GOF_D_LST_ALAmax         1000 // Max of Alarms



#define SIG_DBA_D_IND_VH    0   // very high
#define SIG_DBA_D_IND_H     1   // high
#define SIG_DBA_D_IND_L     2   // low
#define SIG_DBA_D_IND_VL    3   // very low
#define SIG_DBA_D_IND_G     4   // gradient

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

// Subtypes definition
// -------------------
#define ECH_D_TELEINFORMATION       'I'
#define ECH_D_ALARM                 'A'
#define ECH_D_STATE                 'S'
#define ECH_D_ORDER                 'O'
#define ECH_D_HISTORIZATION         'P'
#define ECH_D_HISTORIC              'H'
#define ECH_D_THRESHOLD             'T'
#define ECH_D_REQUEST               'R'
#define ECH_D_CHGHOUR               'G'
#define ECH_D_INCIDENT              'D'

#define ECH_D_RQBASONLY             "R0009"
#define ECH_D_RQGENCONT             "R0018"
#define ECH_D_THRESHOLDS_SET        "T0008"
#define ECH_D_THRESHOLDS_CNT        "T0019"
#define ECH_D_ALARM_TM    	        "A0013"
#define ECH_D_ALARM_TS    	        "A0014"
#define ECH_D_STATE_VALUE           "S0017"
#define ECH_D_SYNTHSTATE_VALUE      "I0017"
#define ECH_D_CHGHOUR_VALUE         "G0025"
#define ECH_D_HISTAL_DATE           "H0026"
#define ECH_D_HISTAL_TYPE           "H0027"
#define ECH_D_HISTAL_DESC           "H0028"
#define ECH_D_ORDER_FREETXT         "O0030"
#define ECH_D_ORDER_PLANIF          "O0031"
#define ECH_D_ORDER_ELEM_PLANIF	    "O0032"
#define ECH_D_ORDER_SECUR_EQTYP     "O0033"
#define ECH_D_ORDER_SECUR_NAME      "O0034"
#define ECH_D_ORDER_CONDUC_EQTYP    "O0035"
#define ECH_D_ORDER_CONDUC_NAME     "O0036"
#define ECH_D_ORDER_TECHNO_EQTYP    "O0037"
#define ECH_D_ORDER_TECHNO_NAME     "O0038"
#define ECH_D_ORDER_CONTRACT        "O0039"
#define ECH_D_ORDER_MANOEUVRE       "O0040"
#define ECH_D_ORDER_QUOTAS          "O0041"
#define ECH_D_ORDER_PLAN_RECEP      "O0042"
#define ECH_D_ORDER_ELEM_APPLI      "O0043"
#define ECH_D_ORDER_MANO_AUTOR      "O0044"
#define ECH_D_INCIDENT_ACQDIFF      "D0045"
#define ECH_D_HISTTI_PERIOD             "P0060"
#define ECH_D_HISTTI_QHSUBT_DVDOUBLE    "P0061"
#define ECH_D_HISTTI_QHSUBT_DVSHORT     "P0062"
#define ECH_D_HISTTI_HOURSUBT_DVDOUBLE  "P0063"
#define ECH_D_HISTTI_HOURSUBT_DVSHORT   "P0064"
#define ECH_D_HISTTI_DAYSUBT_DVDOUBLE   "P0065"
#define ECH_D_HISTTI_DAYSUBT_DVSHORT    "P0066"
#define ECH_D_HISTTI_MONTHSUBT_DVDOUBLE "P0067"
#define ECH_D_HISTTI_MONTHSUBT_DVSHORT  "P0068"
#define ECH_D_RQHHISTTI             "R0070"
#define ECH_D_HHISTTI_TYPE          "P0071"
#define ECH_D_HHISTTI_TI            "P0072"
#define ECH_D_HHISTTI_VAL           "P0073"
#define ECH_D_TELECMD_REQ           "O0074" // TC request contents subtype
#define ECH_D_DISP_NAME             "R0075" // Dispatcher name (added in TI file)
#define ECH_D_TELEREGU_REQ          "O0076" // TR request contents subtype
#define ECH_D_EMERGENCY_REQ         "R0077" // Subtype of the request for emergency cycle
#define ECH_D_ACD_UN                "I0020" // Subtype of the request for ACD list
#define ECH_D_ACD_CNT               "A0021"
#define ECH_D_ACDLIST               "A0022"
#define ECH_D_ACDQUERY              "A0023" // ACD query to dipl

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

#define ESG_ESG_D_LOGGEDTEXTLENGTH 200

// --------------------------------
typedef enum {
    GOF_D_OPE_ALA,
    GOF_D_NON_OPE_ALA
} gof_t_AlaListType;

// reply identification
// --------------------------------
typedef enum 	{
  ECH_D_ACCEPT_REPLY,
  ECH_D_REFUSAL_REPLY,
  ECH_D_DEFERED_REPLY,
  ECH_D_INTEXEC_REPLY,
  ECH_D_ENDEXEC_REPLY
} ech_t_ReplyId;

typedef enum {
  GOF_D_PER_NOHISTO=-1,
  GOF_D_PER_QH=0,
  GOF_D_PER_HOUR=1,
  GOF_D_PER_DAY=2,
  GOF_D_PER_MONTH=3,
  GOF_D_PER_SAMPLE=4,
  GOF_D_PER_YEAR=5,
  GOF_D_PER_5MN=6
} gof_t_PeriodType;

// service segments
// ----------------
// interchange segment
//   . identification
//   . version number
//   . sender identification
//   . receiver identification
//   . timestamp of transmission
//   . number of messages
// --------------------------------
typedef struct {
  gof_t_ExchangeId i_IdExchange;
  float f_InterChgVersion;
  gof_t_UniversalName s_IdSender;
  gof_t_UniversalName s_IdReceiver;
  timeval d_InterChgDate;
  int h_InterChgMsgNb;
} esg_esg_t_HeaderInterChg;

// --------------------------------
typedef struct {
  gof_t_ExchangeId i_IdExchange;
} esg_esg_t_EndInterChg;

// message segment
//   . order number
//   . type
//   . number of application segments
// --------------------------------
typedef struct {
  int h_MsgOrderNb;
  int i_MsgType;
  int h_MsgApplNb;
} esg_esg_t_HeaderMsg;

// --------------------------------
typedef struct {
  int h_MsgOrderNb;
} esg_esg_t_EndMsg;

// application segments
// --------------------
//   . order number
//   . length
// --------------------------------
typedef struct {
  int h_ApplOrderNb;
  int i_ApplType;
  int i_ApplLength;
} esg_esg_t_HeaderAppl;

// Internal elementary data
// ------------------------
// internal strings
//   . string pointer
//   . string length
// --------------------------------
typedef struct {
  char*  ps_String;
  size_t i_LgString;
} ech_t_InternalString;

// internal data types
// --------------------------------
typedef union {
  bool      b_Logical;
  uint8_t   o_Uint8;
  uint16_t  h_Uint16;
  uint32_t  i_Uint32;
  int8_t    o_Int8;
  int16_t   h_Int16;
  int32_t   i_Int32;
  float     f_Float;
  double    g_Double;
  timeval   d_Timeval;
  ech_t_InternalString r_Str;
} ech_t_InternalVal;

//
// esg_acq_dac_t_HhistTIHeader structure
// --------------------------------------------
typedef struct {
  gof_t_UniversalName s_DIPL;
  gof_t_PeriodType    e_HistPeriod;
  uint16_t            h_NbInfosHist;
} esg_acq_dac_t_HhistTIHeader;

//
// esg_acq_dac_t_HhistTIBody
// NB: хранится в двоичном виде, для обеспечения совместимости с ГОФО использовать эти размерности
// --------------------------------------------
typedef struct {
  gof_t_UniversalName     s_Info;
  long      ai_Status;
  uint32_t  h_NbEch;
  uint8_t   ai_Type;
} esg_acq_dac_t_HhistTIBody;

//
// --------------------------------------------
typedef struct {
  struct timeval   ad_Date;
} esg_acq_dac_t_HhistTIDate;

//
// --------------------------------------------
typedef struct {
  double    g_Value;
} esg_acq_dac_t_HhistTIValue;

//
// --------------------------------------------
typedef struct {
  uint16_t  ah_Valid;
} esg_acq_dac_t_HhistTIValid;

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
// --------------------------------
typedef struct {
	field_type_t type;
	ech_t_InternalVal u_val;
} esg_esg_edi_t_StrElementaryData;

//
// qualify of composed data
//   - b_QualifyUse --> used for coding Composed data
//       Input parameter
//          set to TRUE : if qualify "L3800" has to be taken to account
//          otherwise set to FALSE
//   - i_QualifyValue is set :
//          for coding by the caller of EDI services
//          for decoding by EDI services
//   - b_QualifyExist --> Output parameter updated by decoding procedure
//          (i.e EDI services) to TRUE : qualify "L3800" exists in the DCD entity
// --------------------------------
typedef struct {
  bool b_QualifyUse;
  int  i_QualifyValue;
  bool b_QualifyExist;
} esg_esg_edi_t_StrQualifyComposedData;

//
// internal composed data table
//   . number of internal elementary data
//   . elementary data table
// --------------------------------
typedef struct {
  size_t i_NbEData;
  esg_esg_edi_t_StrElementaryData ar_EDataTable[ECH_D_COMPELEMNB];
} esg_esg_edi_t_StrComposedData;

//
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Эти данные, возможно, не потребуются по завершении интеграции в ExchangeTranslator
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TM Thresholds
// =============
#define GOF_D_BDR_CNT_THRESHOLDS	5   // Количество уставок и пределов: VL, L, H, VH, G
typedef struct {
  double  ag_value[GOF_D_BDR_CNT_THRESHOLDS];     // the set of thresholds values
  uint8_t ao_degree[GOF_D_BDR_CNT_THRESHOLDS];    // the set of degrees values
  uint8_t ao_categ[GOF_D_BDR_CNT_THRESHOLDS];     // the set of categories values
} gof_t_MsgThresholds;

typedef struct {
  gof_t_UniversalName  s_uni_name;        // universal name
  gof_t_MsgThresholds  r_MsgThresholds;   // thresholds values
} gof_t_TiThreshold;

typedef struct  {
  int /* gof_t_ExchangeId */        i_exchange_id;  // exchange identifier
  uint16_t         h_Cnt;      // count of list elements
  gof_t_TiThreshold    ar_TiThreshold[1];   // thresholds data
} sig_t_msg_MultiThresholds;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//============================================================================
class AcqSiteEntry;
class SMED;
class EGSA;

class ExchangeTranslator
{
  public:
    // Основной конструктор
    ExchangeTranslator(EGSA*, elemtype_item_t*, elemstruct_item_t*);
#ifdef _FUNCTIONAL_TEST
    // Создание транслятора без активного EGSA - для отладки
    ExchangeTranslator(AcqSiteList*, SMED*, elemtype_item_t*, elemstruct_item_t*);
#endif
   ~ExchangeTranslator();

    // Разбор файла с данными от смежной системы ESG
    int esg_acq_dac_Switch(const char*);

  private:
    DISALLOW_COPY_AND_ASSIGN(ExchangeTranslator);
    EGSA* m_egsa_instance;
#ifdef _FUNCTIONAL_TEST
    AcqSiteList* m_sites;
    SMED* m_smed;
    SMED* smed() { return (m_egsa_instance)? m_egsa_instance->smed() : m_smed; };
#else
    SMED* smed() { return m_egsa_instance->smed(); };
#endif
    std::map <const std::string, elemtype_item_t>   m_elemtypes;
    std::map <const std::string, elemstruct_item_t> m_elemstructs;

    // Получить экземпляр Сайта по его имени
    AcqSiteEntry* esg_esg_odm_ConsultAcqSiteEntry(const char*);

    // Прочитать из потока столько данных, сколько ожидается форматом Элементарного типа
    int read_lex_value(std::stringstream&, elemtype_item_t&, char*);

    // Consulting of an entry in the exchanged composed data Table
    // --------------------------------------------------------------
    elemstruct_item_t* esg_esg_odm_ConsultExchCompArr(const char*);

    // По названию ассоциации получить ссылку на элемент с характеристиками атрибутов БДРВ
    // --------------------------------------------------------------
    elemstruct_item_t* ech_typ_ConsultSubType(const char*);

    // Consulting of an entry in the exchanged elementary data Table
    // --------------------------------------------------------------
    elemtype_item_t* esg_esg_odm_ConsultExchDataArr(const char*);

    // --------------------------------------------------------------
    int esg_esg_odm_ConsExchInfoLed(// Input parameters
                const gof_t_UniversalName,  // acq site universal name
                const int,  // Exchange data identificator
                // Output parameter
                esg_esg_odm_t_ExchInfoElem*);

    //  Getting length of Coded composed data in ASCII EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_GetLengthCData(
                const char*,                                // composed data DCD identifier
                const esg_esg_edi_t_StrQualifyComposedData*,// Qualify of internal composed data
                const esg_esg_edi_t_StrComposedData*,       // Internal composed data
                int* pi_OLgCodedCData);                // length of coded composed data

    // Coding of elementary data in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_ElementaryDataCoding(
                const char*,
                const bool,
                const esg_esg_edi_t_StrElementaryData*,
                const int i_ILgMaxCodedEData,
                int*,
                char*);

    // Decoding from EDI format in internal elementary data
    // --------------------------------------------------------------
    int esg_esg_edi_ElementaryDataDecoding(
                const char*,
                const bool,
                const char*,
                const int,
                int*,
                esg_esg_edi_t_StrElementaryData*);

    // Coding of composed data in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_ComposedDataCoding(
                const char*,
                const esg_esg_edi_t_StrQualifyComposedData*,
                const esg_esg_edi_t_StrComposedData*,
                const int,
                int*,
                char*);

    // Decoding of composed data in internal format
    // --------------------------------------------------------------
    int esg_esg_edi_ComposedDataDecoding(
                const char*,
                const char*,
                const int,
                int*,
                esg_esg_edi_t_StrQualifyComposedData*,
                esg_esg_edi_t_StrComposedData*);

    // Coding of interchange header in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_HeaderInterChgCoding(
                const esg_esg_t_HeaderInterChg*,
                const int,
                int*,
                char*);

    // Decoding from EDI format in internal interchange header
    // --------------------------------------------------------------
    int esg_esg_edi_HeaderInterChgDecoding(
                const char*,
                const int,
                int*,
                esg_esg_t_HeaderInterChg*);

    // Coding of interchange end in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_EndInterChgCoding(
                const esg_esg_t_EndInterChg*,
                const int,
                int*,
                char*);

    // Decoding from EDI format in internal interchange end
    // --------------------------------------------------------------
    int esg_esg_edi_EndInterChgDecoding(
                const char*,
                const int,
                int*,
                esg_esg_t_EndInterChg*);

    // Coding of interchange message in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_HeaderMsgCoding(
                const esg_esg_t_HeaderMsg*,
                const int,
                int*,
                char*);
    // --------------------------------------------------------------
    // Decoding from EDI format in internal interchange header
    int esg_esg_edi_HeaderMsgDecoding(
                const char*,    // coded composed data
                const int, // max length of coded composed data
                int*,      // length of coded composed data
                esg_esg_t_HeaderMsg*);  // Internal message header

    // Coding of message end in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_EndMsgCoding(
                const esg_esg_t_EndMsg*,// Internal message end
                const int, // max length of coded composed data
                int*,      // length of coded composed data
                char*);         // pointer to coded composed data

    // Decoding from EDI format in internal message end
    // --------------------------------------------------------------
    int esg_esg_edi_EndMsgDecoding(
                const char*,    // coded composed data
                const int, // max length of coded composed data
                int*,      // length of coded composed data
                esg_esg_t_EndMsg*);  // Internal message header

    // Coding of application header in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_HeaderApplCoding(
                const esg_esg_t_HeaderAppl*,   // Internal interchange application
                const int, // max length of coded composed data
                int* ,     // length of coded composed data
                char*);         // pointer to coded composed data

    // Decoding from EDI format in internal interchange header
    // --------------------------------------------------------------
    int esg_esg_edi_HeaderApplDecoding(
                const char*,    // coded composed data
                const int, // max length of coded composed data
                int*,      // length of coded composed data
                esg_esg_t_HeaderAppl*); // Internal application header

    // Coding of application end in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_EndApplCoding(
                const int, // max length of coded label
                int*,      // length of coded label
                char*);         // pointer to coded label

    // Decoding of application end in EDI format
    // --------------------------------------------------------------
    int esg_esg_edi_EndApplDecoding(
                const char*,    // pointer to coded label
                const int, // max length of coded label
                int*);     // length of coded label
    // --------------------------------------------------------------
    int esg_esg_edi_GetLengthEData(
                const elemtype_item_t*,
                const int,
                int*);

    // get format and length of elementary data qualifier
    // --------------------------------------------------------------
    int esg_esg_edi_GetForLgQuaEData(elemtype_item_t*,
                      int* pi_OLgEData);

    // get completed length of elementary data ASCII string type
    // variable / fixed control of the associated format
    // --------------------------------------------------------------
    int esg_esg_edi_GetLengthFullCtrlEData(const elemtype_item_t*,
                      const bool,
                      const int,
                      int*,
                      int*,
                      int*);

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

    // Read A File Header
    // --------------------------------------------------------------
    int esg_esg_fil_HeadFileRead(const char*,
                      esg_esg_t_HeaderInterChg*,
                      esg_esg_t_HeaderMsg*,
                      int32_t*,
                      int32_t*,
                      FILE **);
    // Read An Applicative Header
    // --------------------------------------------------------------
    int esg_esg_fil_HeadApplRead(FILE*,
                      esg_esg_t_HeaderAppl*,
                      int32_t*);
    // --------------------------------------------------------------
    int esg_esg_fil_StringRead(FILE*, const int32_t, char*);
    // --------------------------------------------------------------
    int esg_esg_fil_DataWrite(
                      FILE*,
                      const void*, // pointes the array to be written
                      const int,   // length of an item
                      const int);  // number of items to be written
    // --------------------------------------------------------------
    int esg_esg_fil_EndApplRead(FILE*);

    // --------------------------------------------------------------
    int esg_acq_dac_TeleinfoAcq(
                      // Input parameters
                      FILE*,
                      const int,
                      const int,
                      const gof_t_UniversalName,
                      const struct timeval);

    // --------------------------------------------------------------
    int esg_acq_dac_TIThresholdAcq(
                      // Input parameters
                      const gof_t_UniversalName,
                      const esg_esg_edi_t_StrComposedData,
                      const esg_esg_edi_t_StrQualifyComposedData,
                      // Output parameters
                      gof_t_TiThreshold*);

    // --------------------------------------------------------------
    int esg_acq_dac_HHTISndMsg(
                      // Input parameters
                      const gof_t_UniversalName,
                      const gof_t_PeriodType,
                      const char*);

    // --------------------------------------------------------------
    int esg_acq_dac_MultiThresAcq(FILE*,
                                  int,
                                  const char*);

    // --------------------------------------------------------------
    int esg_acq_dac_HistAlAcq(FILE*,
                              int,
                              int,
                              const char*,
                              int*,                     // ALO count
                              esg_esg_t_HistAlElem*,    // ALO data
                              int*,                     // ALN count
                              esg_esg_t_HistAlElem*);   // ALN data

    // --------------------------------------------------------------
    int esg_acq_dac_HistTiAcq(bool,
                              FILE*,
                              int,
                              const char*);

    // --------------------------------------------------------------
    int esg_acq_dac_HHTITIProcess(
                              const gof_t_UniversalName,
                              const gof_t_UniversalName,
                              const esg_esg_edi_t_StrComposedData*,
                              const elemstruct_item_t*,
                              FILE*,
                              int*,
                              int*,
                              esg_acq_dac_t_HhistTIDate**,
                              esg_acq_dac_t_HhistTIValue**,
                              esg_acq_dac_t_HhistTIValid**);

    // --------------------------------------------------------------
    int esg_acq_dac_HHTIValProcess(
                              // Input parameters
                              const esg_esg_edi_t_StrComposedData*,
                              const elemstruct_item_t*,
                              // Input/Output parameters
                              esg_acq_dac_t_HhistTIDate*,
                              esg_acq_dac_t_HhistTIValue*,
                              esg_acq_dac_t_HhistTIValid*);

    // --------------------------------------------------------------
    int esg_acq_dac_SmdProcessing(
                      const gof_t_UniversalName,
                      const esg_esg_edi_t_StrComposedData*,
                      const esg_esg_edi_t_StrQualifyComposedData*,
                      const struct timeval);

    // Отправка полученной телеметрии в SIDL
    // --------------------------------------------------------------
    int esg_acq_dac_SendAcqTIToSidl(const gof_t_UniversalName);   // name of the distant SA site that has send its dispatcher name

    // --------------------------------------------------------------
    int esg_acq_dac_HHistTiAcq(int, int, const char*, const char*, FILE*, int*);

    // --------------------------------------------------------------
    int esg_acq_dac_DispNameAcq(
                      // Input parameters
                      FILE*,
                      const int,
                      const int,
                      const gof_t_UniversalName,    // sender site
                      const struct timeval);

    // --------------------------------------------------------------
    int esg_acq_dac_InitInternalBis(esg_esg_edi_t_StrComposedData*);

    // Put Historized Alarms for SINF
    // --------------------------------------------------------------
    int esg_acq_dac_HistAlPut(
                      const gof_t_UniversalName,  // name of distant site
                      const char*,         // name of the file
                      const int,           // alarms type
                      const int,           // number of alarms
                      const esg_esg_t_HistAlElem*);

    // Отправка полученного имени Диспетчера в SIDL
    // --------------------------------------------------------------
    int esg_acq_inm_SendDispNameToSidl(
                      const gof_t_UniversalName,    // name of the distant SA site that has send its dispatcher name
                      const char*);                 // dispatcher name


    // Отправка сообщения в SINF
    // --------------------------------------------------------------
    int sig_ext_msg_p_InpSendMessageToSinf(const rtdbMsgType, int, const char*);

    // --------------------------------------------------------------
    int esg_ine_man_CDProcessing(
                      const char*,   // begining of the composed data in the buffer
                      const int,     //length of the segment body from the composed data to process
                      int*,          // composed data length
                      esg_esg_edi_t_StrComposedData*, // decoded composed data buffer
                      elemstruct_item_t*,
                      esg_esg_edi_t_StrQualifyComposedData*);  // Quality data buffer

    // Удаление памяти, выделенной под значение атрибута, прочитанного из файла ESG
    // --------------------------------------------------------------
    int esg_ine_man_FreeCompData(esg_esg_edi_t_StrComposedData*);

    // --------------------------------------------------------------
    int ech_typ_GetAtt(const ech_t_InternalVal*,  // value from composed data
                      field_type_t,  // type of the attribute to get
                      void*,         // attribute to fill
                      field_type_t); // тип ожидаемого атрибута

    // --------------------------------------------------------------
    int ech_typ_StoreAtt(const void*,       // value of the attribute to store
                      const int,            //  type of the attribute to store
                      ech_t_InternalVal*,   // attribute to fill: internal value
                      field_type_t);        // Тип атрибута для занесения

    // --------------------------------------------------------------
    float getGoodFloatValue(float, float);
    // --------------------------------------------------------------
    double getGoodDoubleValue(double, double);
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    int processing_STATE(FILE*, int, int, const char*);
    int processing_REPLY(FILE*, int, int, const char*);
    int processing_REQUEST(FILE*, int, int, const char*);
};

//============================================================================
#endif
