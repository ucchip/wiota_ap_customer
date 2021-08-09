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
#include <rtdbg.h>
//#include <board.h>
#include <rtdevice.h>
#include "trcPortDefines.h"
#include "trcRecorder.h"
#include "trace_interface.h"
#include "uc_wiota_interface_test.h"

#define  L1_TEST_SUBMODULE

void ExtISR()
{
//    return NULL;
    return ;

}

int main(void)
{
    vTraceEnable(TRC_START_AWAIT_HOST);
    rt_thread_idle_sethook(trace_control);

    app_task_init();
}
