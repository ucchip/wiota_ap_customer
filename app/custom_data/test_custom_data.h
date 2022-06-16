#ifndef _TEST_CUSTOM_DATA_H_
#define _TEST_CUSTOM_DATA_H_


#ifdef __cplushplus
extern "C"
{
#endif

// Test custom data interface.
void test_custom_data(void);

// Test set server infomation.
void test_set_server_info(void);

// Test get server infomation.
void test_get_server_info(void);

// Test set client id.
void test_set_client_id(void);

// Test get client id.
void test_get_client_id(void);

// Test set login info.
void test_set_login_info(void);

// Test get login info.
void test_get_login_info(void);

#ifdef __cplushplus
}
#endif // !__cplushplus

#endif // !_TEST_CUSTOM_DATA_H_
