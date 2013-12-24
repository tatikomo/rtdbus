#include <vector>

#include <time.h>

#include "config.h"
#include "xdb_rtap_const.hpp"

using namespace xdb;

rtDataElemDescription xdb::rtDataElem[] =
{
    { rtRESERVED0,   0,               "RESERVED(0)" },
    { rtLOGICAL, sizeof(uint8_t),     "LOGICAL" },
    { rtINT8,    sizeof(int8_t),      "INT8" },
    { rtUINT8,   sizeof(uint8_t),     "UINT8" },
    { rtINT16,   sizeof(int16_t),     "INT16" },
    { rtUINT16,  sizeof(uint16_t),    "UINT16" },
    { rtINT32,   sizeof(int32_t),     "INT32" },
    { rtUINT32,  sizeof(uint32_t),    "UINT32" },
    { rtFLOAT,   sizeof(float),       "FLOAT" },
    { rtDOUBLE,  sizeof(double),      "DOUBLE" },
    { rtPOLAR,       0,               "POLAR" },         // not used
    { rtRECTANGULAR, 0,               "RECTANGULAR" },   // not used
    { rtRESERVED12,  0,               "RESERVED(12)" },  // not used
    { rtRESERVED13,  0,               "RESERVED(13)" },  // not used
    { rtRESERVED14,  0,               "RESERVED(14)" },  // not used
    { rtRESERVED15,  0,               "RESERVED(15)" },  // not used
    { rtBYTES4,  sizeof(uint8_t)*4,   "BYTES4" },
    { rtBYTES8,  sizeof(uint8_t)*8,   "BYTES8" },
    { rtBYTES12, sizeof(uint8_t)*12,  "BYTES12" },
    { rtBYTES16, sizeof(uint8_t)*16,  "BYTES16" },
    { rtBYTES20, sizeof(uint8_t)*20,  "BYTES20" },
    { rtBYTES32, sizeof(uint8_t)*32,  "BYTES32" },
    { rtBYTES48, sizeof(uint8_t)*48,  "BYTES48" },
    { rtBYTES64, sizeof(uint8_t)*64,  "BYTES64" },
    { rtBYTES80, sizeof(uint8_t)*80,  "BYTES80" },
    { rtBYTES128,sizeof(uint8_t)*128, "BYTES128" },
    { rtBYTES256,sizeof(uint8_t)*256, "BYTES256" },
    { rtRESERVED27,  0,               "RESERVED(27)" },
    { rtDB_XREF,     0,               "XREF" },
    { rtDATE,        sizeof(timeval), "DATE" },
    { rtTIME_OF_DAY, sizeof(timeval), "TIME_OF_DAY" },
    { rtASB_TIME,    sizeof(timeval), "ABSTIME" },
    { rtUNDEFINED,   0,               "UNDEFINED" }
};


ClassDescription_t xdb::ClassDescriptionTable[] = {
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

