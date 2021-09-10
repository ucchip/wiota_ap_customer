
#ifndef __BOARD__
#define __BOARD__

#include <stdint.h>

#include "drv_onchip_flash.h"

#define BSP_USING_UART0
#define BSP_USING_UART1

/*-------------------------- ROM/RAM CONFIG BEGIN --------------------------*/

#define ROM_START              ((uint32_t)0x00000000)
#define ROM_SIZE               (2048 * 1024)
#define ROM_END                ((uint32_t)(ROM_START + ROM_SIZE))

#define RAM_START              (0x00300000)
#define RAM_SIZE               (256 * 1024)
#define RAM_END                (RAM_START + RAM_SIZE)

/*-------------------------- CLOCK CONFIG BEGIN --------------------------*/

//#define BSP_CLOCK_SOURCE                  ("HSI")
//#define BSP_CLOCK_SOURCE_FREQ_MHZ         ((int32_t)0)
//#define BSP_CLOCK_SYSTEM_FREQ_HZ         ((int32_t)131000000)
#define BSP_CLOCK_SYSTEM_FREQ_HZ         ((int32_t)131072000)

/*-------------------------- CLOCK CONFIG END --------------------------*/

extern uint32_t _end;
extern uint32_t _heap_end;
#define HEAP_BEGIN  &_end
//#define HEAP_END    &_heap_end
//#define HEAP_END    (HEAP_BEGIN + 0x20000)
#define HEAP_END    (RAM_END - 0x3400)

#ifdef RT_USING_RTC
void rtc_calibrate(uint8_t auto_calib, uint32_t *freq_val, uint32_t *bias_val);
#endif
void uc8088_chip_reset(void);

void uc8088_systick_init(void);

void timer1_compare_handler(void);

#endif
