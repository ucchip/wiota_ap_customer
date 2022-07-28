#ifndef __UC_BOOT_DOWNLOAD_H__
#define __UC_BOOT_DOWNLOAD_H__

#include <stdint.h>

#define FLASH_PAGE_LEN 4096
#define SYSTEMSTART "+SYSTEM:START\r\n"

#define ENTERMODEM "+Select modem,enter follow char:\r\n\
a. Only ymodem down file\r\n\
b. OTA update\r\n\
c. Flash rtthread\r\n\
d. Flash all\r\n\
e. Ymodem down bin, flash rtthread\r\n\
f. Ymodem down bin, flash uboot and rtthread\r\n\
g. Ymodem down ota page, ota update\r\n"

typedef enum
{
    BOOT_OTA_DEFAULT = 'a' - 1,
    BOOT_YMODEM_DOWN_FILE = 'a', 
    BOOT_RUN_OTA_ONLY_UPDATE,
    BOOT_ONLY_FLASH_RT, 
    BOOT_FLASH_ALL,
    BOOT_YMODEM_DOWN_BIN_AND_FLASH_RT, 
    BOOT_YMODEM_DOWN_BIN_AND_FLASH_ALL, 
    BOOT_YMODEM_DOWN_PAGE_ADN_OTA_UPDATE,
    BOOT_OPTION_MAX,
} BOOT_OPTION_FLAG;



typedef enum
{
    BOOT_SHARE_DEFAULT = 1,
    BOOT_SHARE_ENTER_DOWNLOAD,
    BOOT_SHARE_8288_DOWNLOAD,
    BOOT_SHARE_8288_REFLASH,
    
} BOOT_SHARE_FLAG;

typedef enum
{
    BOOT_SHARE_UART_BAUD_DEFAULT = 1,
    BOOT_SHARE_UART_BAUD_HAVE_SET,
    
} BOOT_SHARE_UART_FLAG;

typedef struct
{
    unsigned int head_flag;
    unsigned int baud_rate;
    unsigned int file_size;
    unsigned char version[4];
    unsigned char baud_flag;
    unsigned char flag;
    unsigned char already_reboot;
    char reserved;
} share_data;

/*
* parment           mean
*     0             no input state
*     1             input current state
*/

/**
 * @brief enter download
 * @param input_flag
 */
void boot_download(int input_flag);
void boot_set_uart0_baud_rate(unsigned int  baud_rate);
void boot_riscv_reboot(void);
void boot_set_mode(unsigned int modem);
void boot_get_version(char *version);

#endif
