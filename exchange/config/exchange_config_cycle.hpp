#ifndef EXCHANGE_CONFIG_CYCLE_HPP
#define EXCHANGE_CONFIG_CYCLE_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

class AcqSiteEntry;

// Общесистемные заголовочные файлы
#include <iostream>
#include <vector>
#include <string>

// Служебные заголовочные файлы сторонних утилит

// Служебные файлы RTDBUS
#include "exchange_config_egsa.hpp"

#if 0
#include "tool_timer.hpp"
// ===================================================================================================
class CycleTrigger : public TimerTimeoutHandler
{
  public:
    CycleTrigger(const std::string&);
    CycleTrigger(const std::string&, int);
   ~CycleTrigger() {};

    void handlerFunction( void );

  private:
    std::string m_cycle_name;
    int m_fd;
};
#endif

// ===================================================================================================
class Cycle {
  public:
    // Конструктор экземпляра "Цикл"
    Cycle(const char*, int, cycle_id_t, ech_t_ReqId, cycle_family_t);
   ~Cycle();

    // Род цикла
    cycle_family_t family() { return m_CycleFamily; }
    // Название цикла
    const char* name() { return m_CycleName; }
    int period() { return m_CyclePeriod; }
    cycle_id_t id() { return m_CycleId; }
    ech_t_ReqId req_id() { return m_RequestId; }
    // Зарегистрировать указанную СС в этом Цикле
    int link(AcqSiteEntry*);
    // Список сайтов
    const std::vector<AcqSiteEntry*>& sites() { return m_SiteList; }
    void dump();

  protected:
    // name of the cyclic operation
	char            m_CycleName[EGA_EGA_D_LGCYCLENAME+1];
    // family of the cyle : normal or high CPU loading
	cycle_family_t  m_CycleFamily;
    // Цикличность в секундах
	int             m_CyclePeriod;
    // Идентификатор Цикла
    cycle_id_t      m_CycleId;
    // Идентификатор Запроса, связанного с Циклом
    ech_t_ReqId     m_RequestId;
    // identifiers of the concerned acquisition sites
	std::vector<AcqSiteEntry*> m_SiteList;
	//Timer*          m_CycleTimer;
    //CycleTrigger*   m_CycleTrigger;
 
  private:
    DISALLOW_COPY_AND_ASSIGN(Cycle);
};

// ===================================================================================================
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
    DISALLOW_COPY_AND_ASSIGN(CycleList);
    std::vector<Cycle*> m_Cycles;
};

#endif

