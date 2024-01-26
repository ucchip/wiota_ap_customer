/******************************************************************************
* @file      test_wiota_uboot_ota.c
* @brief     Simulate OTA upgrade
* @author    ypzhang
* @version   1.0
* @date      2023.12.20
*
* @copyright Copyright (c) 2018 UCchip Technology Co.,Ltd. All rights reserved.
*
******************************************************************************/
#include "uc_wiota_api.h"
#include "uc_uboot.h"
#include "partition_info.h"
#include "rthw.h"

#ifdef WIOTA_AP_UBOOT_OTA_TEST
void uboot_reflash_update_test(void)
{
    int ota_data_size;

    //模拟的数据文件
    // 06 00 00 1B 01 00 70 00 00 00 00 E5 B8 01 00 00 00 10 F2 00 00 70 04 E0 04 6F F9 文件.patch里的顺序
    unsigned char ota_data[28] = {0x1B, 0x00, 0x00, 0x06,
                                  0x00, 0x70, 0x00, 0x01,
                                  0xE5, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x01, 0xB8,
                                  0x00, 0xF2, 0x10, 0x00,
                                  0xE0, 0x04, 0x70, 0x00,
                                  0x00, 0xF9, 0x6F, 0x04}; // 0X00补齐

    unsigned int address = UC_OTA_PARTITION_START_ADDRESS; // 写flash地址

    ota_data_size = sizeof(ota_data) - 1;

    // 如果 数据大小超过限制，退出
    if (ota_data_size > MAX_DOWN_FILE_8088)
    {
        return 0;
    }
    //分页
    unsigned int num = ota_data_size / FLASH_PAGE_LEN;

    /*
    // 写整数倍的完全页
    for (int i = 0; i < num; i++)
    {
        // 先擦再写
        uc_wiota_flash_erase_4K(address);
        uc_wiota_flash_write((unsigned char*)ota_data + i * FLASH_PAGE_LEN,
                            address,
                            FLASH_PAGE_LEN);
        address += FLASH_PAGE_LEN;
    }
    */

    //写不足一页的数据
    uc_wiota_flash_erase_4K(address);
    uc_wiota_flash_write((unsigned char *)ota_data + num * FLASH_PAGE_LEN,
                         address,
                         ota_data_size % FLASH_PAGE_LEN);

    // 设置数据文件长度
    boot_set_file_size(ota_data_size);

    // 设置uboot启动模式
    boot_set_mode('b');

    // 关闭相关中断
    rt_hw_interrupt_disable();

    // 软重启
    boot_riscv_reboot();
}
#endif // WIOTA_AP_UBOOT_OTA_TEST