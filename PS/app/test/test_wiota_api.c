/*
 * test_wiota_api.c
 *
 *  Created on: 2021.08.02
 *  Author: jpwang
 */

#include <rtthread.h>
#ifdef WIOTA_API_TEST
#include "uc_wiota_api.h"
#include "test_wiota_api.h"

#define BAN_8288_FREQ (135)

u8_t g_freq_point = 0xff;
boolean is_need_to_restart = FALSE;

u8_t *generate_fake_data(u32_t data_len, u8_t repeat_num)
{
    u16_t index = 0;
    u16_t target = 33;
    u8_t *fake_data = NULL;

    fake_data = (u8_t *)rt_malloc(data_len);
    if (fake_data == NULL)
    {
        rt_kprintf("generate_fake_data rt_malloc failed\n");
        return NULL;
    }
    rt_memset(fake_data, 0, data_len);
    while (index < data_len)
    {
        for (int i = 0; i < repeat_num && index < data_len; ++i)
        {
            fake_data[index++] = target;
        }
        target++;
    }
    return fake_data;
}

void test_show_drop_func(u32_t user_id)
{
    rt_kprintf("user_id 0x%x dropped\n", user_id);
}

void test_show_result(uc_send_recv_t *result)
{
    rt_kprintf("send data to 0x%x, result %d\n", result->user_id, result->result);
}

void test_show_recv_data(u32_t user_id, uc_dev_pos_t dev_pos, u8_t *recv_data, u16_t data_len, uc_recv_data_type_e type)
{
    if (type == DATA_TYPE_ACCESS)
    {
        rt_kprintf("user_id 0x%x accessed\n", user_id);
    }
    rt_kprintf("user_id 0x%x, type %d, recv_data ", user_id, type);
    for (u16_t index = 0; index < data_len; index++)
    {
        rt_kprintf("0x%x ", *recv_data++);
    }
    rt_kprintf(", data_len %d\n", data_len);
}

// test! set/get all parameter
void test_set_all_para(void)
{
    //init dynamic parameter
    sub_system_config_t config = {0};
    // get config first
    uc_wiota_get_system_config(&config);
    config.symbol_length = 3;
    // set config
    uc_wiota_set_system_config(&config);
}

// test! scan frequency point collection
void test_handle_scan_freq(void)
{
    u8_t freq_array[20] = {5, 15, 25, 35, 45, 55, 65, 75, 85, 95, 105, 115, 125, 135, 145, 155, 165, 175, 185, 195};
    s32_t timeout = 60000;
    uc_scan_recv_t scan_info = {0};
    u8_t freq_num = sizeof(freq_array) / sizeof(u8_t);

    uc_wiota_scan_freq(freq_array, freq_num, 0, timeout, NULL, &scan_info);
    // uc_wiota_scan_freq(NULL, 0, 0, -1, NULL, &scan_info); // if freq_array == NULL && freq_num == 0, default scan all,about 3 min
    if (scan_info.result == UC_OP_SUCC)
    {
        uc_scan_freq_t *freqList = (uc_scan_freq_t *)scan_info.data;
        for (u8_t idx = 0; idx < scan_info.data_len / sizeof(uc_scan_freq_t); idx++)
        {
            rt_kprintf("freq_idx = %u, snr = %d, rssi = %d, is_synced = %d\n", freqList->freq_idx, freqList->snr, freqList->rssi, freqList->is_synced);
            freqList++;
        }
    }
    else
    {
        rt_kprintf("scan freq failed or timeout\n");
    }
    // according to the scan result , choose the best and set
    // uc_wiota_set_frequency_point(fPoint[0]/* fPoint[x] */);
    rt_free(scan_info.data); // !!!need be manually released after use
    scan_info.data = NULL;
    g_freq_point = freq_array[12];
    is_need_to_restart = TRUE;
}

// test! set frequency point
void test_set_frequency_point(void)
{
    uc_wiota_set_freq_info(g_freq_point);
}

// test! set hopping freq
void test_set_hopping_freq(void)
{
    uc_wiota_set_hopping_freq(g_freq_point + 1);
    uc_wiota_set_hopping_mode(2, 3); // after working for 2 frames at original freq_point, skip to the freq hopping freq_point and work for 3 frames
}

