#ifndef CE_UNITTEST_HPP
#define CE_UNITTEST_HPP

#include <stdint.h>
#include <time.h>   // struct timeval

////////////////////////////////////////////////////////////////////////////////////
// RTAP definitions
////////////////////////////////////////////////////////////////////////////////////
typedef const char      rtChar;
typedef unsigned int    rtDbQuality;
typedef unsigned int    rtDeType;
typedef uint16_t        rtPlin;
typedef uint8_t         rtAin;
typedef uint8_t         rtUInt8;
typedef uint16_t        rtUInt16;
typedef uint32_t        rtUInt32;
typedef int32_t         rtInt;
typedef bool            rtLogical;
typedef double          rtDouble;
typedef	uint8_t         ResultType;
typedef uint16_t        rtTermType;

#define rtSUCCESS   0
#define rtCONTINUE  (rtSUCCESS + 1)
#define rtFAILED    -1
#define rtATTR_OK           0
#define rtATTR_SUSPECT      1
#define rtATTR_ERROR        2
#define rtATTR_DISABLED     4

#define rtSCALAR_ELEMENT    0
#define rtSCALAR_INDEXER    1
#define rtVECTOR_ELEMENT    2
#define rtVECTOR_INDEXER    3
#define rtMULTI_INDEXER     4
#define rtNULL_PARAMETER    5
#define rtCE_FUNCTION       6
#define rtJUMP              7

#define rtUNDEFINED		((rtDeType) 0)
#define rtLOGICAL		((rtDeType) 1)
#define rtINT8			((rtDeType) 2)
#define rtUINT8			((rtDeType) 3)
#define rtINT16			((rtDeType) 4)
#define rtUINT16		((rtDeType) 5)
#define rtINT32			((rtDeType) 6)
#define rtUINT32		((rtDeType) 7)
#define rtFLOAT			((rtDeType) 8)
#define rtDOUBLE		((rtDeType) 9)
#define rtPOLAR			((rtDeType) 10)
#define rtRECTANGULAR   ((rtDeType) 11)
#define rtBYTES4		((rtDeType) 16)
#define rtBYTES8		((rtDeType) 17)
#define rtBYTES12		((rtDeType) 18)
#define rtBYTES16		((rtDeType) 19)
#define rtBYTES20		((rtDeType) 20)
#define rtBYTES32		((rtDeType) 21)
#define rtBYTES48		((rtDeType) 22)
#define rtBYTES64		((rtDeType) 23)
#define rtBYTES80		((rtDeType) 24)
#define rtBYTES128		((rtDeType) 25)
#define rtBYTES256		((rtDeType) 26)
#define rtDB_XREF		((rtDeType) 28)
#define rtDATE			((rtDeType) 29)
#define rtTIME_OF_DAY   ((rtDeType) 30)
#define rtABS_TIME		((rtDeType) 31)

// rtAllDeTypes - объединение всех возможных типов данных
#define rtAllDeTypes 256

////////////////////////////////////////////////////////////////////////////////////
// RTAP types
////////////////////////////////////////////////////////////////////////////////////
typedef struct timeval rtTime;

typedef struct  {
  rtUInt8     bitdata;    /* all bitfields */
  rtAin       ain;
  rtPlin      plin;
  rtUInt16    startRecEl; /* start rec. or elem. */
  rtUInt16    endRecEl;   /* end record or element */
  rtUInt8     startField; /* start field */
  rtUInt8     endField;   /* ending field */
} rtIndexer;

typedef struct  {
  rtDeType    deType;
  rtTermType  operandType;
  rtUInt16    numElems;  /* rtVECTOR_ELEMENT only */
  rtDouble    data[sizeof(rtAllDeTypes)/sizeof(rtDouble)];
  /* pointer to data if this is an rtVECTOR_ELEMENT */
} rtOperand;

/*
 * a value on the value stack
 */
typedef struct  {
  char name[20];
  rtOperand   operand;
  rtDbQuality quality;
} rtValue;

