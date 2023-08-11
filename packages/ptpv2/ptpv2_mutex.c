/**
 * @file        mutex.c
 * @brief       本地互斥量接口实现
 * @par         Copyright (c):
 *              北京御芯微科技有限公司
 * @par         History:
 *              <V0.1>    <XK>  | <2021-01-26>  | <创建>
 */

#include <rtthread.h>
#include "ptpv2_mutex.h"

//count 只能为0/1
void *ptp_mutex_create(const char *name)
{
    return rt_mutex_create(name, RT_IPC_FLAG_FIFO);
}

void ptp_mutex_del(void *mutex)
{
    if (mutex == RT_NULL)
    {
        return;
    }
    rt_mutex_delete(mutex);
}

int32_t ptp_mutex_pend(void *pmutex, uint32_t timeout)
{
    if (pmutex == RT_NULL)
    {
        return 0;
    }

    if (rt_mutex_take(pmutex, timeout) == RT_EOK)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

void ptp_mutex_post(void *pmutex)
{
    if (pmutex != RT_NULL)
    {
        rt_mutex_release(pmutex);
    }
}
