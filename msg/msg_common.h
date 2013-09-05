#if !defined MSG_COMMON_H
#define MSG_COMMON_H
#pragma once

#include <stdint.h>

typedef uint32_t rtdbExchangeId;/* системный(служебный) идентификатор сообщения */
typedef uint32_t rtdbPid;       /* идентификатор процесса-отправителя */
typedef uint16_t rtdbMsgType;   /* тип сообщения */

typedef char rtdbProcessId[10]; /* имя процесса */

#define GOF_D_BASE_MSG_INTERNAL 1000

/* base number for SINF */
/*----------------------*/
#define GOF_D_BASE_MSG_SINF     2000

/* base number for ECH */
/*----------------------*/
#define GOF_D_BASE_MSG_ECH      3000

/* base number for IHM */
/*----------------------*/
#define GOF_D_BASE_MSG_IHM      4000

/* base number for ADDX */
/*----------------------*/
#define GOF_D_BASE_MSG_ADDX     5000

/*==============================*/
/* Exchanged messages with ADDL */
/*==============================*/
const rtdbMsgType ADG_D_MSG_EXECRESULT                  = GOF_D_BASE_MSG_ADDX + 1;
const rtdbMsgType ADG_D_MSG_ENDINITACK                  = GOF_D_BASE_MSG_ADDX + 2;
const rtdbMsgType ADG_D_MSG_INIT                        = GOF_D_BASE_MSG_ADDX + 3;
const rtdbMsgType ADG_D_MSG_STOP                        = GOF_D_BASE_MSG_ADDX + 4;
const rtdbMsgType ADG_D_MSG_ENDALLINIT                  = GOF_D_BASE_MSG_ADDX + 5;
const rtdbMsgType ADG_D_MSG_ASKLIFE                     = GOF_D_BASE_MSG_ADDX + 6;
const rtdbMsgType ADG_D_MSG_LIVING                      = GOF_D_BASE_MSG_ADDX + 7;
const rtdbMsgType ADG_D_MSG_STARTAPPLI                  = GOF_D_BASE_MSG_ADDX + 8;
const rtdbMsgType ADG_D_MSG_STOPAPPLI                   = GOF_D_BASE_MSG_ADDX + 9;
const rtdbMsgType ADG_D_MSG_ADJTIME                     = GOF_D_BASE_MSG_ADDX + 10;
const rtdbMsgType ADG_D_MSG_DIFINIT                     = GOF_D_BASE_MSG_ADDX + 11;
const rtdbMsgType ADG_D_MSG_PROCSTOP                    = GOF_D_BASE_MSG_ADDX + 12;
const rtdbMsgType ADG_D_MSG_PROCSTART                   = GOF_D_BASE_MSG_ADDX + 13;
const rtdbMsgType ADG_D_MSG_STOPPROCNUM                 = GOF_D_BASE_MSG_ADDX + 14;
const rtdbMsgType ADG_D_MSG_STARTPROCNUM                = GOF_D_BASE_MSG_ADDX + 15;
const rtdbMsgType ADG_D_MSG_STOPPARTIAL                 = GOF_D_BASE_MSG_ADDX + 16;
const rtdbMsgType ADG_D_MSG_STARTPARTIAL                = GOF_D_BASE_MSG_ADDX + 17;
const rtdbMsgType ADG_D_MSG_RUNPROCBYNAME               = GOF_D_BASE_MSG_ADDX + 18;
const rtdbMsgType ADG_D_MSG_TRACELEVEL                  = GOF_D_BASE_MSG_ADDX + 19;
const rtdbMsgType ADG_D_MSG_SPY_ANALYZE                 = GOF_D_BASE_MSG_ADDX + 20;
const rtdbMsgType ADG_D_MSG_INITSTBY                    = GOF_D_BASE_MSG_ADDX + 21;
const rtdbMsgType ADG_D_MSG_STBYSTARTED                 = GOF_D_BASE_MSG_ADDX + 22;
const rtdbMsgType ADG_D_MSG_SWITCHOVER                  = GOF_D_BASE_MSG_ADDX + 23;
const rtdbMsgType ADG_D_MSG_UPDATESTBY                  = GOF_D_BASE_MSG_ADDX + 24;
const rtdbMsgType ADG_D_MSG_ENDUPDATESTBY               = GOF_D_BASE_MSG_ADDX + 25;
const rtdbMsgType ADG_D_MSG_STBYOFF                     = GOF_D_BASE_MSG_ADDX + 26;
const rtdbMsgType ADG_D_MSG_CLUSTERREADY                = GOF_D_BASE_MSG_ADDX + 27;
const rtdbMsgType ADG_D_MSG_CLUSTEROFF                  = GOF_D_BASE_MSG_ADDX + 28;
const rtdbMsgType ADG_D_MSG_ERRORWRITESTBY              = GOF_D_BASE_MSG_ADDX + 29;
const rtdbMsgType ADG_D_MSG_SWITCHAPPLI                 = GOF_D_BASE_MSG_ADDX + 30;
const rtdbMsgType ADG_D_MSG_MODIFVARENV                 = GOF_D_BASE_MSG_ADDX + 31;

