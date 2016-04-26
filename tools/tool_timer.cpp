//
// gcc --std=c++11 -g timer.cpp -o timer -lrt
//
#include <string>
#include <iostream>
#include <unistd.h>

#include "tool_timer.hpp"

using namespace std;

Timer::Timer( TimerTimeoutHandler * timeHandler )
{
    m_timeOutHandlerImp = timeHandler;
    m_Duration = 0;

    TimerHandler handler_cb = &timeOutHandler;
    createTimer( &m_timerid, handler_cb );
}

Timer::~Timer()
{
    stopTimer( m_timerid );
    timer_delete( m_timerid );
}

void Timer::setDuration(long int seconds)
{
    m_Duration = seconds;
}

void Timer::start()
{
    startTimer(m_timerid, m_Duration, m_Duration);
}

void Timer::restart()
{
    stopTimer(m_timerid);
    startTimer(m_timerid, m_Duration, 0);
}

void Timer::stop()
{
    stopTimer(m_timerid);
}

void Timer::createTimer(timer_t *timerid, TimerHandler handler_cb)
{
    sigevent sev;
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    sched_param parm;

    parm.sched_priority = 255;
    pthread_attr_setschedparam(&attr, &parm);

    sev.sigev_notify_attributes = &attr;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = handler_cb;
    sev.sigev_signo = SIG;
    sev.sigev_value.sival_ptr = this;

    timer_create(CLOCKID, &sev, timerid);
}

void Timer::startTimer(timer_t timerid, int startTimeout, int cyclicTimeout)
{
    itimerspec its;

    /* Start the timer */
    its.it_value.tv_sec = startTimeout;
    its.it_value.tv_nsec = 0;

    /* for cyclic timer */
    its.it_interval.tv_sec = cyclicTimeout;
    its.it_interval.tv_nsec = 0;

    timer_settime(timerid, 0, &its, NULL);
}

void Timer::stopTimer(timer_t timerid)
{
    itimerspec its;

    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    timer_settime(timerid, 0, &its, NULL);
}

void Timer::timeOutHandler( sigval_t This )
{
    Timer * timer = (Timer*) This.sival_ptr;
    timer->callbackWrapper();
}

void Timer::callbackWrapper( void )
{
    m_timeOutHandlerImp->handlerFunction();
//    stopTimer( m_timerid );
}

TimeTimeoutHandlerImp::TimeTimeoutHandlerImp(const std::string& msg)
  : m_message(msg)
{
}

void TimeTimeoutHandlerImp::handlerFunction( void )
{
    std::cout << "time handler invoked: " << m_message << std::endl;
}

#if 0
int main()
{
    TimeTimeoutHandlerImp * timerImp1 = new TimeTimeoutHandlerImp("one");
    TimeTimeoutHandlerImp * timerImp2 = new TimeTimeoutHandlerImp("two");
    Timer * timer1 = new Timer( timerImp1 );
    Timer * timer2 = new Timer( timerImp2 );

    timer1->setDuration( 5 );
    timer2->setDuration( 7 );

    timer1->start();
    timer2->start();
    
    for (int i = 1; i <= 100; i++) {
      std::cout << (unsigned int)i << " "; // std::endl;
      std::cout << std::flush;
      sleep( 1 );

      if (!(i % 10)) {
        std::cout << "restart timer1" << std::endl;
        timer1->restart();
      }
      
      if (!(i % 25)) {
        std::cout << "restart timer2" << std::endl;
        timer2->restart();
      }
    }

    return EXIT_SUCCESS;
}
#endif

