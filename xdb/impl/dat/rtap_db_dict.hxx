#pragma once
#ifndef XDB_RTAP_DB_DICT_HXX
#define XDB_RTAP_DB_DICT_HXX

#include <string>
#include <vector>

#include "config.h"

namespace rtap_db_dict
{

// NB: Оригинальные ограничения заданы в rtap_db_dict.xsd
// ValueEntry
#define VALUE_ENTRY_MAX      12
// UnityIdEntry
#define UNITY_ENTRY_MAX     100
// UnityIdType
#define UNITY_TYPE_MAX      100
// ObjClassEntry
#define OBJCLASS_ENTRY_MAX   80
// UnityDimensionType
#define DIMENSION_ENTRY_MAX  30

   typedef enum {
      TS = 0
    , TM = 1
    , TR = 2
    , TSA = 3
    , TSC = 4
    , TC = 5
    , AL = 6
    , ICS = 7
    , ICM = 8
    , PIPE = 11
    , PIPELINE = 15
    , TL = 16
    , VA = 19
    , SC = 20
    , ATC = 21
    , GRC = 22
    , SV = 23
    , SDG = 24
    , RGA = 25
    , SSDG = 26
    , BRG = 27
    , SCP = 28
    , STG = 29
    , DIR = 30
    , DIPL = 31
    , METLINE = 32
    , ESDG = 33
    , SVLINE = 34
    , SCPLINE = 35
    , TLLINE = 36
    , INVT = 37
    , AUX1 = 38
    , AUX2 = 39
    , SITE = 45
    , SA = 50
    , FIXEDPOINT = 79
    , HIST = 80
    , UNUSED = 127
   } ObjClass_t;

   typedef unsigned int id_t;

   typedef std::string Label_t;

   typedef struct {
     id_t       dimension_id;
     Label_t    dimension_entry;
     id_t       unity_id;
     Label_t    unity_entry;
     Label_t    designation_entry;
   } UnityLabel_t;

   typedef struct {
     ObjClass_t objclass;
     id_t       value;
     Label_t    label;
   } ValLabel_t;

   typedef struct {
     ObjClass_t objclass;
     Label_t    label;
     Label_t    designation;
   } InfoTypes_t;

   // Хранилище прочитанных таблиц НСИ
   typedef struct {
     std::vector <UnityLabel_t> unity_dict;
     std::vector <ValLabel_t>   values_dict;
     std::vector <InfoTypes_t>  infotypes_dict;
   } Dictionaries_t;

} // namespace

#endif

