
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <rtthread.h>
#include <board.h>
#include <rtdevice.h>
#include <string.h>

#include "slave_uc8x88.h"
#include "slave_uc8x88_cfg.h"

#define DBG_TAG "slave_uc8x88"
#ifdef SLAVE_UC8X88_DEBUG
#define DBG_LVL DBG_LOG
#else
#define DBG_LVL DBG_INFO
#endif
#include <rtdbg.h>

#define BSWAP_32(x)                                         \
    ((((x)&0xff000000u) >> 24) | (((x)&0x00ff0000u) >> 8) | \
     (((x)&0x0000ff00u) << 8) | (((x)&0x000000ffu) << 24))
#define BSWAP_16(x) \
    ((((x)&0xff00u) >> 8) | (((x)&0x00ffu) << 8))

#define FL_WRITE_ENABLE 0x06
#define FL_ERASE_32 0x20
#define FL_ERASE_64 0xD8
#define FL_READ 0x03
#define FL_WRITE 0x02
#define FL_READ_STATUS 0x05
#define FL_ERASE_ALL 0x60

#define MEM_READ 0x0B
#define MEM_WRITE 0x02

#define BLOCK_SIZE 0x10000
#define SECTOR_SIZE 0x1000
#define PAGE_SIZE 256

// #define BSWAP_ENABLE
/* UC8x88存在标志，1：存在，0不存在 */
static uint8_t g_slave_uc8x88_exist[SLAVE_UC8X88_MAX_COUNT] = {0};
/* UC8x88芯片类型 */
static slave_uc8x88_chip_type_e g_slave_uc8x88_chip_type[SLAVE_UC8X88_MAX_COUNT] = {0};
/* 操作互斥量 */
static rt_mutex_t p_slave_uc8x88_mutex[SLAVE_UC8X88_MAX_COUNT] = {RT_NULL};

