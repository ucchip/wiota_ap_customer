#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "manager_task.h"

int manager_thread_create_task(void **thread,
                               char *name, void (*entry)(void *parameter),
                               void *parameter, unsigned int stack_size,
                               unsigned char priority,
                               unsigned int tick)
{
    *thread = rt_malloc(sizeof(struct rt_thread));
    void *start_stack = rt_malloc(stack_size * 4);

    if (RT_NULL == start_stack || RT_NULL == *thread)
    {
        return 1;
    }

    if (RT_EOK != rt_thread_init(*thread, name, entry, parameter, start_stack, stack_size * 4, priority, tick))
    {
        return 2;
    }

    return 0;
}

int manager_thread_del(void *thread)
{
    rt_thread_detach((rt_thread_t)thread);
    rt_free(((rt_thread_t)thread)->stack_addr);
    rt_free(thread);
    return 0;
}

#endif