/*==============================*/
/* Exchanged messages with ADDL */
/*==============================*/
/* define for the requests from EGSA to ESEN message type */
const rtdbMsgType ECH_D_MSG_INT_REQUEST                 = GOF_D_BASE_MSG_ECH + 1;
/* define for the replies from ESEN to EGSA  message type */
const rtdbMsgType ECH_D_MSG_INT_REPLY                   = GOF_D_BASE_MSG_ECH + 2;
/* define for the requests from acu_proc1 to acu_proc2 message type           */
const rtdbMsgType ACU_D_MSG_INT_REQUEST                 = GOF_D_BASE_MSG_ECH + 21;
/* define for the replies from acu_proc2 to acu_proc1  message type           */
const rtdbMsgType ACU_D_MSG_INT_REPLY                   = GOF_D_BASE_MSG_ECH + 22;

/*==============================*/
/* Exchanged messages with SINF */
/*==============================*/

const rtdbMsgType SIG_D_MSG_EXECRESULT              = GOF_D_BASE_MSG_SINF + 1;
const rtdbMsgType SIG_D_MSG_INTERMRESULT            = GOF_D_BASE_MSG_SINF + 2;
const rtdbMsgType SIG_D_MSG_CMDACK                  = GOF_D_BASE_MSG_SINF + 3;
const rtdbMsgType SIG_D_MSG_ACQNOACK                = GOF_D_BASE_MSG_SINF + 4;
const rtdbMsgType SIG_D_MSG_DIAGACK                 = GOF_D_BASE_MSG_SINF + 5;
const rtdbMsgType SIG_D_MSG_RESP_CREATE             = GOF_D_BASE_MSG_SINF + 6;
const rtdbMsgType SIG_D_MSG_RESP_MODSUP             = GOF_D_BASE_MSG_SINF + 7;
const rtdbMsgType SIG_D_MSG_ACQUIREDDATA            = GOF_D_BASE_MSG_SINF + 8;
const rtdbMsgType SIG_D_MSG_GRPSBS                  = GOF_D_BASE_MSG_SINF + 9;
const rtdbMsgType SIG_D_MSG_LSTSBS                  = GOF_D_BASE_MSG_SINF + 10;
const rtdbMsgType SIG_D_MSG_RESPENDHISTO            = GOF_D_BASE_MSG_SINF + 11;
const rtdbMsgType SIG_D_MSG_ACKREFALA               = GOF_D_BASE_MSG_SINF + 12;
const rtdbMsgType SIG_D_MSG_ACKKEYALA               = GOF_D_BASE_MSG_SINF + 13;
const rtdbMsgType SIG_D_MSG_ECHCTLPRESS             = GOF_D_BASE_MSG_SINF + 14;
const rtdbMsgType SIG_D_MSG_MMICTLPRESS             = GOF_D_BASE_MSG_SINF + 15;
const rtdbMsgType SIG_D_MSG_ECHCONTROL              = GOF_D_BASE_MSG_SINF + 16;
const rtdbMsgType SIG_D_MSG_MMICONTROL              = GOF_D_BASE_MSG_SINF + 17;
const rtdbMsgType SIG_D_MSG_MAINTENANCE             = GOF_D_BASE_MSG_SINF + 18;
const rtdbMsgType SIG_D_MSG_VALVEPOS                = GOF_D_BASE_MSG_SINF + 19;
const rtdbMsgType SIG_D_MSG_MANUALVALVE             = GOF_D_BASE_MSG_SINF + 20;
const rtdbMsgType SIG_D_MSG_PASSWD                  = GOF_D_BASE_MSG_SINF + 21;
const rtdbMsgType SIG_D_MSG_THRESHOLDVALUES         = GOF_D_BASE_MSG_SINF + 22;
const rtdbMsgType SIG_D_MSG_SETMANUALVALUE          = GOF_D_BASE_MSG_SINF + 23;
const rtdbMsgType SIG_D_MSG_SETFORCINGVALUE         = GOF_D_BASE_MSG_SINF + 24;
const rtdbMsgType SIG_D_MSG_ENDFORCINGVALUE         = GOF_D_BASE_MSG_SINF + 25;
const rtdbMsgType SIG_D_MSG_INITSA                  = GOF_D_BASE_MSG_SINF + 26;
const rtdbMsgType SIG_D_MSG_ASYNACQSA               = GOF_D_BASE_MSG_SINF + 27;  
const rtdbMsgType SIG_D_MSG_ASYNACQSPECSA           = GOF_D_BASE_MSG_SINF + 28;
const rtdbMsgType SIG_D_MSG_ASYNACQEQP              = GOF_D_BASE_MSG_SINF + 29;
const rtdbMsgType SIG_D_MSG_CHEXPMODSA              = GOF_D_BASE_MSG_SINF + 30;
const rtdbMsgType SIG_D_MSG_GLOBLOADSA              = GOF_D_BASE_MSG_SINF + 31;
const rtdbMsgType SIG_D_MSG_UPLOADSA                = GOF_D_BASE_MSG_SINF + 32;
const rtdbMsgType SIG_D_MSG_PARTLOADSA              = GOF_D_BASE_MSG_SINF + 33;
const rtdbMsgType SIG_D_MSG_INHIBOPER               = GOF_D_BASE_MSG_SINF + 34;
const rtdbMsgType SIG_D_MSG_INHIBALARM              = GOF_D_BASE_MSG_SINF + 35;
const rtdbMsgType SIG_D_MSG_SHIFT                   = GOF_D_BASE_MSG_SINF + 36;
const rtdbMsgType SIG_D_MSG_SETINQUIRED             = GOF_D_BASE_MSG_SINF + 37;
const rtdbMsgType SIG_D_MSG_FASTRESULT              = GOF_D_BASE_MSG_SINF + 38;
const rtdbMsgType SIG_D_MSG_REQPRTBYREF             = GOF_D_BASE_MSG_SINF + 39;
const rtdbMsgType SIG_D_MSG_REQREPBYREF             = GOF_D_BASE_MSG_SINF + 40;

