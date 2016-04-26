#if !defined EXCHANGE_EGSA_INIT_HPP
#define EXCHANGE_EGSA_INIT_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tool_timer.hpp"
#include "exchange_config.hpp"

#define REQUESTNAME_MAX     17
#define ECH_D_MAXNBSITES    28
#define EGA_EGA_D_LGCYCLENAME 23

// Target object class
typedef enum 
{
  INFO,     // information
  EQUIP,    // technological equipment,
  ACQSYS    // acquisition system
} ega_ega_t_ObjectClass;

// acquisition mode
typedef enum {
  DIFF,
  NONDIFF,
  NOT_SPECIFIED
} ech_t_AcqMode;

typedef enum {
  // Нормальный цикл инициализации
  TN_INITAUTO,
  // Цикл инициализации, требующий повышенной производительности
  // (более требователен к ресурсам)
  /* TH_INITAUTO, */
  // Цикл определения устаревших запросов
  TC_OLDREQUEST,
  // Цикл отправки ответов
  TC_SENDREPLY
} cycle_family_2_t; //GEV непонятно пока к чему относится

typedef enum {
  // Нормальный цикл
  CYCLE_NORMAL      = 1,
  // Цикл, требующий повышенной производительности
  // (более требователен к ресурсам)
  CYCLE_HCPULOAD    = 2,
  // Цикл управления
  CYCLE_CONTROL     = 3
} cycle_family_t;

// Типы запросов
// NB: Поддерживать синхронизацию с типами запросов в common.proto
// Нумерация начинается с нуля и вохрастает монотонно без разрывов
typedef enum {
  ECH_D_GENCONTROL   = 0,   /* general control */
  ECH_D_INFOSACQ     = 1,   /* global acquisition */
  ECH_D_URGINFOS     = 2,   /* urgent data acquisition */
  ECH_D_GAZPROCOP    = 3,   /* gaz volume count acquisition */
  ECH_D_EQUIPACQ     = 4,   /* equipment acquisition */
  ECH_D_ACQSYSACQ    = 5,   /* Sac data acquisition */
  ECH_D_ALATHRES     = 6,   /* alarms and thresholds acquisition */
  ECH_D_TELECMD      = 7,   /* telecommand */
  ECH_D_TELEREGU     = 8,   /* teleregulation */
  ECH_D_SERVCMD      = 9,   /* service command */
  ECH_D_GLOBDWLOAD   = 10,  /* global download */
  ECH_D_PARTDWLOAD   = 11,  /* partial download */
  ECH_D_GLOBUPLOAD   = 12,  /* Sac configuration global upload */
  ECH_D_INITCMD      = 13,  /* initialisation of the exchanges */
  ECH_D_GCPRIMARY    = 14,  /* Primary general control */
  ECH_D_GCSECOND     = 15,  /* Secondary general control */
  ECH_D_GCTERTIARY   = 16,  /* Tertiary general control */
  ECH_D_DIFFPRIMARY  = 17,  /* Primary differential acquisition */
  ECH_D_DIFFSECOND   = 18,  /* Secondary differential acquisition */
  ECH_D_DIFFTERTIARY = 19,  /* Tertiary differential acquisition */
  ECH_D_INFODIFFUSION= 20,  /* Information diffusion              */
  ECH_D_DELEGATION   = 21   /* Process order delegation-end of delegation */
} ech_t_ReqId;

// NB: Должен быть последним значением ech_t_ReqId + 1
#define NBREQUESTS          (ECH_D_DELEGATION + 1)

