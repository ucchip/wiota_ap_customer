
#ifndef _UC_BOOT_SPIM2FLASH_H
#define _UC_BOOT_SPIM2FLASH_H

#include <stdint.h>
#include <stdbool.h>

#define FLASH_PAGE_8288 (256)
#define FLASH_SECTOR_8288 (4 * 1024)
// Last 4 Byte for bin's timestamp
#define MAX_DOWN_FILE_8288 (512 * 1024 - FLASH_SECTOR_8288)
#define FLASH_8288_LAST_SECTOR_ADDRESS (512 * 1024 - FLASH_SECTOR_8288)
#define FLASH_8288_TIMESTAMP_ADDRESS (512 * 1024 - 4)
#define FLASH_8288_ID (0x000B6013)

#define UC8288_ID_REG (0x1A107018)
#define UC8288_ID (0x55438288)

/// big endian and little endian convert
#define BSWAP_32(x) \
    ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >>  8) |         \
     (((x) & 0x0000ff00u) <<  8) | (((x) & 0x000000ffu) << 24))
#define BSWAP_16(x) \
    ((((x) & 0xff00u) >>  8) | (((x) & 0x00ffu) << 8))

typedef enum _spim_mode
{
    SPIM_MODE_MEM,
    SPIM_MODE_FLASH,
}SPIM_MODE;

/**
 * @brief SPIM access 8288 memory or reg
 */
typedef enum _spim_op_mem
{
    SPIM_OP_MEM_WRITE   = 0x02,
    SPIM_OP_MEM_READ    = 0x0B,
}SPIM_OP_MEM;

typedef enum _boot_init_8288 {
    INIT_8288_FLASH_OK,
    INIT_8288_REG_OK,
    INIT_8288_ERROR = -1
} BOOT_INIT_8288;

//void boot_flash_init_8288(void);
BOOT_INIT_8288 boot_spim_init(void);
/**
 * @brief set spim access mode
 * @param nMode     define by SPIM_MODE
 */
void boot_spim_set_mode(SPIM_MODE nMode);
bool boot_spim_watchdog_disable_8288(void);
void boot_spim_watchdog_reboot_8288(int nPeriodMs);
void boot_flash_busy(void);
void boot_flash_erase_sector_8288(uint32_t nAddrBegin);
void boot_flash_erase_8288(uint32_t nEraseSize);
bool boot_flash_write_8288(int nAddrBegin,const uint8_t *pData, int nWriteLen);
bool boot_need_reflash_8288(void);
bool boot_reflash_8288(void);
//void boot_spim_read(void *pOut, int nOutLen, uint8_t nCmd, int nAddr, int nAddrLen);
//void boot_spim_write(uint8_t nCmd, int nAddr, int nAddrLen, const void *pData, int nDataLen);

#endif