const rtdbMsgType SIG_D_MSG_CREATE_INCIDENT         = GOF_D_BASE_MSG_SINF + 41;
const rtdbMsgType SIG_D_MSG_DELETE_INCIDENT         = GOF_D_BASE_MSG_SINF + 42;
const rtdbMsgType SIG_D_MSG_MODIFY_INCIDENT         = GOF_D_BASE_MSG_SINF + 43;

const rtdbMsgType SIG_D_MSG_CREATE_ORDER            = GOF_D_BASE_MSG_SINF + 44;
const rtdbMsgType SIG_D_MSG_DELETE_ORDER            = GOF_D_BASE_MSG_SINF + 45;
const rtdbMsgType SIG_D_MSG_MODIFY_ORDER            = GOF_D_BASE_MSG_SINF + 46;

const rtdbMsgType SIG_D_MSG_AUTOR_MAN_ORDER         = GOF_D_BASE_MSG_SINF + 47;
const rtdbMsgType SIG_D_MSG_VALID_MAN_ORDER         = GOF_D_BASE_MSG_SINF + 48;
const rtdbMsgType SIG_D_MSG_ACK_ORDER               = GOF_D_BASE_MSG_SINF + 49;
const rtdbMsgType SIG_D_MSG_APPLI_ELEM_ORDER        = GOF_D_BASE_MSG_SINF + 50;

