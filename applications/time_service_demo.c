#include <rtthread.h>
#include <rtdevice.h>
#include "uc_wiota_api.h"

rt_sem_t location_sem = RT_NULL;

void send_test(void)
{
#ifdef WIOTA_IOTE_INFO
    u16_t online_num, offline_num;
    u8_t fake_data[] = {"Hello WIoTa IoTe"};

    uc_iote_info_t *head_node = uc_wiota_get_iote_info(&online_num, &offline_num);
    uc_iote_info_t *curr_node = NULL;

    rt_slist_for_each_entry(curr_node, &head_node->node, node)
    {
        u32_t user_id = curr_node->user_id;

        if (UC_OP_SUCC == uc_wiota_send_data(fake_data, rt_strlen((const char *)fake_data), user_id, 10000, RT_NULL, fake_data))
        {
            rt_kprintf("send data to 0x%x suc!\n", user_id);
        }
        else
        {
            rt_kprintf("send data to 0x%x failed!\n", user_id);
        }
    }
#endif

    u8_t bc_data[] = {"AP ready!"};
    if (UC_OP_SUCC == uc_wiota_send_broadcast_data(bc_data, rt_strlen((const char *)bc_data), 1, 10000, NULL, bc_data))
    {
        rt_kprintf("send bc suc!\n");
    }
    else
    {
        rt_kprintf("send bc failed!\n");
    }
}

static void uc_wiota_time_service_state_cb(time_service_state_e state)
{
    const char *str[7] = {"TS NULL",
                          "TS START",
                          "TS SUC",
                          "TS FAIL",
                          "TS INIT END",
                          "TS ALIGN END",
                          "TS STOP"};

    rt_kprintf("%s\n", str[state]);

    switch (state)
    {
    case TIME_SERVICE_INIT_END:
    {
        if (location_sem)
        {
            // location completed, release sem.
            rt_sem_release(location_sem);
        }
        break;
    }

    default:
        break;
    }
}

void uc_wiota_time_service_demo(void)
{
    // 1.enable the frame boundary alignment function.
    // note:if GPS/1588 is not supported, the frame boundary will be calculated using the default dfe counter method.
    uc_wiota_set_frame_boundary_align_func(1);

    // 2.if GPS/1588 is supprted, enable the GPS/1588 time service function. if not enable, will be calculated using the default dfe counter method.
    // note:if GPS/1588 is not supported, not setting is required.
    uc_wiota_set_time_service_func(TIME_SERVICE_GNSS, 1);
    // uc_wiota_set_time_service_func(TIME_SERVICE_1588_PS, 1);

    // 3.register the time service state callback function.
    uc_wiota_register_time_service_state_callback(uc_wiota_time_service_state_cb);

    // 4.init wiota.
    uc_wiota_init();

    // 5.set frequency point.
    uc_wiota_set_freq_info(145);

    // 6.time sevice start
    // note:the time can be served only after the location completed. the localtion takes some time
    uc_wiota_time_service_start();

    // 7.create gps location completed sem.
    location_sem = rt_sem_create("location_sem", 0, RT_IPC_FLAG_PRIO);
    RT_ASSERT(location_sem);

    // 8.waiting for gps location completed.
    // note:you must wait for the completion of GPS location before wiota run, and once the location is completed, you must run wiota immediately.
    if (RT_EOK == rt_sem_take(location_sem, RT_WAITING_FOREVER))
    {
        // after location completed, run wiota.
        uc_wiota_run();
    }

    // 9.delete location_sem.
    rt_sem_delete(location_sem);

    // other operation...
    while (1)
    {
        send_test();
        rt_thread_mdelay(1000);
    }
}