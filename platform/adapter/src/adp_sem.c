
#if defined(_FREERTOS_)
#include  "FreeRTOS.h"
#include  "queue.h"
#include  "task.h"
#elif defined( _RT_THREAD_)
#include <rtthread.h>
#else
#include "stdio.h"
#endif

#include "adp_sem.h"



void *uc_create_sem(char *name, unsigned int value, unsigned char flag)
{
#if defined(_FREERTOS_)
    return xSemaphoreCreateMutex();
#elif defined( _RT_THREAD_)
    return rt_sem_create(name,  value,  flag);
#else

#endif
}

int uc_wait_sem(void *sem, signed   int  timeout)
{
#if defined(_FREERTOS_)
    xSemaphoreTake(sem, portMAX_DELAY);
    return 0;
#elif defined( _RT_THREAD_)
    return rt_sem_take(sem, timeout );
#else
    
#endif
}


int uc_signed_sem(void *sem)
{
#if defined(_FREERTOS_)
    xSemaphoreGive(SemaphoreDataSocket);
    return 0;
#elif defined( _RT_THREAD_)
    return rt_sem_release(sem);
#else
    
#endif
}


/*lock*/
void *uc_create_lock(char *name)
{
#if defined(_FREERTOS_)
    return xSemaphoreCreateMutex();
#elif defined( _RT_THREAD_)
    return rt_mutex_create(name,  RT_IPC_FLAG_FIFO);
#else

#endif
}

int uc_lock(void *sem, signed   int  timeout)
{
#if defined(_FREERTOS_)
    xSemaphoreTake(sem, timeout);
    return 0;
#elif defined( _RT_THREAD_)
    return rt_mutex_take(sem, timeout );
#else
    
#endif
}


int uc_unlock(void *sem)
{
#if defined(_FREERTOS_)
    xSemaphoreGive(SemaphoreDataSocket);
    return 0;
#elif defined( _RT_THREAD_)
    return rt_mutex_release(sem);
#else
    
#endif
}



