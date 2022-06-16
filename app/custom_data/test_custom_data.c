#include "rtthread.h"
#ifdef WIOTA_APP_DEMO
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "custom_data.h"
#include "test_custom_data.h"
#include "custom_data.h"


// Test custom data interface.
void test_custom_data(void)
{
	uc_wiota_init();
	
	// Test get server infomation.
	test_get_server_info();
	// Test get client id.
	test_get_client_id();
	// Test get login info.
	test_get_login_info();
	
	// Test set server infomation.
	test_set_server_info();
	// Test set client id.
	test_set_client_id();
	// Test set login info.
	test_set_login_info();

	// Save static data to flash.
	uc_wiota_save_static_info(1);
}

// Test set server infomation.
void test_set_server_info(void)
{
	unsigned char ip[16] = "192.168.1.20";
	unsigned char len    = 12;
	unsigned int  port   = 5000;
	custom_set_server_info(ip, len, port);
}

// Test get server infomation.
void test_get_server_info(void)
{
	unsigned char ip[16] = {0};
	unsigned char len    = 0;
	unsigned int  port   = 0;
	custom_get_server_info(ip, &len, &port);
	rt_kprintf("ip: %s\n", ip);
	rt_kprintf("port: %d\n", port);
}

// Test set client id.
void test_set_client_id(void)
{
	unsigned char id[16] = "client001";
	unsigned char len    = 9;
	custom_set_client_id(id, len);
}

// Test get client id.
void test_get_client_id(void)
{
	unsigned char id[16] = {0};
	unsigned char len    = 0;
	custom_get_client_id(id, &len);
	rt_kprintf("client id: %s\n", id);
}

// Test set login info.
void test_set_login_info(void)
{
	unsigned char account[32]  = "user";
	unsigned char password[32] = "pwd";
	unsigned char acc_len      = 4;
	unsigned char pwd_len      = 3;
	custom_set_login_info(account, acc_len, password, pwd_len);
}

// Test get login info.
void test_get_login_info(void)
{
	unsigned char account[32]  = {0};
	unsigned char password[32] = {0};
	unsigned char acc_len      = 0;
	unsigned char pwd_len      = 0;
	custom_get_login_info(account, &acc_len, password, &pwd_len);
	rt_kprintf("account: %s\n", account);
	rt_kprintf("password: %s\n", password);
}

#endif // WIOTA_APP_DEMO