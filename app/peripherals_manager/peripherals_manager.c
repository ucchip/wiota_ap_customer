#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>
#include "manager_queue.h"

// user peripheral control

static void *peripherals_queue_handle;

int peripherals_manager_create_queue(void)
{
    // create wiota app manager queue.
    peripherals_queue_handle = manager_create_queue("custom_manager", 4, 16, UC_SYSTEM_IPC_FLAG_PRIO);
    if (peripherals_queue_handle == RT_NULL)
    {
        rt_kprintf("manager_create_queue error\n");
        return 1;
    }

    return 0;
}

int peripherals_manager_send_page(int src_task, void *data)
{
    return manager_send_page(peripherals_queue_handle, src_task, data);
}

void peripherals_manager_task(void *pPara)
{
    t_app_manager_message *page;
    while (1)
    {
        if (QUEUE_EOK != manager_recv_queue(peripherals_queue_handle, (void *)&page, UC_QUEUE_WAITING_FOREVER))
            continue;

        rt_free(page);
    }
}

#endif
