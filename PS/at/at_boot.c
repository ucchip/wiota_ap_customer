#include <at.h>
#include <stdlib.h>
#include <stdint.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "ati_prs.h"
#include "uc_boot_uart.h"
#include "uc_boot_download.h"
#ifdef _WATCHDOG_APP_
#include "uc_watchdog_app.h"
#endif
//static at_result_t at_ymodem_exec(void)
//{
//    boot_set_modem(BOOT_SHARE_ENTER_DOWNLOAD);
//
//#ifdef _WATCHDOG_APP_
//    watchdog_app_disable();
//#endif
//
//    at_server_printfln("OK");
//
//    boot_uart_wait_tx_done();
//    rt_hw_interrupt_disable();
//
//    boot_riscv_reboot();
//    return AT_RESULT_NULL;
//}
//AT_CMD_EXPORT("AT+YMODEM", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_ymodem_exec);

#ifdef AT_USING_SERVER
static at_result_t at_ymodem_setup(const char *args)
{
    at_result_t ret = AT_RESULT_PARSE_FAILE;
    uint16_t usType = 0;

//    argc = at_req_parse_args(args, req_expr, &id);
    if (parse((char *)(++args), "d", &usType))
    {
        switch(usType)
        {
        case 8088:
            boot_set_modem(BOOT_SHARE_ENTER_DOWNLOAD);
            ret = AT_RESULT_OK;
            break;
        case 8288:
            boot_set_modem(BOOT_SHARE_8288_DOWNLOAD);
            ret = AT_RESULT_OK;
            break;
        default:
            break;
        }
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

static at_result_t at_reflash_exec(void)
{
    boot_set_modem(BOOT_SHARE_8288_REFLASH);

#ifdef _WATCHDOG_APP_
    watchdog_app_disable();
#endif

    at_server_printfln("OK");

    boot_uart_wait_tx_done();
    rt_hw_interrupt_disable();

    boot_riscv_reboot();
    return AT_RESULT_NULL;
}

AT_CMD_EXPORT("AT+YMODEM", "=<type>", RT_NULL, RT_NULL, at_ymodem_setup, RT_NULL);
AT_CMD_EXPORT("AT+REFLASH", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_reflash_exec);

#endif // #ifdef AT_USING_SERVER
