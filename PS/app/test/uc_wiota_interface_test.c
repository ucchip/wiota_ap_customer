/*
 * uc_wiota_interface_test.c
 *
 *  Created on: 2021.08.02
 *  Author: jpwang
 */

#include "uc_wiota_interface.h"
#include "uc_wiota_interface_test.h"
#include "rtthread.h"

#define BAN_8288_FREQ (170)
#define BAN_8288_DCXO (0x36000)

typedef struct connected_iote
{
    u32_t user_id;
    struct connected_iote *next;
} connected_iote_t;

u8_t g_freqPoint = 0xff;
boolean is_need_to_restart = FALSE;
connected_iote_t *g_userHead = NULL;

void user_id_list_init(void)
{
    g_userHead = (connected_iote_t *)rt_malloc(sizeof(connected_iote_t));
    if (g_userHead == NULL)
    {
        return;
    }
    rt_memset(g_userHead, 0, sizeof(connected_iote_t));
    g_userHead->next = NULL;
}

void user_id_list_deinit(void)
{
    connected_iote_t *pNodeNext = g_userHead->next;
    connected_iote_t *pNode = g_userHead;

    while (pNodeNext != NULL)
    {
        pNode->next = pNodeNext->next;
        rt_free(pNodeNext);
        pNodeNext = NULL;
        pNodeNext = pNode->next;
    }
    rt_free(pNode);
    pNode = NULL;
}

void save_user_id_of_connected_iote(u32_t user_id)
{
    connected_iote_t *pNodeNext = g_userHead->next;
    connected_iote_t *pNode = g_userHead;

    while (pNodeNext != NULL)
    {
        if (pNodeNext->user_id == user_id)
        {
            pNode->next = pNodeNext->next;
            rt_free(pNodeNext);
            pNodeNext = NULL;
            break;
        }
        pNodeNext = pNode->next;
    }
    connected_iote_t *pNewNode = (connected_iote_t *)rt_malloc(sizeof(connected_iote_t));
    if (pNewNode == NULL)
    {
        return;
    }
    rt_memset(pNewNode, 0, sizeof(connected_iote_t));

    pNewNode->user_id = user_id;
    pNewNode->next = g_userHead->next;
    g_userHead->next = pNewNode;
}

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

void test_show_access_func(u32_t user_id)
{
    rt_kprintf("test_show_access_func user_id 0x%x accessed\n", user_id);
}

void test_show_drop_func(u32_t user_id)
{
    rt_kprintf("test_show_drop_func user_id 0x%x dropped\n", user_id);
}

void test_show_result(uc_send_recv_t *result)
{
    rt_kprintf("test_show_result result %d\n", result->result);
}

void test_show_report_data(u32_t user_id, u8_t *report_data, u32_t report_data_len)
{
//    u8_t *fake_data = NULL;

    rt_kprintf("test_show_report_data user_id 0x%x, reportData ", user_id);
    for (u16_t index = 0; index < report_data_len; index++)
    {
        rt_kprintf("%u ", *report_data++);
    }
    rt_kprintf(", reportDataLen %d\n", report_data_len);

    save_user_id_of_connected_iote(user_id);
    // fake_data = generate_fake_data(80, 10);
    // uc_wiota_paging_and_send_normal_data(fake_data, 80, &user_id, 1, 100, test_show_result);
    // rt_free(fake_data);
    // fake_data = NULL;
    // //for test send two
    // rt_thread_mdelay(100);
    // fake_data = generate_fake_data(80, 10);
    // uc_wiota_paging_and_send_normal_data(fake_data, 80, &user_id, 1, 100, test_show_result);
    // rt_free(fake_data);
    // fake_data = NULL;
}

// test! set single parameter
void test_set_single_parameter(void)
{
    //set systemId
    {
        u32_t systemId = 0x11223344;

        uc_wiota_set_system_id(systemId);
    }

    //set subsystemId
    {
        u32_t subsystemId = 0x21456981;

        uc_wiota_set_subsystem_id(subsystemId);
    }

    //set user id lenth
    {
        u32_t userIdLen = 8;

        uc_wiota_set_user_id_len(userIdLen);
    }

    //set pnNum
    {
        u8_t pnNum = 1;

        uc_wiota_set_pn_number(pnNum);
    }

    //set symbolLength
    {
        u8_t symbolLength = SYMBOL_LENGTH_256;

        uc_wiota_set_symbol_length(symbolLength);
    }

    //set dlUlRatio
    {
        u8_t dlUlRatio = DL_UL_ONE_TO_ONE;

        uc_wiota_set_dlul_ratio(dlUlRatio);
    }

    //set btValue
    {
        u8_t btValue = BT_VALUE_0_POINT_3;

        uc_wiota_set_bt_value(btValue);
    }

    //set groupNumber
    {
        u8_t groupNumber = GROUP_NUMBER_2;

        uc_wiota_set_group_number(groupNumber);
    }
}

