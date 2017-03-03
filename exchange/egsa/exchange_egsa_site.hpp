#if !defined EXCHANGE_EGSA_SITE_HPP
#define EXCHANGE_EGSA_SITE_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <vector>
#include <map>
#include <cstring>

// Служебные заголовочные файлы сторонних утилит
// Служебные файлы RTDBUS
// Конфигурация
// Внешняя память, под управлением EGSA
#include "exchange_egsa_init.hpp"
#include "exchange_config_egsa.hpp"
#include "exchange_config.hpp"

class SystemAcquisition;

// ==============================================================================
// Acquisition Site Entry Structure
class AcqSiteEntry {
  public:
    AcqSiteEntry(const std::string& name, gof_t_SacNature nature, int ai, int agc)
    : m_NatureAcqSite(nature),
      m_AutomaticalInit(ai),
      m_AutomaticalGenCtrl(agc),
      m_FunctionalState(SA_STATE_UNKNOWN),
      m_Level(LEVEL_LOCAL),
      m_InterfaceComponentActive(false),
      m_ProgList(NULL),
      m_AcquiredData(NULL)
    {
      strncpy(m_IdAcqSite, name.c_str(), TAG_NAME_MAXLEN);
    };

    AcqSiteEntry(const char* name, gof_t_SacNature nature, int ai, int agc)
    : m_NatureAcqSite(nature),
      m_AutomaticalInit(ai),
      m_AutomaticalGenCtrl(agc),
      m_FunctionalState(SA_STATE_UNKNOWN),
      m_Level(LEVEL_LOCAL),
      m_InterfaceComponentActive(false),
      m_ProgList(NULL),
      m_AcquiredData(NULL)
    {
      strncpy(m_IdAcqSite, name, TAG_NAME_MAXLEN);
    };

    AcqSiteEntry(const egsa_config_site_item_t* entry)
    : m_NatureAcqSite(entry->nature),
      m_AutomaticalInit(entry->auto_init),
      m_AutomaticalGenCtrl(entry->auto_gencontrol),
      m_FunctionalState(SA_STATE_UNKNOWN),
      m_Level(entry->level),
      m_InterfaceComponentActive(false),
      m_ProgList(NULL),
      m_AcquiredData(NULL)
    {
      strncpy(m_IdAcqSite, entry->name.c_str(), TAG_NAME_MAXLEN);
    };

   ~AcqSiteEntry() {};

    const char* name()      const { return m_IdAcqSite; };
    gof_t_SacNature nature()const { return m_NatureAcqSite; };
    bool        auto_i()    const { return m_AutomaticalInit; };
    bool        auto_gc()   const { return m_AutomaticalGenCtrl; };
    sa_state_t  state()     const { return m_FunctionalState; };
    sa_object_level_t level() const { return m_Level; };

  private:
    // identifier of the acquisition site (name)
    char m_IdAcqSite[TAG_NAME_MAXLEN + 1];

    // nature of the acquisition site (acquisition system, adjacent DIPL)
    gof_t_SacNature m_NatureAcqSite;

    // indicator of automatical initialisation
    // TRUE -> has to be performed
    // FALSE-> has not to be performed
    bool m_AutomaticalInit;

    // indicator of automatical general control
    // TRUE -> has to be performed
    // FALSE-> has not to be performed
    bool m_AutomaticalGenCtrl;

    // data base information subscription for synthetic state, inhibition state, exploitation mode
    //ega_ega_odm_t_SubscriptedInfo r_SubscrInhibState;
    //ega_ega_odm_t_SubscriptedInfo r_SubscrSynthState;
    //ega_ega_odm_t_SubscriptedInfo r_SubscrExploitMode;
    
    // functional EGSA state of the acquisition site
    // (acquisition site command only, all operations, all dispatcher requests only, waiting for no inhibition)
    sa_state_t m_FunctionalState;

    sa_object_level_t m_Level;

    // interface component state : active / stopped (TRUE / FALSE)
    bool m_InterfaceComponentActive;

    SystemAcquisition* m_sa_instance;
    // requests in progress list access
    void* m_ProgList;

    // acquired data access
    void* m_AcquiredData;
};

// ==============================================================================
// Acquisition Site Entry Structure
class AcqSiteList {
  public:
    AcqSiteList();
   ~AcqSiteList();

    int attach_to_smad(const char*);
    int detach_from_smad(const char*);
    int detach();
    void insert(AcqSiteEntry*);
    size_t size() const { return m_items.size(); };

    // Вернуть элемент по имени Сайта
    AcqSiteEntry* operator[](const char*);
    // Вернуть элемент по индексу
    AcqSiteEntry* operator[](const std::size_t);

  private:
    struct map_cmp_str {
      // Оператор сравнения строк в m_site_map.
      // Иначе происходит арифметическое сравнение значений указателей, но не содержимого строк
      bool operator()(char const *a, char const *b) { return std::strcmp(a, b) < 0; }
    };
    // Связь между названием СС и её данными
    typedef std::map<const char*, size_t, map_cmp_str> system_acquisition_list_t;

    std::vector<AcqSiteEntry*> m_items;
    // Связь между названием СС и её индексом в m_items
    system_acquisition_list_t  m_site_map;
};

#endif

