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

#ifdef RT_USING_AT
#include "at.h"
#endif
#ifdef WIOTA_API_TEST
#include "test_wiota_api.h"
#endif
#ifdef _WATCHDOG_APP_
#include "uc_watchdog_app.h"
#endif
#ifdef WIOTA_APP_DEMO
#include "manager_app.h"
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
#endif // _WATCHDOG_APP_
#ifdef WIOTA_APP_DEMO
    manager_enter();
#else
#ifdef WIOTA_API_TEST
    wiota_api_test();
#else
#ifdef M8_GATEWAY_MODE_SUPPORT
    extern int uc_wiota_mac_init(void);
    if (0 == uc_wiota_mac_init())
    {
        rt_kprintf("uc_wiota_mac_init suc\n");
    }
#endif
#ifdef RT_USING_AT
#ifdef AT_USING_SERVER
    at_server_init();
#endif // AT_USING_SERVER
#endif // RT_USING_AT
#endif // WIOTA_API_TEST
#endif // WIOTA_APP_DEMO

    // extern void uc_wiota_time_service_demo(void);
    // uc_wiota_time_service_demo();
}