// test! set/get all parameter
void test_set_all_para(void)
{
    //init dynamic parameter
    dynamic_para_t dynaPara =
        {
            .id_len = 0x1,
            .pn_num = 0x1,
            .symbol_length = SYMBOL_LENGTH_256,
            .dlul_ratio = DL_UL_ONE_TO_ONE,
            .bt_value = BT_VALUE_1_POINT_2,
            .group_number = GROUP_NUMBER_1,
            .ap_max_power = 21,
            .spectrum_idx = 3,
            .system_id = 0x11223344,
            .subsystem_id = 0x12345678,
            .na = {0},
        };
    uc_wiota_set_all_dynamic_parameter(&dynaPara);
}

void test_get_all_para(void)
{
    dynamic_para_t *dyna_para = uc_wiota_get_all_dynamic_parameter();
    rt_kprintf("test_get_all_para--systemId 0x%x, subSystemId 0x%x\n, idLen %d,  pnNum %d, symbolLength %d, dlUlRatio %d, btValue %d, groupNumber %d\n, ap_max_power %d, spectrum_idx %d",
               dyna_para->system_id, dyna_para->subsystem_id, dyna_para->id_len, dyna_para->pn_num, dyna_para->symbol_length, dyna_para->dlul_ratio, dyna_para->bt_value, dyna_para->group_number, dyna_para->ap_max_power, dyna_para->spectrum_idx);
    if (dyna_para != NULL)
    {
        rt_free(dyna_para); // !!!need be manually released after use
        dyna_para = NULL;
    }
}

// test! set dcxo
void test_set_dcxo(void)
{
    uc_wiota_set_dcxo(BAN_8288_DCXO);
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
    if (scan_info.result == UC_SUCCESS)
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
    // according to the scan result , cchoose the best and set
    // uc_wiota_set_frequency_point(fPoint[0]/* fPoint[x] */);
    rt_free(scan_info.data); // !!!need be manually released after use
    scan_info.data = NULL;
    g_freqPoint = fPoint[10];
    is_need_to_restart = TRUE;
}

// test! set frequency point
void test_set_frequency_point(void)
{
    uc_wiota_set_frequency_point(g_freqPoint);
}

// test! set hopping freq
void test_set_hopping_freq(void)
{
    uc_wiota_set_hopping_freq(g_freqPoint + 1);
    uc_wiota_set_hopping_mode(HOPPING_MODE_1); // hopping_mode_e
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
    uc_wiota_register_report_ul_data_callback(test_show_report_data);
}

// test! send normal data to iote
void test_send_normal_data(void)
{
    connected_iote_t *tempNode = g_userHead->next;
    u8_t *fake_data = NULL;

    if (tempNode == NULL)
    {
        rt_kprintf("no iote connected\n");
        return;
    }

    fake_data = generate_fake_data(120, 10);
    while (tempNode != NULL)
    {
        u32_t user_id = tempNode->user_id;
        if (UC_SUCCESS == uc_wiota_paging_and_send_normal_data(fake_data, 120, &user_id, 1, 10000, test_show_result))
        {
            rt_kprintf("send data to 0x%x suc!", user_id);
        }
        else
        {
            rt_kprintf("send data to 0x%x failed!", user_id);
        }
        tempNode = tempNode->next;
    }
    rt_free(fake_data);
    fake_data = NULL;
}

// test! send normal/ota broadcast data
void test_send_broadcast_data(broadcast_mode_e mode)
{
    u8_t *testData;
    u8_t *tempData = NULL;
    s32_t timeout = 10000; //ms
    uc_result_e result = UC_FAILED;
    u32_t offset = 0;

    if (NORMAL_BROADCAST == mode)
    {
        testData = generate_fake_data(50, 10);
        uc_wiota_send_broadcast_data(testData, 50, mode, timeout, NULL);
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
                if (result == UC_SUCCESS)
                {
                    offset += 1024;
                    rt_kprintf("test_send_broadcast_data send next broadcast %u\n", i);
                    break;
                }
            }
        }
    }
    else
    {
        rt_kprintf("test_send_broadcast_data invalid mode %u\n", mode);
        return;
    }

    rt_free(testData); // !!!need be manually released after use
    testData = NULL;
    rt_free(tempData);
    testData = NULL;
}