static void flash_init(uint8_t chip_index)
{
    uint8_t wr_buf[5] = {0xFF, 0x00, 0x00, 0x00, 0x01};
    uint8_t wr_buf2[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (slave_uc8x88_cfg_spi_send_recv(chip_index, wr_buf, 5, RT_NULL, 0, wr_buf2, 5, RT_NULL, 0) != 0)
    {
        LOG_D("flash_init Err");
    }
}

static uint16_t flash_read_id(uint8_t chip_index)
{
    uint8_t wr_buf[4] = {0x90, 0x00, 0x00, 0x00};
    uint8_t flash_id[2] = {0x00, 0x00};

    if (slave_uc8x88_cfg_spi_send_recv(chip_index, wr_buf, 4, flash_id, 2, RT_NULL, 0, RT_NULL, 0) != 0)
    {
        LOG_D("flash_read_id Err");
    }

    return (flash_id[0] << 8 | flash_id[1]);
}

static void flash_write_enable(uint8_t chip_index)
{
    uint8_t wr_buf = FL_WRITE_ENABLE;

    if (slave_uc8x88_cfg_spi_send_recv(chip_index, &wr_buf, 1, RT_NULL, 0, RT_NULL, 0, RT_NULL, 0) != 0)
    {
        LOG_D("flash_write_enable Err");
    }
}

static uint8_t flash_get_satus(uint8_t chip_index)
{
    uint8_t status = 0;
    uint8_t wr_buf = FL_READ_STATUS;

    if (slave_uc8x88_cfg_spi_send_recv(chip_index, &wr_buf, 1, &status, 1, RT_NULL, 0, RT_NULL, 0) != 0)
    {
        LOG_D("flash_get_satus Err");
    }

    return status;
}

static void flash_write_down(uint8_t chip_index)
{
    //uint32_t wait_count = 10000;
    uint32_t wait_count = 100000;

    while ((flash_get_satus(chip_index) & 0x01) == 0x01)
    {
        wait_count--;
        if (wait_count == 0)
        {
            LOG_D("flash_write_down timeout!!!");
            break;
        }
    }
}

static void flash_write(uint8_t chip_index, uint32_t addr, uint8_t *data, uint32_t len)
{
    uint32_t index;
    uint8_t *wr_buf = RT_NULL;

    if ((len == 0) || (len > 256))
    {
        return;
    }

    wr_buf = (uint8_t *)rt_malloc(len + 4);
    if (wr_buf == RT_NULL)
    {
        return;
    }

    wr_buf[0] = FL_WRITE;
    wr_buf[1] = (uint8_t)((addr >> 16) & 0xff);
    wr_buf[2] = (uint8_t)((addr >> 8) & 0xff);
    wr_buf[3] = (uint8_t)((addr >> 0) & 0xff);

    for (index = 0; index < len; index++)
    {
        wr_buf[4 + index] = data[index];
    }

    flash_write_enable(chip_index);

    if (slave_uc8x88_cfg_spi_send_recv(chip_index, wr_buf, (len + 4), RT_NULL, 0, RT_NULL, 0, RT_NULL, 0) != 0)
    {
        LOG_D("flash_write Err");
    }

    flash_write_down(chip_index);

    rt_free(wr_buf);
}

static void flash_read(uint8_t chip_index, uint32_t addr, uint8_t *read_data, uint32_t len)
{
    uint8_t wr_buf[4];

    if ((read_data == RT_NULL) || (len == 0))
    {
        return;
    }

    wr_buf[0] = FL_READ;
    wr_buf[1] = (uint8_t)((addr >> 16) & 0xff);
    wr_buf[2] = (uint8_t)((addr >> 8) & 0xff);
    wr_buf[3] = (uint8_t)((addr >> 0) & 0xff);

    //flash_write_enable(chip_index);

    if (slave_uc8x88_cfg_spi_send_recv(chip_index, wr_buf, 4, read_data, len, RT_NULL, 0, RT_NULL, 0) != 0)
    {
        LOG_D("flash_read Err");
    }

    flash_write_down(chip_index);
}

static void flash_erase_64(uint8_t chip_index, uint32_t addr)
{
    uint8_t wr_buf[4];
    //LOG_D("flash_erase_64");

    flash_write_enable(chip_index);
    flash_write_down(chip_index);

    wr_buf[0] = FL_ERASE_64;
    wr_buf[1] = (uint8_t)((addr >> 16) & 0xff);
    wr_buf[2] = (uint8_t)((addr >> 8) & 0xff);
    wr_buf[3] = (uint8_t)((addr >> 0) & 0xff);

    if (slave_uc8x88_cfg_spi_send_recv(chip_index, wr_buf, 4, RT_NULL, 0, RT_NULL, 0, RT_NULL, 0) != 0)
    {
        LOG_D("flash_erase_64 Err");
    }

    flash_write_down(chip_index);
}

static void flash_erase_32(uint8_t chip_index, uint32_t addr)
{
    uint8_t wr_buf[4];
    //LOG_D("flash_erase_32");

    flash_write_enable(chip_index);
    flash_write_down(chip_index);

    wr_buf[0] = FL_ERASE_32;
    wr_buf[1] = (uint8_t)((addr >> 16) & 0xff);
    wr_buf[2] = (uint8_t)((addr >> 8) & 0xff);
    wr_buf[3] = (uint8_t)((addr >> 0) & 0xff);

    if (slave_uc8x88_cfg_spi_send_recv(chip_index, wr_buf, 4, RT_NULL, 0, RT_NULL, 0, RT_NULL, 0) != 0)
    {
        LOG_D("flash_erase_32 Err");
    }

    flash_write_down(chip_index);
}

static void flash_erase(uint8_t chip_index, uint32_t addr_offset, uint32_t file_len)
{
    uint32_t index;
    uint32_t erase_num = 0;
    uint32_t start_num = 0;

    //LOG_D("begin to erase flash.");

    if (0) //(file_len > BLOCK_SIZE)
    {
        start_num = addr_offset / BLOCK_SIZE;
        erase_num = file_len / BLOCK_SIZE;
        if (file_len % BLOCK_SIZE)
        {
            erase_num++;
        }
        for (index = 0; index < erase_num; index++)
        {
            flash_erase_64(chip_index, (start_num + index) * BLOCK_SIZE);
        }
    }
    else
    {
        start_num = addr_offset / SECTOR_SIZE;
        erase_num = file_len / SECTOR_SIZE;
        if (file_len % SECTOR_SIZE)
        {
            erase_num++;
        }
        for (index = 0; index < erase_num; index++)
        {
            flash_erase_32(chip_index, (start_num + index) * SECTOR_SIZE);
        }
    }
    //LOG_D("erase flash end.");
}

static void flash_ctr(uint8_t chip_index, uint8_t data)
{
    uint8_t wr_buf[5] = {0xff, 0x00, 0x00, 0x00, 0x00};
    wr_buf[4] = data;

    if (slave_uc8x88_cfg_spi_send_recv(chip_index, wr_buf, 5, RT_NULL, 0, RT_NULL, 0, RT_NULL, 0) != 0)
    {
        LOG_D("flash_ctr Err");
    }
}

static int8_t flash_write_data(uint8_t chip_index, uint32_t addr_offset, uint8_t *data_buf, uint32_t data_len, uint8_t order_verify)
{
    uint32_t page_num = 0;
    uint32_t write_page_size = 0;
    uint32_t index = 0;
    uint32_t page_offset = addr_offset / PAGE_SIZE;
    uint8_t *read_buff = RT_NULL;
    uint32_t try_cnt = 0;
    int res = 0;

    if ((data_buf == RT_NULL) || (data_len == 0) || ((addr_offset % PAGE_SIZE) != 0))
    {
        return -1;
    }

    //LOG_D("begin to write flash.");
    page_num = data_len / PAGE_SIZE;
    if ((data_len % PAGE_SIZE) != 0)
    {
        page_num++;
    }

    for (index = 0; index < page_num; index++)
    {
        if ((data_len - index * PAGE_SIZE) >= PAGE_SIZE)
        {
            write_page_size = PAGE_SIZE;
        }
        else
        {
            write_page_size = data_len - index * PAGE_SIZE;
        }

        for (try_cnt = 0; try_cnt < 5; try_cnt++)
        {
            flash_write(chip_index, (index + page_offset) * PAGE_SIZE, &data_buf[index * PAGE_SIZE], write_page_size);

            if (order_verify)
            {
                if (RT_NULL == read_buff)
                {
                    read_buff = rt_malloc(PAGE_SIZE);
                    if (read_buff == RT_NULL)
                    {
                        LOG_D("verify error 1, rt_malloc fail!!");
                        return -2;
                    }
                }
                rt_memset(read_buff, 0, write_page_size);

                flash_read(chip_index, (index + page_offset) * PAGE_SIZE, read_buff, write_page_size);
                if (0 != rt_memcmp(read_buff, &data_buf[index * PAGE_SIZE], write_page_size))
                {
                    LOG_E("chip_index %d verify error, pagenum %d\n", chip_index, (index + page_offset));
                    continue;
                }
                break;
            }
            break;
        }

        if (try_cnt >= 5)
        {
            LOG_E("chip_index %d flash_write_data fail\n", chip_index);
            res = 1;
            break;
        }
        rt_thread_mdelay(1);
    }

    if (RT_NULL != read_buff)
    {
        rt_free(read_buff);
    }
    return res;
}

static void mem_dummy_init(uint8_t chip_index)
{
    uint8_t wr_buf[2] = {0x11, 31};

    if (slave_uc8x88_cfg_spi_send_recv(chip_index, wr_buf, 2, RT_NULL, 0, RT_NULL, 0, RT_NULL, 0) != 0)
    {
        LOG_D("mem_dummy_init Err");
    }
}

void slave_uc8x88_mem_dummy_init(uint8_t chip_index)
{
    mem_dummy_init(chip_index);
}

static void bswap32_buf(uint8_t *data_buf, uint32_t data_len)
{
    if (data_len % 4 != 0)
    {
        return;
    }

    for (uint32_t index = 0; index < data_len; index += 4)
    {
        uint8_t temp_data = 0;

        temp_data = data_buf[index + 0];
        data_buf[index + 0] = data_buf[index + 3];
        data_buf[index + 3] = temp_data;

        temp_data = data_buf[index + 1];
        data_buf[index + 1] = data_buf[index + 2];
        data_buf[index + 2] = temp_data;
    }
}

static void mem_read(uint8_t chip_index, uint32_t addr, uint8_t *read_data, uint32_t len)
{
    uint8_t wr_buf[8];
    uint8_t re_buf[4];

    if ((read_data == RT_NULL) || (len == 0))
    {
        return;
    }

    wr_buf[0] = MEM_READ;
    wr_buf[1] = (uint8_t)((addr >> 24) & 0xff);
    wr_buf[2] = (uint8_t)((addr >> 16) & 0xff);
    wr_buf[3] = (uint8_t)((addr >> 8) & 0xff);
    wr_buf[4] = (uint8_t)((addr >> 0) & 0xff);
    bswap32_buf(wr_buf, 8);
    if (slave_uc8x88_cfg_spi_send_recv(chip_index, wr_buf, 5, re_buf, 4, RT_NULL, 0, read_data, len) != 0)
    {
        LOG_D("mem_read Err");
    }
}

static void mem_write(uint8_t chip_index, uint32_t addr, uint8_t *write_data, uint32_t len)
{
    uint8_t wr_buf[8];

    if ((write_data == RT_NULL) || (len == 0))
    {
        return;
    }

    wr_buf[0] = MEM_WRITE;
    wr_buf[1] = (uint8_t)((addr >> 24) & 0xff);
    wr_buf[2] = (uint8_t)((addr >> 16) & 0xff);
    wr_buf[3] = (uint8_t)((addr >> 8) & 0xff);
    wr_buf[4] = (uint8_t)((addr >> 0) & 0xff);
    bswap32_buf(wr_buf, 8);
    if (slave_uc8x88_cfg_spi_send_recv(chip_index, wr_buf, 5, RT_NULL, 0, write_data, len, RT_NULL, 0) != 0)
    {
        LOG_D("mem_read Err");
    }
}

void slave_uc8x88_init(uint8_t chip_index)
{
    uint32_t mem_data = 0;
    uint8_t mutex_name[8];

    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return;
    }

    rt_thread_mdelay(10);
    if (slave_uc8x88_cfg_spi_init(chip_index) != 0)
    {
        return;
    }

    mem_dummy_init(chip_index);

    for (int i = 0; i < 3; i++)
    {
        mem_read(chip_index, 0x1a107018, (uint8_t *)&mem_data, 4);
#ifdef BSWAP_ENABLE
        mem_data = BSWAP_32(mem_data);
#endif
        LOG_D("slave_uc8x88_init, (0x1a107018) = 0x%08x", mem_data);

        if (mem_data == 0xabcd0001)
        {
            g_slave_uc8x88_chip_type[chip_index] = CHIP_TYPE_UC8088;
            g_slave_uc8x88_exist[chip_index] = 1;
            LOG_I("slave_uc8x88_init No.%d chip is UC8088!", chip_index);
            break;
        }
        else if (mem_data == 0x55438288)
        {
            g_slave_uc8x88_chip_type[chip_index] = CHIP_TYPE_UC8288;
            g_slave_uc8x88_exist[chip_index] = 1;
            LOG_I("slave_uc8x88_init No.%d chip is UC8288!", chip_index);
            break;
        }
        else
        {
            g_slave_uc8x88_chip_type[chip_index] = CHIP_TYPE_UNKNOWN;
            g_slave_uc8x88_exist[chip_index] = 0;
            LOG_I("slave_uc8x88_init No.%d chip is nonexist!", chip_index);
        }
    }
    if (p_slave_uc8x88_mutex[chip_index] == RT_NULL)
    {
        rt_sprintf((char *)mutex_name, "s_uc%d", chip_index);
        p_slave_uc8x88_mutex[chip_index] = rt_mutex_create((const char *)mutex_name, RT_IPC_FLAG_FIFO);
    }
}

