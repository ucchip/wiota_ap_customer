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

static at_result_t at_uboot_setup(const char *args)
{
    at_result_t ret = AT_RESULT_PARSE_FAILE;
    uint16_t usType = 0;

    if (parse((char *)(++args), "s",1, &usType))
    {
            
            if (usType >= 'a' && usType <= 'g')
            {
                boot_set_mode(usType);
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
    at_result_t ret = AT_RESULT_PARSE_FAILE;
    
    uint8_t select_flag = 0;
    uint8_t log_flag = 0;
    uint8_t uart_flag = 0;

    args = parse((char *)(++args), "d,d,d", &select_flag, &log_flag, &uart_flag);
    if (!args)
    {
        ret =  AT_RESULT_PARSE_FAILE;
    }
    if ((0 == select_flag || 1 == select_flag) && (0 == log_flag || 1 == log_flag) && (0 == uart_flag || 1 == uart_flag))
    {
        boot_set_select_flag(select_flag);
        boot_set_log_flag(log_flag);
        boot_set_uart_flag(uart_flag);
        
        ret = AT_RESULT_OK;
    }

    if (AT_RESULT_OK == ret)
    {
        at_server_printfln("OK\n");
    }

    return ret;
}

AT_CMD_EXPORT("AT+UBOOT", "=<type>", RT_NULL, RT_NULL, at_uboot_setup, RT_NULL);
AT_CMD_EXPORT("AT+SETUBOOT", "=<select>,<log>,<uart>", RT_NULL, RT_NULL, at_uboot_config_set, RT_NULL);
#endif // AT_USING_SERVER
#endif // RT_USING_AT