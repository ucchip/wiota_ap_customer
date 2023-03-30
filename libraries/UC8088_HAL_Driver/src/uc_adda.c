#include "uc_adda.h"
#include "pulpino.h"
#include "uc_event.h"

#define _AUXDAC_BUFF_OUTPUT_EN 1

#define AVDD_CAP_TRIM 5 //AVDD_CAP=1.589V
#define ADC_VCM_TRIM 5
#define ADC_VOLTAGE_TRIM 12

#define BIT(x) (1 << (x))

static void avdd_cap_calibrate(ADDA_TypeDef *ADDA)
{
    REG(0x1A104230) = (REG(0x1A104230) & (~(0x0f << 20))) | (ADC_VOLTAGE_TRIM << 20); //calibrate voltage

    ADDA->ADC_CTRL1 &= ~(0x0F << 7);
    ADDA->ADC_CTRL1 |= AVDD_CAP_TRIM << 7;

    ADDA->ADC_CTRL1 = (ADDA->ADC_CTRL1 & (~(0x07 << 18))) | (ADC_VCM_TRIM << 18); //set vcm trim
}

void adc_power_set(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));

    avdd_cap_calibrate(ADDA);
    ADDA->ADC_CTRL0 &= ~(1 << 31);

    ADDA->ADC_CTRL0 &= ~(0x0F << 28);                                                                                            //disable channel a, channel b, channel c and inside temp channel
    ADDA->ADC_CTRL0 |= BIT(22) | BIT(21) | BIT(20) | BIT(18) | BIT(17) | BIT(16) | BIT(12) | BIT(7) | BIT(6) | BIT(27) | BIT(8); //|BIT(19);

    ADDA->ADC_CTRL1 &= ~BIT(2); //disable battery channel
    ADDA->ADC_CTRL1 |= BIT(17); // |BIT(27)|BIT(26);

    //ADDA->ADC_CTRL1 &= ~(0x0F<<7);
    //ADDA->ADC_CTRL1 |= AVDD_CAP_TRIM<<7;

    ADDA->ADC_CTRL1 &= ~(0x03 << 26);
    ADDA->ADC_CTRL1 |= 0x03 << 26; //adc_clk=26M/32, adc_sample_rate=adc_clk/18

    //ADDA->ADC_CTRL1 =  (ADDA->ADC_CTRL1 & (~(0x07<<18))) | (ADC_VCM_TRIM << 18);//set vcm trim
}

void temperature_set(ADDA_TypeDef *ADDA)
{
    ADDA->ADC_CTRL0 = 0x8AFF1080;
    ADDA->ADC_CTRL1 = 0x0C540280; //0x0C340280 2 PT1000
}

void adc_set_sample_rate(ADDA_TypeDef *ADDA, ADC_SAMPLE_RATE sample_rate)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->ADC_CTRL1 = (ADDA->ADC_CTRL1 & (~(0x03 << 26))) | sample_rate;
}

void adc_channel_select(ADDA_TypeDef *ADDA, ADC_CHANNEL CHANNEL)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->ADC_CTRL0 &= ~(0x0F << 28); //disable channel a, channel b, channel c and inside temp channel
    ADDA->ADC_CTRL1 &= ~BIT(2);       //disable battery channel

    if (CHANNEL != ADC_CHANNEL_BAT)
    {
        ADDA->ADC_CTRL0 |= CHANNEL;
    }
    else
    {
        ADDA->ADC_CTRL1 |= CHANNEL;
    }
}

void adc_reset(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));

    ADDA->ADC_CTRL0 &= ~(1 << 8);
    //delay 10us
    for (int i = 0; i < 131 * 10; i++)
    {
        asm("nop");
    }
    ADDA->ADC_CTRL0 |= 1 << 8;
}

void adc_int_enable(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->ADDA_IRQ_CTRL |= (1 << 0);
    IER |= (1 << 20);
}

void adc_int_disable(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->ADDA_IRQ_CTRL &= ~(1 << 0);
    IER &= ~(1 << 20);
}

void adc_int_clear_pending(void)
{
    ICP |= 1 << 20;
}

void adc_wait_data_ready(ADDA_TypeDef *ADDA)
{
    while ((ADDA->ADC_FIFO_CTRL & (1 << 19)) != 0) //wait data ready
    {
        asm("nop");
    }
}

uint16_t adc_read(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));

    return ADDA->ADC_FIFO_READ;
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
    if (over_watermark)
        return true;
    else
        return false;
}