uint8_t slave_uc8x88_get_exist_status(uint8_t chip_index)
{
    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return 0;
    }
    return g_slave_uc8x88_exist[chip_index];
}

slave_uc8x88_chip_type_e slave_uc8x88_get_chip_type(uint8_t chip_index)
{
    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return CHIP_TYPE_UNKNOWN;
    }
    return g_slave_uc8x88_chip_type[chip_index];
}

void slave_uc8x88_flash_init(uint8_t chip_index)
{
    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return;
    }
    if (g_slave_uc8x88_exist[chip_index] == 0)
    {
        return;
    }
    if (p_slave_uc8x88_mutex[chip_index] == RT_NULL)
    {
        return;
    }
    rt_mutex_take(p_slave_uc8x88_mutex[chip_index], RT_WAITING_FOREVER);
    flash_init(chip_index);
    rt_mutex_release(p_slave_uc8x88_mutex[chip_index]);
}

uint16_t slave_uc8x88_flash_read_id(uint8_t chip_index)
{
    uint16_t flash_id = 0x0000;

    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return 0;
    }
    if (g_slave_uc8x88_exist[chip_index] == 0)
    {
        return 0;
    }
    if (p_slave_uc8x88_mutex[chip_index] == RT_NULL)
    {
        return 0;
    }
    rt_mutex_take(p_slave_uc8x88_mutex[chip_index], RT_WAITING_FOREVER);
    flash_id = flash_read_id(chip_index);
    rt_mutex_release(p_slave_uc8x88_mutex[chip_index]);

    return flash_id;
}

