/*
 * test_wiota_api.c
 *
 *  Created on: 2021.08.02
 *  Author: jpwang
 */

#include "uc_wiota_api.h"
#include "test_wiota_api.h"
#include "rtthread.h"

#define BAN_8288_FREQ (135)

u8_t g_freqPoint = 0xff;
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

void test_show_access_func(u32_t user_id, u8_t group_idx, u8_t burst_idx, u8_t slot_idx)
{
    rt_kprintf("user_id 0x%x accessed\n", user_id);
}

void test_show_drop_func(u32_t user_id)
{
    rt_kprintf("user_id 0x%x dropped\n", user_id);
}

void test_show_result(uc_send_recv_t *result)
{
    rt_kprintf("send data to 0x%x, result %d\n", result->user_id, result->result);
}

void test_show_recv_data(u32_t user_id, u8_t *recv_data, u32_t data_len, u8_t type)
{
    //    u8_t *fake_data = NULL;

    rt_kprintf("user_id 0x%x, type %d, reportData ", user_id, type);
    for (u16_t index = 0; index < data_len; index++)
    {
        rt_kprintf("%u ", *recv_data++);
    }
    rt_kprintf(", reportDataLen %d\n", data_len);

    // fake_data = generate_fake_data(80, 10);
    // uc_wiota_send_data(fake_data, 80, user_id, 100, test_show_result);
    // rt_free(fake_data);
    // fake_data = NULL;
    // //for test send two
    // rt_thread_mdelay(100);
    // fake_data = generate_fake_data(80, 10);
    // uc_wiota_send_data(fake_data, 80, user_id, 100, test_show_result);
    // rt_free(fake_data);
    // fake_data = NULL;
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
    u8_t fPoint[20] = {5, 15, 25, 35, 45, 55, 65, 75, 85, 95, 105, 115, 125, 135, 145, 155, 165, 175, 185, 195};
    s32_t timeout = 60000;
    uc_scan_recv_t scan_info = {0};
    u8_t fPointNum = sizeof(fPoint) / sizeof(u8_t);

    uc_wiota_scan_freq(fPoint, fPointNum, timeout, NULL, &scan_info);
    // uc_wiota_scan_freq(NULL, 0, -1, NULL, &scan_info); // if fPoint == NULL && fPointNum == 0, default scan all,about 3 min
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
    g_freqPoint = fPoint[12];
    is_need_to_restart = TRUE;
}

// test! set frequency point
void test_set_frequency_point(void)
{
    uc_wiota_set_freq_info(g_freqPoint);
}

// test! set hopping freq
void test_set_hopping_freq(void)
{
    uc_wiota_set_hopping_freq(g_freqPoint + 1);
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
    uc_wiota_register_iote_access_callback(test_show_access_func);
    uc_wiota_register_iote_dropped_callback(test_show_drop_func);
    uc_wiota_register_recv_data_callback(test_show_recv_data);
}

// test! send normal data to iote
void test_send_normal_data(void)
{
    u16_t conNum, disConNum;

    uc_iote_info_t *conNode = uc_wiota_get_iote_info(&conNum, &disConNum);
    uc_iote_info_t *tempNode = NULL;

    // u8_t *fake_data = generate_fake_data(8, 10);
    u8_t fake_data[17] = {"Hello WIoTa IoTe"};

    rt_slist_for_each_entry(tempNode, &conNode->node, node)
    {
        // active send
        if (conNode->iote_status == STATUS_ONLINE)
        {
            u32_t user_id = conNode->user_id;
            if (UC_OP_SUCC == uc_wiota_send_data(fake_data, 16, user_id, 10000, RT_NULL))
            {
                rt_kprintf("send data to 0x%x suc!\n", user_id);
            }
            else
            {
                rt_kprintf("send data to 0x%x failed!\n", user_id);
            }
        }

        // paging
        else if (conNode->iote_status == STATUS_OFFLINE)
        {
            u32_t user_id = conNode->user_id;
            if (UC_OP_SUCC == uc_wiota_send_data(fake_data, 16, user_id, 10000, RT_NULL))
            {
                rt_kprintf("send data to 0x%x suc!\n", user_id);
            }
            else
            {
                rt_kprintf("send data to 0x%x failed!\n", user_id);
            }
        }
        else
        {
            // do something
        }
    }

    // rt_free(fake_data);
    // fake_data = NULL;
}

// test! send normal/ota broadcast data
void test_send_broadcast_data(uc_bc_mode_e mode)
{
    u8_t *testData;
    u8_t *tempData = NULL;
    s32_t timeout = 10000; //ms
    uc_result_e result = UC_OP_FAIL;
    u32_t offset = 0;

    if (NORMAL_BROADCAST == mode)
    {
        // testData = generate_fake_data(10, 10);
        u8_t bcData[10] = {"AP ready!"};
        uc_wiota_send_broadcast_data(bcData, 9, mode, timeout, NULL);
    }
    else if (OTA_BROADCAST == mode)
    {
        testData = generate_fake_data(1024 * 120, 11);
        tempData = (u8_t *)rt_malloc(1024);
        if (testData == NULL)
        {
            rt_kprintf("test_send_broadcast_data rt_malloc failed\n");
            rt_free(testData); // !!!need be manually released after use
            testData = NULL;
            return;
        }
        rt_memset(tempData, 0, 1024);

        for (u8_t i = 0; i < 120; i++)
        {
            rt_memset(tempData, 0, 1024);
            rt_memcpy(tempData, testData + offset, 1024);
            result = uc_wiota_send_broadcast_data(tempData, 1024, mode, timeout, NULL);
            while (1)
            {
                if (result == UC_OP_SUCC)
                {
                    offset += 1024;
                    rt_kprintf("test_send_broadcast_data send next broadcast %u\n", i);
                    break;
                }
            }
        }
        rt_free(testData); // !!!need be manually released after use
        testData = NULL;
        rt_free(tempData);
        tempData = NULL;
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

// test! add iote to blacklist
void test_add_iote_to_blacklist(void)
{
    u32_t userIdArry[5] = {0x4c00ccdb, 0xfb3eae00, 0x38f8c8d8, 0x8aff8783, 0x33139955};
    u16_t userIdNum = sizeof(userIdArry) / sizeof(u32_t);

    uc_wiota_add_iote_to_blacklist(userIdArry, userIdNum);
    uc_wiota_print_blacklist();
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

// test! remove iote from blacklist
void test_remove_iote_from_blacklist(void)
{
    u32_t userIdArry[4] = {0x38f8c8d8, 0xf6149b11, 0x4eaac480, 0x4c00ccdb};
    u16_t userIdNum = sizeof(userIdArry) / sizeof(u32_t);

    uc_wiota_remove_iote_from_blacklist(userIdArry, userIdNum);
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

void app_interface_main_task(void *pPara)
{
    // wiota init
    uc_wiota_init();

    uc_wiota_set_freq_info(145);

    test_set_all_para();

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
    test_register_callback();

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
        // test_send_broadcast_data(OTA_BROADCAST);

        rt_thread_mdelay(4000);

        // test! send normal data to iote
        test_send_normal_data();

        // test! query iote information after wiota start
        // test_query_iote_info();

        // test! read temperature of ap8288
        // test_read_temp();

        rt_thread_mdelay(2000);
    }
    return;
}

void app_task_init(void)
{
    rt_thread_t testTaskHandle = rt_thread_create("app_task", app_interface_main_task, NULL, 1024, 3, 3);
    if (testTaskHandle != NULL)
    {
        rt_thread_startup(testTaskHandle);
    }
}