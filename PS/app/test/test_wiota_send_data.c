/*
 * test_wiota_send_data.c
 *
 *  Created on: 2023.11.02
 *  Author: ypzhang
 */
#include <rtthread.h>

#ifdef WIOTA_AP_SEND_DATA_DEMO
#include "uc_wiota_api.h"
#include "test_wiota_api.h"
#include "rtdevice.h"

static rt_mq_t recv_mq = NULL;

typedef struct
{
    uint32_t user_id;
    uint32_t data_len;
    uint8_t *data;
} recv_mq_t;

static void uc_wiota_recv_callback(uint32_t user_id, uc_dev_pos_t dev_pos, uint8_t *recv_data, uint16_t data_len, uc_recv_data_type_e type)
{
    recv_mq_t msg = {0};
    uint8_t *data = NULL;

    if (type == DATA_TYPE_ACCESS)
    {
        rt_kprintf("0x%x accessed\n", user_id);
    }

    data = rt_malloc(data_len);
    RT_ASSERT(data);
    rt_memset(data, 0, data_len);

    rt_memcpy(data, recv_data, data_len);

    msg.user_id = user_id;
    msg.data_len = data_len;
    msg.data = data;

    if (RT_EOK != rt_mq_send(recv_mq, &msg, sizeof(recv_mq_t)))
    {
        rt_free(data);
        rt_kprintf("%s line %d rt_mq_send fail\n", __FUNCTION__, __LINE__);
    }
}

static void uc_wiota_send_callback(uc_send_recv_t *result)
{
    rt_kprintf("0x%x non_blocking_send result %d\n", result->user_id, result->result);
}

static void wiota_ap_send_data_task(void *pPara)
{
    recv_mq_t msg = {0};
    uc_result_e send_result = UC_OP_FAIL;

    while (1)
    {
        if (RT_EOK != rt_mq_recv(recv_mq, &msg, sizeof(recv_mq_t), RT_WAITING_FOREVER))
        {
            continue;
        }

        rt_kprintf("0x%x, recv_data data_len %d\n", msg.user_id, msg.data_len);
        for (uint16_t index = 0; index < msg.data_len; index++)
        {
            if (index != 0 && index % 16 == 0)
            {
                rt_kprintf("\n");
            }
            rt_kprintf("%x ", msg.data[index]);
        }
        rt_kprintf("\n");

        // non blocking send
        uc_wiota_send_data(msg.data, msg.data_len, msg.user_id, 60000, uc_wiota_send_callback, RT_NULL);

        // blocking send
        send_result = uc_wiota_send_data(msg.data, msg.data_len, msg.user_id, 60000, RT_NULL, RT_NULL);
        rt_kprintf("0x%x blocking_send result %d\n", msg.user_id, send_result);

        rt_free(msg.data);
    }
}

void wiota_ap_data_recv_and_send_demo(void)
{
    // wiota init
    uc_wiota_init();

    // set the frequency point. IOTE and AP need to set the same frequency point to synchronize
    uc_wiota_set_freq_info(50);

    // wiota start
    uc_wiota_run();

    // register callback after wiota start or init
    uc_wiota_register_recv_data_callback(uc_wiota_recv_callback);

    // create ul_data recv mq
    recv_mq = rt_mq_create("recv_mq", sizeof(recv_mq_t), 16, RT_IPC_FLAG_FIFO);
    RT_ASSERT(recv_mq);

    // create dl_data send task
    rt_thread_t demo_task_handler = rt_thread_create("demo_task", wiota_ap_send_data_task, RT_NULL, 1024, 3, 3);
    RT_ASSERT(demo_task_handler);
    rt_thread_startup(demo_task_handler);
}

#endif // WIOTA_AP_SEND_DATA_DEMO