void slave_uc8x88_flash_erase(uint8_t chip_index, uint32_t addr_offset, uint32_t length)
{
    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return;
    }
    if (g_slave_uc8x88_exist[chip_index] == 0)
    {
        return;
    }
    if (p_slave_uc8x88_mutex[chip_index] == RT_NULL)
    {
        return;
    }
    rt_mutex_take(p_slave_uc8x88_mutex[chip_index], RT_WAITING_FOREVER);
    flash_erase(chip_index, addr_offset, length);
    rt_mutex_release(p_slave_uc8x88_mutex[chip_index]);
}

uint32_t slave_uc8x88_get_flash_erase_min_size(uint8_t chip_index)
{
    return SECTOR_SIZE;
}

int8_t slave_uc8x88_flash_write_data(uint8_t chip_index, uint32_t addr_offset, uint8_t *data_buf, uint32_t data_len, uint8_t order_verify)
{
    int8_t ret_val = 0;

    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return -1;
    }
    if (g_slave_uc8x88_exist[chip_index] == 0)
    {
        return -1;
    }
    if (p_slave_uc8x88_mutex[chip_index] == RT_NULL)
    {
        return -1;
    }
    rt_mutex_take(p_slave_uc8x88_mutex[chip_index], RT_WAITING_FOREVER);
    ret_val = flash_write_data(chip_index, addr_offset, data_buf, data_len, order_verify);
    rt_mutex_release(p_slave_uc8x88_mutex[chip_index]);

    return ret_val;
}

