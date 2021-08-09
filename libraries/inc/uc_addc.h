#ifndef _UC_ADC_H
#define _UC_ADC_H

#include <stdbool.h>
#include "pulpino.h"

#define PARAM_CLK_DIV_RATE(clk_div)         (clk_div <= (0x7FFF) && clk_div >= (0x00))
#define PARAM_ELE_LV_RATE(ele_lv)           (ele_lv  <= (0x3FF)  && ele_lv  >= (0x00))
#define PARAM_DAC_WT_RATE(dac_wt)           (dac_wt  <= (0x3FF)  && dac_wt  >= (0x00))
#define PARAM_ADDC(adda)                     (adda    == UC_ADDA)

typedef enum{
    ADC_INT  = 0x1,
    DAC_INT  = 0x2,
    ADDA_INT = 0x3
}ADDA_EN;

typedef enum{
    CHANNEL_A = 1<<30,
    CHANNEL_B = 1<<29,
    CHANNEL_C = 1<<28
}ADC_CHANNEL;

void adc_reset(ADDA_TypeDef *ADDA);
void adc_power_set(ADDA_TypeDef *ADDA);
void temperature_set(ADDA_TypeDef *ADDA);
void dac_power_set(ADDA_TypeDef *ADDA);
void adc_channel_select(ADDA_TypeDef *ADDA, ADC_CHANNEL CHANNEL);
void adda_int_enable(ADDA_TypeDef *ADDA,ADDA_EN ad_or_da);
void dac_clkdiv_set(ADDA_TypeDef *ADDA, uint16_t clk_div);
void dac_level_set(ADDA_TypeDef *ADDA, uint16_t ele_level);
uint16_t adc_read(ADDA_TypeDef *ADDA);
void dac_write(ADDA_TypeDef *ADDA, uint16_t wdata);
void adc_watermark_set(ADDA_TypeDef *ADDA, uint8_t water_mark);
bool is_adc_fifo_over_watermark(ADDA_TypeDef *ADDA);
void adc_fifo_clear(ADDA_TypeDef *ADDA);
void dac_watermark_set(ADDA_TypeDef *ADDA, uint8_t water_mark);
bool is_dac_fifo_over_watermark(ADDA_TypeDef *ADDA);
void dac_fifo_clear(ADDA_TypeDef *ADDA);
void aux_dac_power_set(ADDA_TypeDef *ADDA);
void aux_dac_write(uint16_t wdata);


#endif
