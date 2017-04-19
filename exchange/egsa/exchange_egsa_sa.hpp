#if !defined EXCHANGE_EGSA_SA_HPP
#define EXCHANGE_EGSA_SA_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит

// Служебные файлы RTDBUS
#include "exchange_config.hpp"

class EGSA;
class AcqSiteEntry;
class InternalSMAD;

// Состояние Систем Сбора:
// Конфигурационные файлы (cnf)
// Способ подключения (comm)
// Перечень параметров обмена (params)
// Состояние связи (link)
//
// UNKNOWN (Неизвестное - первоначальное значение)
//  |       cnf-,comm-params-,link-
//  v
// DISCONNECT (используется EGSA до завершения инициализации СС)
//  |       cnf+,comm-params-,link-
//  v
// UNREACH  <---------------------------+---+   (СС недоступна)
//  |       cnf+,comm+params+,link-     |   |
//  v                                   |   |
// PRE_OPERATIONAL ---------------------+   |   (СС в процессе инициализации связи)
//  |       cnf+,comm+params+,link+-        |
//  v                                       |
//  OPERATIONAL ----------------------------+   (СС в рабочем режиме)
//          cnf+,comm+params+,link+

// -----------------------------------------------------------------------------------
class SystemAcquisition
{
  public:
    // Ссылка на массив циклов - для создания у текущего экземпляра только используемых им таймеров и циклов
    // Уровень иерархии объекта - 
    // Тип СС - для учета особенностей работы каждого из известныхт типов
    // Название СС
    SystemAcquisition(EGSA*, AcqSiteEntry*);
   ~SystemAcquisition();

    // Получить состояние СС
    sa_state_t state();

    // Послать сообщение указанного типа
    int send(int);

  private:
    DISALLOW_COPY_AND_ASSIGN(SystemAcquisition);
    EGSA            *m_egsa;
    // Ссылка в EGSA на данные состояния СС, доступные локально
    AcqSiteEntry    *m_info;
    // Ссылка на внутреннюю SMAD
    InternalSMAD    *m_smad;

#if 0
    // Набор таймеров внутренних событий
    Timer *m_timer_CONNECT;
    Timer *m_timer_RESPONSE;
    Timer *m_timer_INIT;
    Timer *m_timer_ENDALLINIT;
    Timer *m_timer_GENCONTROL;
    Timer *m_timer_DIFFUSION;
    Timer *m_timer_TELEREGULATION;
#endif

    // Послать сообщение инициализации
    void init();
    void process_end_all_init(); // Сообщение о завершении инициализации 
    void process_end_init_acq(); // Запрос состояния завершения инициализации
    void process_init();         // Конец инициализации
    void process_dif_init();     // Запрос завершения инициализации после аварийного завершения
    // Отключение
    void shutdown();
    // Выполнить указанное действие с системой сбора
    int control(int /* тип сообщения */);
    // Прочитать изменившиеся данные
    int pop(sa_parameters_t&);
    // Передать в СС указанные значения
    int push(sa_parameters_t&);
    // Запросить готовность
    int ask_ENDALLINIT();
    // Проверить поступление ответа готовности
    void check_ENDALLINIT();
};

// -----------------------------------------------------------------------------------

#endif

