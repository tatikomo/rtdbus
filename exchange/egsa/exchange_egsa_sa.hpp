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
#include "exchange_smad_int.hpp"

// -----------------------------------------------------------------------------------
class SystemAcquisition
{
  public:
    SystemAcquisition(sa_object_level_t, gof_t_SacNature, const char*);
   ~SystemAcquisition();

    // Получить состояние СС
    sa_state_t state();
    // Выполнить указанное действие с системой сбора
    int control(int /* тип сообщения */);
    // Прочитать изменившиеся данные
    int pop(sa_parameters_t&);
    // Передать в СС указанные значения
    int push(sa_parameters_t&);

  private:
    DISALLOW_COPY_AND_ASSIGN(SystemAcquisition);
    // Код СС
    char* m_name;
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
};

// -----------------------------------------------------------------------------------
#endif

