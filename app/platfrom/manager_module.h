#ifndef _MANAGER_MODULE_H_
#define _MANAGER_MODULE_H_

#include <rtthread.h>

enum manager_tasks_identification
{
    MANAGER_MODULE_DEFAULT = 0,
    MANAGER_OPERATION_INDENTIFICATION,
    MANAGER_LOGIC_INDENTIFICATION,
    MANAGER_NETWORK_INDENTIFICATION
};

#define MEMORY_ASSERT(data)                                                      \
    if (RT_NULL == data)                                                         \
    {                                                                            \
        rt_kprintf("%s line %d error:memroy is null\n", __FUNCTION__, __LINE__); \
        *((unsigned int *)0) = (~0);                                             \
    }

#endif
