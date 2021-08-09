// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include <uc_gpio.h>

void gpio_init(GPIO_TypeDef *GPIO,GPIO_CFG_TypeDef *GPIO_CFG,GPIO_CFG_Type *gpio_cfg)
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_CFG(GPIO_CFG));
    CHECK_PARAM(PARAM_GPIO_PIN(gpio_cfg->pinnumber));
    CHECK_PARAM(PARAM_GPIO_FUNC(gpio_cfg->func));
    CHECK_PARAM(PARAM_GPIO_STATUS(gpio_cfg->status));
    
    //set pin mux
    if(gpio_cfg->func)
        GPIO_CFG->PADMUX |= (1 << gpio_cfg->pinnumber);
    else
        GPIO_CFG->PADMUX &= ~(1 << gpio_cfg->pinnumber); 
    
    //set pin direction
    if(gpio_cfg->direction)
        GPIO->PADDIR &= ~(1 << gpio_cfg->pinnumber); 
    else
        GPIO->PADDIR |= (1 << gpio_cfg->pinnumber); 
    
    //set pin status
    if(gpio_cfg->status)
        GPIO_CFG->PADCFG |= (1 << gpio_cfg->pinnumber);
    else
        GPIO_CFG->PADCFG &= ~(1 << gpio_cfg->pinnumber);
        
}

void set_gpio_init(uint8_t pin_number, uint8_t en_func, uint8_t en_pullup)
{
    volatile GPIO_CFG_TypeDef *GPIO_CFG = (GPIO_CFG_TypeDef *)SOC_CTRL_BASE_ADDR;
    if (pin_number > 31)
    {
        return;
    }
    
    //set pin mux
    if(en_func)
        GPIO_CFG->PADMUX |= (1 << pin_number);
    else
        GPIO_CFG->PADMUX &= ~(1 << pin_number); 
        
    //set pin status
    if(en_pullup)
        GPIO_CFG->PADCFG |= (1 << pin_number);
    else
        GPIO_CFG->PADCFG &= ~(1 << pin_number);        
}


void set_gpio_pin_direction(GPIO_TypeDef *GPIO, uint8_t pinnumber, GPIO_DIRECTION direction) 
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_PIN(pinnumber));
    CHECK_PARAM(PARAM_GPIO_DERECTION(direction));
    
    
    if(direction == 0)
        GPIO->PADDIR &= ~(1 << pinnumber); 
    else
        GPIO->PADDIR |= 1 << pinnumber; 
}

uint8_t get_gpio_pin_direction(GPIO_TypeDef *GPIO, uint8_t pinnumber) 
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_PIN(pinnumber));
    
    uint8_t pin_direction = (GPIO->PADDIR >> pinnumber) & 0x01;
    return pin_direction;
}

void set_gpio_pin_value(GPIO_TypeDef *GPIO, uint8_t pinnumber, GPIO_VALUE value) 
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_PIN(pinnumber));
    
    if(value == 0)
        GPIO->PADOUT &= ~(1 << pinnumber);
    else
        GPIO->PADOUT |= 1 << pinnumber;
}

uint8_t get_gpio_pin_value(GPIO_TypeDef *GPIO, uint8_t pinnumber)
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_PIN(pinnumber));
    
	uint8_t output_pin_value = 0;
	uint8_t pin_direction = get_gpio_pin_direction(GPIO, pinnumber);
	
	if(pin_direction)
		output_pin_value = (GPIO->PADIN >> pinnumber) & 0x01;
	else
		output_pin_value = (GPIO->PADOUT >> pinnumber) & 0x01;
		
    return output_pin_value;
}

void set_gpio_pin_irq_en(GPIO_TypeDef *GPIO, uint8_t pinnumber, uint8_t enable) 
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_PIN(pinnumber));
    
    if(enable == 0)
        GPIO->INTEN &= ~(1 << pinnumber);
    else
        GPIO->INTEN |= 1 << pinnumber;
}

