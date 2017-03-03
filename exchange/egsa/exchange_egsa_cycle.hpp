#if !defined EXCHANGE_EGSA_CYCLE_HPP
#define EXCHANGE_EGSA_CYCLE_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <iostream>

#include "exchange_egsa_init.hpp"

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
#endif

typedef struct {
  // Тег сайта
  char site[TAG_NAME_MAXLEN + 1];
  // State indicator of the high CPU loading request: 1-> request treated, 0-> request to treat
  int HCpuLoadReqState; // TODO: определить допустимые значения
} acq_site_state_t;

//class Timer;
//class CycleTrigger;

class Cycle {
  public:
    ~Cycle();

    // Род цикла
    cycle_family_t family() { return m_CycleFamily; };
    // Название цикла
    const char* name() { return m_CycleName; };
    int period() { return m_CyclePeriod; };
#if 0
    // Активировать цикл, взведя его таймер
    int activate(int);
    // Деактивировать цикл, остановив его таймер
    int deactivate();
#endif
    // Проверить, зарегистрирована ли указанная СС в данном Цикле
    bool exist_for_SA(const std::string&);
    // Зарегистрировать указанную СС в этом Цикле
    int register_SA(const std::string&);
    // Список сайтов
    std::vector<acq_site_state_t>& sites() { return m_AcqSites; };

    Cycle(const char* _name, int _period, ech_t_ReqId _linked_req, cycle_family_t _family)
      : m_CycleFamily(_family),
        m_CyclePeriod(_period),
        m_LinkedRequest(_linked_req) /*,
        m_CycleTimer(NULL),
        m_CycleTrigger(NULL)*/
        
    {
      strncpy(m_CycleName, _name, EGA_EGA_D_LGCYCLENAME);
      m_CycleName[EGA_EGA_D_LGCYCLENAME] = '\0';

      m_AcqSites.clear();
    };

    void dump() {
      std::cout << "Cycle name:" << m_CycleName << " family:" << (int)m_CycleFamily
                << " period:" << (int)m_CyclePeriod << " linked_req:" << (int)m_LinkedRequest
                << std::endl;
    };

  protected:
    // name of the cyclic operation
	char            m_CycleName[EGA_EGA_D_LGCYCLENAME+1];
    // family of the cyle : normal or high CPU loading
	cycle_family_t  m_CycleFamily;
    // Цикличность в секундах
	int             m_CyclePeriod;
    // Ассоциированный запрос
    // request associated to be transmitted to the acquisition site
	ech_t_ReqId     m_LinkedRequest;
    // identifiers of the concerned acquisition sites
	std::vector<acq_site_state_t> m_AcqSites;
	//Timer*          m_CycleTimer;
    //CycleTrigger*   m_CycleTrigger;

  private:

};

#endif

