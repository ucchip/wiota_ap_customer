/*
 * Copyright (c) 2022, Chongqing UCchip InfoTech Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @brief Custom data application program interface.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-09     Lujun        the first version
 * 2022-08-04     Lujun        replace memcpy and memset with rt_memcpy and rt_memset
 */

#ifndef _CUSTOM_DATA_H_
#define _CUSTOM_DATA_H_

#ifdef __cplushplus
extern "C"
{
#endif

/**
 * @brief custom data
 *
 * @note  Not more than 256 bytes
 */
typedef struct CustomDataT
{
    unsigned char  ip[16];             /**< IP address */
    unsigned int   port;               /**< port */
    unsigned char  client_id[16];      /**< client ID */
    unsigned char  account[32];        /**< login account */
    unsigned char  password[32];       /**< login password */
} custom_data_t;

/**
 * @brief  set server information
 *
 * @param  ip the IP address
 * @param  len the length of IP
 * @param  port the port
 * @return 0: if successfull
 *         !0: otherwise
 * @note   the maximum length of IP is 16
 */
int custom_set_server_info(unsigned char *ip, unsigned char len, unsigned int port);

/**
 * @brief  get server information
 *
 * @param  ip the IP address
 * @param  len the length of IP
 * @param  port the port
 * @note   the maximum length of IP is 16
 */
void custom_get_server_info(unsigned char *ip, unsigned char *len, unsigned int *port);

/**
 * @brief  set client ID
 *
 * @param  id the client ID
 * @param  len the length of client ID
 * @return 0: if successfull
 *         !0: otherwise
 * @note   the maximum length of client ID is 16
 */
int custom_set_client_id(unsigned char *id, unsigned char len);

/**
 * @brief  get client ID
 *
 * @param  id the client ID
 * @param  len the length of client ID
 * @note   the maximum length of client ID is 16
 */
void custom_get_client_id(unsigned char *id, unsigned char *len);

/**
 * @brief  set login information
 *
 * @param  account the login account
 * @param  acc_len the length of account
 * @param  password the login password
 * @param  pwd_len the length of password
 * @return 0: if successfull
 *         !0: otherwise
 * @note   the maximum length of account is 32
 *         the maximum length of password is 32
 */
int custom_set_login_info(unsigned char *account, unsigned char acc_len, unsigned char *password, unsigned char pwd_len);

/**
 * @brief  get login information
 *
 * @param  account the login account
 * @param  acc_len the length of account
 * @param  password the login password
 * @param  pwd_len the length of password
 * @note   the maximum length of account is 32
 *         the maximum length of password is 32
 */
void custom_get_login_info(unsigned char *account, unsigned char *acc_len, unsigned char *password, unsigned char *pwd_len);

#ifdef __cplushplus
}
#endif // __cplushplus

#endif // _CUSTOM_DATA_H_
