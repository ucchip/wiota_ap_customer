/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-26     RT-Thread    first version
 */

#include <rtthread.h>
#include "uc_wiota_static.h"
#ifdef UC8088_MODULE
#ifdef RT_USING_AT
#include "at.h"
#endif
#else
#ifndef WIOTA_APP_DEMO
#include "test_wiota_api.h"
#endif
#endif
#ifdef _WATCHDOG_APP_
#include "uc_watchdog_app.h"
#endif
#ifdef WIOTA_APP_DEMO
#include "manager_app.h"
#endif
#define L1_TEST_SUBMODULE
#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
extern void at_handle_log_uart(int uart_number);
#endif

void ExtISR()
{
    return;
}

int main(void)
{
    uc_wiota_static_data_init();
#ifdef _WATCHDOG_APP_
    if (RT_EOK == watchdog_app_init())
    {
        watchdog_app_enable();
    }
#endif
#ifdef WIOTA_APP_DEMO
    manager_enter();
#else
#ifdef UC8088_MODULE
#ifdef AT_USING_SERVER
    at_server_init();
#endif
#else
    app_task_init();
#endif
#endif
}
