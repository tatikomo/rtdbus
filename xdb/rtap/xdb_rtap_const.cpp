#include <vector>

#include <time.h>

#include "config.h"
#include "xdb_rtap_const.hpp"
#include "xdb_core_attribute.hpp"

using namespace xdb::rtap;

// Перекодировочная таблица замены типам RTAP на тип eXtremeDB
// Индекс элемента - код типа данных RTAP
// Значение элемента - соответствующий индексу тип данных в eXtremeDB
// 
const DeTypeToDbTypeLink xdb::rtap::DeTypeToDbType[] = 
{
    { rtRESERVED0,  xdb::core::DB_TYPE_UNDEF },
    { rtLOGICAL,    xdb::core::DB_TYPE_INT8 },
    { rtINT8,       xdb::core::DB_TYPE_INT8 },
    { rtUINT8,      xdb::core::DB_TYPE_UINT8 },
    { rtINT16,      xdb::core::DB_TYPE_INT16 },
    { rtUINT16,     xdb::core::DB_TYPE_UINT16 },
    { rtINT32,      xdb::core::DB_TYPE_INT32 },
    { rtUINT32,     xdb::core::DB_TYPE_UINT32 },
    { rtFLOAT,      xdb::core::DB_TYPE_FLOAT },
    { rtDOUBLE,     xdb::core::DB_TYPE_DOUBLE },
    { rtPOLAR,      xdb::core::DB_TYPE_UNDEF },
    { rtRECTANGULAR,xdb::core::DB_TYPE_UNDEF },
    { rtRESERVED12, xdb::core::DB_TYPE_UNDEF },
    { rtRESERVED13, xdb::core::DB_TYPE_UNDEF },
    { rtRESERVED14, xdb::core::DB_TYPE_UNDEF },
    { rtRESERVED15, xdb::core::DB_TYPE_UNDEF },
    { rtBYTES4,     xdb::core::DB_TYPE_BYTES4 },
    { rtBYTES8,     xdb::core::DB_TYPE_BYTES8 },
    { rtBYTES12,    xdb::core::DB_TYPE_BYTES12 },
    { rtBYTES16,    xdb::core::DB_TYPE_BYTES16 },
    { rtBYTES20,    xdb::core::DB_TYPE_BYTES20 },
    { rtBYTES32,    xdb::core::DB_TYPE_BYTES32 },
    { rtBYTES48,    xdb::core::DB_TYPE_BYTES48 },
    { rtBYTES64,    xdb::core::DB_TYPE_BYTES64 },
    { rtBYTES80,    xdb::core::DB_TYPE_BYTES80 },
    { rtBYTES128,   xdb::core::DB_TYPE_BYTES128 },
    { rtBYTES256,   xdb::core::DB_TYPE_BYTES256 },
    { rtRESERVED27, xdb::core::DB_TYPE_UNDEF },
    { rtDB_XREF,    xdb::core::DB_TYPE_UINT64 },
    { rtDATE,       xdb::core::DB_TYPE_UINT64 },
    { rtTIME_OF_DAY,xdb::core::DB_TYPE_UINT64 },
    { rtASB_TIME,   xdb::core::DB_TYPE_UINT64 },
    { rtUNDEFINED,  xdb::core::DB_TYPE_UNDEF },
};

// Соответствие между кодом типа БДРВ и его аналогом в RTAP
const xdb::rtap::DbTypeToDeTypeLink DbTypeToDeType[] = 
{
  { xdb::core::DB_TYPE_UNDEF,  rtUNDEFINED },
  { xdb::core::DB_TYPE_INT8,   rtINT8 },
  { xdb::core::DB_TYPE_UINT8,  rtUINT8 },
  { xdb::core::DB_TYPE_INT16,  rtINT16 },
  { xdb::core::DB_TYPE_UINT16, rtUINT16 },
  { xdb::core::DB_TYPE_INT32,  rtINT32 },
  { xdb::core::DB_TYPE_UINT32, rtUINT32 },
  { xdb::core::DB_TYPE_INT64,  rtUNDEFINED }, // RTAP не поддерживает
  { xdb::core::DB_TYPE_UINT64, rtUNDEFINED }, // RTAP не поддерживает
  { xdb::core::DB_TYPE_FLOAT,  rtFLOAT },
  { xdb::core::DB_TYPE_DOUBLE, rtDOUBLE },
  { xdb::core::DB_TYPE_BYTES,  rtUNDEFINED }, // RTAP не поддерживает
  { xdb::core::DB_TYPE_BYTES4, rtBYTES4 },
  { xdb::core::DB_TYPE_BYTES8, rtBYTES8 },
  { xdb::core::DB_TYPE_BYTES12, rtBYTES12 },
  { xdb::core::DB_TYPE_BYTES16, rtBYTES16 },
  { xdb::core::DB_TYPE_BYTES20, rtBYTES20 },
  { xdb::core::DB_TYPE_BYTES32, rtBYTES32 },
  { xdb::core::DB_TYPE_BYTES48, rtBYTES48 },
  { xdb::core::DB_TYPE_BYTES64, rtBYTES64 },
  { xdb::core::DB_TYPE_BYTES80 , rtBYTES80 },
  { xdb::core::DB_TYPE_BYTES128, rtBYTES128 },
  { xdb::core::DB_TYPE_BYTES256, rtBYTES256 }
};

