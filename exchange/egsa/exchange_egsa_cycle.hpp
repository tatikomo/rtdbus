#ifndef EXCHANGE_EGSA_CYCLE_HPP
#define EXCHANGE_EGSA_CYCLE_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

class AcqSiteEntry;
class AcqSiteList;

// Общесистемные заголовочные файлы
#include <iostream>
#include <vector>
#include <string>

// Служебные заголовочные файлы сторонних утилит

// Служебные файлы RTDBUS
#include "tool_timer.hpp"
#include "exchange_egsa_init.hpp"
#include "exchange_config_egsa.hpp"


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

// ===================================================================================================
class Cycle {
  public:
    // Конструктор экземпляра "Цикл"
    Cycle(const char*, int, cycle_id_t, cycle_family_t);
    ~Cycle();

    // Род цикла
    cycle_family_t family() { return m_CycleFamily; }
    // Название цикла
    const char* name() { return m_CycleName; }
    int period() { return m_CyclePeriod; }
    int id() { return m_CycleId; }
    // Зарегистрировать указанную СС в этом Цикле
    int link(AcqSiteEntry*);
    // Список сайтов
    AcqSiteList* sites() { return m_SiteList; }
    void dump();

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
	AcqSiteList*     m_SiteList;
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

