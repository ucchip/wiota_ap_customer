#if defined(_FREERTOS_)
#include  "FreeRTOS.h"
#include  "queue.h"
#include  "task.h"
#elif defined(_RT_THREAD_)
#include <rtthread.h>
#else

#endif
#include "adp_task.h"

 int uc_thread_create(void ** thread, char *name, void (*entry)(void *parameter), void *parameter, unsigned int  stack_size, unsigned char   priority, unsigned int  tick)
{
#if defined(_FREERTOS_)
    return xTaskCreate( entry, name, stack_size, parameter, priority, NULL );
#elif defined(_RT_THREAD_)
    *thread =  rt_thread_create(name, entry, parameter,  stack_size * 4, priority,  tick);
    if (NULL == *thread)
        return 1;
#else

#endif
    return 0;
}

int uc_thread_start(void * thread)
{
#if defined(_FREERTOS_)
    vTaskStartScheduler();
    return 0;
#elif defined(_RT_THREAD_)
    return rt_thread_startup((rt_thread_t)thread);
#else

#endif
}

int uc_thread_del(void * thread)
{
#if defined(_FREERTOS_)
    vTaskDelete((TaskHandle_t)thread);
#elif defined(_RT_THREAD_)
    return rt_thread_delete((rt_thread_t)thread);
#else

#endif
}

int uc_thread_delay(signed int  time)
{
#if defined(_FREERTOS_)
    vTaskDelay(pdMS_TO_TICKS(time));
#elif defined(_RT_THREAD_)
    rt_thread_mdelay(time);
#else

#endif
    return 0;
}

