// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

/**
 * @file
 * @brief GPIO library.
 *
 * Provides GPIO helper function like setting
 * input/output direction and reading/writing the pins.
 *
 */
#ifndef _GPIO_H_
#define _GPIO_H_

#include <pulpino.h>

typedef enum
{
    GPIO_PIN_0  = 0,
    GPIO_PIN_1  = 1,
    GPIO_PIN_2  = 2,
    GPIO_PIN_3  = 3,
    GPIO_PIN_4  = 4,
    GPIO_PIN_5  = 5,
    GPIO_PIN_6  = 6,
    GPIO_PIN_7  = 7,
    GPIO_PIN_8  = 8,
    GPIO_PIN_9  = 9,
    GPIO_PIN_10 = 10,
    GPIO_PIN_11 = 11,
    GPIO_PIN_12 = 12,
    GPIO_PIN_13 = 13,
    GPIO_PIN_14 = 14,
    GPIO_PIN_15 = 15,
    GPIO_PIN_16 = 16,
    GPIO_PIN_17 = 17,
    GPIO_PIN_18 = 18,
    GPIO_PIN_19 = 19,
    GPIO_PIN_20 = 20,
    GPIO_PIN_21 = 21,
    GPIO_PIN_22 = 22,
    GPIO_PIN_24 = 24,
    GPIO_PIN_25 = 25,
    GPIO_PIN_26 = 26,
    GPIO_PIN_27 = 27,
    GPIO_PIN_28 = 28,
    GPIO_PIN_29 = 29,
} GPIO_PIN; /* pin number */

typedef enum
{
    GPIO_FUNC_0 = 0, /* just normal gpio */
    GPIO_FUNC_1 = 1, /* the second multiplexing function */
    GPIO_FUNC_2 = 2, /* the third multiplexing function */
} GPIO_FUNCTION; /* multiplexing function */

typedef enum
{
    GPIO_DIR_OUT = 0,
    GPIO_DIR_IN  = 1,
} GPIO_DIRECTION; /* pin direction */

typedef enum
{
    GPIO_VALUE_LOW  = 0,
    GPIO_VALUE_HIGH = 1
} GPIO_VALUE;

typedef enum
{
    GPIO_IT_HIGH_LEVEL = 0x0,
    GPIO_IT_LOW_LEVEL  = 0x1,
    GPIO_IT_RISE_EDGE  = 0x2,
    GPIO_IT_FALL_EDGE  = 0x3
} GPIO_IRQ_TYPE;

typedef enum
{
    GPIO_PUPD_NONE = 0,
    GPIO_PUPD_UP   = 1
} GPIO_PUPD;

#define GPIO_REG_PADDIR         (GPIO_BASE_ADDR + 0x00)
#define GPIO_REG_PADIN          (GPIO_BASE_ADDR + 0x04)
#define GPIO_REG_PADOUT         (GPIO_BASE_ADDR + 0x08)
#define GPIO_REG_INTEN          (GPIO_BASE_ADDR + 0x0C)
#define GPIO_REG_INTTYPE0       (GPIO_BASE_ADDR + 0x10)
#define GPIO_REG_INTTYPE1       (GPIO_BASE_ADDR + 0x14)
#define GPIO_REG_INTSTATUS      (GPIO_BASE_ADDR + 0x18)

#define GPIO_REG_PADCFG0        (GPIO_BASE_ADDR + 0x20)
#define GPIO_REG_PADCFG1        (GPIO_BASE_ADDR + 0x24)
#define GPIO_REG_PADCFG2        (GPIO_BASE_ADDR + 0x28)
#define GPIO_REG_PADCFG3        (GPIO_BASE_ADDR + 0x2C)
#define GPIO_REG_PADCFG4        (GPIO_BASE_ADDR + 0x30)
#define GPIO_REG_PADCFG5        (GPIO_BASE_ADDR + 0x24)
#define GPIO_REG_PADCFG6        (GPIO_BASE_ADDR + 0x38)
#define GPIO_REG_PADCFG7        (GPIO_BASE_ADDR + 0x3C)

#define SOC_CTRL_PADFUN         (SOC_CTRL_BASE_ADDR + 0x00)

#define PADDIR                  REGP(GPIO_REG_PADDIR)
#define PADIN                   REGP(GPIO_REG_PADIN)
#define PADOUT                  REGP(GPIO_REG_PADOUT)
#define INTEN                   REGP(GPIO_REG_INTEN)
#define INTTYPE0                REGP(GPIO_REG_INTTYPE0)
#define INTTYPE1                REGP(GPIO_REG_INTTYPE1)
#define INTSTATUS               REGP(GPIO_REG_INTSTATUS)

#define PADCFG0                 REGP(GPIO_REG_PADCFG0)
#define PADCFG1                 REGP(GPIO_REG_PADCFG1)
#define PADCFG2                 REGP(GPIO_REG_PADCFG2)
#define PADCFG3                 REGP(GPIO_REG_PADCFG3)
#define PADCFG4                 REGP(GPIO_REG_PADCFG4)
#define PADCFG5                 REGP(GPIO_REG_PADCFG5)
#define PADCFG6                 REGP(GPIO_REG_PADCFG6)
#define PADCFG7                 REGP(GPIO_REG_PADCFG7)

#define PADFUN                  REGP(SOC_CTRL_PADFUN)

void gpio_set_pin_function(int pinnumber, int function);
int gpio_get_pin_function(int pinnumber);

void gpio_set_pin_direction(int pinnumber, int direction);
int gpio_get_pin_direction(int pinnumber);

void gpio_set_pin_value(int pinnumber, int value);
int gpio_get_pin_value(int pinnumber);
void gpio_set_pin_value_reverse(int pinnumber);

void gpio_set_pin_irq_type(int pinnumber, int type);
void gpio_set_pin_irq_en(int pinnumber, int enable);
int gpio_get_irq_status();

void gpio_init(void);
void gpio_deinit(void);
void gpio_set_init(uint8_t pin_number, uint8_t en_func, uint8_t en_pullup);
void gpio_set_pin_mux(GPIO_CFG_TypeDef* GPIO_CFG, GPIO_PIN pin, GPIO_FUNCTION func);
GPIO_FUNCTION gpio_get_pin_mux(GPIO_CFG_TypeDef* GPIO_CFG, GPIO_PIN pin);
void gpio_set_pin_pupd(GPIO_CFG_TypeDef *GPIO_CFG, GPIO_PIN pin, GPIO_PUPD pupd);
void soc_hw_ldo_on(void);
void gpoi_8088_to_8288_change_value();
#endif // _GPIO_H_