const rtdbMsgType SIG_D_MSG_GENAL                   = GOF_D_BASE_MSG_SINF + 51;
const rtdbMsgType SIG_D_MSG_USERAL                  = GOF_D_BASE_MSG_SINF + 52;

const rtdbMsgType SIG_D_MSG_REQPRTBYID              = GOF_D_BASE_MSG_SINF + 53;
const rtdbMsgType SIG_D_MSG_REQFILEBYID             = GOF_D_BASE_MSG_SINF + 54;
const rtdbMsgType SIG_D_MSG_REQREPSHIFT             = GOF_D_BASE_MSG_SINF + 55;
const rtdbMsgType SIG_D_MSG_PRTREPSHIFT             = GOF_D_BASE_MSG_SINF + 56;
const rtdbMsgType SIG_D_MSG_CHEXIST                 = GOF_D_BASE_MSG_SINF + 57;

const rtdbMsgType SIG_D_MSG_LISTAPPLIMODIFY         = GOF_D_BASE_MSG_SINF + 58;
const rtdbMsgType SIG_D_MSG_MANUALACTIVATION        = GOF_D_BASE_MSG_SINF + 59;
const rtdbMsgType SIG_D_MSG_REQMULTAUGT             = GOF_D_BASE_MSG_SINF + 60;
const rtdbMsgType SIG_D_MSG_ANSMULTAUGT             = GOF_D_BASE_MSG_SINF + 61;
const rtdbMsgType SIG_D_MSG_ALDIFF                  = GOF_D_BASE_MSG_SINF + 62;
const rtdbMsgType SIG_D_MSG_ALFILE                  = GOF_D_BASE_MSG_SINF + 63;

const rtdbMsgType SIG_D_MSG_CREATDISPATCH           = GOF_D_BASE_MSG_SINF + 64;
const rtdbMsgType SIG_D_MSG_SUPDISPATCH             = GOF_D_BASE_MSG_SINF + 65;
const rtdbMsgType SIG_D_MSG_FIRSTDISPATCH           = GOF_D_BASE_MSG_SINF + 66;
const rtdbMsgType SIG_D_MSG_ACQHISTDATA             = GOF_D_BASE_MSG_SINF + 67;
const rtdbMsgType SIG_D_MSG_REQDIFF_ORDER           = GOF_D_BASE_MSG_SINF + 68;
const rtdbMsgType SIG_D_MSG_ACQDIFF_ORDER           = GOF_D_BASE_MSG_SINF + 69;


const rtdbMsgType SIG_D_MSG_APPRESULTFILE           = GOF_D_BASE_MSG_SINF + 70;

const rtdbMsgType SIG_D_MSG_SENDRESULTTOGTEK        = GOF_D_BASE_MSG_SINF + 71;
const rtdbMsgType SIG_D_MSG_SENDALARMTOGTEK         = GOF_D_BASE_MSG_SINF + 72;