void set_gpio_pin_irq_type(GPIO_TypeDef *GPIO, uint8_t pinnumber, GPIO_IRQ_TYPE type) 
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_PIN(pinnumber));
    CHECK_PARAM(PARAM_GPIO_IRQ(type));
    
    if((type & 0x1) == 0)
        GPIO->INTTYPE0 &= ~(1 << pinnumber);
    else
        GPIO->INTTYPE0 |= 1 << pinnumber;
        
    if((type & 0x2) == 0)
        GPIO->INTTYPE1 &= ~(1 << pinnumber);
    else
        GPIO->INTTYPE1 |= 1 << pinnumber;
}

uint32_t get_gpio_irq_status(GPIO_TypeDef *GPIO) 
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    
    return GPIO->INTSTATUS;
}

/* set ldo to 3.3v */
void soc_hw_ldo_on(void)
{
    unsigned int ldo_re = 0xe00000;
    int a = (*((volatile unsigned int *)(0x1a10422c)));
    int b = (a | ldo_re);
    *(volatile unsigned int *)(0x1a10422c) = b;
}

#define SOC_CTRL_PADFUN     (SOC_CTRL_BASE_ADDR + 0x00)

void set_pin_function(int pinnumber, int function)
{
    volatile int old_function;
    //int addr;

    old_function = *(volatile int *)(SOC_CTRL_PADFUN);
    old_function = old_function & (~(1 << pinnumber));
    old_function = old_function | (function << pinnumber);
    *(volatile int *)(SOC_CTRL_PADFUN) = old_function;
}

static void gprs_io_delay(int value)
{
    int i,j;
    for(i=0;i<100;i++)
        for(j=0;j<100;j++)
            ;
}

static void gprs_io_store(uint32_t addr, uint32_t data)
{
    volatile uint32_t *ptr = (uint32_t *)addr;
    *ptr = data;
}

static void gprs_io_load(uint32_t addr, uint32_t *data)
{
    volatile uint32_t *ptr = (uint32_t *)addr;
    *data = *ptr ;
}

static volatile uint32_t gprs_io_out_reg = 0;

void gprs_io_init(void)
{
    uint32_t CUR_TIME = 0;
        
    gprs_io_store(0x3B0004, 0x30408000);
    gprs_io_load(0x3B0014, &CUR_TIME);
    gprs_io_store(0x3B0180, CUR_TIME+64);
    gprs_io_store(0x3B0180, 0x30000000);   
    gprs_io_store(0x3B076C, 0x80000001);
    gprs_io_delay(150);
    gprs_io_load(0x3B0014, &CUR_TIME);
    gprs_io_store(0x3B0180, CUR_TIME+64);
    gprs_io_store(0x3B0180, 0x20100008);
    gprs_io_store(0x3B0180, CUR_TIME+128);
    //gprs_io_store(0x3B0180, 0x20301100);
    gprs_io_store(0x3B0180, 0x20300100);
    gprs_io_out_reg = 0x20300100;
}

uint8_t gprs_io_read(uint8_t pin_num)
{
    uint8_t ret_val = PIN_OUT_LOW;
    uint8_t offset = 0xff;
    
    if (pin_num == 134)
    {
        /* Pin:34 TX_EN */
        offset = 15;
    }
    else if (pin_num == 136)
    {
        /* Pin:36 CTRL_0 */
        offset = 12;
    }

    if (offset != 0xff)
    {
        if (gprs_io_out_reg & (1 << offset))
        {
            ret_val = PIN_OUT_HIGH;
        }
    }

    return ret_val;
}

void gprs_io_write(uint8_t pin_num, GPIO_VALUE value)
{
    uint32_t CUR_TIME = 0;
    uint8_t offset = 0xff;

    if (pin_num == 134)
    {
        /* Pin:34 TX_EN */
        offset = 15;
    }
    else if (pin_num == 136)
    {
        /* Pin:36 CTRL_0 */
        offset = 12;
    }

    if (offset == 0xff)
    {
        return;
    }

    if (value)
    {
        gprs_io_out_reg |= 1 << offset;
    }
    else
    {
        gprs_io_out_reg &= ~(1 << offset);
    }
        
    gprs_io_load(0x3B0014, &CUR_TIME);
    gprs_io_store(0x3B0180, CUR_TIME+64);
    gprs_io_store(0x3B0180, gprs_io_out_reg);
}

