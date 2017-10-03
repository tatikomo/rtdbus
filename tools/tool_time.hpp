#if !defined GEV_TOOLS_TIME_HPP
#define GEV_TOOLS_TIME_HPP
#pragma once

#include <time.h>

/*
 * Структура для хранения отметок времени таймера MONOLITIC,
 * включающая секунды и наносекунды. Хранится в БД в виде 
 * структуры из двух 4-х байтовых целых полей.
 */
typedef struct timespec timer_mark_t;

#define BILLION 1000000000
#define MILLION  100000000

int GetTimerValue(timer_mark_t&);
int GetTimerValueByClockId(clockid_t, timer_mark_t&);
long get_seconds_to_minute_edge();
long get_realtime_seconds_to_minute_edge();
long get_monotonic_seconds_to_minute_edge();

static inline timer_mark_t getTime() {
    static struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time;
}

static inline const timer_mark_t timeAdd(timer_mark_t oldTime, timer_mark_t time) {
    if (time.tv_nsec + oldTime.tv_nsec >= BILLION)
        return (timer_mark_t){ time.tv_sec + oldTime.tv_sec + 1, time.tv_nsec + oldTime.tv_nsec - BILLION };
    else
        return (timer_mark_t){ time.tv_sec + oldTime.tv_sec, time.tv_nsec + oldTime.tv_nsec };
}

static inline const timer_mark_t timeDiff(timer_mark_t oldTime, timer_mark_t time) {
    if (time.tv_nsec < oldTime.tv_nsec)
        return (timer_mark_t){ time.tv_sec - 1 - oldTime.tv_sec, BILLION + time.tv_nsec - oldTime.tv_nsec };
    else
        return (timer_mark_t){ time.tv_sec - oldTime.tv_sec, time.tv_nsec - oldTime.tv_nsec };
}

static inline double timeSeconds(timer_mark_t time) {
    return time.tv_sec + (double)time.tv_nsec/BILLION;
}

static inline double timeSince(timer_mark_t oldTime) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return timeSeconds(timeDiff(oldTime, time));
}

// Return 1 if a > b, -1 if a < b, 0 if a == b
static inline int timeCmp(timer_mark_t a, timer_mark_t b) {
    if (a.tv_sec != b.tv_sec) {
        if (a.tv_sec > b.tv_sec)
            return 1;
        else 
            return -1;
    } else {
        if (a.tv_nsec > b.tv_nsec)
            return 1;
        else if (a.tv_nsec < b.tv_nsec)
            return -1;
        else
            return 0;
    }
}

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

