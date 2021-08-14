#include "uc_addc.h"
#include "pulpino.h"

void adc_power_set(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    #if 0
    ADDA->ADC_CTRL0 &= ~(1<<31);
    ADDA->ADC_CTRL0 |= 0x087F11C0;
    ADDA->ADC_CTRL1  = 0x0C020000;
    #else
    ADDA->ADC_CTRL0 = 0x287F1084;
    ADDA->ADC_CTRL1  = 0x0C140280;
    uint32_t *ptr = (uint32_t *)0x1a104230;
    //*ptr |= 1 << 22;
    *ptr &= ~(1 << 22);
    #endif
}

void dac_power_set(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    
    ADDA->ADC_CTRL0 &= ~(1<<31);
    ADDA->ADC_CTRL0 |= 0x708800;
}

void aux_dac_power_set(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    
    ADDA->ADC_CTRL0 &= ~(1<<31);
    ADDA->ADC_CTRL0 |= 0x70CA00;
}

void temperature_set(ADDA_TypeDef *ADDA)
{
    ADDA->ADC_CTRL0 = 0x8AFF1080;
    ADDA->ADC_CTRL1 = 0x0C540280;  //0x0C340280 2 PT1000
}

void adc_channel_select(ADDA_TypeDef *ADDA, ADC_CHANNEL CHANNEL)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    
    ADDA->ADC_CTRL0 |= CHANNEL;
}

void adc_reset(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    
    ADDA->ADC_CTRL0 &= ~(1<<8);

    ADDA->ADC_CTRL0 |= 1<<8;    
}

void adda_int_enable(ADDA_TypeDef *ADDA,ADDA_EN ad_or_da)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    
    ADDA->ADDA_IRQ_CTRL = ad_or_da;
}

void dac_clkdiv_set(ADDA_TypeDef *ADDA, uint16_t clk_div)
{
    CHECK_PARAM(PARAM_CLK_DIV_RATE(clk_div));
    CHECK_PARAM(PARAM_ADDC(ADDA));
    
    ADDA->DAC_CLK_DIV = clk_div << 1;
}

void dac_level_set(ADDA_TypeDef *ADDA, uint16_t ele_level)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    CHECK_PARAM(PARAM_ELE_LV_RATE(ele_level));
    
    ADDA->AUX_DAC_LV = ele_level;
}

uint16_t adc_read(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    
    return ADDA->ADC_FIFO_READ;
}

void dac_write(ADDA_TypeDef *ADDA, uint16_t wdata)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    CHECK_PARAM(PARAM_DAC_WT_RATE(wdata));
    
    ADDA->DAC_FIFO_WRITE = wdata;
}

void aux_dac_write(uint16_t wdata)
{
    CHECK_PARAM(PARAM_DAC_WT_RATE(wdata));
    volatile uint32_t *ptr;
    ptr = (volatile uint32_t *)0x1A109020;
    *ptr = wdata;
}

void adc_watermark_set(ADDA_TypeDef *ADDA, uint8_t water_mark)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->ADC_FIFO_CTRL = water_mark << 8;
}

bool is_adc_fifo_over_watermark(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    int over_watermark = (ADDA->ADC_FIFO_CTRL >> 17) & 0x1;
    if(over_watermark)
        return true;
    else
        return false;
}

void adc_fifo_clear(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->ADC_FIFO_CTRL |= 1 << 31;
}

void dac_watermark_set(ADDA_TypeDef *ADDA, uint8_t water_mark)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->DAC_FIFO_CTRL = water_mark << 8;
}

bool is_dac_fifo_over_watermark(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    
    int over_watermark = (ADDA->DAC_FIFO_CTRL >> 17) & 0x1;
    if(over_watermark)
        return true;
    else
        return false;
}

void dac_fifo_clear(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->DAC_FIFO_CTRL |= 1 << 31;
}
