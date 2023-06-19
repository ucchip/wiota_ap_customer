
#ifndef _SLAVE_UC8X88_H_
#define _SLAVE_UC8X88_H_

#include <stdint.h>
#include <stdbool.h>

/* 从机芯片类型 */
typedef enum
{
    CHIP_TYPE_UNKNOWN = 0,  /* unknown */
    CHIP_TYPE_UC8088,       /* uc8088 */
    CHIP_TYPE_UC8288,       /* uc8288 */
    CHIP_TYPE_MAX,
} slave_uc8x88_chip_type_e;

void slave_uc8x88_init(uint8_t chip_index);
uint8_t slave_uc8x88_get_exist_status(uint8_t chip_index);
slave_uc8x88_chip_type_e slave_uc8x88_get_chip_type(uint8_t chip_index);
void slave_uc8x88_flash_init(uint8_t chip_index);
uint16_t slave_uc8x88_flash_read_id(uint8_t chip_index);
void slave_uc8x88_flash_erase(uint8_t chip_index, uint32_t addr_offset, uint32_t length);
uint32_t slave_uc8x88_get_flash_erase_min_size(uint8_t chip_index);
int8_t slave_uc8x88_flash_write_data(uint8_t chip_index, uint32_t addr_offset, uint8_t* data_buf, uint32_t data_len, uint8_t order_verify);
int8_t slave_uc8x88_flash_read_data(uint8_t chip_index, uint32_t addr, uint8_t* data_buf, uint32_t data_len);
void slave_uc8x88_flash_start_app(uint8_t chip_index);
int8_t slave_uc8x88_mem_read_data(uint8_t chip_index, uint32_t addr, uint8_t* data_buf, uint32_t data_len);
int8_t slave_uc8x88_mem_write_data(uint8_t chip_index, uint32_t addr, uint8_t* data_buf, uint32_t data_len);
void slave_uc8x88_mem_dummy_init(uint8_t  chip_index);
#endif