////////////////////////////////////////////////////////////////////////////////////
// RTAP functions
////////////////////////////////////////////////////////////////////////////////////
int rtCeRead(rtIndexer&, void*, void*);
rtValue* rtPopValue();
void rtIncValueStack();
int rtCeWriteOutput(rtIndexer&, void*);
int rtCeFatalError(const char*, rtValue*, rtDeType, int);
int rtCeTrigger(rtPlin, rtAin, int);

////////////////////////////////////////////////////////////////////////////////////
// GOFO definitions
////////////////////////////////////////////////////////////////////////////////////
#define GOF_D_FALSE     0
#define GOF_D_TRUE      1
#define GOF_D_NOERROR   0
#define GOF_D_MSGERROR -1
#define GOF_D_NULL      0

#define	RESULT_TYPE	rtUINT8		/* define function result type	      */
#define SIL_TRT_CAL_D_SASTATE_VIDE   99 /*define un param de type SASTATE vide*/

/* coefficient for hysteresis calculation */
#define SIL_TRT_CAL_D_HYSTER_GRAD	(0.1)

#define SIL_TRT_CAL_VALTI_MAXPARAMS     12
#define SIL_TRT_CAL_VALEQ_MAXPARAMS     1
#define SIL_TRT_CAL_SASTA_MAXPARAMS     2
#define SIL_TRT_CAL_VALRE_MAXPARAMS     1
#define SIL_TRT_CAL_VALAL_MAXPARAMS     10
#define SIL_TRT_CAL_TSCSA_MAXPARAMS     11
#define SIL_TRT_CAL_TSCRGA_MAXPARAMS    6
#define SIL_TRT_CAL_TSCDA_MAXPARAMS     10
#define SIL_TRT_CAL_TSCVA_MAXPARAMS     10
#define SIL_TRT_CAL_TSCGR_MAXPARAMS     26
#define SIL_TRT_CAL_TSCGR2_MAXPARAMS    18
#define SIL_TRT_CAL_TSCAT_MAXPARAMS     16
#define SIL_TRT_CAL_TSCAT2_MAXPARAMS    12
#define SIL_TRT_CAL_TSCSD_MAXPARAMS     8
#define SIL_TRT_CAL_TSCSC_MAXPARAMS     8
#define SIL_TRT_CAL_VALTSC_MAXPARAMS    11
#define SIL_TRT_CAL_GRAD_MAXPARAMS      7
#define SIR_TRT_CAL_VALTS_MAXPARAMS     4
#define SIR_TRT_CAL_VALTSC_MAXPARAMS    4
#define SIL_TRT_CAL_VALQH_MAXPARAMS     3
#define SIL_TRT_CAL_VALH_MAXPARAMS      4
#define SIL_TRT_CAL_VALJ_MAXPARAMS      4
#define SIL_TRT_CAL_VALM_MAXPARAMS      4
#define SIL_TRT_CAL_TSCAUX1_MAXPARAMS   24
#define SIL_TRT_CAL_TSCAUX2_MAXPARAMS   16
#define SIL_TRT_CAL_ALINH_MAXPARAMS     6

#define GOF_D_BDR_SASTATE_FAULT         0
#define GOF_D_BDR_SASTATE_INHIB         1
#define GOF_D_BDR_SASTATE_OP            2

#define GOF_D_BDR_VALIDCHANGE_FAULT         0
#define GOF_D_BDR_VALIDCHANGE_VALID         1
#define GOF_D_BDR_VALIDCHANGE_FORCED        2
#define GOF_D_BDR_VALIDCHANGE_INHIB         3
#define GOF_D_BDR_VALIDCHANGE_MANUAL        4
#define GOF_D_BDR_VALIDCHANGE_END_INHIB     5
#define GOF_D_BDR_VALIDCHANGE_END_FORCED    6
#define GOF_D_BDR_VALIDCHANGE_INHIB_GBL     7
#define GOF_D_BDR_VALIDCHANGE_END_INHIB_GBL 8
#define GOF_D_BDR_VALIDCHANGE_VIDE          9
#define GOF_D_BDR_VALIDCHANGE_FAULT_GBL     10
#define GOF_D_BDR_VALIDCHANGE_INHIB_SA      11
#define GOF_D_BDR_VALIDCHANGE_END_INHIB_SA  12

