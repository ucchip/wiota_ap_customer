
#ifndef _UC_BOOT_SPIM2FLASH_H
#define _UC_BOOT_SPIM2FLASH_H

#include <stdint.h>

#define FLASH_PAGE_LEN 4096
#define MAX_DOWN_FILE_8288 ((512 * 1024 - 8 * 1024) / 2)
//258048 -> 3f000
#define BACK_FLASH_ADDR_8288 ((512 * 1024 - 8 * 1024) / 2)

void boot_spim2flash_read_id(void);

#endif