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
#ifndef _UC_GPIO_H_
#define _UC_GPIO_H_

#include <pulpino.h>

typedef enum{
    PIN_IN  = 1,
    PIN_OUT = 0
}GPIO_DIRECTION;

typedef enum{
    PIN_OUT_LOW  = 0,
    PIN_OUT_HIGH = 1
}GPIO_VALUE;

typedef enum{
    HIGH_TRIG = 0x0,
    LOW_TRIG  = 0x1,
    RISE_TRIG = 0x2,
    FALL_TRIG = 0x3
}GPIO_IRQ_TYPE;

typedef enum{
    PIN_DOWN = 0,
    PIN_UP   = 1
}PIN_STATUS;



typedef struct{
    uint8_t pinnumber;
    FunctionalState func;
    PIN_STATUS status;      /* parity bit enable */
    GPIO_DIRECTION direction;
}GPIO_CFG_Type;


#define PARAM_GPIO(gpio)                   ((gpio==UC_GPIO))

#define PARAM_GPIO_CFG(gpio_cfg)           ((gpio_cfg==UC_GPIO_CFG))
#define PARAM_GPIO_PIN(pin)                ((pin<=29)||(pin>=0))
#define PARAM_GPIO_FUNC(func)              ((func==ENABLE)||(func==DISABLE))
#define PARAM_GPIO_STATUS(status)          ((status==PIN_DOWN)||(status==PIN_UP))
#define PARAM_GPIO_DIRECTION(direction)    ((direction==PIN_IN)||(direction==PIN_OUT))
#define PARAM_GPIO_IRQ(irq_type)           ((irq_type==HIGH_TRIG)||(irq_type==LOW_TRIG) \
                                           ||(irq_type==RISE_TRIG)||(irq_type==FALL_TRIG))


void gpio_init(GPIO_TypeDef *GPIO,GPIO_CFG_TypeDef *GPIO_CFG,GPIO_CFG_Type *gpio_cfg);
void set_gpio_init(uint8_t pin_number, uint8_t en_func, uint8_t en_pullup);

void set_gpio_pin_direction(GPIO_TypeDef *GPIO, uint8_t pinnumber, GPIO_DIRECTION direction);
uint8_t get_gpio_pin_direction(GPIO_TypeDef *GPIO, uint8_t pinnumber);

void set_gpio_pin_value(GPIO_TypeDef *GPIO, uint8_t pinnumber, GPIO_VALUE value);
uint8_t get_gpio_pin_value(GPIO_TypeDef *GPIO, uint8_t pinnumber);

void set_gpio_pin_irq_type(GPIO_TypeDef *GPIO, uint8_t pinnumber, GPIO_IRQ_TYPE type);
void set_gpio_pin_irq_en(GPIO_TypeDef *GPIO, uint8_t pinnumber, uint8_t enable);
uint32_t get_gpio_irq_status(GPIO_TypeDef *GPIO);

void soc_hw_ldo_on(void);
void set_pin_function(int pinnumber, int function);

void gprs_io_init(void);
uint8_t gprs_io_read(uint8_t pin_num);
void gprs_io_write(uint8_t pin_num, GPIO_VALUE value);

#endif // _GPIO_H_
