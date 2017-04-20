#ifndef EXCHANGE_EGSA_SITE_HPP
#define EXCHANGE_EGSA_SITE_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <memory>
#include <vector>
#include <map>

// Служебные заголовочные файлы сторонних утилит

// Служебные файлы RTDBUS
#include "exchange_config_egsa.hpp"
#include "exchange_config.hpp"

class EGSA;
class SystemAcquisition;
class Cycle;

// ==============================================================================
// Acquisition Site Entry Structure
class AcqSiteEntry {
  public:
    AcqSiteEntry(EGSA*, const egsa_config_site_item_t*);
   ~AcqSiteEntry();

    const char* name()      const { return m_IdAcqSite; }
    gof_t_SacNature nature()const { return m_NatureAcqSite; }
    bool        auto_i()    const { return m_AutomaticalInit; }
    bool        auto_gc()   const { return m_AutomaticalGenCtrl; }
    sa_state_t  state()     const { return m_FunctionalState; }
    sa_object_level_t level() const { return m_Level; }

    // Управление состоянием
    sa_state_t  change_state_to(sa_state_t);
    // Регистрация Запросов в указанном Цикле
    int push_request_for_cycle(Cycle*, ech_t_ReqId*);

    // TODO: СС и EGSA могут работать на разных хостах, в этом случае подключение EGSA к smad СС
    // не получит доступа к реальным данным от СС. Их придется EGSA туда заносить самостоятельно.
    int attach_smad();
    int detach_smad();

  private:
    DISALLOW_COPY_AND_ASSIGN(AcqSiteEntry);
    // Для доступа SystemAcquisition к объекту egsa ?
    // TODO: А нужен ли тут вообще SystemAcquisition? Может слить его с AcqSiteEntry?
    EGSA* m_egsa;

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
    //void* m_ProgList;

    // acquired data access
    //void* m_AcquiredData;
};

// ==============================================================================
// Acquisition Site Entry Structure
class AcqSiteList {
  public:
    AcqSiteList();
   ~AcqSiteList();

    int attach_to_smad(const char*);
    int detach_from_smad(const char*);
    // Отключиться от SMAD
    int detach();
    // Очистить ресурсы
    int release();
    void insert(AcqSiteEntry*);
    size_t size() const { return m_items.size(); };
    void set_egsa(EGSA*);

    // Вернуть элемент по имени Сайта
    AcqSiteEntry* operator[](const char*);
    // Вернуть элемент по имени Сайта
    AcqSiteEntry* operator[](const std::string&);
    // Вернуть элемент по индексу
    AcqSiteEntry* operator[](const std::size_t);

  private:
    DISALLOW_COPY_AND_ASSIGN(AcqSiteList);

    EGSA* m_egsa;
    struct map_cmp_str {
      // Оператор сравнения строк в m_site_map.
      // Иначе происходит арифметическое сравнение значений указателей, но не содержимого строк
      bool operator()(const std::string& a, const std::string& b) { return a.compare(b) < 0; }
    };
    // Связь между названием СС и её данными
    typedef std::map<const std::string, size_t, map_cmp_str> system_acquisition_list_t;

    std::vector<std::shared_ptr<AcqSiteEntry*>> m_items;
    // Связь между названием СС и её индексом в m_items
    system_acquisition_list_t  m_site_map;
};

#endif

