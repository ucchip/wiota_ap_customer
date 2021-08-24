// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include <gpio.h>
#include <stdio.h>
#include <sectdefs.h>
#include "uc_event.h"
#include "uc_int.h"
//#include "vsi.h"
//#include "scheduler_frame_for_gpio.h"

extern void scheduler_tick_notify(void);

void set_pin_function(int pinnumber, int function)
{
    volatile int old_function;

    old_function = *(volatile int*) (SOC_CTRL_PADFUN);
    old_function = old_function & (~(1 << pinnumber));
    old_function = old_function | (function << pinnumber);
    *(volatile int*) (SOC_CTRL_PADFUN) = old_function;
}

int get_pin_function(int pinnumber)
{
    volatile int old_function;
    old_function = *(volatile int*) (SOC_CTRL_PADFUN);
    old_function = (old_function >> pinnumber & 0x01);
    return old_function;
}

void set_gpio_pin_direction(int pinnumber, int direction)
{
    volatile int old_dir;
    old_dir = *(volatile int*) (GPIO_REG_PADDIR);
    if (direction == 0)
        old_dir &= ~(1 << pinnumber);
    else
        old_dir |= 1 << pinnumber;
    *(volatile int*) (GPIO_REG_PADDIR) = old_dir;
}

int get_gpio_pin_direction(int pinnumber)
{
    volatile int old_dir;
    old_dir = *(volatile int*) (GPIO_REG_PADDIR);
    old_dir = (old_dir >> (pinnumber) & 0x01);
    return old_dir;

}

void set_gpio_pin_value(int pinnumber, int value)
{
    volatile int v;
    v = *(volatile int*) (GPIO_REG_PADOUT);
    if (value == 0)
        v &= ~(1 << pinnumber);
    else
        v |= 1 << pinnumber;
    *(volatile int*) (GPIO_REG_PADOUT) = v;
}

int get_gpio_pin_value(int pinnumber)
{
    volatile int v;
    v = *(volatile int*) (GPIO_REG_PADIN);
    v = (v >> pinnumber) & 0x01;
    return v;
}

void set_gpio_pin_value_reverse(int pinnumber)
{
    static int last_value = 0;
    if (last_value)
    {
        set_gpio_pin_value(pinnumber, 0);
        last_value = 0;
    }
    else
    {
        set_gpio_pin_value(pinnumber, 1);
        last_value = 1;
    }
}

void set_gpio_pin_irq_en(int pinnumber, int enable)
{
    int v;
    v = *(volatile int*) (GPIO_REG_INTEN);
    if (enable == 0)
        v &= ~(1 << pinnumber);
    else
        v |= 1 << pinnumber;
    *(volatile int*) (GPIO_REG_INTEN) = v;
}

void set_gpio_pin_irq_type(int pinnumber, int type)
{
    int type0;
    int type1;

    type0 = *(volatile int*) (GPIO_REG_INTTYPE0);
    type1 = *(volatile int*) (GPIO_REG_INTTYPE1);

    if ((type & 0x1) == 0)
        type0 &= ~(1 << pinnumber);
    else
        type0 |= 1 << pinnumber;

    if ((type & 0x2) == 0)
        type1 &= ~(1 << pinnumber);
    else
        type1 |= 1 << pinnumber;

    *(volatile int*) (GPIO_REG_INTTYPE0) = type0;
    *(volatile int*) (GPIO_REG_INTTYPE1) = type1;
}

int get_gpio_irq_status()
{
    return *(volatile int*) (GPIO_REG_INTSTATUS);
}

#define PIN_NUMBER  24

void gpio_init(void)
{
    int_enable();
    set_gpio_pin_direction(PIN_NUMBER, DIR_IN);
    set_gpio_pin_irq_type(PIN_NUMBER, GPIO_IRQ_RISE);
    set_gpio_pin_irq_en(PIN_NUMBER, 1);

    enable_event_iqr(GPIO_INT_ID);
    return;
}

void gpio_deinit(void)
{
    int_disable();
    set_gpio_pin_irq_en(PIN_NUMBER, 0);
    disable_event_iqr(GPIO_INT_ID);
    return;
}

//unsigned int g_gpio_count = 0;
void ISR_GPIO(void)
{
    int temp = *(volatile int*) (GPIO_REG_INTSTATUS);

    if (0 != ((1 << PIN_NUMBER) & temp))
    {
//        g_gpio_count = *(u32_t*)0x3b0014;
        scheduler_tick_notify();
    }

    ICP |= 1 << GPIO_INT_ID;
    //enable_event_iqr(GPIO_INT_ID);

    return;
}
