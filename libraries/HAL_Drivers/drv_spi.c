/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-5      SummerGift   first version
 * 2018-12-11     greedyhao    Porting for stm32f7xx
 * 2019-01-03     zylx         modify DMA initialization and spixfer function
 * 2020-01-15     whj4674672   Porting for stm32h7xx
 */

#include "board.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>

#ifdef RT_USING_SPI

#include "drv_spi.h"
//#include "drv_config.h"
#include "spim.h"
#include "gpio.h"

//#define DRV_DEBUG
#define LOG_TAG "drv.spi"
#include <drv_log.h>

#ifdef RT_USING_SFUD
#include <spi_flash_sfud.h>
#endif

#define SPIM_CMD_RD 0
#define SPIM_CMD_WR 1
#define SPIM_CMD_QRD 2
#define SPIM_CMD_QWR 3
#define SPIM_CMD_SWRST 4

#define SPIM_CSN0 0
#define SPIM_CSN1 1
#define SPIM_CSN2 2
#define SPIM_CSN3 3

struct uc8088_hw_spi_cs
{
    GPIO_TypeDef *GPIOx;
    uint16_t GPIO_Pin;
};

static struct rt_spi_bus uc8088_spi_bus;

static rt_err_t uc8088_spi_init(struct rt_spi_configuration *cfg)
{
    RT_ASSERT(cfg != RT_NULL);
    SPIM_CFG_Type SPI_ConfigStruc;
    uint32_t clk_dlv = 255;

    if (cfg->max_hz > 0)
    {
        clk_dlv = BSP_CLOCK_SYSTEM_FREQ_HZ / cfg->max_hz / 2;
        if (clk_dlv > 0)
        {
            clk_dlv -= 1;
        }
        if (clk_dlv > 255)
        {
            clk_dlv = 255;
        }
    }
    SPI_ConfigStruc.Clk_rate = clk_dlv & 0xff;
    spim_init(UC_SPIM, &SPI_ConfigStruc);

    LOG_D("%s init done", "spim");

    return RT_EOK;
}

