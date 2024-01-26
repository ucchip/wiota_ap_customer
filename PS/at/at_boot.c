#include <rtthread.h>
#ifdef RT_USING_AT
#ifdef AT_USING_SERVER
#include <at.h>
#include <stdlib.h>
#include <stdint.h>
#include <rthw.h>
#include <rtdevice.h>
#include "ati_prs.h"
#include "uc_boot_uart.h"
#include "uc_uboot.h"
#ifdef _WATCHDOG_APP_
#include "uc_watchdog_app.h"
#endif

typedef enum
{
    UBOOT_SELECT_FLAG_SET = 0,
    UBOOT_LOG_FLAG_SET = 1,
    UBOOT_UART_FLAG_SET = 2,
    UBOOT_FILE_SIZE_SET = 3
} set_uboot_e;

static at_result_t at_uboot_setup(const char *args)
{
    at_result_t ret = AT_RESULT_PARSE_FAILE;
    unsigned int mode = 0;

    if (parse((char *)(++args), "s", 1, &mode))
    {

        if (mode >= 'a' && mode <= 'g')
        {
            boot_set_mode(mode);
            ret = AT_RESULT_OK;
        }
        else
            ret = AT_RESULT_FAILE;
    }

    if (AT_RESULT_OK == ret)
    {
#ifdef _WATCHDOG_APP_
        watchdog_app_disable();
#endif
        at_server_printfln("OK\n");

        boot_uart_wait_tx_done();
        rt_hw_interrupt_disable();

        boot_riscv_reboot();
    }

    return ret;
}

static at_result_t at_uboot_config_set(const char *args)
{
    unsigned int type = 0;
    unsigned int value = 0;

    args = parse((char *)(++args), "d,d", &type, &value);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    switch (type)
    {
    case UBOOT_SELECT_FLAG_SET:
        boot_set_select_flag(value);
        break;

    case UBOOT_LOG_FLAG_SET:
        boot_set_log_flag(value);
        break;

    case UBOOT_UART_FLAG_SET:
        boot_set_uart_flag(value);
        break;

    case UBOOT_FILE_SIZE_SET:
        boot_set_file_size(value);
        break;

    default:
        break;
    }

    return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+UBOOT", "=<mode>", RT_NULL, RT_NULL, at_uboot_setup, RT_NULL);
AT_CMD_EXPORT("AT+SETUBOOT", "=<type>,<value>", RT_NULL, RT_NULL, at_uboot_config_set, RT_NULL);
#endif // AT_USING_SERVER
#endif // RT_USING_AT