/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-26     RT-Thread    first version
 */

#ifdef UC8088_MODULE
#include "at.h"
#else
#include "uc_wiota_interface_test.h"
#endif
#ifdef _WATCHDOG_APP_
#include "uc_watchdog_app.h"
#endif

#define L1_TEST_SUBMODULE

void ExtISR()
{
    return;
}

int main(void)
{
#ifdef _WATCHDOG_APP_
    if (0 == watchdog_app_init())
    {
        watchdog_app_enable();
    }
#endif

#ifdef UC8088_MODULE
    at_server_init();
#else
    app_task_init();
#endif
}
