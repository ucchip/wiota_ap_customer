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

#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "custom_data.h"

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
int custom_set_server_info(unsigned char *ip, unsigned char len, unsigned int port)
{
    custom_data_t *custom_data;
    if (len == 0 || len > 16)
    {
        rt_kprintf("error len! len = %d\n", len);
        return UC_OP_FAIL;
    }
    custom_data = (custom_data_t *)uc_wiota_get_user_info();
    // set IP address
    rt_memset(custom_data->ip, 0, 16);
    rt_memcpy(custom_data->ip, ip, len);
    // set port
    custom_data->port = port;
    return UC_OP_SUCC;
}

/**
 * @brief  get server information
 *
 * @param  ip the IP address
 * @param  len the length of IP
 * @param  port the port
 * @note   the maximum length of IP is 16
 */
void custom_get_server_info(unsigned char *ip, unsigned char *len, unsigned int *port)
{
    custom_data_t *custom_data = (custom_data_t *)uc_wiota_get_user_info();
    // get IP address
    rt_memcpy(ip, custom_data->ip, 16);
    // calculate the number of IP
    *len = 0;
    for (int i = 0; i < 16; ++i)
    {
        if (!ip[i])
            break;
        *len += 1;
    }
    // get port
    *port = custom_data->port;
}

/**
 * @brief  set client ID
 *
 * @param  id the client ID
 * @param  len the length of client ID
 * @return 0: if successfull
 *         !0: otherwise
 * @note   the maximum length of client ID is 16
 */
int custom_set_client_id(unsigned char *id, unsigned char len)
{
    custom_data_t *custom_data;
    if (len == 0 || len > 16)
    {
        rt_kprintf("error len! len = %d\n", len);
        return UC_OP_FAIL;
    }
    custom_data = (custom_data_t *)uc_wiota_get_user_info();
    // set client ID
    rt_memset(custom_data->client_id, 0, 16);
    rt_memcpy(custom_data->client_id, id, len);
    return UC_OP_SUCC;
}

/**
 * @brief  get client ID
 *
 * @param  id the client ID
 * @param  len the length of client ID
 * @note   the maximum length of client ID is 16
 */
void custom_get_client_id(unsigned char *id, unsigned char *len)
{
    custom_data_t *custom_data = (custom_data_t *)uc_wiota_get_user_info();
    // get client ID
    rt_memcpy(id, custom_data->client_id, 16);
    // calculate the number of client ID
    *len = 0;
    for (int i = 0; i < 16; ++i)
    {
        if (!id[i])
            break;
        *len += 1;
    }
}

/**
 * @brief  set login information
 *
 * @param  account the login account
 * @param  acc_len the length of account
 * @param  password the login password
 * @param  pwd_len the length of password.
 * @return 0: if successfull
 *         !0: otherwise
 * @note   the maximum length of account is 32
 *         the maximum length of password is 32
 */
int custom_set_login_info(unsigned char *account, unsigned char acc_len, unsigned char *password, unsigned char pwd_len)
{
    custom_data_t *custom_data = (custom_data_t *)uc_wiota_get_user_info();
    if (account != NULL)
    {
        if (acc_len == 0 || acc_len > 32)
        {
            rt_kprintf("error acc_len! acc_len = %d\n", acc_len);
            return UC_OP_FAIL;
        }
        // set login account
        rt_memset(custom_data->account, 0, 32);
        rt_memcpy(custom_data->account, account, acc_len);
    }
    if (password != NULL)
    {
        if (pwd_len == 0 || pwd_len > 32)
        {
            rt_kprintf("error pwd_len! pwd_len = %d\n", pwd_len);
            return UC_OP_FAIL;
        }
        // set login password
        rt_memset(custom_data->password, 0, 32);
        rt_memcpy(custom_data->password, password, pwd_len);
    }
    return UC_OP_SUCC;
}

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
void custom_get_login_info(unsigned char *account, unsigned char *acc_len, unsigned char *password, unsigned char *pwd_len)
{
    custom_data_t *custom_data = (custom_data_t *)uc_wiota_get_user_info();
    // get login account
    rt_memcpy(account, custom_data->account, 32);
    // get login password
    rt_memcpy(password, custom_data->password, 32);
    // calculate the length of account
    *acc_len = 0;
    for (int i = 0; i < 32; ++i)
    {
        if (!account[i])
            break;
        *acc_len += 1;
    }
    // calculate the length of password
    *pwd_len = 0;
    for (int i = 0; i < 32; ++i)
    {
        if (!password[i])
            break;
        *pwd_len += 1;
    }
}

#endif // WIOTA_APP_DEMO
