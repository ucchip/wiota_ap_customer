
#ifndef _UC_BOOT_WATCHDOG_H
#define _UC_BOOT_WATCHDOG_H

#include "pulpino.h"

#define WATCHDOG_BASE_ADDR_8288 (0x1A104200)
#define WATCHDOG_8288_REG(x) (*((volatile unsigned int*)(WATCHDOG_BASE_ADDR_8288 + (x))))
#define WATCHDOG_8088_REG(x) (*((volatile unsigned int*)(WATCHDOG_BASE_ADDR + (x))))

typedef enum {
    WDT_EN = 0x00,
    WDT_INIT_VAL = 0x04,
    WDT_FEED = 0x08
} WDT_REG;

#define WDG_ENABLE_MASK         0x1
#define WDG_FEED_MASK           0x1

#define UC_BOOT_WDT_INIT(chip, PERIOD_MS) \
do { \
    WATCHDOG_##chip##_REG(WDT_INIT_VAL) = 0xFFFFFFFFU - PERIOD_MS * 32768U / 1000; \
    WATCHDOG_##chip##_REG(WDT_FEED) |= WDG_FEED_MASK;   \
}while(0)

#define UC_BOOT_WDT_ENABLE(chip) \
do { \
    WATCHDOG_##chip##_REG(WDT_EN) |= WDG_ENABLE_MASK;   \
}while(0)

#define UC_BOOT_WDT_DISABLE(chip) \
do { \
    WATCHDOG_##chip##_REG(WDT_EN) &= ~(WDG_ENABLE_MASK);   \
}while(0)

#define UC_BOOT_WDT_FEED(chip) \
do { \
    WATCHDOG_##chip##_REG(WDT_FEED) |= WDG_FEED_MASK;   \
}while(0)
    
//#define PARAM_WDG(WDG)              (WDG==UC_WATCHDOG)
//void wdt_init(WDG_TYPE* WDG, uint32_t period_ms);
//void wdt_enable(WDG_TYPE* WDG);
//void wdt_disable(WDG_TYPE* WDG);
//void wdt_feed(WDG_TYPE* WDG);

#endif