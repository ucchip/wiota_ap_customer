/*
 * Copyright (c) 2022, Chongqing UCchip InfoTech Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @brief MQTT communication
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-01-30     Lujun        the first version
 */
#include <rtthread.h>
#include "resource_manager.h"

static char *g_status[] = {
    "init",    /* RT_THREAD_INIT */
    "ready",   /* RT_THREAD_READY */
    "suspend", /* RT_THREAD_SUSPEND */
    "running", /* RT_THREAD_RUNNING */
    "close",   /* RT_THREAD_CLOSE */
};

/**
 * @brief resource manager
 *
 * @param mode the print mode
 */
void resource_manager(int mode)
{
    if (mode == RESOURCE_DETAIL_MODE)
    {
        print_thread_resource();
        print_memory_resource();
        rt_kprintf("-----------------------------------------------------------------\r\n");
    }
    else
        print_memory_resource();
}

/**
 * @brief print memory resource
 *
 */
void print_memory_resource(void)
{
    unsigned int total = 0;
    unsigned int used = 0;
    unsigned int max_used = 0;

    rt_memory_info(&total, &used, &max_used);
    rt_kprintf("total memory %d used %d rate %d%% max_used %d\r\n", total, used, used * 100 / total, max_used);
}

/**
 * @brief print thread resource
 *
 */
void print_thread_resource(void)
{
    struct rt_thread *thread = RT_NULL;
    struct rt_list_node *node = RT_NULL;
    struct rt_object_information *info = RT_NULL;

    rt_kprintf("-----------------------------------------------------------------\r\n");
    rt_kprintf("%-*.s priority  cpu  stack   used  rate   status    handler\r\n", RT_NAME_MAX, "thread");
    rt_kprintf("-----------------------------------------------------------------\r\n");
    /* for each thread */
    info = rt_object_get_information((enum rt_object_class_type)RT_Object_Class_Thread);
    for (node = info->object_list.next; node != &(info->object_list); node = node->next)
    {
        thread = rt_list_entry(node, struct rt_thread, list);
        /* free stack size */
        rt_uint8_t *ptr = (rt_uint8_t *)thread->stack_addr;
        while (*ptr == '#')
            ptr++;
        rt_ubase_t free_size = (rt_ubase_t)ptr - (rt_ubase_t)thread->stack_addr;
        rt_ubase_t used_rate = (thread->stack_size - free_size) * 100 / thread->stack_size;
        /* print thread information */
        rt_kprintf("%-*.s %5d   %5d %6d %6d %4d%% %8s    %x\r\n",
                   RT_NAME_MAX, thread->name,
                   thread->current_priority,
                   thread->task_run_all_time,
                   thread->stack_size,
                   thread->stack_size - free_size,
                   used_rate,
                   g_status[thread->stat & RT_THREAD_STAT_MASK],
                   thread);
        /* reset counter */
        thread->task_run_all_time = 0;
    }
    rt_kprintf("-----------------------------------------------------------------\r\n");
}