bool is_adc_fifo_empty(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    if (((ADDA->ADC_FIFO_CTRL >> 19) & 0x1) == 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void adc_fifo_clear(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->ADC_FIFO_CTRL |= 1 << 31;
    ADDA->ADC_FIFO_CTRL &= ~BIT(31); //must clear the bit manually
}

void adc_vbat_measure_enable(bool enable)
{
    REG(0x1A104228) = (REG(0x1A104228) & (~BIT(4))) | (enable << 4);
    REG(0X1a104228) = (REG(0x1A104228) & (~BIT(23))) | (enable << 23);
}

void adc_temp_source_sel(ADDA_TypeDef *ADDA, ADC_TEMP_SRC temp_src)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->ADC_CTRL0 &= ~(0x07 << 24);
    ADDA->ADC_CTRL0 |= temp_src;
}

void adc_temp_sensor_enable(ADDA_TypeDef *ADDA, bool enable)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->ADC_CTRL0 &= ~BIT(6);                                           //disable imax mode
    ADDA->ADC_CTRL0 |= BIT(27) | BIT(21) | BIT(19);                       //enable vcm for temp measure, ADC_TEMP power and ADC_TEMP current limit
    ADDA->ADC_CTRL0 = (ADDA->ADC_CTRL0 & (~BIT(23))) | (enable << 23);    //enable ADC_TEMP sensor
    ADDA->ADC_CTRL0 &= ~0x07;                                             //set ADC_TEMP internal trim
    ADDA->ADC_CTRL1 = (ADDA->ADC_CTRL1 & (~(0x03 << 21))) | (0x03 << 21); //set ADC_TEMP PGA gain
                                                                          //ADDA->ADC_CTRL1 =  (ADDA->ADC_CTRL1 & (~(0x07<<18))) | (0x05 << 18);//set vcm trim
                                                                          //REG(0x1A104230) = (REG(0x1A104230) & (~(0x0f<<20))) | (12<<20);//calibrate voltage
}

// void internal_temp_measure(ADDA_TypeDef *ADDA)
// {
//     ADDA->ADC_CTRL0 = 0x89F71080;
//     ADDA->ADC_CTRL1 = 0x0C740280;
// }

void avdd_cap_adj(void)
{
    uint32_t *ptr = (uint32_t *)(0x1a104230);
    //	*ptr |= 1<<22;
    //	*ptr |= 1<<21;
    *ptr |= 1 << 20;
    *ptr &= ~(1 << 13);
}

void temp_in_pt1000(ADDA_TypeDef *ADDA)
{
    ADDA->ADC_CTRL0 = 0x8AFF1080;
    ADDA->ADC_CTRL1 = 0x0C540280;
}

void temp_in_a(ADDA_TypeDef *ADDA)
{
    REG(0x1a104230) |= 1 << 22;
    REG(0x1a104230) |= 1 << 20;

    ADDA->ADC_CTRL0 = 0x8CFF1180;
    ADDA->ADC_CTRL1 = 0x5C540280;;
}

void dac_power_set(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));

    avdd_cap_calibrate(ADDA);

    ADDA->ADC_CTRL0 &= ~(1 << 31);
    ADDA->ADC_CTRL0 |= 0x708800;

    ADDA->ADC_CTRL1 &= ~(0x0F << 7);
    ADDA->ADC_CTRL1 |= AVDD_CAP_TRIM << 7;
}

void dac_clkdiv_set(ADDA_TypeDef *ADDA, uint16_t clk_div)
{
    CHECK_PARAM(PARAM_CLK_DIV_RATE(clk_div));
    CHECK_PARAM(PARAM_ADDC(ADDA));

    ADDA->DAC_CLK_DIV = clk_div << 1;
}

void dac_int_enable(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->ADDA_IRQ_CTRL |= (1 << 1);
    IER |= (1 << 21);
}

void dac_int_disable(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->ADDA_IRQ_CTRL &= ~(1 << 1);
    IER &= ~(1 << 21);
}

void dac_int_clear_pending(void)
{
    ICP |= 1 << 21;
}

void dac_write(ADDA_TypeDef *ADDA, uint16_t wdata)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    CHECK_PARAM(PARAM_DAC_WT_RATE(wdata));

    ADDA->DAC_FIFO_WRITE = wdata;
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
    if (over_watermark)
        return true;
    else
        return false;
}

bool is_dac_fifo_full(ADDA_TypeDef *ADDA)
{
    int full = (ADDA->DAC_FIFO_CTRL >> 18) & 0x1;
    if (full)
        return true;
    else
        return false;
}

void dac_fifo_clear(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    ADDA->DAC_FIFO_CTRL |= 1 << 31;
    ADDA->DAC_FIFO_CTRL &= ~BIT(31); //must clear the bit manually
}

void auxdac_init(ADDA_TypeDef *ADDA)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    avdd_cap_calibrate(ADDA);

    ADDA->ADC_CTRL0 |= BIT(21); //AUXDAC LDO enable, shared with ADC_TEMP
    ADDA->ADC_CTRL0 |= BIT(14); //AUXDAC enable

#if _AUXDAC_BUFF_OUTPUT_EN
    ADDA->ADC_CTRL0 |= BIT(13); //AUXDAC BUFF output enable
    ADDA->ADC_CTRL0 &= ~BIT(9); //AUXDAC output without through BUFF disable
#else
    ADDA->ADC_CTRL0 &= ~BIT(13); //AUXDAC BUFF output disable
    ADDA->ADC_CTRL0 |= BIT(9);   //AUXDAC output without through BUFF enable
#endif

    //ADDA->ADC_CTRL1 &= ~(0x0F<<7);
    //ADDA->ADC_CTRL1 |= AVDD_CAP_TRIM<<7;
}

void auxdac_level_set(ADDA_TypeDef *ADDA, uint16_t ele_level)
{
    CHECK_PARAM(PARAM_ADDC(ADDA));
    CHECK_PARAM(PARAM_ELE_LV_RATE(ele_level));

    ADDA->AUX_DAC_LV = ele_level;
}
