#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include "manager_queue.h"
#include "manager_app.h"
#include "peripherals_manager.h"
#include "net_passthrough.h"
#include "manager_wiota_freq.h"
#include "manager_wiota_frame.h"
#include "manager_logic.h"
#include "manager_task.h"
#include "http_downloader.h"

void manager_enter(void)
{
    void *app_manager_handle = RT_NULL;
    void *app_passthrough_handle = RT_NULL;
    void *app_operation_handle = RT_NULL;
    void *app_downloader_handle = RT_NULL;

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

    manager_create_passthrough_queue();

    if (0 != manager_thread_create_task(&app_passthrough_handle, "passthrough_manag", passthrough_manager_task, RT_NULL, 1024, 3, 3))
    {
        rt_kprintf("passthrough_manager_task create error\n");
        return;
    }

    manager_create_downloader_queue();

    if (0 != manager_thread_create_task(&app_downloader_handle, "downloader_manager", manager_downloader_task, RT_NULL, 512, 3, 3))
    {
        rt_kprintf("manager_downloader_task create error\n");
        return;
    }

    // start tasks
    rt_thread_startup((rt_thread_t)app_operation_handle);
    rt_thread_startup((rt_thread_t)app_manager_handle);
    rt_thread_startup((rt_thread_t)app_passthrough_handle);
    rt_thread_startup((rt_thread_t)app_downloader_handle);
}

#endif
