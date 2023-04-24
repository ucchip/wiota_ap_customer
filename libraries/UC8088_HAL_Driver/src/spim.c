// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
#include <spim.h>
#include <gpio.h>
#include <sectdefs.h>
#include <stdint.h>

#if 0
void spim_setup_slave() {
    gpio_set_pin_function(PIN_SSPIM_SIO0, FUNC_SPI);
    gpio_set_pin_function(PIN_SSPIM_SIO1, FUNC_SPI);
    gpio_set_pin_function(PIN_SSPIM_SIO2, FUNC_SPI);
    gpio_set_pin_function(PIN_SSPIM_SIO3, FUNC_SPI);
    gpio_set_pin_function(PIN_SSPIM_CSN, FUNC_SPI);
}

#endif

void spim_init(SPI_TypeDef *SPIx, SPIM_CFG_Type *SPI_ConfigStruc)
{
    CHECK_PARAM(PARAM_SPIM(SPIx));
    spim_setup_master(1);
    SPIx->CLKDIV = SPI_ConfigStruc->Clk_rate;
}

void spim_setup_master(int numcs)
{
    // uint32_t * pad_mux = (uint32_t *)0x1a107000;
    // uint32_t * pad_cfg = (uint32_t *)0x1a107020;
    // // (*pad_mux) |= 0x3F00; //pad 8-13
    // (*pad_mux) |= 0x2700; //pad 8-13,no 11 12

    //    pad_cfg += 2; //8-11;
    //    *pad_cfg =0x01010100; //pin 9-11 pull up
    //    pad_cfg += 1;
    //    *pad_cfg = 0x01; //12-13 pull up

    gpio_set_pin_function(8, 1);
    gpio_set_pin_function(9, 1);
    gpio_set_pin_function(10, 1);
    if (numcs == 0)
        gpio_set_pin_function(13, 1);

    uint32_t *pad_cfg = (uint32_t *)0x1a107020;
    (*pad_cfg) |= (1 << 8);
    (*pad_cfg) |= (1 << 9);
    (*pad_cfg) |= (1 << 10);

    // rt_kprintf("gpio 8:%d\n",gpio_get_pin_function(8));
    // rt_kprintf("gpio 9:%d\n",gpio_get_pin_function(9));
    // rt_kprintf("gpio 12:%d\n",gpio_get_pin_function(12));
}

void spim_send_data_noaddr(int cmd, char *data, int datalen, int useQpi);

void spim_setup_cmd_addr(int cmd, int cmdlen, int addr, int addrlen)
{
    int cmd_reg;
    cmd_reg = cmd << (32 - cmdlen);
    *(volatile int *)(SPIM_REG_SPICMD) = cmd_reg;
    *(volatile int *)(SPIM_REG_SPIADR) = addr;
    *(volatile int *)(SPIM_REG_SPILEN) = (cmdlen & 0x3F) | ((addrlen << 8) & 0x3F00);
}

void spim_setup_dummy(int dummy_rd, int dummy_wr)
{
    *(volatile int *)(SPIM_REG_SPIDUM) = ((dummy_wr << 16) & 0xFFFF0000) | (dummy_rd & 0xFFFF);
}

void spim_set_datalen(int datalen)
{
    volatile int old_len;
    old_len = *(volatile int *)(SPIM_REG_SPILEN);
    old_len = ((datalen << 16) & 0xFFFF0000) | (old_len & 0xFFFF);
    *(volatile int *)(SPIM_REG_SPILEN) = old_len;
}

void spim_start_transaction(int trans_type, int csnum)
{
    *(volatile int *)(SPIM_REG_STATUS) = ((1 << (csnum + 8)) & 0xF00) | ((1 << trans_type) & 0xFF);
}

int spim_get_status()
{
    volatile int status;
    status = *(volatile int *)(SPIM_REG_STATUS);
    return status;
}

void spim_write_fifo(int *data, int datalen)
{
    volatile int num_words, i;

    num_words = (datalen >> 5) & 0x7FF;

    if ((datalen & 0x1F) != 0)
        num_words++;

    for (i = 0; i < num_words; i++)
    {
        while ((((*(volatile int *)(SPIM_REG_STATUS)) >> 24) & 0xFF) >= 8)
            ;
        *(volatile int *)(SPIM_REG_TXFIFO) = data[i];
    }
}

void spim_read_fifo(int *data, int datalen)
{
    volatile int num_words;
    /* must use register for, i,j*/
    register int i, j;
    num_words = (datalen >> 5) & 0x7FF;

    if ((datalen & 0x1F) != 0)
        num_words++;
    i = 0;
    while (1)
    {
        do
        {
            j = (((*(volatile int *)(SPIM_REG_STATUS)) >> 16) & 0xFF);
        } while (j == 0);
        do
        {
            data[i++] = *(volatile int *)(SPIM_REG_RXFIFO);
            j--;
        } while (j);
        if (i >= num_words)
            break;
    }
}

/* last function in spi lib, linked to bootstrap code.
 * calling this cause cache to fill 2nd block, so we have
 * 2 blocks of code in cache */
