#if defined(_FREERTOS_)
#include  "FreeRTOS.h"
#include  "queue.h"
#include  "task.h"
#elif defined(_RT_THREAD_)
#include <rtthread.h>
#else
#include <stdlib.h>
#include <stdio.h>
#endif
#include "adp_mem.h"

void *uc_malloc(unsigned int size)
{
#if defined(_FREERTOS_)
    return pvPortMalloc(size);
#elif defined(_RT_THREAD_)
    return rt_malloc(size);
#else
    return malloc(size);
#endif
}
void uc_free(void* pv)
{
#if defined(_FREERTOS_)
    vPortFree(pv);
#elif defined(_RT_THREAD_)
    rt_free(pv);
#else
    free(pv);
#endif
}

int  uc_heap_size(void)
{
#ifdef  _FREERTOS_
    return xPortGetFreeHeapSize();
#else
    unsigned int total = 0;
    unsigned int used = 0;
    unsigned int max_used = 0;
    rt_memory_info(&total,&used,&max_used);
    return total - used;
#endif
}
