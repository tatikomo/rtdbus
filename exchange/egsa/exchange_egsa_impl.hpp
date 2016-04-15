#if !defined EXCHANGE_EGSA_IMPL_HPP
#define EXCHANGE_EGSA_IMPL_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
//#include <assert.h>
// Служебные заголовочные файлы сторонних утилит
// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_config_egsa.hpp"
// Внешняя память, под управлением EGSA
#include "exchange_smad_ext.hpp"
#include "exchange_egsa_init.hpp"
#include "exchange_egsa_sa.hpp"

class EGSA {
  public:
    typedef std::map<std::string, SystemAcquisition*> system_acquisition_list_t;

    EGSA();
   ~EGSA();

    int run();

  private:
    // Экземпляр ExternalSMAD для хранения конфигурации и данных EGSA
    ExternalSMAD *m_ext_smad;
    EgsaConfig   *m_config;
    system_acquisition_list_t m_sa_list;
    // Хранилище изменивших своё значение параметров, используется для всех СС
    sa_parameters_t m_list;

    // General data
    //static ega_ega_odm_t_GeneralData ega_ega_odm_r_GeneralData;

    // Acquisition Sites Table
    //static ega_ega_odm_t_AcqSiteEntry ega_ega_odm_ar_AcqSites[ECH_D_MAXNBSITES];
	
    // Cyclic Operations Table
    //static ega_ega_odm_t_CycleEntity ega_ega_odm_ar_Cycles[NBCYCLES];

    // Request Table
    static ega_ega_odm_t_RequestEntry m_requests_table[/*NBREQUESTS*/]; // ega_ega_odm_ar_Requests
    
    // Инициализация, создание/подключение к внутренней SMAD
    int init();
    // Изменение состояния подключенных систем сбора и отключение от их внутренней SMAD 
    int detach();
};

#endif

