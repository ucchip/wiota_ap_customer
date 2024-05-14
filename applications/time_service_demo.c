#include <rtthread.h>
#include <rtdevice.h>
#include "uc_wiota_api.h"

void send_test(void)
{
#ifdef WIOTA_IOTE_INFO
    u16_t online_num, offline_num;
    uint8_t fake_data[] = {"Hello WIoTa IoTe"};

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

    uint8_t bc_data[] = {"AP ready!"};
    if (UC_OP_SUCC == uc_wiota_send_broadcast_data(bc_data, rt_strlen((const char *)bc_data), 1, 10000, NULL, bc_data))
    {
        rt_kprintf("send bc suc!\n");
    }
    else
    {
        rt_kprintf("send bc failed!\n");
    }
}

void uc_wiota_time_service_demo(void)
{
    // 1.enable the frame boundary alignment function.
    // note:if GPS/1588/sync assiatant is not supported, the frame boundary will be calculated using the default dfe counter method.
    uc_wiota_set_frame_boundary_align_func(1);

    // 2.if GPS/1588/sync assiatant is supprted, enable the GPS/1588/sync assiatant time service function. if not enable, will be calculated using the default dfe counter method.
    // note:if GPS/1588/sync assiatant is not supported, not setting is required.
    uc_wiota_set_time_service_func(TIME_SERVICE_GNSS, 1); // gps
    // uc_wiota_set_time_service_func(TIME_SERVICE_1588_PS, 1); // 1588
    // uc_wiota_set_time_service_func(TIME_SERVICE_SYNC_ASSISTANT, 1); // sync assiatant
    /* if you want to test he time sync accuracy throught instruments such as oscilloscopes, you can activate the PPS of sync asstistant, by measuring
     the PPS of GPS and AP8288, calculate the sync accuracy
     it is best to turn it off after the measurement is completed. uc_wiota_set_sync_assistant_pps(0); */
    // uc_wiota_set_sync_assistant_pps(1);

    // 3.init wiota.
    uc_wiota_init();

    // 4.set frequency point.
    uc_wiota_set_freq_info(145);

    // 5. wiota run
    uc_wiota_run();

    // 6.time sevice start
    // note:the time can be served only after the location completed. the localtion takes some time
    uc_wiota_time_service_start();

    // other operation...
    while (1)
    {
        send_test();
        rt_thread_mdelay(1000);
    }
}