// Описание типов данных БДРВ
const DbTypeDescription_t xdb::rtap::DbTypeDescription[] = 
{
  { xdb::core::DB_TYPE_UNDEF,  "UNDEF",    0 },
  { xdb::core::DB_TYPE_INT8,   "INT8",     sizeof(int8_t) },
  { xdb::core::DB_TYPE_UINT8,  "UINT8",    sizeof(uint8_t) },
  { xdb::core::DB_TYPE_INT16,  "INT16",    sizeof(int16_t) },
  { xdb::core::DB_TYPE_UINT16, "UINT16",   sizeof(uint16_t) },
  { xdb::core::DB_TYPE_INT32,  "INT32",    sizeof(int32_t) },
  { xdb::core::DB_TYPE_UINT32, "UINT32",   sizeof(uint32_t) },
  { xdb::core::DB_TYPE_INT64,  "INT64",    sizeof(int32_t) },
  { xdb::core::DB_TYPE_UINT64, "UINT64",   sizeof(uint32_t) },
  { xdb::core::DB_TYPE_FLOAT,  "FLOAT",    sizeof(float) },
  { xdb::core::DB_TYPE_DOUBLE, "DOUBLE",   sizeof(double) },
  { xdb::core::DB_TYPE_BYTES,  "BYTES",    0 },    // может варьироваться
  { xdb::core::DB_TYPE_BYTES4, "BYTES4",   sizeof(wchar_t)*4 },
  { xdb::core::DB_TYPE_BYTES8, "BYTES8",   sizeof(wchar_t)*8 },
  { xdb::core::DB_TYPE_BYTES12, "BYTES12", sizeof(wchar_t)*12 },
  { xdb::core::DB_TYPE_BYTES16, "BYTES16", sizeof(wchar_t)*16 },
  { xdb::core::DB_TYPE_BYTES20, "BYTES20", sizeof(wchar_t)*20 },
  { xdb::core::DB_TYPE_BYTES32, "BYTES32", sizeof(wchar_t)*32 },
  { xdb::core::DB_TYPE_BYTES48, "BYTES48", sizeof(wchar_t)*48 },
  { xdb::core::DB_TYPE_BYTES64, "BYTES64", sizeof(wchar_t)*64 },
  { xdb::core::DB_TYPE_BYTES80 , "BYTES80",    sizeof(wchar_t)*80 },
  { xdb::core::DB_TYPE_BYTES128, "BYTES128",   sizeof(wchar_t)*128 },
  { xdb::core::DB_TYPE_BYTES256, "BYTES256",   sizeof(wchar_t)*256 }
};

