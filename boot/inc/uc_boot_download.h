#ifndef __UC_BOOT_DOWNLOAD_H__
#define __UC_BOOT_DOWNLOAD_H__

#include <stdint.h>

#define FLASH_PAGE_LEN 4096
#define MAX_DOWN_FILE_8088 ((2 * 1024 * 1024 - 8 * 1024) / 2)
#define BACK_FLASH_ADDR_8088 ((2 * 1024 * 1024 - 8 * 1024) / 2)
#define ENTERMODEM "+CHOOSEMODEM: D or M\r\n"
#define SYSTEMSTART "+SYSTEM:START\r\n"

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
    unsigned int baud_flag;
    unsigned int baud_rate;
    unsigned int flag;
    unsigned int already_reboot;
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
void boot_set_modem(unsigned char modem);

#endif
