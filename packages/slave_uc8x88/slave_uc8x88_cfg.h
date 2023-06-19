
#ifndef _SLAVE_UC8X88_CFG_H_
#define _SLAVE_UC8X88_CFG_H_

#include <stdint.h>
#include <stdbool.h>

#define SLAVE_UC8X88_DEBUG

#define SLAVE_UC8X88_MAX_COUNT      8

#define SLAVE_UC8x88_SPI_NAME      "spim"
#define SLAVE_UC8x88_CS_PORT       RT_NULL

#define CHIP0_UC8x88_CS_PIN        (13)
#define CHIP1_UC8x88_CS_PIN        (1 << 11 | 1 << 8 | 13)
#define CHIP2_UC8x88_CS_PIN        (1 << 11 | 2 << 8 | 13)
#define CHIP3_UC8x88_CS_PIN        (1 << 11 | 3 << 8 | 13)
#define CHIP4_UC8x88_CS_PIN        (1 << 11 | 4 << 8 | 13)
#define CHIP5_UC8x88_CS_PIN        (1 << 11 | 5 << 8 | 13)
#define CHIP6_UC8x88_CS_PIN        (1 << 11 | 6 << 8 | 13)
#define CHIP7_UC8x88_CS_PIN        (1 << 11 | 7 << 8 | 13)

int slave_uc8x88_cfg_spi_init(uint8_t index);
int slave_uc8x88_cfg_spi_send_recv(uint8_t index, uint8_t* send_buf1, uint32_t send_length1, uint8_t* recv_buf1, uint32_t recv_length1, uint8_t* send_buf2, uint32_t send_length2, uint8_t* recv_buf2,
                                   uint32_t recv_length2);

#endif
