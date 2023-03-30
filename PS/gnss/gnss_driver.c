#include "rtthread.h"
#include "uc_adda.h"
#include "uc_uart.h"

//purpose: read temper
void dac_output_voltage(float vol);
void auxdac_output_voltage(float vol);
void internal_temp_measure(ADDA_TypeDef *ADDA);
void dc_off_control(int control);

void dac_output_voltage(float vol)
{
    uint16_t val;

    val = vol / 1.6 * 1024; //formula
    dac_power_set(UC_ADDA);
    dac_clkdiv_set(UC_ADDA, 60);
    dac_watermark_set(UC_ADDA, 128);

    dac_fifo_clear(UC_ADDA);
    for (int i = 0; i < 8; i++)
    {
        dac_write(UC_ADDA, val);
    }

    //delay 10ms
    for (int i = 0; i < 131 * 10 * 1000L; i++)
    {
        asm("nop");
    }
}

void auxdac_output_voltage(float vol)
{
    uint16_t val;

    val = vol / 1.6 * 1024; //formula

    auxdac_level_set(UC_ADDA, val);

    //delay 10ms
    for (int i = 0; i < 131 * 10 * 1000L; i++)
    {
        asm("nop");
    }
}

void internal_temp_measure(ADDA_TypeDef *ADDA)
{
    REG(0x1a104230) |= 1 << 22;
    REG(0x1a104230) |= 1 << 20; //set avdd_cap 1.6v

    ADDA->ADC_CTRL0 |= 0xC9FF1080;
    ADDA->ADC_CTRL1 |= 0x1C540280; //0x1C540280
}

void dc_off_control(int control)
{
    uint32_t *ptr = (uint32_t *)(0x1a109004);
    if (control)
        *ptr |= 1 << 28;
    if (!control)
        *ptr &= ~(1 << 28);
}
//GNSS update from th <===20221102====================

void UartSend(const uint8_t *pData, uint16_t usLen)
{
    // uart_send(pData, usLen);
    rt_kprintf("%s", pData);
}