// Names of External System Requests
// ---------------------------------
#define EGA_EGA_D_STRGENCONTROL "A_GENCONTROL"  // general control     (none differential)
#define EGA_EGA_D_STRINFOSACQ   "A_INFOSACQ"    // all informations         (differential)
#define EGA_EGA_D_STRURGINFOS   "A_URGINFOS"    // urgent informations      (differential)
#define EGA_EGA_D_STRALATHRES   "A_ALATHRES"    // alarms and thresholds    (differential)
#define EGA_EGA_D_STRGAZPROCOP  "A_GAZPROCOP"   // gaz volume count    (none differential)
#define EGA_EGA_D_STREQUIPACQ   "A_EQUIPACQ"    // equipment           (none differential)
#define EGA_EGA_D_STRACQSYSACQ  "A_ACQSYSACQ"   // SATO informations   (none differential)
#define EGA_EGA_D_STRTELECMD    "P_TELECMD"	    // telecommande
#define EGA_EGA_D_STRTELEREGU   "P_TELEREGU"    // teleregulation
#define EGA_EGA_D_STRSERVCMD    "S_SERVCMD"	    // SATO service command
#define EGA_EGA_D_STRGLOBDWLOAD "C_GLOBDWLOAD"  // global downloading
#define EGA_EGA_D_STRPARTDWLOAD "C_PARTDWLOAD"  // partial downloading
#define EGA_EGA_D_STRGLOBUPLOAD "C_GLOBUPLOAD"  // global uploading
#define EGA_EGA_D_STRINITCMD    "I_INITCMD"	    // initialisation
#define EGA_EGA_D_STRGCPRIMARY  "A_GCPRIMARY"   // prim. general control (none differential)
#define EGA_EGA_D_STRGCSECOND   "A_GCSECOND"    // sec. general control  (none differential)
#define EGA_EGA_D_STRGCTERTIARY "A_GCTERTIARY"  // tert. general control (none differential)
#define EGA_EGA_D_STRIAPRIMARY  "A_IAPRIMARY"   // acq.prim.informations    (differential)
#define EGA_EGA_D_STRIASECOND   "A_IASECOND"    // acq.sec. informations    (differential)
#define EGA_EGA_D_STRIATERTIARY "A_IATERTIARY"  // acq.tert. informations   (differential)
#define EGA_EGA_D_STRINFOSDIFF    "D_INFODIFF"  // diffusion informations
#define EGA_EGA_D_STRDIFFPRIMARY  "D_DIFFPRIMARY"   // diffusion primary
#define EGA_EGA_D_STRDIFFSECOND   "D_DIFFSECONDARY" // diffusion secondary
#define EGA_EGA_D_STRDIFFTERTIARY "D_DIFFTERTIARY"  // diffusion tertiary
#define EGA_EGA_D_STRDELEGATION   "P_DELEGATION"    // delegation tele cmd

// ==============================================================================
// Acquisition Site Entry Structure
typedef struct {
  // identifier of the acquisition site (name)
  char s_IdAcqSite[20 + 1];

  // nature of the acquisition site (acquisition system, adjacent DIPL)
  gof_t_SacNature i_NatureAcqSite;

  // Не требуется хранить номер интерфейсного процесса o_IdInterfaceComponent
  // identifier of the interface component (procnum)
  //gof_t_Uint8 o_IdInterfaceComponent;
  // indicator of automatical initialisation (TRUE -> has to be performed,
  //                                          FALSE-> has not to be performed)
  bool b_AutomaticalInit;

  // indicator of automatical general control (TRUE -> has to be performed,
  //                                           FALSE-> has not to be performed)
  bool b_AutomaticalGenCtrl;

  // Не требуется
  // data base information subscription for synthetic state, inhibition state, exploitation mode
  //ega_ega_odm_t_SubscriptedInfo r_SubscrInhibState;
  //ega_ega_odm_t_SubscriptedInfo r_SubscrSynthState;
  //ega_ega_odm_t_SubscriptedInfo r_SubscrExploitMode;
    
  // functional EGSA state of the acquisition site
  // (acquisition site command only, all operations, all dispatcher requests only, waiting for no inhibition)
  int i_FunctionalState;

  // interface component state : active / stopped (TRUE / FALSE)
  bool b_InterfaceComponentActive;

  // requests in progress list access
  void* p_ProgList;

  // acquired data access
  void* p_AcquiredData;
} ega_ega_odm_t_AcqSiteEntry;

// ==============================================================================
typedef struct {
  // Идентификатор запроса
  ech_t_ReqId   e_RequestId;
  // Название запроса
  char  s_RequestName[REQUESTNAME_MAX + 1];
  // Приоритет
  int   i_RequestPriority;
  // Тип объекта - информация, СС, оборудование
  ega_ega_t_ObjectClass e_RequestObject;
  // Режим сбора - дифференциальный или нет
  ech_t_AcqMode e_RequestMode;
  // Признак отношения запроса к технологическим данным (1)
  // или к состоянию самой системы сбора как опрашиваемого объекта (2)
  bool  b_Requestprocess;
  // Включенные в данный запрос подзапросы
#warning "Включенные запросы вероятно не используются"
//  ega_ega_t_Requests r_IncludingRequests;
} ega_ega_odm_t_RequestEntry;


// Найти по таблице запрос с заданным идентификатором.
// Есть такой - присвоить параметру его адрес и вернуть OK
// Нет такого - присвоить второму параметру NULL и вернуть NOK
int get_request_by_id(ech_t_ReqId, ega_ega_odm_t_RequestEntry*&);
// Найти по таблице запрос с заданным названием.
// Есть такой - присвоить параметру его адрес и вернуть OK
// Нет такого - присвоить второму параметру NULL и вернуть NOK
int get_request_by_name(const std::string&, ega_ega_odm_t_RequestEntry*&);

#endif

