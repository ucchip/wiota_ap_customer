
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <rtthread.h>
#include <board.h>
#include <rtdevice.h>
#include <string.h>

#include "slave_uc8x88_cfg.h"
#include "drv_spi.h"

#define DBG_TAG    "slave_uc8x88.cfg"
#ifdef SLAVE_UC8X88_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif
#include <rtdbg.h>

static struct rt_spi_device* chip_spi_dev_uc8x88[SLAVE_UC8X88_MAX_COUNT] = {RT_NULL};

int slave_uc8x88_cfg_spi_init(uint8_t chip_index)
{
    char *chip_uc8x88_spi_name = RT_NULL;
    char chip_uc8x88_spi_dev_name[12];
    void *chip_uc8x88_cs_port = RT_NULL;
    uint32_t chip_uc8x88_cs_pin = 0;

    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return -1;
    }

    if (chip_index == 0)
    {
        chip_uc8x88_cs_pin = CHIP0_UC8x88_CS_PIN;
    }
    else if (chip_index == 1)
    {
        chip_uc8x88_cs_pin = CHIP1_UC8x88_CS_PIN;
    }
    else if (chip_index == 2)
    {
        chip_uc8x88_cs_pin = CHIP2_UC8x88_CS_PIN;
    }
    else if (chip_index == 3)
    {
        chip_uc8x88_cs_pin = CHIP3_UC8x88_CS_PIN;
    }
    else if (chip_index == 4)
    {
        chip_uc8x88_cs_pin = CHIP4_UC8x88_CS_PIN;
    }
    else if (chip_index == 5)
    {
        chip_uc8x88_cs_pin = CHIP5_UC8x88_CS_PIN;
    }
    else if (chip_index == 6)
    {
        chip_uc8x88_cs_pin = CHIP6_UC8x88_CS_PIN;
    }
    else if (chip_index == 7)
    {
        chip_uc8x88_cs_pin = CHIP7_UC8x88_CS_PIN;
    }

    chip_uc8x88_spi_name = SLAVE_UC8x88_SPI_NAME;
    chip_uc8x88_cs_port = SLAVE_UC8x88_CS_PORT;
    rt_sprintf(chip_uc8x88_spi_dev_name, "%s_s%d", chip_uc8x88_spi_name, chip_index);

    if (rt_device_find(chip_uc8x88_spi_dev_name) == RT_NULL)
    {
        if (rt_hw_spi_device_attach(chip_uc8x88_spi_name, chip_uc8x88_spi_dev_name, chip_uc8x88_cs_port, chip_uc8x88_cs_pin) != RT_EOK)
        {
            LOG_D("CHIP%d_UC8x88_init bus attach error", chip_index);
            return -1;
        }
    }

    /* 查找 spi 设备获取设备句柄 */
    chip_spi_dev_uc8x88[chip_index] = (struct rt_spi_device*)rt_device_find(chip_uc8x88_spi_dev_name);
    if (chip_spi_dev_uc8x88[chip_index] == RT_NULL)
    {
        LOG_D("CHIP%d_UC8x88_init can't find %s device!", chip_index, chip_uc8x88_spi_dev_name);
        return -1;
    }
    else
    {
        /* 配置SPI*/
        struct rt_spi_configuration cfg;
        cfg.data_width = 8;
        cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
        cfg.max_hz = 5 * 1000 * 1000;                          /* 10M */
        rt_spi_configure(chip_spi_dev_uc8x88[chip_index], &cfg);
    }

    return 0;
}

int slave_uc8x88_cfg_spi_send_recv(uint8_t chip_index, uint8_t* send_buf1, uint32_t send_length1, uint8_t* recv_buf1, uint32_t recv_length1, uint8_t* send_buf2, uint32_t send_length2,
                                   uint8_t* recv_buf2, uint32_t recv_length2)
{
    int res = 0;

    struct rt_spi_message msg1, msg2, msg3, msg4;
    struct rt_spi_message* last_msg = RT_NULL;
    struct rt_spi_message* end_msg = RT_NULL;

    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return -1;
    }
    if (chip_spi_dev_uc8x88[chip_index] == RT_NULL)
    {
        return -1;
    }

    if ((send_buf1 == RT_NULL) || (send_length1 == 0))
    {
        return -1;
    }

    msg1.send_buf = send_buf1;
    msg1.recv_buf = RT_NULL;
    msg1.length = send_length1;
    msg1.cs_take = 1;
    msg1.cs_release = 0;
    msg1.next = RT_NULL;
    last_msg = &msg1;
    end_msg = &msg1;

    if (recv_buf1 != RT_NULL)
    {
        msg2.send_buf = RT_NULL;
        msg2.recv_buf = recv_buf1;
        msg2.length = recv_length1;
        msg2.cs_take = 0;
        msg2.cs_release = 0;
        msg2.next = RT_NULL;
        last_msg->next = &msg2;
        last_msg = &msg2;
        end_msg = &msg2;
    }
    if (send_buf2 != RT_NULL)
    {
        msg3.send_buf = send_buf2;
        msg3.recv_buf = RT_NULL;
        msg3.length = send_length2;
        msg3.cs_take = 0;
        msg3.cs_release = 0;
        msg3.next = RT_NULL;
        last_msg->next = &msg3;
        last_msg = &msg3;
        end_msg = &msg3;
    }
    if (recv_buf2 != RT_NULL)
    {
        msg4.send_buf = RT_NULL;
        msg4.recv_buf = recv_buf2;
        msg4.length = recv_length2;
        msg4.cs_take = 0;
        msg4.cs_release = 0;
        msg4.next = RT_NULL;
        last_msg->next = &msg4;
        last_msg = &msg4;
        end_msg = &msg4;
    }
    end_msg->cs_release = 1;

    rt_spi_transfer_message(chip_spi_dev_uc8x88[chip_index], &msg1);

    return res;
}