int8_t slave_uc8x88_flash_read_data(uint8_t chip_index, uint32_t addr, uint8_t *data_buf, uint32_t data_len)
{
    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return -1;
    }
    if (g_slave_uc8x88_exist[chip_index] == 0)
    {
        return -1;
    }
    if (p_slave_uc8x88_mutex[chip_index] == RT_NULL)
    {
        return -1;
    }
    rt_mutex_take(p_slave_uc8x88_mutex[chip_index], RT_WAITING_FOREVER);
    flash_read(chip_index, addr, data_buf, data_len);
    rt_mutex_release(p_slave_uc8x88_mutex[chip_index]);

    return 0;
}

void slave_uc8x88_flash_start_app(uint8_t chip_index)
{
    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return;
    }
    if (g_slave_uc8x88_exist[chip_index] == 0)
    {
        return;
    }
    if (p_slave_uc8x88_mutex[chip_index] == RT_NULL)
    {
        return;
    }
    rt_mutex_take(p_slave_uc8x88_mutex[chip_index], RT_WAITING_FOREVER);
    flash_ctr(chip_index, 0x0);
    mem_dummy_init(chip_index);
    rt_mutex_release(p_slave_uc8x88_mutex[chip_index]);
}

int8_t slave_uc8x88_mem_read_data(uint8_t chip_index, uint32_t addr, uint8_t *data_buf, uint32_t data_len)
{
    uint32_t index = 0;

    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return -1;
    }
    if (g_slave_uc8x88_exist[chip_index] == 0)
    {
        return -1;
    }
    if (p_slave_uc8x88_mutex[chip_index] == RT_NULL)
    {
        return -1;
    }
    rt_mutex_take(p_slave_uc8x88_mutex[chip_index], RT_WAITING_FOREVER);
    // mem_dummy_init(chip_index);
    // rt_thread_mdelay(1);
    if ((addr % 4 == 0) && (data_len % 4 == 0))
    {
        mem_read(chip_index, addr, data_buf, data_len);
#ifdef BSWAP_ENABLE
        bswap32_buf(data_buf, data_len);
#endif
    }
    else
    {
        while (index < data_len)
        {
            uint32_t read_addr = 0;
            uint32_t read_data = 0;
            uint8_t order_len = 0;
            uint8_t addr_offset = 0;
            uint8_t *read_buf = RT_NULL;

            read_buf = (uint8_t *)&read_data;

            read_addr = ((addr + index) / 4) * 4;
            if ((addr + index) % 4)
            {
                addr_offset = (addr + index) % 4;
            }

            if ((index + 4 - addr_offset) > data_len)
            {
                order_len = data_len - index;
            }
            else
            {
                order_len = 4 - addr_offset;
            }

            mem_read(chip_index, read_addr, read_buf, 4);
#ifdef BSWAP_ENABLE
            read_data = BSWAP_32(read_data);
#endif
            memcpy(&data_buf[index], &read_buf[addr_offset], order_len);

            index += order_len;
        }
    }
    rt_mutex_release(p_slave_uc8x88_mutex[chip_index]);

    return 0;
}

