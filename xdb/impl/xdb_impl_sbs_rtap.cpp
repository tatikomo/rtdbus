#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include <assert.h>

#include "xdb_impl_sbs_rtap.hpp"

using namespace xdb;

// Таблица тегов Точки, при изменении которых запись этой точки в группе
// подписки активируется (точки считается изменившейся).
// Атрибуты точки типа XDBPoint и связанные с ней паспортные характеристики
static const read_attribs_on_sbs_event_t g_attribs_on_event[] = {
/*  0*/ {GOF_D_BDR_OBJCLASS_TS,  5, {
                                    RTDB_ATT_IDX_VAL,
                                    RTDB_ATT_IDX_VALID,
                                    RTDB_ATT_IDX_TSSYNTHETICAL,
                                    RTDB_ATT_IDX_FLGREMOTECMD,
                                    RTDB_ATT_IDX_ALDEST}},           /* Телесигнализация */
/*  1*/ {GOF_D_BDR_OBJCLASS_TM,  4, {
                                    RTDB_ATT_IDX_VAL,
                                    RTDB_ATT_IDX_VALID,
                                    RTDB_ATT_IDX_ALARMSYNTH,
                                    RTDB_ATT_IDX_ALDEST}},           /* Телеизмерение */
/*  2*/ {GOF_D_BDR_OBJCLASS_TR,  0, {}},           /* Телерегулировка */
/*  3*/ {GOF_D_BDR_OBJCLASS_TSA, 0, {}},
/*  4*/ {GOF_D_BDR_OBJCLASS_TSC, 0, {}},
/*  5*/ {GOF_D_BDR_OBJCLASS_TC,  0, {}},           /* Телеуправление */
/*  6*/ {GOF_D_BDR_OBJCLASS_AL,  8, {
                                    RTDB_ATT_IDX_VAL,
                                    RTDB_ATT_IDX_VALID,
                                    RTDB_ATT_IDX_ALDEST,
                                    RTDB_ATT_IDX_INHIBLOCAL,
                                    RTDB_ATT_IDX_ALARMBEGIN,
                                    RTDB_ATT_IDX_ALARMBEGINACK,
                                    RTDB_ATT_IDX_ALARMENDACK,
                                    RTDB_ATT_IDX_ALARMSYNTH
                                    }},           /* Полученный аварийный сигнал */
/*  7*/ {GOF_D_BDR_OBJCLASS_ICS, 0, {}},           /* Двоичные расчетные данные */
/*  8*/ {GOF_D_BDR_OBJCLASS_ICM, 0, {}},           /* Аналоговые расчетные данные */
/*  9*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* "REF" - Не используется */
/* 10*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* -------------------------------------  Network objects       */
/* 11*/ {GOF_D_BDR_OBJCLASS_PIPE,     0, {}},      /* Труба */
/* 12*/ {GOF_D_BDR_OBJCLASS_BORDER,   0, {}},      /* Граница */
/* 13*/ {GOF_D_BDR_OBJCLASS_CONSUMER, 0, {}},      /* Потребитель */
/* 14*/ {GOF_D_BDR_OBJCLASS_CORRIDOR, 0, {}},      /* Коридор */
/* 15*/ {GOF_D_BDR_OBJCLASS_PIPELINE, 0, {}},      /* Трубопровод */
/* -------------------------------------  Equipments objects    */
/* NB: update GOF_D_BDR_OBJCLASS_EQTLAST when adding an Equipments objects */
/* 16*/ {GOF_D_BDR_OBJCLASS_TL,     0, {}},      /* Линейный участок */
/* 17*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 18*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 19*/ {GOF_D_BDR_OBJCLASS_VA,     5, {
                                    RTDB_ATT_IDX_VAL,
                                    RTDB_ATT_IDX_VALID,
                                    RTDB_ATT_IDX_TSSYNTHETICAL,
                                    RTDB_ATT_IDX_FLGREMOTECMD,
                                    RTDB_ATT_IDX_ALDEST}},      /* Кран */
/* 20*/ {GOF_D_BDR_OBJCLASS_SC,     5, {
                                    RTDB_ATT_IDX_VAL,
                                    RTDB_ATT_IDX_VALID,
                                    RTDB_ATT_IDX_TSSYNTHETICAL,
                                    RTDB_ATT_IDX_FLGREMOTECMD,
                                    RTDB_ATT_IDX_ALDEST}},      /* Компрессорная станция */
/* 21*/ {GOF_D_BDR_OBJCLASS_ATC,    5, {
                                    RTDB_ATT_IDX_VAL,
                                    RTDB_ATT_IDX_VALID,
                                    RTDB_ATT_IDX_TSSYNTHETICAL,
                                    RTDB_ATT_IDX_FLGREMOTECMD,
                                    RTDB_ATT_IDX_ALDEST}},      /* Цех */
/* 22*/ {GOF_D_BDR_OBJCLASS_GRC,    6, {
                                    RTDB_ATT_IDX_VAL,
                                    RTDB_ATT_IDX_VALID,
                                    RTDB_ATT_IDX_TSSYNTHETICAL,
                                    RTDB_ATT_IDX_FLGREMOTECMD,
                                    RTDB_ATT_IDX_ALDEST,
                                    RTDB_ATT_IDX_CONFREMOTECMD}},      /* Агрегат */
/* 23*/ {GOF_D_BDR_OBJCLASS_SV,     5, {
                                    RTDB_ATT_IDX_VAL,
                                    RTDB_ATT_IDX_VALID,
                                    RTDB_ATT_IDX_TSSYNTHETICAL,
                                    RTDB_ATT_IDX_FLGREMOTECMD,
                                    RTDB_ATT_IDX_ALDEST}},      /* Крановая площадка */
/* 24*/ {GOF_D_BDR_OBJCLASS_SDG,    0, {}},      /* Газораспределительная станция */
/* 25*/ {GOF_D_BDR_OBJCLASS_RGA,    0, {}},      /* Станция очистного поршня */
/* 26*/ {GOF_D_BDR_OBJCLASS_SSDG,   0, {}},      /* Выход ГРС */
/* 27*/ {GOF_D_BDR_OBJCLASS_BRG,    0, {}},      /* Газораспределительный блок */
/* 28*/ {GOF_D_BDR_OBJCLASS_SCP,    5, {
                                    RTDB_ATT_IDX_VAL,
                                    RTDB_ATT_IDX_VALID,
                                    RTDB_ATT_IDX_TSSYNTHETICAL,
                                    RTDB_ATT_IDX_FLGREMOTECMD,
                                    RTDB_ATT_IDX_ALDEST}},      /* Пункт замера расхода */
/* 29*/ {GOF_D_BDR_OBJCLASS_STG,    0, {}},      /* Установка подготовки газа */
/* 30*/ {GOF_D_BDR_OBJCLASS_DIR,    1, {
                                    RTDB_ATT_IDX_STATUS}},      /* Предприятие */
/* 31*/ {GOF_D_BDR_OBJCLASS_DIPL,   4, {
                                    RTDB_ATT_IDX_INHIB,
                                    RTDB_ATT_IDX_INHIBLOCAL,
                                    RTDB_ATT_IDX_ALINHIB,
                                    RTDB_ATT_IDX_CONFREMOTECMD
                                    }},      /* ЛПУ */
/* 32*/ {GOF_D_BDR_OBJCLASS_METLINE,0, {}},      /* Газоизмерительная нитка */
/* 33*/ {GOF_D_BDR_OBJCLASS_ESDG,   0, {}},      /* Вход ГРС */
/* 34*/ {GOF_D_BDR_OBJCLASS_SVLINE, 0, {}},      /* Точка КП на газопроводе */
/* 35*/ {GOF_D_BDR_OBJCLASS_SCPLINE,0, {}},      /* Нитка ПЗРГ */
/* 36*/ {GOF_D_BDR_OBJCLASS_TLLINE, 0, {}},      /* Нитка ЛУ */
/* 37*/ {GOF_D_BDR_OBJCLASS_INVT,   0, {}},      /* Инвертор */
/* 38*/ {GOF_D_BDR_OBJCLASS_AUX1,   5, {
                                    RTDB_ATT_IDX_VAL,
                                    RTDB_ATT_IDX_VALID,
                                    RTDB_ATT_IDX_TSSYNTHETICAL,
                                    RTDB_ATT_IDX_FLGREMOTECMD,
                                    RTDB_ATT_IDX_ALDEST}},      /* Вспом объект I-го уровня */
/* 39*/ {GOF_D_BDR_OBJCLASS_AUX2,   5, {
                                    RTDB_ATT_IDX_VAL,
                                    RTDB_ATT_IDX_VALID,
                                    RTDB_ATT_IDX_TSSYNTHETICAL,
                                    RTDB_ATT_IDX_FLGREMOTECMD,
                                    RTDB_ATT_IDX_ALDEST}},      /* Вспом объект II-го уровня */
/* 40*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 41*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 42*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 43*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 44*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* -------------------------------------  Fixed objects    */
/* 45*/ {GOF_D_BDR_OBJCLASS_SITE,   0, {}},       /* Корневая точка с информацией о локальном объекте */
/* 46*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 47*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 48*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 49*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* -------------------------------------  System objects    */
/* NB: update GOF_D_BDR_OBJCLASS_SYSTLAST when adding an System objects */
/* 50*/ {GOF_D_BDR_OBJCLASS_SA,     0, {}},         /* Система сбора данных */
/* -------------------------------------  Alarms objects    */
/* 51*/ {GOF_D_BDR_OBJCLASS_GENAL,  0, {}},         /*  */
/* 52*/ {GOF_D_BDR_OBJCLASS_USERAL, 0, {}},         /*  */
/* 53*/ {GOF_D_BDR_OBJCLASS_HISTSET,0, {}},         /*  */

/* 54*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 55*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 56*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 57*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 58*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 59*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */

/* 60*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 61*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 62*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 63*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 64*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 65*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 66*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 67*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /* Missing */
/* 68*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 69*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 70*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 71*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 72*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 73*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 74*/ {GOF_D_BDR_OBJCLASS_FIXEDPOINT, 0, {}}, /*  */
/* 75*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 76*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 77*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 78*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 79*/ {GOF_D_BDR_OBJCLASS_FIXEDPOINT, 0, {}}, /*  */
/* 80*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 81*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 82*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 83*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 84*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 85*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 86*/ {GOF_D_BDR_OBJCLASS_UNUSED, 0, {}}, /*  */
/* 87*/ {GOF_D_BDR_OBJCLASS_FIXEDPOINT, 0, {}}  /*  */
    };

AttributesHolder::AttributesHolder()
{
};

AttributesHolder::~AttributesHolder()
{
};

const read_attribs_on_sbs_event_t& AttributesHolder::info(int objclass)
{
  assert(GOF_D_BDR_OBJCLASS_TS <= objclass);
  assert(objclass <= GOF_D_BDR_OBJCLASS_LASTUSED);

  return g_attribs_on_event[objclass];
};

