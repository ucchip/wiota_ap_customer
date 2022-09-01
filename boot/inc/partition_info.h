#ifndef PARATITION_INFO_H_
#define PARATITION_INFO_H_

#define OTA_ADDRESS_ALIGNMENT(address) ( (address) & (~(4096 - 1)))

#define FLASH_PAGE_LEN 4096

#define UC_BOOT_LEN                     (1024*28) // 28k
#define UC_MAX_BIN_LEN                        (1024*640) 
#define UC_FLASH_RESERVED_LEN  (1024*668) 
#define UC_OTA_PARTITION_MAX_LEN    (1024 * 704)
#define UC_FLASH_STATUS_LEN     (1024*4)
#define UC_BOOT_STRUP_LEN                    (12*1024)

#define UC_BIN_UBOOT_START_ADDR 0
#define UC_BIN_UBOOT_END_ADDR (UC_BIN_UBOOT_START_ADDR + UC_BOOT_LEN) // 28K

#define UC_BIN_START_ADDRESS  UC_BIN_UBOOT_END_ADDR // 28k
#define UC_BIN_END_ADDRESS OTA_ADDRESS_ALIGNMENT(UC_BIN_START_ADDRESS + UC_MAX_BIN_LEN) // 668K

#define UC_FLASH_RESERVED_STRAT_ADDRESS OTA_ADDRESS_ALIGNMENT(UC_BIN_END_ADDRESS) // 668k
#define UC_FLASH_RESERVED_END_ADDRESS OTA_ADDRESS_ALIGNMENT(UC_FLASH_RESERVED_STRAT_ADDRESS + UC_FLASH_RESERVED_LEN) // 1336k

#define UC_OTA_PARTITION_START_ADDRESS  OTA_ADDRESS_ALIGNMENT(UC_FLASH_RESERVED_END_ADDRESS) // 1336k 
#define UC_OTA_PARTITION_END_ADDRESS  OTA_ADDRESS_ALIGNMENT(UC_OTA_PARTITION_START_ADDRESS + UC_OTA_PARTITION_MAX_LEN)  // 2040k

#define MAX_DOWN_FILE_8088 UC_OTA_PARTITION_MAX_LEN
//#define BACK_FLASH_ADDR_8088 UC_OTA_PARTITION_START_ADDRESS

#endif