// Описание типов данных RTAP
const DeTypeDescription_t xdb::rtap::rtDataElem[] =
{
    { rtRESERVED0,   "RESERVED0",     0 },
    { rtLOGICAL,     "rtLOGICAL",  sizeof(uint8_t) },
    { rtINT8,        "rtINT8",     sizeof(int8_t) },
    { rtUINT8,       "rtUINT8",    sizeof(uint8_t) },
    { rtINT16,       "rtINT16",    sizeof(int16_t) },
    { rtUINT16,      "rtUINT16",   sizeof(uint16_t) },
    { rtINT32,       "rtINT32",    sizeof(int32_t) },
    { rtUINT32,      "rtUINT32",   sizeof(uint32_t) },
    { rtFLOAT,       "rtFLOAT",    sizeof(float) },
    { rtDOUBLE,      "rtDOUBLE",   sizeof(double) },
    { rtPOLAR,       "rtPOLAR",       0 }, // not used
    { rtRECTANGULAR, "rtRECTANGULAR", 0 }, // not used
    { rtRESERVED12,  "RESERVED12",    0 }, // not used
    { rtRESERVED13,  "RESERVED13",    0 }, // not used
    { rtRESERVED14,  "RESERVED14",    0 }, // not used
    { rtRESERVED15,  "RESERVED15",    0 }, // not used
    { rtBYTES4,      "rtBYTES4",   sizeof(uint8_t)*4 },
    { rtBYTES8,      "rtBYTES8",   sizeof(uint8_t)*8 },
    { rtBYTES12,     "rtBYTES12",  sizeof(uint8_t)*12 },
    { rtBYTES16,     "rtBYTES16",  sizeof(uint8_t)*16 },
    { rtBYTES20,     "rtBYTES20",  sizeof(uint8_t)*20 },
    { rtBYTES32,     "rtBYTES32",  sizeof(uint8_t)*32 },
    { rtBYTES48,     "rtBYTES48",  sizeof(uint8_t)*48 },
    { rtBYTES64,     "rtBYTES64",  sizeof(uint8_t)*64 },
    { rtBYTES80,     "rtBYTES80",  sizeof(uint8_t)*80 },
    { rtBYTES128,    "rtBYTES128", sizeof(uint8_t)*128 },
    { rtBYTES256,    "rtBYTES256", sizeof(uint8_t)*256 },
    { rtRESERVED27,  "RESERVED27",    0 },
    { rtDB_XREF,     "rtXREF",        0 }, // TODO: возможно, это д.б. символьная ссылка
    { rtDATE,        "rtDATE",        sizeof(timeval) },
    { rtTIME_OF_DAY, "rtTIME_OF_DAY", sizeof(timeval) },
    { rtASB_TIME,    "rtABSTIME",     sizeof(timeval) },
    { rtUNDEFINED,   "rtUNDEFINED",   0 }
};

