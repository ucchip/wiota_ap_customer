/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-04-01     armink       first version
 * 2018-04-04     chenyong     add base commands
 */

#include <at.h>
#include "ati_prs.h"
#include <stdlib.h>
#include <string.h>
#include <rtdevice.h>
#include <board.h>
#include "uc_uboot.h"

#ifdef AT_USING_SERVER

#define AT_ECHO_MODE_CLOSE 0
#define AT_ECHO_MODE_OPEN 1

extern at_server_t at_get_server(void);

static at_result_t at_exec(void)
{
    return AT_RESULT_OK;
}
AT_CMD_EXPORT("AT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_exec);

//static at_result_t atz_exec(void)
//{
//    at_server_printfln("OK");
//
//    at_port_factory_reset();
//
//    return AT_RESULT_NULL;
//}
//AT_CMD_EXPORT("ATZ", RT_NULL, RT_NULL, RT_NULL, RT_NULL, atz_exec);

static at_result_t at_rst_exec(void)
{
    at_server_printfln("OK");
    rt_thread_mdelay(100);

    uc8088_chip_reset();

    return AT_RESULT_OK;
}
AT_CMD_EXPORT("AT+RST", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_rst_exec);

// static at_result_t ate_setup(const char *args)
// {
//     int echo_mode = atoi(args);

//     if (echo_mode == AT_ECHO_MODE_CLOSE || echo_mode == AT_ECHO_MODE_OPEN)
//     {
//         at_get_server()->echo_mode = echo_mode;
//     }
//     else
//     {
//         return AT_RESULT_FAILE;
//     }

//     return AT_RESULT_OK;
// }
// AT_CMD_EXPORT("ATE", "<value>", RT_NULL, RT_NULL, ate_setup, RT_NULL);

static at_result_t at_show_cmd_exec(void)
{
    extern void rt_at_server_print_all_cmd(void);

    rt_at_server_print_all_cmd();

    return AT_RESULT_OK;
}
AT_CMD_EXPORT("AT&L", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_show_cmd_exec);

static at_result_t at_uart_test(void)
{
    at_server_printfln("AT+UART=<baudrate[%d/%d/%d/%d]>", BAUD_RATE_9600, BAUD_RATE_115200, BAUD_RATE_460800, BAUD_RATE_921600);

    return AT_RESULT_OK;
}

static at_result_t at_uart_query(void)
{
    struct rt_serial_device *serial = (struct rt_serial_device *)at_get_server()->device;

    at_server_printfln("+UART:%d", serial->config.baud_rate);

    return AT_RESULT_OK;
}

static at_result_t at_uart_setup(const char *args)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    int baudrate;

    args = parse((char *)(++args), "d", &baudrate);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    int bd_array[4] = {BAUD_RATE_9600, BAUD_RATE_115200, BAUD_RATE_460800, BAUD_RATE_921600};
    for (int i = 0; i < sizeof(bd_array) / sizeof(int); i++)
    {
        if (baudrate == bd_array[i])
        {
            config.baud_rate = baudrate;
            if (rt_device_control(at_get_server()->device, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK)
            {
                return AT_RESULT_FAILE;
            }

            boot_set_uart0_baud_rate((unsigned int)baudrate);
            return AT_RESULT_OK;
        }
    }

    return AT_RESULT_CMD_ERR;
}

AT_CMD_EXPORT("AT+UART", "=<baudrate>", at_uart_test, at_uart_query, at_uart_setup, RT_NULL);

#endif /* AT_USING_SERVER */