const rtdbMsgType SIG_D_MSG_ACQHISTFILE             = GOF_D_BASE_MSG_SINF + 73;
const rtdbMsgType SIG_D_MSG_ENDHISTFILE             = GOF_D_BASE_MSG_SINF + 74;
const rtdbMsgType SIG_D_MSG_MODTIMESYSTEM           = GOF_D_BASE_MSG_SINF + 75;
const rtdbMsgType SIG_D_MSG_FIL_FRTXT_ORDER         = GOF_D_BASE_MSG_SINF + 76;
const rtdbMsgType SIG_D_MSG_WST                     = GOF_D_BASE_MSG_SINF + 77;

const rtdbMsgType SIG_D_MSG_ACQDIFF_INCIDENT        = GOF_D_BASE_MSG_SINF + 78;

const rtdbMsgType SIG_D_MSG_POWERFAIL               = GOF_D_BASE_MSG_SINF + 79;
const rtdbMsgType SIG_D_MSG_POWERRESTO              = GOF_D_BASE_MSG_SINF + 80;
const rtdbMsgType SIG_D_MSG_CHGTMODE                = GOF_D_BASE_MSG_SINF + 81;
const rtdbMsgType SIG_D_MSG_COPY_EQUIP_FILE         = GOF_D_BASE_MSG_SINF + 82;
const rtdbMsgType SIG_D_MSG_FILL_EQUIP_FILE         = GOF_D_BASE_MSG_SINF + 83;
const rtdbMsgType SIG_D_MSG_ACK_RECEP_ORDER         = GOF_D_BASE_MSG_SINF + 84;

const rtdbMsgType SIG_D_MSG_CMDSERVSA               = GOF_D_BASE_MSG_SINF + 85;

const rtdbMsgType SIG_D_MSG_FIL_FRTXT_INCIDENT      = GOF_D_BASE_MSG_SINF + 86;
const rtdbMsgType SIG_D_MSG_ELEMPLANUPD             = GOF_D_BASE_MSG_SINF + 87;
const rtdbMsgType SIG_D_MSG_ENDEXCPLAN              = GOF_D_BASE_MSG_SINF + 88;
const rtdbMsgType SIG_D_MSG_ENDEXCMAN               = GOF_D_BASE_MSG_SINF + 89;

const rtdbMsgType SIG_D_MSG_PRTORDBYID              = GOF_D_BASE_MSG_SINF + 90;
const rtdbMsgType SIG_D_MSG_PRTINCBYID              = GOF_D_BASE_MSG_SINF + 91;

const rtdbMsgType SIG_D_MSG_PTDISPLAY               = GOF_D_BASE_MSG_SINF + 92;
const rtdbMsgType SIG_D_MSG_SASTATUS                = GOF_D_BASE_MSG_SINF + 93;

const rtdbMsgType SIG_D_MSG_SETTMVALUE              = GOF_D_BASE_MSG_SINF + 94;
const rtdbMsgType SIG_D_MSG_SETEQTVALUE             = GOF_D_BASE_MSG_SINF + 95;

const rtdbMsgType SIG_D_MSG_ACQ_GENCTRL             = GOF_D_BASE_MSG_SINF + 96;

const rtdbMsgType SIG_D_MSG_SETTMPASS               = GOF_D_BASE_MSG_SINF + 97;
const rtdbMsgType SIG_D_MSG_SETEQTPASS              = GOF_D_BASE_MSG_SINF + 98;

const rtdbMsgType SIG_D_MSG_CONSULT_ORDER           = GOF_D_BASE_MSG_SINF + 99;
const rtdbMsgType SIG_D_MSG_CONSULT_INCIDENT        = GOF_D_BASE_MSG_SINF + 100;
const rtdbMsgType SIG_D_MSG_ACKKEYALAPAGE       = GOF_D_BASE_MSG_SINF + 101;

