#if !defined EXCHANGE_EGSA_SA_HPP
#define EXCHANGE_EGSA_SA_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "zmq.hpp"

// Служебные файлы RTDBUS
#include "tool_timer.hpp"
#include "exchange_config.hpp"
#include "exchange_smad_int.hpp"
#include "exchange_egsa_cycle.hpp"

class EGSA;

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
    SystemAcquisition(EGSA* egsa, sa_object_level_t, gof_t_SacNature, const std::string&);
   ~SystemAcquisition();

    // Получить состояние СС
    sa_state_t state();

    // Послать сообщение указанного типа
    int send(int);

  private:
    DISALLOW_COPY_AND_ASSIGN(SystemAcquisition);
    EGSA* m_egsa; 
    // Код СС
    std::string m_name;
    // Иерархия СС
    sa_object_level_t m_level;
    // Тип СС
    gof_t_SacNature   m_nature;
    // Ссылка на внутреннюю SMAD
    InternalSMAD     *m_smad;
    // Состояние СС
    sa_state_t        m_state;
    // Состояние подключения к внутренней SMAD
    smad_connection_state_t m_smad_state;

    // Набор таймеров внутренних событий
    Timer *m_timer_CONNECT;
    Timer *m_timer_RESPONSE;
    Timer *m_timer_INIT;
    Timer *m_timer_ENDALLINIT;
    Timer *m_timer_GENCONTROL;
    Timer *m_timer_DIFFUSION;
    Timer *m_timer_TELEREGULATION;

    // Найти циклы, в которых участвует данная система сбора
    void look_my_cycles(const std::vector<Cycle*>&);
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