// test! set/get connection timeout
void test_set_connection_timeout(void)
{
    u32_t active_time = 3;

    uc_wiota_set_active_time(active_time);
}

void test_get_connection_timeout(void)
{
    u32_t active_time = uc_wiota_get_active_time();
    rt_kprintf("test_get_connection_timeout active_time %u\n", active_time);
}

// test! register callback
void test_register_callback(void)
{
    uc_wiota_register_iote_dropped_callback(test_show_drop_func);
    uc_wiota_register_recv_data_callback(test_show_recv_data);
}

#ifdef WIOTA_IOTE_INFO
// test! send normal data to iote
void test_send_normal_data(void)
{
    u16_t online_num, offline_num;
    u8_t fake_data[] = {"Hello WIoTa IoTe"};

    uc_iote_info_t *head_node = uc_wiota_get_iote_info(&online_num, &offline_num);
    uc_iote_info_t *curr_node = NULL;

    rt_slist_for_each_entry(curr_node, &head_node->node, node)
    {
        u32_t user_id = curr_node->user_id;

        if (UC_OP_SUCC == uc_wiota_send_data(fake_data, rt_strlen((const char *)fake_data), user_id, 10000, RT_NULL, RT_NULL))
        {
            rt_kprintf("send data to 0x%x suc!\n", user_id);
        }
        else
        {
            rt_kprintf("send data to 0x%x failed!\n", user_id);
        }
    }
}
#endif

// test! send normal/ota broadcast data
void test_send_broadcast_data(uc_bc_mode_e mode)
{
    u8_t *test_data;
    s32_t timeout = 10000; //ms
    uc_result_e result = UC_OP_FAIL;
    u32_t offset = 0;

    if (NORMAL_BROADCAST == mode)
    {
        u8_t bc_data[] = {"AP ready!"};
        uc_wiota_send_broadcast_data(bc_data, rt_strlen((const char *)bc_data), mode, timeout, NULL, NULL);
    }
    else if (OTA_BROADCAST == mode)
    {
        test_data = generate_fake_data(512 * 30, 11);

        for (u8_t i = 0; i < 30; i++)
        {
            result = uc_wiota_send_broadcast_data(test_data + offset, 512, mode, timeout, NULL, NULL);

            if (result == UC_OP_SUCC)
            {
                offset += 512;
                rt_kprintf("send next ota data %u\n", i);
            }
        }
        rt_free(test_data); // !!!need be manually released after use
        test_data = NULL;
    }
    else
    {
        rt_kprintf("test_send_broadcast_data invalid mode %u\n", mode);
        return;
    }
}

// test! query iote infomation
void test_query_iote_info(void)
{
    uc_wiota_print_iote_info();
}

// test! read temperature of ap8288
void test_read_temp(void)
{
    uc_temp_recv_t read_temp = {0};
    u16_t timeout = 10000;

    if (UC_OP_SUCC == uc_wiota_read_temperature(NULL, &read_temp, timeout))
    {
        rt_kprintf("test_read_temp read_temp %d\n", read_temp.temp);
    }
    else
    {
        rt_kprintf("read temp failed\n");
    }
}

// test! set ap8288 rf power
void test_set_ap8288_rf_power(void)
{
    s8_t rf_power = 24; //value range:-1~29
    uc_wiota_set_ap_max_power(rf_power);
}

// test! get version of sw
void test_get_version()
{
    u8_t wiota_version_8088[15] = {0};
    u8_t git_info_8088[36] = {0};
    u8_t make_time_8088[36] = {0};
    u8_t wiota_version_8288[15] = {0};
    u8_t git_info_8288[36] = {0};
    u8_t make_time_8288[36] = {0};
    u32_t cce_version = 0;

    uc_wiota_get_version(wiota_version_8088, git_info_8088, make_time_8088, wiota_version_8288, git_info_8288, make_time_8288, &cce_version);
    rt_kprintf("wiota lib version: %s,%s\n", wiota_version_8088, wiota_version_8288);
    rt_kprintf("wiota lib git info: %s,%s\n", git_info_8088, git_info_8288);
    rt_kprintf("wiota lib make time: %s,%s\n", make_time_8088, make_time_8288);
    rt_kprintf("cce version: %x\n", cce_version);
}

