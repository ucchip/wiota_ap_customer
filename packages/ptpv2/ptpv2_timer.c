/* timer.c */

#include "ptpv2_main.h"

#define TIMER_INTERVAL 1000
unsigned int elapsed;

#if 0

void catch_alarm(int sig)
{
  elapsed++;
  DBGV("catch_alarm: elapsed %d\n", elapsed);
}

void initTimer(void)
{
  struct itimerval itimer;

  DBG("initTimer\n");

  elapsed = 0;

  itimer.it_value.tv_sec = itimer.it_interval.tv_sec = 0;
  itimer.it_value.tv_usec = itimer.it_interval.tv_usec = TIMER_INTERVAL;

  elapsed++;

  DBGV("catch_alarm: elapsed %d\n", elapsed);

  setitimer(ITIMER_REAL, &itimer, 0);


}

#else

static rt_timer_t g_1ms_timer = RT_NULL;

void catch_alarm(void *parameter)
{
    elapsed++;
    //  DBGV("catch_alarm: elapsed %d\n", elapsed);
}

void initTimer(void)
{
    rt_tick_t tickVal = rt_tick_from_millisecond(1);

    if (g_1ms_timer == RT_NULL)
    {
        g_1ms_timer = rt_timer_create("my_1ms", catch_alarm, RT_NULL, tickVal, RT_TIMER_FLAG_PERIODIC);
    }
    else
    {
        rt_timer_stop(g_1ms_timer);
        rt_timer_control(g_1ms_timer, RT_TIMER_CTRL_SET_TIME, (void *)&tickVal);
    }
    rt_timer_start(g_1ms_timer);
}

#endif

void timerUpdate(IntervalTimer *itimer)
{

    int i, delta;

    delta = elapsed;
    elapsed = 0;

    if (delta <= 0)
        return;

    for (i = 0; i < TIMER_ARRAY_SIZE; ++i)
    {
        if ((itimer[i].interval) > 0 && ((itimer[i].left) -= delta) <= 0)
        {
            itimer[i].left = itimer[i].interval;
            itimer[i].expire = TRUE;
            DBGV("timerUpdate: timer %u expired\n", i);
        }
    }
}

void timerStop(UInteger16 index, IntervalTimer *itimer)
{
    if (index >= TIMER_ARRAY_SIZE)
        return;

    itimer[index].interval = 0;
}

void timerStart(UInteger16 index, float interval, IntervalTimer *itimer)
{
    if (index >= TIMER_ARRAY_SIZE)
        return;

    itimer[index].expire = FALSE;
    itimer[index].left = interval * 1000; //Factor 1000 used because resolution time is ms for the variable "elasped"
    itimer[index].interval = itimer[index].left;

    DBGV("timerStart: set timer %d to %d\n", index, interval);
}

Boolean timerExpired(UInteger16 index, IntervalTimer *itimer)
{
    timerUpdate(itimer);

    if (index >= TIMER_ARRAY_SIZE)
        return FALSE;

    if (!itimer[index].expire)
        return FALSE;

    itimer[index].expire = FALSE;

    return TRUE;
}