int8_t slave_uc8x88_mem_write_data(uint8_t chip_index, uint32_t addr, uint8_t *data_buf, uint32_t data_len)
{
    uint32_t index = 0;

    if (chip_index >= SLAVE_UC8X88_MAX_COUNT)
    {
        return -1;
    }
    if (g_slave_uc8x88_exist[chip_index] == 0)
    {
        return -1;
    }
    if (p_slave_uc8x88_mutex[chip_index] == RT_NULL)
    {
        return -1;
    }
    rt_mutex_take(p_slave_uc8x88_mutex[chip_index], RT_WAITING_FOREVER);
    // mem_dummy_init(chip_index);
    // rt_thread_mdelay(1);
    if ((addr % 4 == 0) && (data_len % 4 == 0))
    {
#ifdef BSWAP_ENABLE
        bswap32_buf(data_buf, data_len);
#endif
        mem_write(chip_index, addr, data_buf, data_len);
#ifdef BSWAP_ENABLE
        bswap32_buf(data_buf, data_len);
#endif
    }
    else
    {
        while (index < data_len)
        {
            uint32_t write_addr = 0;
            uint32_t write_data = 0;
            uint8_t order_len = 0;
            uint8_t addr_offset = 0;
            uint8_t *write_buf = RT_NULL;

            write_buf = (uint8_t *)&write_data;

            write_addr = ((addr + index) / 4) * 4;
            if ((addr + index) % 4)
            {
                addr_offset = (addr + index) % 4;
            }

            if ((index + 4 - addr_offset) > data_len)
            {
                order_len = data_len - index;
            }
            else
            {
                order_len = 4 - addr_offset;
            }

            if ((addr_offset > 0) || (order_len < 4))
            {
                mem_read(chip_index, write_addr, write_buf, 4);
#ifdef BSWAP_ENABLE
                write_data = BSWAP_32(write_data);
#endif
            }
            memcpy(&write_buf[addr_offset], &data_buf[index], order_len);
#ifdef BSWAP_ENABLE
            write_data = BSWAP_32(write_data);
#endif
            mem_write(chip_index, write_addr, write_buf, 4);

            index += order_len;
        }
    }
    rt_mutex_release(p_slave_uc8x88_mutex[chip_index]);

    return 0;
}
