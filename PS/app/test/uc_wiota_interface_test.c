/*
 * uc_wiota_interface_test.c
 *
 *  Created on: 2021.08.02
 *  Author: jpwang
 */

#include "uc_wiota_interface.h"
#include "uc_wiota_interface_test.h"
#include "rtthread.h"

void uc_wiota_show_access_func(u32_t user_id)
{
    rt_kprintf("uc_wiota_show_access_func user_id 0x%x accessed\n", user_id);
}

void uc_wiota_show_drop_func(u32_t user_id)
{
    rt_kprintf("uc_wiota_show_drop_func user_id 0x%x dropped\n", user_id);
}

void uc_wiota_show_report_data(u32_t user_id, u8_t *report_data, u32_t report_data_len)
{
    rt_kprintf("uc_wiota_show_report_data user_id 0x%x, reportData ", user_id);
    for (u16_t index = 0; index < report_data_len; index++)
    {
        rt_kprintf("%u", *report_data++);
    }
    rt_kprintf(", reportDataLen %d\n", report_data_len);
}

void uc_wiota_show_result(uc_result_e result)
{
    rt_kprintf("uc_wiota_show_result result %d\n", result);
}

void uc_wiota_set_single_parameter(void)
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

void test_app_interface_main_task(void* pPara)
{
    // u16_t timeout = 100;

    //init dynamic parameter
    dynamic_para_t dynaPara =
    {
        .reserved = 0x1,
        .id_len = 4,
        .pn_num = 1,
        .symbol_length = SYMBOL_LENGTH_128,
        .dlul_ratio = DL_UL_ONE_TO_ONE,
        .bt_value = BT_VALUE_1_POINT_2,
        .group_number = GROUP_NUMBER_1,
        .system_id = 0x11223344,
        .subsystem_id = 0x21456981,
        .na = {0},
    };

    uc_wiota_init();
#if 0
    // scan frequency point collection
    {
        u8_t fPoint[8] = {100, 101, 102, 103 ,104, 105, 106, 107};

        uc_wiota_scan_frequency_point_collection(fPoint, sizeof(fPoint)/sizeof(u32_t), timeout, uc_wiota_show_result);
    }
#endif
    //set dcxo
    // {
    //     u32_t dcxo = ...;

    //     uc_wiota_set_dcxo(dcxo);
    // }

    //set frequency point
    {
        u8_t fPoint = 100;
        uc_wiota_set_frequency_point(fPoint);
    }

    //set all dynamic parameter
    uc_wiota_set_all_dynamic_parameter(&dynaPara);

#if 0//can be userd as a reference
    //set single parameter
    uc_wiota_set_single_parameter();
#endif

    //wiota start
    uc_wiota_start();

    //register callback
    uc_wiota_register_iote_access_callback(uc_wiota_show_access_func);
    uc_wiota_register_iote_dropped_callback(uc_wiota_show_drop_func);
    uc_wiota_register_proactively_report_data_callback(uc_wiota_show_report_data);

    // TRACE_PRINTF("test_app_interface_main_task start set tick\n");
    // //set timeout period
    // {
    //     u8_t tick = 15;

    //     uc_wiota_set_iote_idle_tick(tick);
    // }

    //add iote to blacklist
    {
        u32_t userIdArry[5] = {0x4c00ccdb, 0xfb3eae00, 0x38f8c8d8, 0x8aff8783, 0x33139955};
        u16_t userIdNum = sizeof(userIdArry)/sizeof(u32_t);
        blacklist_t *headNode = NULL;
        u16_t blacklistNum = 0;

        uc_wiota_add_iote_to_blacklist(userIdArry, userIdNum);
        headNode = uc_wiota_get_blacklist(&blacklistNum);
        uc_wiota_print_blacklist(headNode, blacklistNum);
        if (headNode != NULL)
        {
            rt_free(headNode);
            headNode = NULL;
        }
    }

    //remove iote from blacklist
    {
        u32_t userIdArry[4] = {0x38f8c8d8, 0xf6149b11, 0x4eaac480, 0x4c00ccdb};
        u16_t userIdNum = sizeof(userIdArry)/sizeof(u32_t);
        blacklist_t *headNode = NULL;
        u16_t blacklistNum = 0;

        uc_wiota_remove_iote_from_blacklist(userIdArry, userIdNum);
        headNode = uc_wiota_get_blacklist(&blacklistNum);
        uc_wiota_print_blacklist(headNode, blacklistNum);
        if (headNode != NULL)
        {
            rt_free(headNode);
            headNode = NULL;
        }
    }

    while (1)
    {
        // TRACE_PRINTF("test_app_interface_main_task start send data\n");
        //send broadcast data
        // u8_t testData[20] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};

        // uc_wiota_send_broadcast_data(testData, sizeof(testData)/sizeof(u8_t), timeout, uc_wiota_show_result);

        //query iote infomation
        u16_t ioteNum = 0;
        iote_info_t *ioteInfo = NULL;

        ioteInfo = uc_wiota_query_info_of_currently_connected_iote(&ioteNum);
        uc_wiota_print_iote_info(ioteInfo, ioteNum);
        if (ioteInfo != NULL)
        {
            rt_free(ioteInfo);
            ioteInfo = NULL;
        }

        rt_thread_mdelay(20000);
    }
    return;
}

void app_task_init(void)
{
    rt_thread_t testTaskHandle = rt_thread_create("test_app", test_app_interface_main_task, NULL, 1024, 3, 3);
    if (testTaskHandle != NULL)
    {
        rt_thread_startup(testTaskHandle);
    }
}