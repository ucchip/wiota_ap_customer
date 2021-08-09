#if defined(  _FREERTOS_)
#include  "FreeRTOS.h"
#include  "queue.h"
#include  "task.h"
#elif defined(_RT_THREAD_)
#include <rtthread.h>
#else

#endif
#include "adp_time.h"

void *uc_timer_create( const char *name, void (*timeout_function)(void *parameter), void *parameter, unsigned int time, unsigned char  flag)
{
#if defined(  _FREERTOS_)
      return ( void * ) xTimerCreate( name,                  /* Timer name. */
                                                          time,            /* Initial timer period. Timers are created disarmed. */
                                                          flag,                  /* Don't auto-reload timer. */
                                                          ( void * ) parameter,       /* Timer id. */
                                                          timeout_function        /* Timer expiration callback. */
                                                       ); /* Pre-allocated memory for timer. */
#elif defined(_RT_THREAD_)
    return  (void *)rt_timer_create(name,  timeout_function, parameter, time, flag);
#else

#endif

}


int uc_timer_del(void * timer)
{
#if defined(  _FREERTOS_)
     xTimerStop(*timer, 0);
     xTimerDelete(*timer, 0);
     return 0;
#elif defined(_RT_THREAD_)
    return rt_timer_delete((rt_timer_t) timer);
#else
    
#endif
}


int uc_timer_start(void *timer, unsigned int tiemout /*only freertos*/)
{
#if defined(  _FREERTOS_)
    xTimerStart( timer, tiemout );
    return 0;
#elif defined(_RT_THREAD_)  
    return rt_timer_start((rt_timer_t) timer);
#else
    
#endif
}

int uc_timer_stop(void *timer)
{
#if defined(  _FREERTOS_)
     xTimerStop(*timer, 0);
    return 0;
#elif defined(_RT_THREAD_)  
    return rt_timer_stop((rt_timer_t) timer);
#else
    
#endif
}