#define GOF_D_BDR_VALID_FAULT           0
#define GOF_D_BDR_VALID_VALID           1
#define GOF_D_BDR_VALID_FORCED          2
#define GOF_D_BDR_VALID_INHIB           3
#define GOF_D_BDR_VALID_FAULT_INHIB     4
#define GOF_D_BDR_VALID_FAULT_FORCED    5
#define GOF_D_BDR_VALID_INQUIRED        6
#define GOF_D_BDR_VALID_NO_INSTRUM      7
#define GOF_D_BDR_VALID_GLOBAL_FAULT    8

#define GOF_D_BDR_VALIDR_VALID          1
#define GOF_D_BDR_VALIDR_MANUAL         2
#define GOF_D_BDR_VALIDR_DUBIOUS        3
#define GOF_D_BDR_VALIDR_UNAVAILABLE    4
/* Remark : As in some calculation engines, some arguments can be the         */
/* VALIDCHANGE attribute of a TSA as well as the VALIDR attribute of a TS, AL,*/
/* TSC or ICS, GOF_D_BDR_VALIDCHANGE_VALID and GOF_D_BDR_VALID_VALID must     */
/* have the same value */
/* values of the VALIDACQ attributes */
/* ================================= */
#define   GOF_D_BDR_VALIDACQ_FAULT      0
#define   GOF_D_BDR_VALIDACQ_VALID      1

/* values of the TSSYNTHETICAL attributes */
/* ====================================== */
#define GOF_D_BDR_NO_ALARM              0
#define GOF_D_BDR_ALARM_NOTACK          1
#define GOF_D_BDR_ALARM_ACK             2

#define SIL_TRT_D_MSG_FAULT_INF_LIS_PUT     1200
#define SIL_TRT_D_MSG_FAULT_INF_LIS_SUP     1201
#define SIL_TRT_D_MSG_INHIB_INF_LIS_PUT     1202
#define SIL_TRT_D_MSG_INHIB_INF_LIS_SUP     1203

#define VALIDCHANGE_IDX     0
#define VALACQ_IDX          1
#define DATEHOURM_IDX       2
#define VALIDACQ_IDX        3
#define VAL_IDX             4
#define VALID_IDX           5
#define SASTATE_IDX         6
#define OLDSASTATE_IDX      7
#define INHIBLOCAL_IDX      8
#define INHIB_IDX           9
////////////////////////////////////////////////////////////////////////////////////
// GOFO types
////////////////////////////////////////////////////////////////////////////////////

typedef int32_t     gof_t_Int32;
typedef void*       gof_tp_Void;

typedef struct {
  rtPlin      h_IndexTI;  /* teleinformation plin */
  rtLogical   b_Global;   /* Global or individual default */
  rtTime      d_Date;     /* Date */
  rtDeType    i_deType;   /* Type */
  union
  {
    rtDouble    g_Value;    /* TM value (Forced)*/
    rtUInt16    i_Value;    /* TS value (Forced)*/
  } Value;
} sil_trt_t_mr_msg_InfoList;

typedef int gof_t_Status;

////////////////////////////////////////////////////////////////////////////////////
// GOFO functions
////////////////////////////////////////////////////////////////////////////////////
gof_t_Status gof_tim_p_GetTimeOfDay(rtTime*);
gof_t_Status sig_ext_msg_p_OutSendInternMessage
(
  gof_t_Int32     i_MessType,
  gof_t_Int32     i_MessSize,
  gof_tp_Void     p_MessBody
);

////////////////////////////////////////////////////////////////////////////////////
// RTDBM functions
////////////////////////////////////////////////////////////////////////////////////
void set_init_val(rtUInt16 validchange);
void set_init_val();
void push();
void pop();

#endif

