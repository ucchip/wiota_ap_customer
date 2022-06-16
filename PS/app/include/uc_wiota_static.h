/******************************************************************************
* Chongqing UCchip InfoTech Co.,Ltd
* Copyright (c) 2022 UCchip
* 
* @file    uc_wiota_static.h
* @brief   Static data application program interface.
* 
* @author  lujun
* @email   lujun@ucchip.cn
* @data    2022-06-01
* @license ???
******************************************************************************/
#ifndef _UC_WIOTA_STATIC_H_
#define _UC_WIOTA_STATIC_H_

#ifdef __cplushplus
extern "C"
{
#endif

/*
* @brief Data transfer unit.
*/
typedef struct DtuInfoT
{
    unsigned char  reserved[2];        /* reserved */
    unsigned char  dtu_status;         /* status: 0 or 1 */
    unsigned char  dtu_at_show;        /* show AT format: 0 or 1 */
    unsigned short dtu_timeout;        /* send timeout */
    unsigned short dtu_wait;           /* wait time */
    unsigned char  dtu_exit[8];        /* exit string */
    unsigned char  na[24];             /* undefined */
} dtu_info_t;


/*
* @brief   Initialize static data.
* @note    Must initialize first.
*/
void uc_wiota_static_data_init(void);

/*
* @brief   Get user id.
* @param   id:  User ID.
* @param   len: Length of user ID.
* @note    Length may be 0-8 bytes.
*/
void uc_wiota_get_userid(unsigned char* id, unsigned char* len);

/*
* @brief   Get device name.
* @param   name: Device name.
* @note    The maximum length is 16 bytes.
*/
void uc_wiota_get_dev_name(unsigned char* name);

/*
* @brief   Get device serial.
* @param   serial: Device serial.
* @note    The maximum length is 16 bytes.
*/
void uc_wiota_get_dev_serial(unsigned char* serial);

/*
* @brief   Get software version.
* @param   hardware_ver: Software version.
* @note    The maximum length is 16 bytes.
*/
//void uc_wiota_get_software_ver(unsigned char* software_ver);

/*
* @brief   Get manufacture name.
* @param   name: Manufacture name.
* @note    The maximum length is 16 bytes.
*/
void uc_wiota_get_manufacture_name(unsigned char* name);

/*
* @brief   Get hardware version.
* @param   hardware_ver: Hardware version.
* @note    The maximum length is 16 bytes.
*/
void uc_wiota_get_hardware_ver(unsigned char* hardware_ver);

/*
* @brief   Get auto run flag.
* @return  Auto run flag: 0 or 1.
*/
unsigned char uc_wiota_get_auto_run_flag(void);

/*
* @brief   Get DTU config.
* @param   cfg: DTU config information.
*/
void uc_wiota_get_dtu_config(dtu_info_t *cfg);

/*
* @brief   Set frequency point list.
* @param   freq_list: Frequency point list.
* @param   num:       Number of frequency point.
* @return  0 on success, otherwise 1.
* @note    The maximum number of frequency point is 16.
*/
int uc_wiota_set_freq_list(unsigned char* list, unsigned char num);

/*
* @brief   Get frequency point list.
* @param   freq_list: frequency point list.
*/
void uc_wiota_get_freq_list(unsigned char* list);

/*
* @brief   Get the first address of user defined data.
* @return  The first address of user defined data.
* @note    The maximum length is 256 bytes. If the pointer is out of range,
*          serious consequences will result.
*/
unsigned char* uc_wiota_get_user_info(void);

/*
* @brief   Save static data to flash.
* @param   is_direct: Whether to save data directly.
* @note    Parameter is_direct is ignored in AP.
*/
void uc_wiota_save_static_info(unsigned char is_direct);

#ifdef __cplushplus
}
#endif // !__cplushplus

#endif // !_UC_WIOTA_STATIC_H_