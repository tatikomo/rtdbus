#if !defined EXCHANGE_SMED_HPP
#define EXCHANGE_SMED_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <time.h>

// Служебные заголовочные файлы сторонних утилит
#include "sqlite3.h"

// Служебные файлы RTDBUS

// структура записи SMED для Телеметрии
typedef struct {
  gof_t_UniversalName    s_Name;    // name of the exchanged data
  int   i_LED;                      // identifier of the exchanged data
  int   h_RecId;                    // SMED record identifier 
  int   o_Categ;                    // category (Primary/Secondary/Exploitation)
  bool  b_AlarmIndic;               // Alarm indication
  int   i_HistoIndic;               // Historical indication
  float f_ConvertCoeff;             // Conversion coefficient 
  struct timeval d_LastUpdDate;
  //esg_esg_odm_t_SubTypeElem   ar_SubType[ECH_D_MAX_NBSUBTYPES]; // . Sub types structures 
} esg_esg_odm_t_ExchInfoElem;

class SMED
{
  public:

    SMED(const char*);
   ~SMED();

    smad_connection_state_t state() { return m_state; };
    smad_connection_state_t connect();
    // Найти и вернуть информацию по параметру с идентификатором LED для указанного Сайта
    int get_info(const gof_t_UniversalName, const int, esg_esg_odm_t_ExchInfoElem*);

  private:
    DISALLOW_COPY_AND_ASSIGN(SMED);
    sqlite3* m_db;
    smad_connection_state_t  m_state;
    char*    m_db_err;
    char*    m_snapshot_filename;
};

#endif