// test! add iote to blacklist
void test_add_iote_to_blacklist(void)
{
    u32_t user_id_array[5] = {0x4c00ccdb, 0xfb3eae00, 0x38f8c8d8, 0x8aff8783, 0x33139955};
    u16_t user_id_num = sizeof(user_id_array) / sizeof(u32_t);

    uc_wiota_add_iote_to_blacklist(user_id_array, user_id_num);
    uc_wiota_print_blacklist();
}

// test! remove iote from blacklist
void test_remove_iote_from_blacklist(void)
{
    u32_t user_id_array[4] = {0x38f8c8d8, 0xf6149b11, 0x4eaac480, 0x4c00ccdb};
    u16_t user_id_num = sizeof(user_id_array) / sizeof(u32_t);

    uc_wiota_remove_iote_from_blacklist(user_id_array, user_id_num);
    uc_wiota_print_blacklist();
}

// test! wiota exit and restart
void test_wiota_exit_and_restart(void)
{
    uc_wiota_exit();

    rt_thread_mdelay(2000);
    uc_wiota_init();

    // test! set frequency point after wiota start, before wiota start
    test_set_frequency_point();

    // test! set/get connection timeout after wiota init, before wiota start
    // test_set_connection_timeout();

    // test! set all dynamic parameter after wiota init, before wiota start
    test_set_all_para();

    uc_wiota_run();
}

#ifdef RAMP_RF_SET_SUPPORT
void ramp_rf_set(void)
{
    u8_t freq_array[8] = {55, 70, 85, 100, 115, 130, 145, 160};

    uc_wiota_init();
    uc_wiota_set_freq_info(freq_array[0]);
    rt_thread_mdelay(2000);
    uc_wiota_run();
    rt_thread_mdelay(2000);
    uc_wiota_set_ramp_value(1023);
    uc_wiota_set_rf_ctrl_idx(2);
    while (1)
    {
        rt_thread_mdelay(2000);
    }
}
#endif

void wiota_api_test_task(void *pPara)
{
#ifdef RAMP_RF_SET_SUPPORT
    ramp_rf_set();
#else
    // wiota init
    uc_wiota_init();

    uc_wiota_set_freq_info(145);

    // test_set_all_para();
    // wiota start
    uc_wiota_run();

    // test! scan frequency point collection, after wiota start
    // test_handle_scan_freq();

    // test! wiota exit and restart
    // if (is_need_to_restart)
    // {
    //     test_wiota_exit_and_restart();
    //     is_need_to_restart = FALSE;
    // }

    // test! set hopping freq
    // test_set_hopping_freq();

    // test! register callback after wiota start or init
    // test_register_callback();

    // test! add iote to blacklist after wiota start or init
    // test_add_iote_to_blacklist();

    // test! remove iote from blacklist after wiota start or init
    // test_remove_iote_from_blacklist();

    // test! set ap8288 rf power, after wiota start
    // test_set_ap8288_rf_power();

    // test! get version of sw
    // test_get_version();

    while (1)
    {
        // test! send normal/ota broadcast data after wiota start
        test_send_broadcast_data(NORMAL_BROADCAST);
        test_send_broadcast_data(OTA_BROADCAST);

        rt_thread_mdelay(4000);
#ifdef WIOTA_IOTE_INFO
        // test! send normal data to iote
        test_send_normal_data();
#endif
        // test! query iote information after wiota start
        test_query_iote_info();

        // test! read temperature of ap8288
        // test_read_temp();

        rt_thread_mdelay(2000);
    }
#endif
    return;
}

void wiota_api_test(void)
{
    rt_thread_t testTaskHandle = rt_thread_create("app_task", wiota_api_test_task, NULL, 1024, 3, 3);
    if (testTaskHandle != NULL)
    {
        rt_thread_startup(testTaskHandle);
    }
}
#endif // WIOTA_API_TEST