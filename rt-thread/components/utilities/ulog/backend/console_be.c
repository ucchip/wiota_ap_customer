/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-04     armink       the first version
 */

#include <rthw.h>
#include <ulog.h>

#include "app_config.h"
#include "ulog_easyflash.h"
#include "RTT/SEGGER_RTT.h"

#ifdef ULOG_BACKEND_USING_CONSOLE

#if defined(ULOG_ASYNC_OUTPUT_BY_THREAD) && ULOG_ASYNC_OUTPUT_THREAD_STACK < 384
#error "The thread stack size must more than 384 when using async output by thread (ULOG_ASYNC_OUTPUT_BY_THREAD)"
#endif

static struct ulog_backend console;
static struct ulog_backend server;
static struct ulog_backend plc;
static struct ulog_backend jlink;

void ulog_console_backend_output(struct ulog_backend *backend, rt_uint32_t level, const char *tag, rt_bool_t is_raw,
        const char *log, size_t len)
{
    /*读取通道配置*/
    uint16_t wr_type = 0;
    if (get_dev_appconfig_data(I_CF_AP_LOG_WR_TYPE, &wr_type) < 0)
    {
        return;
    }
    if (!(wr_type & 0x0001))
    {
        return;
    }

#ifdef RT_USING_DEVICE
    rt_device_t dev = rt_console_get_device();

    if (dev == RT_NULL)
    {
        rt_hw_console_output(log);
    }
    else
    {
        rt_uint16_t old_flag = dev->open_flag;

        dev->open_flag |= RT_DEVICE_FLAG_STREAM;
        rt_device_write(dev, 0, log, len);
        dev->open_flag = old_flag;
    }
#else
    rt_hw_console_output(log);
#endif

}

void ulog_server_backend_output(struct ulog_backend *backend, rt_uint32_t level, const char *tag, rt_bool_t is_raw,
        const char *log, size_t len)
{
    void send_log_to_server(const char *log);

    char *buff = (char *)malloc(len + 1);
    rt_memset(buff, 0, len + 1);
    rt_memcpy(buff, log, len);
    send_log_to_server(buff);
    free(buff);
}

void ulog_plc_backend_output(struct ulog_backend *backend, rt_uint32_t level, const char *tag, rt_bool_t is_raw,
        const char *log, size_t len)
{
    /*读取通道配置*/
    uint16_t wr_type = 0;
    if (get_dev_appconfig_data(I_CF_AP_LOG_WR_TYPE, &wr_type) < 0)
    {
        return;
    }
    if (!(wr_type & 0x0008))
    {
        return;
    }

    char *buff = (char *)malloc(len + 1);
    rt_memset(buff, 0, len + 1);
    rt_memcpy(buff, log, len);
    rt_kprintf("\nplc log : %s",buff);
    free(buff);
}

void ulog_jlink_backend_output(struct ulog_backend *backend, rt_uint32_t level, const char *tag, rt_bool_t is_raw,
        const char *log, size_t len)
{
    /*读取通道配置*/
    uint16_t wr_type = 0;
    if (get_dev_appconfig_data(I_CF_AP_LOG_WR_TYPE, &wr_type) < 0)
    {
        return;
    }
    if (!(wr_type & 0x0020))
    {
        return;
    }

    char *buff = (char *)malloc(len + 1);
    rt_memset(buff, 0, len + 1);
    rt_memcpy(buff, log, len);
    SEGGER_RTT_printf(0, buff);
    free(buff);
}

int ulog_console_backend_init(void)
{
    ulog_init();
    console.output = ulog_console_backend_output;

    ulog_backend_register(&console, "console", RT_FALSE);

    //注册后端输出到主服务器
    server.output = ulog_server_backend_output;
    ulog_backend_register(&server, "server", RT_FALSE);

    //注册后端输出到PLC
    plc.output = ulog_plc_backend_output;
    ulog_backend_register(&plc, "plc", RT_FALSE);

    //注册后端输出到jlink
    jlink.output = ulog_jlink_backend_output;
    ulog_backend_register(&jlink, "jlink", RT_FALSE);

    uint8_t log_level = 0;
    if (get_dev_appconfig_data(I_CF_AP_LOG_LEVEL, &log_level) == 0)
    {
        ulog_ef_log_lvl_set(log_level);
        ulog_global_filter_lvl_set(log_level);
    }
    return 0;
}
//INIT_PREV_EXPORT(ulog_console_backend_init);

#endif /* ULOG_BACKEND_USING_CONSOLE */