// Динамическая таблдица описаний классов
// Заполняется из шаблонных файлов вида XX_YYY.dat
// где 
//  XX = номер класса
//  YYY = название класса
ClassDescription_t xdb::rtap::ClassDescriptionTable[] = {
    {"TS",      GOF_D_BDR_OBJCLASS_TS, 0},          /* Телесигнализация */
    {"TM",      1, 0},          /* Телеизмерение */
    {"TR",      2, 0},          /* Телерегулировка */
    {"TSA",     3, 0},
    {"TSC",     4, 0},
    {"TC",      5, 0},          /* Телеуправление */
    {"AL",      6, 0},          /* Полученный аварийный сигнал */
    {"ICS",     7, 0},          /* Двоичные расчетные данные */
    {"ICM",     8, 0},          /* Аналоговые расчетные данные */
    {D_MISSING_OBJCODE,  9, 0}, /* "REF" - Не используется */
    {D_MISSING_OBJCODE, 10, 0}, /* Missing */
/* -------------------------------------  Network objects       */
    {"PIPE",    11, 0},         /* Труба */
    {"BORDER",  12, 0},         /* Граница */
    {"CONSUMER",13, 0},         /* Потребитель */
    {"CORRIDOR",14, 0},         /* Коридор */
    {"PIPELINE",15, 0},         /* Трубопровод */
/* -------------------------------------  Equipments objects    */
/* NB: update GOF_D_BDR_OBJCLASS_EQTLAST when adding an Equipments objects */
    {"TL",      16, 0},         /* Линейный участок */
    {D_MISSING_OBJCODE, 17, 0}, /* Missing */
    {D_MISSING_OBJCODE, 18, 0}, /* Missing */
    {"VA",      19, 0},         /* Кран */
    {"SC",      20, 0},         /* Компрессорная станция */
    {"ATC",     21, 0},         /* Цех */
    {"GRC",     22, 0},         /* Агрегат */
    {"SV",      23, 0},         /* Крановая площадка */
    {"SDG",     24, 0},         /* Газораспределительная станция */
    {"RGA",     25, 0},         /* Станция очистного поршня */
    {"SSDG",    26, 0},         /* Выход ГРС */
    {"BRG",     27, 0},         /* Газораспределительный блок */
    {"SCP",     28, 0},         /* Пункт замера расхода */
    {"STG",     29, 0},         /* Установка подготовки газа */
    {"DIR",     30, 0},         /* Предприятие */
    {"DIPL",    31, 0},         /* ЛПУ */
    {"METLINE", 32, 0},         /* Газоизмерительная нитка */
    {"ESDG",    33, 0},         /* Вход ГРС */
    {"SVLINE",  34, 0},         /* Точка КП на газопроводе */
    {"SCPLINE", 35, 0},         /* Нитка ПЗРГ */
    {"TLLINE",  36, 0},         /* Нитка ЛУ */
    {"INVT",    37, 0},         /* Инвертор */
    {"AUX1",    38, 0},         /* Вспом объект I-го уровня */
    {"AUX2",    39, 0},         /* Вспом объект II-го уровня */
    {D_MISSING_OBJCODE, 40, 0}, /* Missing */
    {D_MISSING_OBJCODE, 41, 0}, /* Missing */
    {D_MISSING_OBJCODE, 42, 0}, /* Missing */
    {D_MISSING_OBJCODE, 43, 0}, /* Missing */
    {D_MISSING_OBJCODE, 44, 0}, /* Missing */
/* -------------------------------------  Fixed objects    */
    {"SITE",    45, 0},         /* Корневая точка с информацией о локальном объекте */
    {D_MISSING_OBJCODE, 46, 0}, /* Missing */
    {D_MISSING_OBJCODE, 47, 0}, /* Missing */
    {D_MISSING_OBJCODE, 48, 0}, /* Missing */
    {D_MISSING_OBJCODE, 49, 0}, /* Missing */
/* -------------------------------------  System objects    */
/* NB: update GOF_D_BDR_OBJCLASS_SYSTLAST when adding an System objects */
    {"SA",      50, 0},         /* Система сбора данных */
/* -------------------------------------  Alarms objects    */
    {"GENAL",   51, 0},         /*  */
    {"USERAL",  52, 0},         /*  */
    {"HISTSET", 53, 0},         /*  */

    {D_MISSING_OBJCODE, 54, 0}, /* Missing */
    {D_MISSING_OBJCODE, 55, 0}, /* Missing */
    {D_MISSING_OBJCODE, 56, 0}, /* Missing */
    {D_MISSING_OBJCODE, 57, 0}, /* Missing */
    {D_MISSING_OBJCODE, 58, 0}, /* Missing */
    {D_MISSING_OBJCODE, 59, 0}, /* Missing */

    {"DRIVE_TYPE",      60, 0}, /*  */
    {"COMPRESSOR_TYPE", 61, 0}, /*  */
    {"GRC_TYPE",        62, 0}, /*  */
    {"ATC_TYPE",        63, 0}, /*  */
    {"SC_TYP",          64, 0}, /*  */
    {"SDG_TYP",         65, 0}, /*  */
    {"SCP_TYP",         66, 0}, /*  */
    {D_MISSING_OBJCODE, 67, 0}, /* Missing */
    {"PIPE_TYPE",       68, 0}, /*  */
    {"VA_TYPE",         69, 0}, /*  */
    {"TS_TYPE",         70, 0}, /*  */
    {"AL_TYPE",         71, 0}, /*  */
    {"TSC_TYPE",        72, 0}, /*  */
    {"AUX_TYPE",        73, 0}, /*  */
    {"VERSION",         74, 0}, /*  */
    {"DISP_TABLE_H",    75, 0}, /*  */
    {"DISP_TABLE_J",    76, 0}, /*  */
    {"DISP_TABLE_M",    77, 0}, /*  */
    {"DISP_TABLE_QH",   78, 0}, /*  */
    {"FIXEDPOINT",      79, 0}, /*  */
    {"HIST_SET",        80, 0}, /*  */
    {"HIST_TABLE_H",    81, 0}, /*  */
    {"HIST_TABLE_J",    82, 0}, /*  */
    {"HIST_TABLE_M",    83, 0}, /*  */
    {"HIST_TABLE_QH",   84, 0}, /*  */
    {"HIST_TABLE_SAMPLE",85,0}, /*  */
    {"TIME_AVAILABLE",  86, 0}, /*  */
    {"config",          87, 0}  /*  */
};

