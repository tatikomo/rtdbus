#if !defined GEV_TOOLS_TIMER_HPP
#define GEV_TOOLS_TIMER_HPP
#pragma once

#include <time.h>

/*
 * Структура для хранения отметок времени таймера MONOLITIC,
 * включающая секунды и наносекунды. Хранится в БД в виде 
 * структуры из двух 4-х байтовых целых полей.
 */
typedef struct timespec timer_mark_t;

int GetTimerValue(timer_mark_t&);

#if defined __cplusplus
extern "C" {
#endif

/* mco_system_get_current_time() must be implmeneted by the
 * application in order to use the High Availability runtime.
 * You may remove this function if you are not using  the
 * HA-enabled runtime
 */
long mco_system_get_current_time(void);

#if defined __cplusplus
}
#endif

#endif

