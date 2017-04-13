#if !defined EXCHANGE_EGSA_CYCLE_HPP
#define EXCHANGE_EGSA_CYCLE_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <iostream>
#include <vector>
#include <string>

// Служебные заголовочные файлы сторонних утилит

// Служебные файлы RTDBUS
#include "exchange_egsa_init.hpp"
#include "exchange_config_egsa.hpp"
#include "exchange_egsa_site.hpp"

#if 0
// ==============================================================================
// Cyclic operation entity
// name of the cyclic operation
// family of the cyle : normal or high CPU loading
// name of the associated high CPU loading cycle (only for normal cycle)
// period of the cycle in milliseconds
// request associated to be transmitted to the acquisition site
// identifiers of the concerned acquisition sites
// state indicator of the high CPU loading request (TRUE -> request treated,
//                                                  FALSE-> request to treat)
typedef struct {
	char            s_CycleName[EGA_EGA_D_LGCYCLENAME+1];
	cycle_family_t  i_CycleFamily;
    // Название ассоциированного HCPU цикла
	char            s_AssociatedHCpuCycle[EGA_EGA_D_LGCYCLENAME+1];
    // Цикличность в секундах
	int             i_CyclePeriod;
    // Ассоциированный запрос
	ech_t_ReqId     e_LinkedRequest;
	ega_ega_t_Sites r_AcqSites;
	bool            ab_HCpuLoadReqState[ECH_D_MAXNBSITES];
	Timer*          r_CycleTimer;
} ega_ega_odm_t_CycleEntity; // GEV =/= ega_ega_odm_t_CycleEntry

typedef struct {
  // Тег сайта
  char site[TAG_NAME_MAXLEN + 1];
  size_t id;
  // State indicator of the high CPU loading request: 1-> request treated, 0-> request to treat
  int HCpuLoadReqState; // TODO: определить допустимые значения
} acq_site_state_t;
#endif

//class Timer;
//class CycleTrigger;

class Cycle {

  protected:
    // name of the cyclic operation
	char            m_CycleName[EGA_EGA_D_LGCYCLENAME+1];
    // family of the cyle : normal or high CPU loading
	cycle_family_t  m_CycleFamily;
    // Цикличность в секундах
	int             m_CyclePeriod;
    // Идентификатор
    cycle_id_t      m_CycleId;
    // identifiers of the concerned acquisition sites
	AcqSiteList     m_SiteList;
	//Timer*          m_CycleTimer;
    //CycleTrigger*   m_CycleTrigger;
    //
  public:
    ~Cycle();

    // Род цикла
    cycle_family_t family() { return m_CycleFamily; };
    // Название цикла
    const char* name() { return m_CycleName; };
    int period() { return m_CyclePeriod; };
    int id() { return m_CycleId; };
#if 0
    // Активировать цикл, взведя его таймер
    int activate(int);
    // Деактивировать цикл, остановив его таймер
    int deactivate();
#endif
    // Проверить, зарегистрирована ли указанная СС в данном Цикле
    //bool exist_for_SA(const std::string&);
    // Зарегистрировать указанную СС в этом Цикле
    int link(AcqSiteEntry*);
    // Список сайтов
    AcqSiteList& sites() { return m_SiteList; };

    // Конструктор экземпляра "Цикл"
    // Все параметры проверяются на корректность при чтении из конфигурационного файла
    // _name    : название Цикла, читается из конфигурации
    // _period  : интервалы между активациями
    // _id      : уникальный числовой идентификатор Цикла
    // _family  : тип Цикла
    Cycle(const char* _name, int _period, cycle_id_t _id, cycle_family_t _family)
      : m_CycleFamily(_family),
        m_CyclePeriod(_period),
        m_CycleId(_id)
        /*m_CycleTimer(NULL),
        m_CycleTrigger(NULL)*/
    {
      strncpy(m_CycleName, _name, EGA_EGA_D_LGCYCLENAME);
      m_CycleName[EGA_EGA_D_LGCYCLENAME] = '\0';

      //m_SiteList.clear();
    };

    void dump() {
      std::cout << "Cycle name:" << m_CycleName << " family:" << (int)m_CycleFamily
                << " period:" << (int)m_CyclePeriod << " id:" << (int)m_CycleId << " ";
      if (m_SiteList.size()) {
        std::cout << "sites: " << m_SiteList.size() << " [";
        for(size_t i=0; i < m_SiteList.size(); i++) {
          std::cout << " " << m_SiteList[i]->name();
        }
        std::cout << " ]";
      }
      std::cout << std::endl;
    };

  private:
};

class CycleList
{
  public:
    CycleList();
   ~CycleList();
    size_t insert(Cycle*);
    size_t size() { return m_Cycles.size(); };
    Cycle* operator[](size_t);
    Cycle* operator[](const std::string&);

  private:
    std::vector<Cycle*> m_Cycles;
};

#endif

