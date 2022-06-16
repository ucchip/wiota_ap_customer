/******************************************************************************
* Chongqing UCchip InfoTech Co.,Ltd
* Copyright (c) 2022 UCchip
* 
* @file    custom_data.h
* @brief   Custom data application program interface.
* 
* @author  lujun
* @email   lujun@ucchip.cn
* @data    2022-06-09
* @license ???
******************************************************************************/
#ifndef _CUSTOM_DATA_H_
#define _CUSTOM_DATA_H_


#ifdef __cplushplus
extern "C"
{
#endif

/*
* @brief Custom data.
* @note  Not more than 256 bytes.
*/
typedef struct CUSTOM_DATA_T
{
    unsigned char ip[16];              /* IP address */
    unsigned int  port;                /* port */
    unsigned char client_id[16];       /* client id */
    unsigned char account[32];         /* account */
    unsigned char password[32];        /* password */
} custom_data_t;


/*
* @brief   Set server information.
* @param   ip:   IP address.
* @param   len:  Length of ip.
* @param   port: Port.
* @return  0 on success, otherwise 1.
* @note    The maximum length of ip is 16.
*/
int custom_set_server_info(unsigned char* ip, unsigned char len, unsigned int port);

/*
* @brief   Get server information.
* @param   ip:   IP address.
* @param   len:  Length of ip.
* @param   port: Port.
* @note    The maximum length of ip is 16.
*/
void custom_get_server_info(unsigned char* ip, unsigned char* len, unsigned int* port);

/*
* @brief   Set client id.
* @param   id:   Client id.
* @param   len:  Length of client id.
* @return  0 on success, otherwise 1.
* @note    The maximum length of client id is 16.
*/
int custom_set_client_id(unsigned char* id, unsigned char len);

/*
* @brief   Get client id.
* @param   id:   Client id.
* @param   len:  Length of client id.
* @note    The maximum length of client id is 16.
*/
void custom_get_client_id(unsigned char* id, unsigned char* len);

/*
* @brief   Set login information.
* @param   account:  Account.
* @param   acc_len:  Length of account.
* @param   password: Password.
* @param   pwd_len:  Length of password.
* @return  0 on success, otherwise 1.
* @note    The maximum length of account is 32.
           The maximum length of Password is 32.
*/
int custom_set_login_info(unsigned char* account, unsigned char acc_len, unsigned char* password, unsigned char pwd_len);

/*
* @brief   Get login information.
* @param   account:  Account.
* @param   acc_len:  Length of account.
* @param   password: Password.
* @param   pwd_len:  Length of password.
* @note    The maximum length of account is 32.
           The maximum length of Password is 32.
*/
void custom_get_login_info(unsigned char* account, unsigned char* acc_len, unsigned char* password, unsigned char* pwd_len);

#ifdef __cplushplus
}
#endif // !__cplushplus

#endif // !_CUSTOM_DATA_H_