const rtdbMsgType SIG_D_MSG_WRITEARCH           = GOF_D_BASE_MSG_SINF + 102;
const rtdbMsgType SIG_D_MSG_RESWRITEARCH        = GOF_D_BASE_MSG_SINF + 103;
const rtdbMsgType SIG_D_MSG_READARCH            = GOF_D_BASE_MSG_SINF + 104;
const rtdbMsgType SIG_D_MSG_RESREADARCH         = GOF_D_BASE_MSG_SINF + 105;
const rtdbMsgType SIG_D_MSG_ABANDARCH           = GOF_D_BASE_MSG_SINF + 106;
const rtdbMsgType SIG_D_MSG_INTERUPTARCH        = GOF_D_BASE_MSG_SINF + 107;
const rtdbMsgType SIG_D_MSG_OUTLINE             = GOF_D_BASE_MSG_SINF + 108;
const rtdbMsgType SIG_D_MSG_MMIDIRCONTROL       = GOF_D_BASE_MSG_SINF + 109;
const rtdbMsgType SIG_D_MSG_ECHDIRCONTROL       = GOF_D_BASE_MSG_SINF + 110;
const rtdbMsgType SIG_D_MSG_MULTITHRESHOLDS     = GOF_D_BASE_MSG_SINF + 111;
const rtdbMsgType SIG_D_MSG_ACKALAEND           = GOF_D_BASE_MSG_SINF + 112;
const rtdbMsgType SIG_D_MSG_DISPNAME            = GOF_D_BASE_MSG_SINF + 113;
const rtdbMsgType SIG_D_MSG_DELEGATION          = GOF_D_BASE_MSG_SINF + 114;
const rtdbMsgType SIG_D_MSG_ECHDIRCTLPRESS      = GOF_D_BASE_MSG_SINF + 115;
const rtdbMsgType SIG_D_MSG_MMIDIRCTLPRESS      = GOF_D_BASE_MSG_SINF + 116;
const rtdbMsgType SIG_D_MSG_EMERGENCY           = GOF_D_BASE_MSG_SINF + 117;
const rtdbMsgType SIG_D_MSG_STOPDIPLEMERG       = GOF_D_BASE_MSG_SINF + 118;
const rtdbMsgType SIG_D_MSG_ENDALLEMERG         = GOF_D_BASE_MSG_SINF + 119;
const rtdbMsgType SIG_D_MSG_NEWDIPLEMERG        = GOF_D_BASE_MSG_SINF + 120;
const rtdbMsgType SIG_D_MSG_EQT_MAINT           = GOF_D_BASE_MSG_SINF + 121;

const rtdbMsgType SIG_D_MSG_SETTSVALUE          = GOF_D_BASE_MSG_SINF + 122;
const rtdbMsgType SIG_D_MSG_SETTSPASS           = GOF_D_BASE_MSG_SINF + 123;
const rtdbMsgType SIG_D_MSG_ACDLIST             = GOF_D_BASE_MSG_SINF + 124;
const rtdbMsgType SIG_D_MSG_ACDQUERY            = GOF_D_BASE_MSG_SINF + 125;
/*
 * NB: Есть дополнительные константы типов, расположенные в sig/ext/dat: 
 * sig_ext_MsgDefInt.dat и sig_ext_MsgProcnumToInt.dat
 */

/* Type of messages */
/* ---------------- */
const rtdbMsgType ESG_D_TRANSMIT_REQUEST      = GOF_D_BASE_MSG_ECH + 11;
const rtdbMsgType ESG_D_TRANSMIT_REPORT       = GOF_D_BASE_MSG_ECH + 12;
const rtdbMsgType ESG_D_RECEIPT_INDIC         = GOF_D_BASE_MSG_ECH + 13;
const rtdbMsgType ESG_D_STATUS_INDIC          = GOF_D_BASE_MSG_ECH + 14;
const rtdbMsgType ESG_D_SYNTHETICAL_STATE     = GOF_D_BASE_MSG_ECH + 15;
const rtdbMsgType ESG_D_INIT_PHASE            = GOF_D_BASE_MSG_ECH + 16;
const rtdbMsgType ESG_D_REINIT_BACKUP         = GOF_D_BASE_MSG_ECH + 17;


#endif