static rt_uint32_t spixfer(struct rt_spi_device *device, struct rt_spi_message *message)
{
    rt_size_t message_length, already_send_length;
    rt_uint16_t send_length;
    rt_uint8_t *recv_buf;
    const rt_uint8_t *send_buf;
    int8_t state = 0;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);
    //RT_ASSERT(device->bus->parent.user_data != RT_NULL);
    RT_ASSERT(message != RT_NULL);

    struct uc8088_hw_spi_cs *cs = device->parent.user_data;

    if (message->cs_take)
    {
        gpio_set_pin_value(cs->GPIO_Pin, GPIO_VALUE_LOW);
    }

    LOG_D("%s transfer prepare and start", "spim");
    LOG_D("%s sendbuf: %X, recvbuf: %X, length: %d",
          "spim",
          (uint32_t)message->send_buf,
          (uint32_t)message->recv_buf, message->length);

    message_length = message->length;
    recv_buf = message->recv_buf;
    send_buf = message->send_buf;
    while (message_length)
    {
        /* the HAL library use uint16 to save the data length */
        if (message_length > 65535)
        {
            send_length = 65535;
            message_length = message_length - 65535;
        }
        else
        {
            send_length = message_length;
            message_length = 0;
        }

        /* calculate the start address */
        already_send_length = message->length - send_length - message_length;
        send_buf = (rt_uint8_t *)message->send_buf + already_send_length;
        recv_buf = (rt_uint8_t *)message->recv_buf + already_send_length;

        /* start once data exchange in DMA mode */
        if (message->send_buf && message->recv_buf)
        {
            //rt_kprintf("spixfer 1 len = %d\r\n", send_length);
            uint32_t offset = 0;

            spim_setup_cmd_addr(0, 0, 0, 0);
            while (offset < send_length)
            {
                uint8_t index = 0;
                uint8_t order_size = 0;
                uint32_t data_len = 0;
                uint8_t data_buf[32];
                uint8_t index_offset = 0;
                if ((send_length - offset) > 32)
                {
                    order_size = 32;
                }
                else
                {
                    order_size = send_length - offset;
                }
                data_len = order_size * 8;
                memset(data_buf, 0x00, 32);
                for (index = 0; index < order_size; index++)
                {
                    data_buf[(index / 4) * 4 + (3 - index % 4)] = send_buf[offset + index];
                }
                spim_set_datalen(data_len);
                spim_setup_dummy(0, 0);
                spim_write_fifo((int *)data_buf, data_len);
                for (int i = 0; i < 10; i++)
                    for (int j = 0; j < 1000; j++)
                        ;

                //spim_start_transaction(UC_SPIM,SPIM_CMD_WR, cs->GPIO_Pin);
                spim_start_transaction(SPIM_CMD_WR, SPIM_CSN0);
                while ((spim_get_status() & 0xFFFF) != 1)
                    ;
                memset(data_buf, 0xff, 32);
                spim_read_fifo((int *)data_buf, data_len);
                for (index_offset = 0; index_offset < order_size; index_offset += 4)
                {
                    uint8_t count = 0;
                    if ((index_offset + 4) > order_size)
                    {
                        count = order_size - index_offset;
                    }
                    else
                    {
                        count = 4;
                    }
                    for (index = index_offset; index < (index_offset + count); index++)
                    {
                        recv_buf[offset + index] = data_buf[(index / 4) * 4 + ((count - 1) - (index - index_offset) % count)];
                    }
                }

                offset += order_size;
            }
        }
        else if (message->send_buf)
        {
            //rt_kprintf("spixfer 2 len = %d\r\n", send_length);
            uint32_t offset = 0;

            spim_setup_cmd_addr(0, 0, 0, 0);
            while (offset < send_length)
            {
                uint8_t index = 0;
                uint8_t order_size = 0;
                uint32_t data_len = 0;
                uint8_t data_buf[32];
                if ((send_length - offset) > 32)
                {
                    order_size = 32;
                }
                else
                {
                    order_size = send_length - offset;
                }
                data_len = order_size * 8;
                if (GPIO_PIN_13 == cs->GPIO_Pin)
                {
                    spim_set_datalen(data_len);
                    spim_setup_dummy(0, 0);
                    spim_write_fifo((int *)&send_buf[offset], data_len);
                }
                else
                {
                    memset(data_buf, 0x00, 32);
                    for (index = 0; index < order_size; index++)
                    {
                        data_buf[(index / 4) * 4 + (3 - index % 4)] = send_buf[offset + index];
                    }
                    spim_set_datalen(data_len);
                    spim_setup_dummy(0, 0);
                    spim_write_fifo((int *)data_buf, data_len);
                    for (int i = 0; i < 10; i++)
                        for (int j = 0; j < 1000; j++)
                            ;
                }
                //spim_start_transaction(UC_SPIM,SPIM_CMD_WR, cs->GPIO_Pin);
                spim_start_transaction(SPIM_CMD_WR, SPIM_CSN0);
                while ((spim_get_status() & 0xFFFF) != 1)
                    ;

                offset += order_size;
            }
        }
        else
        {
            //rt_kprintf("spixfer 3 len = %d\r\n", send_length);
            uint32_t offset = 0;

            spim_setup_cmd_addr(0, 0, 0, 0);
            while (offset < send_length)
            {
                uint8_t index = 0;
                uint8_t order_size = 0;
                uint32_t data_len = 0;
                uint8_t data_buf[32];
                uint8_t index_offset = 0;
                if ((send_length - offset) > 32)
                {
                    order_size = 32;
                }
                else
                {
                    order_size = send_length - offset;
                }
                data_len = order_size * 8;
                memset(data_buf, 0xff, 32);
                spim_set_datalen(data_len);
                //spim_start_transaction(UC_SPIM,SPIM_CMD_RD, cs->GPIO_Pin);
                spim_start_transaction(SPIM_CMD_RD, SPIM_CSN0);
                while ((spim_get_status() & 0xFFFF) != 1)
                    ;

                if (GPIO_PIN_13 == cs->GPIO_Pin)
                {
                    spim_read_fifo((int *)&recv_buf[offset], data_len);
                }
                else
                {
                    spim_read_fifo((int *)data_buf, data_len);
                    for (index_offset = 0; index_offset < order_size; index_offset += 4)
                    {
                        uint8_t count = 0;
                        if ((index_offset + 4) > order_size)
                        {
                            count = order_size - index_offset;
                        }
                        else
                        {
                            count = 4;
                        }
                        for (index = index_offset; index < (index_offset + count); index++)
                        {
                            recv_buf[offset + index] = data_buf[(index / 4) * 4 + ((count - 1) - (index - index_offset) % count)];
                        }
                    }
                }

                offset += order_size;
            }
        }

        if (state != 0)
        {
            LOG_I("spi transfer error : %d", state);
            message->length = 0;
        }
        else
        {
            LOG_D("%s transfer done", "spim");
        }
    }

    if (message->cs_release)
    {
        gpio_set_pin_value(cs->GPIO_Pin, GPIO_VALUE_HIGH);
    }

    return message->length;
}

