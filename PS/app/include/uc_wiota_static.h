/*
 * Copyright (c) 2022, Chongqing UCchip InfoTech Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @brief Static data application program interface.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-01     Lujun        the first version
 * 2022-07-04     Zhujiejing   add flash read/write interface
 * 2022-08-04     Lujun        replace memcpy and memset with rt_memcpy and rt_memset
 */

#ifndef _UC_WIOTA_STATIC_H_
#define _UC_WIOTA_STATIC_H_

#ifdef __cplushplus
extern "C"
{
#endif

#define FLASH_OPEN_START_ADDRESS     0x0             // (1336)*1024
#define FLASH_OPEN_END_ADDRESS       0x1FE000        // (2040)*1024

/**
 * @brief data transfer unit
 *
 */
typedef struct DtuInfoT
{
    unsigned char  reserved[2];        /**< reserved */
    unsigned char  dtu_status;         /**< status: 0 or 1 */
    unsigned char  dtu_at_show;        /**< show AT format: 0 or 1 */
    unsigned short dtu_timeout;        /**< send timeout */
    unsigned short dtu_wait;           /**< wait time */
    unsigned char  dtu_exit[8];        /**< exit string */
    unsigned char  na[24];             /**< undefined */
} dtu_info_t;


/**
 * @brief  initialize static data
 *
 * @note   must initialize first
 */
void uc_wiota_static_data_init(void);

/**
 * @brief  get user ID
 *
 * @param  id the user ID
 * @param  len the length of user ID
 * @note   the length may be 0-8 bytes
 */
void uc_wiota_get_userid(unsigned char *id, unsigned char *len);

/**
 * @brief  get device name
 *
 * @param  name the vevice name
 * @note   the maximum length is 16 bytes
 */
void uc_wiota_get_dev_name(unsigned char *name);

/**
 * @brief  get device serial
 *
 * @param  serial the device serial
 * @note   the maximum length is 16 bytes
 */
void uc_wiota_get_dev_serial(unsigned char *serial);

/**
 * @brief  get software version
 *
 * @param  software_ver the software version
 * @note   the maximum length is 16 bytes
 */
// void uc_wiota_get_software_ver(unsigned char *software_ver);

/**
 * @brief  get manufacture name
 *
 * @param  name the manufacture name
 * @note   the maximum length is 16 bytes
 */
void uc_wiota_get_manufacture_name(unsigned char *name);

/**
 * @brief  get hardware version
 *
 * @param  hardware_ver the hardware version
 * @note   the maximum length is 16 bytes
 */
void uc_wiota_get_hardware_ver(unsigned char *hardware_ver);

/**
 * @brief  get auto run flag
 *
 * @return the auto run flag (0 or 1)
 */
unsigned char uc_wiota_get_auto_run_flag(void);

/**
 * @brief  get DTU config
 *
 * @param  cfg the DTU config information
 */
void uc_wiota_get_dtu_config(dtu_info_t *cfg);

/**
 * @brief  set frequency point list
 *
 * @param  freq_list the frequency point list
 * @param  num the number of frequency point
 * @return 0: if successful
 *         !0: otherwise
 * @note   the maximum number of frequency point is 16
 */
int uc_wiota_set_freq_list(unsigned char *freq_list, unsigned char num);

/**
 * @brief  get frequency point list
 *
 * @param  freq_list the frequency point list
 */
void uc_wiota_get_freq_list(unsigned char *freq_list);

/**
 * @brief  get the first address of user defined data
 *
 * @return the first address of user defined data
 * @note   the maximum length is 256 bytes
 */
unsigned char* uc_wiota_get_user_info(void);

/**
 * @brief  save static data to flash
 *
 * @param  is_direct whether to save data directly
 * @note   parameter is_direct is ignored in AP
 */
void uc_wiota_save_static_info(unsigned char is_direct);

/**
 * @brief  erase 4KB flash with 0xFF
 *
 * @param  flash_addr the flash address
 * @return 0: if successfull
 *         !0: otherwise
 */
unsigned int uc_wiota_flash_erase_4K(unsigned int flash_addr);

/**
 * @brief  write flash without erase
 *
 * @param  data_addr the write data
 * @param  flash_addr the flash address
 * @param  length the length of write data
 * @return 0: if successfull
 *         !0: otherwise
 */
unsigned int uc_wiota_flash_write(unsigned char *data_addr, unsigned int flash_addr, unsigned short length);

/**
 * @brief  read flash
 *
 * @param  data_addr the read data
 * @param  flash_addr the flash address
 * @param  length the length of read data
 * @return 0: if successfull
 *         !0: otherwise
 */
unsigned int uc_wiota_flash_read(unsigned char *data_addr, unsigned int flash_addr, unsigned short length);

#ifdef __cplushplus
}
#endif // __cplushplus

#endif // _UC_WIOTA_STATIC_H_
