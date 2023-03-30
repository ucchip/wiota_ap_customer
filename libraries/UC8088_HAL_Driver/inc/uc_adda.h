#ifndef _UC_ADC_H
#define _UC_ADC_H

#include <stdbool.h>
#include "pulpino.h"

#define PARAM_CLK_DIV_RATE(clk_div) (clk_div <= (0x7FFF) && clk_div >= (0x00))
#define PARAM_ELE_LV_RATE(ele_lv) (ele_lv <= (0x3FF) && ele_lv >= (0x00))
#define PARAM_DAC_WT_RATE(dac_wt) (dac_wt <= (0x3FF) && dac_wt >= (0x00))
#define PARAM_ADDC(adda) (adda == UC_ADDA)

typedef enum
{
    ADC_CONFIG_CHANNEL_A = 1,
    ADC_CONFIG_CHANNEL_B,
    ADC_CONFIG_CHANNEL_C,
    ADC_CONFIG_CHANNEL_BAT, // 4
    ADC_CONFIG_CHANNEL_TEMP_A,
    ADC_CONFIG_CHANNEL_TEMP_B,
    ADC_CONFIG_CHANNEL_TEMP_C, // 7
} ADC_CHANNEL_CONFIG;

typedef enum
{
    ADC_CHANNEL_TEMP = 1 << 31, /* channel for temperature measure */
    ADC_CHANNEL_A = 1 << 30,    /* channel a */
    ADC_CHANNEL_B = 1 << 29,    /* channel b */
    ADC_CHANNEL_C = 1 << 28,    /* channel c, optimized for audio */
    ADC_CHANNEL_BAT = 1 << 2,   /* channel for battery voltage measure */
} ADC_CHANNEL;

typedef enum
{
    ADC_SR_360KSPS = 0 << 26,
    ADC_SR_180KSPS = 1 << 26,
    ADC_SR_90KSPS = 2 << 26,
    ADC_SR_45KSPS = 3 << 26,
} ADC_SAMPLE_RATE;

typedef enum
{
    ADC_TEMP_A = 1 << 26, /* environment temperature */
    ADC_TEMP_B = 1 << 25, /* body temperature */
    ADC_TEMP_C = 1 << 24, /* in-chip temperature */
} ADC_TEMP_SRC;

extern void adc_power_set(ADDA_TypeDef *ADDA);
extern void temperature_set(ADDA_TypeDef *ADDA);
extern void adc_reset(ADDA_TypeDef *ADDA);
extern void adc_set_sample_rate(ADDA_TypeDef *ADDA, ADC_SAMPLE_RATE sample_rate);
extern void adc_channel_select(ADDA_TypeDef *ADDA, ADC_CHANNEL CHANNEL);
extern void adc_int_enable(ADDA_TypeDef *ADDA);
extern void adc_int_disable(ADDA_TypeDef *ADDA);
extern void adc_int_clear_pending(void);
extern void adc_wait_data_ready(ADDA_TypeDef *ADDA);
extern uint16_t adc_read(ADDA_TypeDef *ADDA);
extern void adc_watermark_set(ADDA_TypeDef *ADDA, uint8_t water_mark);
extern bool is_adc_fifo_over_watermark(ADDA_TypeDef *ADDA);
extern bool is_adc_fifo_empty(ADDA_TypeDef *ADDA);
extern void adc_fifo_clear(ADDA_TypeDef *ADDA);
extern void adc_vbat_measure_enable(bool enable);
extern void adc_temp_source_sel(ADDA_TypeDef *ADDA, ADC_TEMP_SRC temp_src);
extern void adc_temp_sensor_enable(ADDA_TypeDef *ADDA, bool enable);
extern void dac_power_set(ADDA_TypeDef *ADDA);
extern void dac_fifo_clear(ADDA_TypeDef *ADDA);
extern void dac_watermark_set(ADDA_TypeDef *ADDA, uint8_t water_mark);
extern bool is_dac_fifo_over_watermark(ADDA_TypeDef *ADDA);
extern bool is_dac_fifo_full(ADDA_TypeDef *ADDA);
extern void dac_write(ADDA_TypeDef *ADDA, uint16_t wdata);
extern void dac_clkdiv_set(ADDA_TypeDef *ADDA, uint16_t clk_div);
extern void dac_int_enable(ADDA_TypeDef *ADDA);
extern void dac_int_disable(ADDA_TypeDef *ADDA);
extern void dac_int_clear_pending(void);
extern void auxdac_init(ADDA_TypeDef *ADDA);
extern void auxdac_level_set(ADDA_TypeDef *ADDA, uint16_t ele_level);
// void internal_temp_measure(ADDA_TypeDef *ADDA);
void avdd_cap_adj(void);
void temp_in_pt1000(ADDA_TypeDef *ADDA);
void temp_in_a(ADDA_TypeDef *ADDA);
#endif
