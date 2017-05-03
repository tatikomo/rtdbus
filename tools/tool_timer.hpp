#ifndef TOOL_TIMER_HELPER_H_
#define TOOL_TIMER_HELPER_H_

#include "config.h"

#include <string>
#include <signal.h>
#include <time.h>
//#include <pthread.h>

#define CLOCKID CLOCK_REALTIME
//#define SIG SIGUSR1
#define SIG SIGRTMIN

typedef void (*TimerHandler)(sigval_t signum);

class TimerTimeoutHandler
{
    public:
        virtual ~TimerTimeoutHandler() {};
        virtual void handlerFunction( void ) = 0;
};

class Timer
{
    public:
        Timer( TimerTimeoutHandler * timeHandler );
        ~Timer();

        void setDuration(long int seconds);
        void start();
        void restart();
        void timeout();
        void stop();
        void callbackWrapper( void );
        static void timeOutHandler( sigval_t This  );

    private:
        DISALLOW_COPY_AND_ASSIGN(Timer);
        void createTimer(timer_t *timerid, TimerHandler handler_cb);
        void startTimer(timer_t timerid, int startTimeout, int cyclicTimeout);
        void stopTimer(timer_t timerid);

        long int             m_Duration;
        TimerTimeoutHandler *m_timeOutHandlerImp;
        timer_t              m_timerid;
};

class TimeTimeoutHandlerImp : public TimerTimeoutHandler
{
    public:
        TimeTimeoutHandlerImp(const std::string&);
        ~TimeTimeoutHandlerImp(){}

        void handlerFunction( void );
    private:
        std::string m_message;
};

#endif /* TIMERHELPER_H_ */