static rt_err_t spi_configure(struct rt_spi_device *device,
                              struct rt_spi_configuration *configuration)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(configuration != RT_NULL);

    return uc8088_spi_init(configuration);
}

static const struct rt_spi_ops uc8088_spi_ops =
    {
        .configure = spi_configure,
        .xfer = spixfer,
};

static int rt_hw_spi_bus_init(void)
{
    rt_err_t result;

    result = rt_spi_bus_register(&uc8088_spi_bus, "spim", &uc8088_spi_ops);
    RT_ASSERT(result == RT_EOK);

    LOG_D("%s bus init done", "spim");

    return result;
}

/**
  * Attach the spi device to SPI bus, this function must be used after initialization.
  */
rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, GPIO_TypeDef *cs_gpiox, uint16_t cs_gpio_pin)
{
    RT_ASSERT(bus_name != RT_NULL);
    RT_ASSERT(device_name != RT_NULL);

    rt_err_t result;
    struct rt_spi_device *spi_device;
    struct uc8088_hw_spi_cs *cs_pin;

    /* initialize the cs pin && select the slave*/
    cs_gpiox = UC_GPIO;
    gpio_set_pin_function(cs_gpio_pin, 0);
    gpio_set_pin_direction(cs_gpio_pin, GPIO_DIR_OUT);
    gpio_set_pin_value(cs_gpio_pin, GPIO_VALUE_HIGH);

    /* attach the device to spi bus*/
    spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    RT_ASSERT(spi_device != RT_NULL);
    cs_pin = (struct uc8088_hw_spi_cs *)rt_malloc(sizeof(struct uc8088_hw_spi_cs));
    RT_ASSERT(cs_pin != RT_NULL);
    cs_pin->GPIOx = cs_gpiox;
    cs_pin->GPIO_Pin = cs_gpio_pin;
    result = rt_spi_bus_attach_device(spi_device, device_name, bus_name, (void *)cs_pin);

    if (result != RT_EOK)
    {
        LOG_E("%s attach to %s faild, %d\n", device_name, bus_name, result);
    }

    RT_ASSERT(result == RT_EOK);

    LOG_D("%s attach to %s done", device_name, bus_name);

    return result;
}

int rt_hw_spi_init(void)
{
    return rt_hw_spi_bus_init();
}
INIT_BOARD_EXPORT(rt_hw_spi_init);
#if 0
static int rt_hw_spi_flash_init(void)
{
    rt_hw_spi_device_attach("spim", "spim0", NULL, 12);

    if (RT_NULL == rt_sfud_flash_probe("norflash0", "spim0"))
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}
/* 导出到自动初始化 */
INIT_COMPONENT_EXPORT(rt_hw_spi_flash_init);
#endif

#endif /* RT_USING_SPI */
