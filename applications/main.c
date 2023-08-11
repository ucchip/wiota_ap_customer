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
#ifdef _ADC_APP_
#include "uc_adc_app.h"
#endif
#ifdef _DAC_APP_
#include "uc_dac_app.h"
#endif
#ifdef _IIC_APP_
#include "uc_iic_app.h"
#endif
#ifdef _PIN_APP_
#include "uc_pin_app.h"
#endif
#ifdef _PWM_APP_
#include "uc_pwm_app.h"
#endif
#ifdef _RTC_APP_
#include "uc_rtc_app.h"
#endif
#ifdef _SPI_FLASH_APP_
#include "uc_spi_flash_app.h"
#endif
#ifdef _SPIM_FLASH_APP_
#include "uc_spim_flash_app.h"
#endif
#ifdef _UART_APP_
#include "uc_uart_app.h"
#endif

void ExtISR()
{
    return;
}

#ifdef WIZ_USING_W5500
#include "drv_spi.h"
#include "gpio.h"
int uc_wiota_w5500_spi_init(void)
{
    rt_hw_spi_device_attach("spim", WIZ_SPI_DEVICE, RT_NULL, GPIO_PIN_3);

    return 0;
}
#endif

#ifdef PKG_USING_WIZNET
#include "wiz.h"
int uc_wiota_wiz_init(void)
{
    rt_thread_mdelay(100);

    char mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    wiz_config(mac, 0, RT_NULL, RT_NULL, RT_NULL);
    wiz_init();

    return 0;
}
#endif

void app_device_demo(void)
{
#ifdef _ADC_APP_
    adc_app_sample();
#endif

#ifdef _DAC_APP_
    dac_app_sample();
#endif

#ifdef _IIC_APP_
    iic_app_sample();
#endif

#ifdef _PIN_APP_
    pin_app_sample();
#endif

#ifdef _PWM_APP_
    pwm_app_sample();
#endif

#ifdef _RTC_APP_
    rtc_app_sample();
//    alarm_app_sample();
#endif

#ifdef _SPI_FLASH_APP_
    spi_flash_app_sample();
#endif

#ifdef _SPIM_FLASH_APP_
    spim_flash_app_sample();
#endif

#ifdef _UART_APP_
    uart_app_sample();
#endif
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
#ifdef RT_USING_AT
#ifdef AT_USING_SERVER
    at_server_init();
#endif // AT_USING_SERVER
#endif // RT_USING_AT
#endif // WIOTA_API_TEST
#endif // WIOTA_APP_DEMO

#ifdef WIZ_USING_W5500
    uc_wiota_w5500_spi_init();
#endif // WIZ_USING_W5500

#ifdef PKG_USING_WIZNET
    uc_wiota_wiz_init();
#endif // PKG_USING_WIZNET
    // extern void uc_wiota_time_service_demo(void);
    // uc_wiota_time_service_demo();
    app_device_demo();
}
