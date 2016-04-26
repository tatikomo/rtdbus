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

class Cycle;
class Request;

class EGSA {
  public:
    typedef std::map<std::string, SystemAcquisition*> system_acquisition_list_t;

    EGSA();
   ~EGSA();

    // Инициализация, создание/подключение к внутренней SMAD
    int init();
    // Основной рабочий цикл
    int run();
    // Ввести в оборот новый Цикл сбора
    int push_cycle(Cycle*);
    // Активировать циклы
    int activate_cycles();
    // Деактивировать циклы
    int deactivate_cycles();
    // Тестовая функция ожидания срабатывания таймеров в течении заданного времени
    int wait(int);

  private:
    // Экземпляр ExternalSMAD для хранения конфигурации и данных EGSA
    ExternalSMAD *m_ext_smad;
    EgsaConfig   *m_config;
    system_acquisition_list_t m_sa_list;
    // Хранилище изменивших своё значение параметров, используется для всех СС
    sa_parameters_t m_list;
    // Сокет получения данных
    int m_socket;

    // General data
    //static ega_ega_odm_t_GeneralData ega_ega_odm_r_GeneralData;

    // Acquisition Sites Table
    //static ega_ega_odm_t_AcqSiteEntry ega_ega_odm_ar_AcqSites[ECH_D_MAXNBSITES];
	
    // Cyclic Operations Table
    //static ega_ega_odm_t_CycleEntity ega_ega_odm_ar_Cycles[NBCYCLES];
    std::vector<Cycle*> ega_ega_odm_ar_Cycles;

    // Request Table - перенёс в exchange_egsa_init.cpp
    //static ega_ega_odm_t_RequestEntry m_requests_table[/*NBREQUESTS*/]; // ega_ega_odm_ar_Requests
    std::vector<Request*> ega_ega_odm_ar_Requests;
    
    // Подключиться к SMAD систем сбора
    int attach_to_sites_smad();
    // Изменение состояния подключенных систем сбора и отключение от их внутренней SMAD 
    int detach();
    // Функция срабатывания при наступлении времени очередного таймера
    static int trigger();
};

#endif

