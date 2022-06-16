/******************************************************************************
* Chongqing UCchip InfoTech Co.,Ltd
* Copyright (c) 2022 UCchip
*
* @file    custom_data.c
* @brief   Custom data application program interface.
*
* @author  lujun
* @email   lujun@ucchip.cn
* @data    2022-06-09
* @license ???
******************************************************************************/
#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <string.h>
#include "uc_wiota_static.h"
#include "custom_data.h"


// Set server information.
int custom_set_server_info(unsigned char* ip, unsigned char len, unsigned int port)
{
    custom_data_t* custom_data;
    if (len == 0 || len > 16)
    {
        rt_kprintf("error len! len = %d\n", len);
        return 1;
    }
    custom_data = (custom_data_t*)uc_wiota_get_user_info();
    // Set IP address.
    memset(custom_data->ip, 0, 16);
    memcpy(custom_data->ip, ip, len);
    // Set port.
    custom_data->port = port;
    return 0;
}

// Get server information.
void custom_get_server_info(unsigned char* ip, unsigned char* len, unsigned int* port)
{
    custom_data_t* custom_data = (custom_data_t*)uc_wiota_get_user_info();
    // Get IP address.
    memcpy(ip, custom_data->ip, 16);
    // Calculate the number of ip.
    *len = 0;
    for (int i = 0; i < 16; ++i)
    {
        if (!ip[i])
            break;
        *len += 1;
    }
    // Get port.
    *port = custom_data->port;
}

// Set client id.
int custom_set_client_id(unsigned char* id, unsigned char len)
{
    custom_data_t* custom_data;
    if (len == 0 || len > 16)
    {
        rt_kprintf("error len! len = %d\n", len);
        return 1;
    }
    custom_data = (custom_data_t*)uc_wiota_get_user_info();
    // Set client id.
    memset(custom_data->client_id, 0, 16);
    memcpy(custom_data->client_id, id, len);
    return 0;
}

// Get client id.
void custom_get_client_id(unsigned char* id, unsigned char* len)
{
    custom_data_t* custom_data = (custom_data_t*)uc_wiota_get_user_info();
    // Get client id.
    memcpy(id, custom_data->client_id, 16);
    // Calculate the number of client id.
    *len = 0;
    for (int i = 0; i < 16; ++i)
    {
        if (!id[i])
            break;
        *len += 1;
    }
}

// Set login information.
int custom_set_login_info(unsigned char* account, unsigned char acc_len, unsigned char* password, unsigned char pwd_len)
{
    custom_data_t* custom_data = (custom_data_t*)uc_wiota_get_user_info();
    if (account != NULL)
    {
        if (acc_len == 0 || acc_len > 32)
        {
            rt_kprintf("error acc_len! acc_len = %d\n", acc_len);
            return 1;
        }
        // Set account.
        memset(custom_data->account, 0, 32);
        memcpy(custom_data->account, account, acc_len);
    }
    if (password != NULL)
    {
        if (pwd_len == 0 || pwd_len > 32)
        {
            rt_kprintf("error pwd_len! pwd_len = %d\n", pwd_len);
            return 2;
        }
        // Set password.
        memset(custom_data->password, 0, 32);
        memcpy(custom_data->password, password, pwd_len);
    }
    return 0;
}

// Get login information.
void custom_get_login_info(unsigned char* account, unsigned char* acc_len, unsigned char* password, unsigned char* pwd_len)
{
    custom_data_t* custom_data = (custom_data_t*)uc_wiota_get_user_info();
    // Get account.
    memcpy(account, custom_data->account, 32);
    // Get password.
    memcpy(password, custom_data->password, 32);
    // Calculate the length of the account.
    *acc_len = 0;
    for (int i = 0; i < 32; ++i)
    {
        if (!account[i])
            break;
        *acc_len += 1;
    }
    // Calculate the length of the password.
    *pwd_len = 0;
    for (int i = 0; i < 32; ++i)
    {
        if (!password[i])
            break;
        *pwd_len += 1;
    }
}

#endif // WIOTA_APP_DEMO
