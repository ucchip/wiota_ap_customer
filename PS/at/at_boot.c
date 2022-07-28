#include <at.h>
#include <stdlib.h>
#include <stdint.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "ati_prs.h"
#include "uc_boot_uart.h"
#include "uc_uboot.h"
#ifdef _WATCHDOG_APP_
#include "uc_watchdog_app.h"
#endif

#ifdef AT_USING_SERVER
static at_result_t at_uboot_setup(const char *args)
{
    at_result_t ret = AT_RESULT_PARSE_FAILE;
    uint16_t usType = 0;

    if (parse((char *)(++args), "d", &usType))
    {
            
            if (usType > 0 && usType < 8)
            {
                boot_set_mode(usType +'a'-1);
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

AT_CMD_EXPORT("AT+UBOOT", "=<type>", RT_NULL, RT_NULL, at_uboot_setup, RT_NULL);
#endif // #ifdef AT_USING_SERVER