// test! query iote infomation
void test_query_iote_info(void)
{
    u16_t ioteNum = 0;
    iote_info_t *ioteInfo = NULL;

    ioteInfo = uc_wiota_query_active_iotes(&ioteNum);
    uc_wiota_print_iote_info(ioteInfo, ioteNum);
    if (ioteInfo != NULL)
    {
        rt_free(ioteInfo); // !!!need be manually released after use
        ioteInfo = NULL;
    }
}

// test! add iote to blacklist
void test_add_iote_to_blacklist(void)
{
    u32_t userIdArry[5] = {0x4c00ccdb, 0xfb3eae00, 0x38f8c8d8, 0x8aff8783, 0x33139955};
    u16_t userIdNum = sizeof(userIdArry) / sizeof(u32_t);
    blacklist_t *headNode = NULL;
    u16_t blacklistNum = 0;

    uc_wiota_add_iote_to_blacklist(userIdArry, userIdNum);
    headNode = uc_wiota_get_blacklist(&blacklistNum);
    uc_wiota_print_blacklist(headNode, blacklistNum);
    if (headNode != NULL)
    {
        rt_free(headNode); // !!!need be manually released after use
        headNode = NULL;
    }
}

// test! read temperature of ap8288
void test_read_temp(void)
{
    uc_temp_recv_t read_temp = {0};
    u16_t timeout = 10000;

    if (UC_SUCCESS == uc_wiota_read_temperature(NULL, &read_temp, timeout))
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
    s8_t rf_power = 24; //value range:-1~34
    uc_wiota_set_ap_max_power(rf_power);
}

// test! get version of sw
void test_get_version()
{
    u8_t version[24] = {0};
    u8_t time[20] = {0};

    uc_wiota_get_version(version, time);

    rt_kprintf("version %s\n", version);
    rt_kprintf("time %s\n", time);
}

// test! remove iote from blacklist
void test_remove_iote_from_blacklist(void)
{
    u32_t userIdArry[4] = {0x38f8c8d8, 0xf6149b11, 0x4eaac480, 0x4c00ccdb};
    u16_t userIdNum = sizeof(userIdArry) / sizeof(u32_t);
    blacklist_t *headNode = NULL;
    u16_t blacklistNum = 0;

    uc_wiota_remove_iote_from_blacklist(userIdArry, userIdNum);
    headNode = uc_wiota_get_blacklist(&blacklistNum);
    uc_wiota_print_blacklist(headNode, blacklistNum);
    if (headNode != NULL)
    {
        rt_free(headNode); // !!!need be manually released after use
        headNode = NULL;
    }
}

// test! wiota exit and restart
void test_wiota_exit_and_restart(void)
{
    uc_wiota_exit();
    user_id_list_deinit();
    rt_thread_mdelay(2000);
    uc_wiota_init();

    // test! set frequency point after wiota start, before wiota start
    test_set_frequency_point();

    // test! set dcxo after wiota init
    test_set_dcxo();

    // test! set/get connection timeout after wiota init, before wiota start
    test_set_connection_timeout();

    // test! set all dynamic parameter after wiota init, before wiota start
    // test_set_all_para();

    // test! set single parameter after wiota init, before wiota start
    // test_set_single_parameter();

    uc_wiota_start();
}

void app_interface_main_task(void *pPara)
{
    // wiota init
    uc_wiota_init();

    // user id list init
    user_id_list_init();

    // wiota start
    uc_wiota_start();

    // test! scan frequency point collection, after wiota start
    test_handle_scan_freq();

    // test! wiota exit and restart
    if (is_need_to_restart)
    {
        test_wiota_exit_and_restart();
        is_need_to_restart = FALSE;
    }

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
        // test_send_broadcast_data(NORMAL_BROADCAST);
        // test_send_broadcast_data(OTA_BROADCAST);

        // test! send normal data to iote
        // test_send_normal_data();

        // test! query iote information after wiota start
        test_query_iote_info();

        // test! read temperature of ap8288
        test_read_temp();

        rt_thread_mdelay(10000);
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