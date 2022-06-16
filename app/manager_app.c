#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include "manager_queue.h"
#include "manager_app.h"
#include "peripherals_manager.h"
#include "net_passthrough.h"
#include "manager_wiota.h"
#include "manager_operation_data.h"
#include "manager_logic.h"
#include "manager_task.h"

void manager_enter(void)
{
    void *app_manager_handle = RT_NULL;
    //void* app_custom_handle = NULL;
    void *app_passthrough_handle = RT_NULL;
    void *app_operation_handle = RT_NULL;

    rt_kprintf("app manager enter\n");

    manager_create_operation_queue();
    if (0 != manager_thread_create_task(&app_operation_handle, "operation_manager", manager_operation_task, RT_NULL, 1024, 3, 3))
    {
        rt_kprintf("manager_thread_create_task error\n");
        return;
    }

    manager_create_logic_queue();

    // create wiota app manager task. wiota business logic management.
    if (0 != manager_thread_create_task(&app_manager_handle, "app_manager", manager_wiota_task, RT_NULL, 1524, 3, 3))
    {
        rt_kprintf("manager_thread_create_task error\n");
        return;
    }
#if 0
    peripherals_manager_create_queue();

    if(0 != manager_thread_create_task(&app_custom_handle,"cusom_manag", peripherals_manager_task, RT_NULL, 256, 3, 3))
    {
        rt_kprintf("custom_manager_task create error\n");
        return ;
    }
#endif
    manager_create_passthrough_queue();

    if (0 != manager_thread_create_task(&app_passthrough_handle, "passthrough_manag", passthrough_manager_task, RT_NULL, 1125, 3, 3))
    {
        rt_kprintf("custom_manager_task create error\n");
        return;
    }

    // start tasks
    rt_thread_startup((rt_thread_t)app_operation_handle);
    rt_thread_startup((rt_thread_t)app_manager_handle);
    //rt_thread_startup((rt_thread_t)app_custom_handle);
    rt_thread_startup((rt_thread_t)app_passthrough_handle);
}

#endif
